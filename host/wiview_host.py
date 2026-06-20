#!/usr/bin/env python3
"""
wiview Host Companion — UDP receiver, live plot, CSV logger, RuView bridge.

Receives CSI data + inferences from a Cardputer running wiview firmware
over UDP port 5005. Renders a live matplotlib dashboard, optionally logs to
CSV, and optionally re-encodes frames into RuView's ADR-018 wire format and
forwards them to a RuView sensing-server (--ruview-compat).

Usage:
    python wiview_host.py                              # listen + live plot
    python wiview_host.py --csv out.csv                # also log to CSV
    python wiview_host.py --headless                   # no GUI (terminal only)
    python wiview_host.py --ruview-compat \\
        --ruview-host 127.0.0.1 --ruview-port 5006     # bridge to RuView

RuView note: RuView's esp32 source listens for ADR-018 frames on UDP 5005
*inside* the container. Because this tool already binds host:5005, run RuView
with a different published port (e.g. -p 5006:5005/udp) and forward to 5006.
"""
import argparse
import csv
import json
import socket
import struct
import sys
import time
from collections import deque

# ── wiview wire format (see docs/protocol.md) ──────────────────────────────
MAGIC = 0x77697669
TYPE_CSI = 0x00
TYPE_INFERENCE = 0x01
HEADER_FMT = ">IBBHI"          # magic, ver, type, payload_len, seq (big-endian)
CSI_SUBHDR_FMT = ">IHH"        # timestamp_ms, num_subcarriers, flags

# ── RuView ADR-018 wire format (firmware/esp32-csi-node/main/csi_collector.c)─
RV_MAGIC_CSI = 0xC5110001
RV_MAGIC_VITALS = 0xC5110002
# CSI header (20 bytes, little-endian, packed):
#   magic u32 | node_id u8 | n_antennas u8 | n_subcarriers u16 | freq_mhz u32 |
#   seq u32 | rssi i8 | noise_floor i8 | ppdu_type u8 | flags u8 | <iq bytes>
RV_CSI_HDR_FMT = "<IBBHIIbbBB"
# Vitals packet (32 bytes, little-endian — edge_processing.h edge_vitals_pkt_t):
#   magic u32 | node_id u8 | flags u8 | breathing_rate u16 (bpm*100) |
#   heartrate u32 (bpm*10000) | rssi i8 | n_persons u8 | pad2 |
#   motion_energy f32 | presence_score f32 | timestamp_ms u32 | reserved u32
RV_VITALS_FMT = "<IBBHIbBxxffII"


class RuViewBridge:
    """Re-encodes wiview frames into RuView ADR-018 and forwards via UDP."""

    def __init__(self, host: str, port: int, freq_mhz: int = 2412,
                 node_id: int = 1, rssi: int = -50, noise_floor: int = -90):
        self.dest = (host, port)
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.freq_mhz = freq_mhz
        self.node_id = node_id
        self.rssi = rssi
        self.noise_floor = noise_floor
        self.seq = 0
        self.csi_fwd = 0
        self.vitals_fwd = 0

    def forward_csi(self, payload: bytes):
        """wiview CSI payload -> RuView 0xC5110001 frame."""
        if len(payload) < 8:
            return
        _ts, _sc, _flags = struct.unpack(CSI_SUBHDR_FMT, payload[:8])
        iq = payload[8:]                       # int8 I/Q pairs, identical encoding
        n_sub = len(iq) // 2                   # trust the actual byte count
        if n_sub == 0:
            return
        hdr = struct.pack(
            RV_CSI_HDR_FMT,
            RV_MAGIC_CSI, self.node_id, 1, n_sub, self.freq_mhz,
            self.seq & 0xFFFFFFFF, self.rssi, self.noise_floor, 0, 0,
        )
        self.seq += 1
        self.sock.sendto(hdr + iq, self.dest)
        self.csi_fwd += 1

    def forward_inference(self, o: dict):
        """wiview inference JSON -> RuView 0xC5110002 vitals packet."""
        flags = 0
        if o.get("presence"):
            flags |= 0x1                       # bit0 = presence
        if o.get("motion", 0.0) > 0.3:
            flags |= 0x4                       # bit2 = motion
        breathing = int(max(0.0, o.get("bpm", 0.0)) * 100) & 0xFFFF
        pkt = struct.pack(
            RV_VITALS_FMT,
            RV_MAGIC_VITALS, self.node_id, flags, breathing,
            0,                                  # heartrate (wiview has none)
            self.rssi,
            1 if o.get("presence") else 0,      # n_persons
            float(o.get("motion", 0.0)),        # motion_energy
            float(o.get("conf", 0.0)),          # presence_score
            int(o.get("t", 0)) & 0xFFFFFFFF,    # timestamp_ms
            0,                                  # reserved
        )
        self.sock.sendto(pkt, self.dest)
        self.vitals_fwd += 1


class WiviewReceiver:
    """Receives and parses wiview UDP packets."""

    def __init__(self, address: str = "0.0.0.0", port: int = 5005):
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        self.sock.bind((address, port))
        self.sock.settimeout(0.1)
        self.csv_file = None
        self.csv_writer = None

        # Rolling data for plotting
        self.csi_rate_hist = deque(maxlen=200)
        self.presence_hist = deque(maxlen=200)
        self.motion_hist = deque(maxlen=200)
        self.bpm_hist = deque(maxlen=200)
        self.times = deque(maxlen=200)
        self.last_seq = -1
        self.csi_count = 0
        self.inf_count = 0
        self.start_time = time.time()

    def open_csv(self, path: str):
        self.csv_file = open(path, "w", newline="")
        self.csv_writer = csv.writer(self.csv_file)
        self.csv_writer.writerow(["time", "type", "seq", "csi_rate", "presence", "motion", "bpm", "device_still"])

    def close(self):
        if self.csv_file:
            self.csv_file.close()

    def recv(self):
        try:
            data, addr = self.sock.recvfrom(2048)
        except socket.timeout:
            return None
        if len(data) < 12:
            return None

        magic, ver, ptype, plen, seq = struct.unpack(HEADER_FMT, data[:12])
        if magic != MAGIC:
            return None

        payload = data[12 : 12 + plen]
        self.last_seq = seq
        t = time.time() - self.start_time

        if ptype == TYPE_CSI:
            self.csi_count += 1
            return {"type": "csi", "seq": seq, "time": t, "payload": payload}
        elif ptype == TYPE_INFERENCE:
            self.inf_count += 1
            try:
                obj = json.loads(payload.decode("utf-8").rstrip("\x00"))
            except json.JSONDecodeError:
                return None

            self.times.append(t)
            self.csi_rate_hist.append(obj.get("csi_hz", 0))
            self.presence_hist.append(1.0 if obj.get("presence") else 0.0)
            self.motion_hist.append(obj.get("motion", 0))
            self.bpm_hist.append(obj.get("bpm", 0))

            if self.csv_writer:
                self.csv_writer.writerow([
                    obj.get("t", 0), "inference", seq,
                    obj.get("csi_hz", 0),
                    1 if obj.get("presence") else 0,
                    obj.get("motion", 0),
                    obj.get("bpm", 0),
                    1 if obj.get("still") else 0,
                ])

            return {"type": "inference", "seq": seq, "time": t, "data": obj}

        return None


def main():
    parser = argparse.ArgumentParser(description="wiview Host Companion")
    parser.add_argument("--port", type=int, default=5005, help="UDP listen port (default: 5005)")
    parser.add_argument("--address", type=str, default="0.0.0.0", help="Bind address")
    parser.add_argument("--csv", type=str, default=None, help="CSV output file")
    parser.add_argument("--headless", action="store_true", help="No GUI (terminal output only)")
    parser.add_argument("--ruview-compat", action="store_true", help="Forward frames to RuView (ADR-018)")
    parser.add_argument("--ruview-host", type=str, default="127.0.0.1", help="RuView host")
    parser.add_argument("--ruview-port", type=int, default=5006, help="RuView UDP port (host-side)")
    parser.add_argument("--ruview-freq", type=int, default=2412, help="freq_mhz to stamp on CSI frames")
    args = parser.parse_args()

    rx = WiviewReceiver(args.address, args.port)
    if args.csv:
        rx.open_csv(args.csv)
        print(f"[CSV] Logging to {args.csv}")

    bridge = None
    if args.ruview_compat:
        bridge = RuViewBridge(args.ruview_host, args.ruview_port, freq_mhz=args.ruview_freq)
        print(f"[ruview] Bridging ADR-018 -> {args.ruview_host}:{args.ruview_port} "
              f"(CSI 0x{RV_MAGIC_CSI:08X}, vitals 0x{RV_MAGIC_VITALS:08X})")

    print(f"[wiview-host] Listening on UDP {args.address}:{args.port}")
    print("  Press Ctrl+C to stop.")

    # ── Optional matplotlib dashboard ──────────────────────────────────────
    fig = axes = None
    if not args.headless:
        try:
            import matplotlib
            matplotlib.use("TkAgg")
            import matplotlib.pyplot as plt
            plt.ion()
            fig, axes = plt.subplots(4, 1, figsize=(10, 8), sharex=True)
            fig.canvas.manager.set_window_title("wiview — Live Dashboard")
        except Exception as e:
            print(f"[plot] GUI unavailable ({e}); falling back to --headless")
            fig = axes = None

    def update_plot():
        if axes is None or len(rx.times) < 2:
            return
        import matplotlib.pyplot as plt
        t = list(rx.times)
        for ax, data, color, label, ylim in [
            (axes[0], rx.csi_rate_hist, "b-", "CSI Hz", None),
            (axes[1], rx.presence_hist, "g-", "Presence", (-0.1, 1.1)),
            (axes[2], rx.motion_hist, "r-", "Motion", (-0.1, 1.1)),
            (axes[3], rx.bpm_hist, "c-", "BPM", None),
        ]:
            ax.clear()
            ax.plot(t, list(data), color, linewidth=1)
            ax.set_ylabel(label)
            ax.grid(True, alpha=0.3)
            if ylim:
                ax.set_ylim(*ylim)
        axes[3].set_xlabel("Time (s)")
        title = f"wiview Live  |  CSI:{rx.csi_count}  INF:{rx.inf_count}"
        if bridge:
            title += f"  |  ->RuView CSI:{bridge.csi_fwd} vitals:{bridge.vitals_fwd}"
        axes[0].set_title(title)
        plt.tight_layout()
        fig.canvas.draw_idle()
        fig.canvas.flush_events()

    last_print = 0.0
    try:
        while True:
            pkt = rx.recv()
            if pkt:
                if pkt["type"] == "csi" and bridge:
                    bridge.forward_csi(pkt["payload"])
                elif pkt["type"] == "inference":
                    if bridge:
                        bridge.forward_inference(pkt["data"])
                    now = time.time()
                    if now - last_print >= 0.5:
                        last_print = now
                        d = pkt["data"]
                        line = (f"  [{pkt['time']:.1f}s] presence={d.get('presence')} "
                                f"motion={d.get('motion',0):.2f} bpm={d.get('bpm',0):.1f} "
                                f"csi={d.get('csi_hz',0):.1f}Hz")
                        if bridge:
                            line += f"  ->RuView csi:{bridge.csi_fwd} vit:{bridge.vitals_fwd}"
                        print(line, flush=True)
            if fig is not None and int(time.time() * 2) % 2 == 0:
                update_plot()
    except KeyboardInterrupt:
        print("\n[wiview-host] Shutting down...")
    finally:
        rx.close()
        if fig is not None:
            import matplotlib.pyplot as plt
            plt.close("all")


if __name__ == "__main__":
    main()
