#!/usr/bin/env python3
"""
wiview Host Companion — UDP receiver, live plot, and CSV logger.

Receives CSI data + inferences from a Cardputer running wiview firmware
over UDP port 5005. Renders a live matplotlib dashboard.

Usage:
    python wiview_host.py                          # listen on default port
    python wiview_host.py --port 5005 --csv out.csv  # log to CSV
    python wiview_host.py --address 192.168.1.100   # listen on specific IP
"""
import argparse
import csv
import json
import socket
import struct
import sys
import time
from collections import deque

import matplotlib
matplotlib.use("TkAgg")
import matplotlib.pyplot as plt
import numpy as np

MAGIC = 0x77697669
TYPE_CSI = 0x00
TYPE_INFERENCE = 0x01
HEADER_FMT = ">IBBHI"


class WiviewReceiver:
    """Receives and parses wiview UDP packets."""

    def __init__(self, address: str = "0.0.0.0", port: int = 5005):
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        self.sock.bind((address, port))
        self.sock.settimeout(0.1)
        self.csv_file = None
        self.csv_writer = None

        # Rolling data
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
    parser.add_argument("--port", type=int, default=5005, help="UDP port (default: 5005)")
    parser.add_argument("--address", type=str, default="0.0.0.0", help="Bind address")
    parser.add_argument("--csv", type=str, default=None, help="CSV output file")
    args = parser.parse_args()

    rx = WiviewReceiver(args.address, args.port)
    if args.csv:
        rx.open_csv(args.csv)
        print(f"[CSV] Logging to {args.csv}")

    print(f"[wiview-host] Listening on UDP {args.address}:{args.port}")
    print("  Press Ctrl+C to stop.")

    # Setup matplotlib
    plt.ion()
    fig, axes = plt.subplots(4, 1, figsize=(10, 8), sharex=True)
    fig.canvas.manager.set_window_title("wiview — Live Dashboard")

    def update_plot():
        if len(rx.times) < 2:
            return
        t = list(rx.times)

        axes[0].clear()
        axes[0].plot(t, list(rx.csi_rate_hist), "b-", linewidth=1)
        axes[0].set_ylabel("CSI Hz")
        axes[0].grid(True, alpha=0.3)

        axes[1].clear()
        axes[1].fill_between(t, list(rx.presence_hist), alpha=0.4, color="green")
        axes[1].plot(t, list(rx.presence_hist), "g-", linewidth=1)
        axes[1].set_ylabel("Presence")
        axes[1].set_ylim(-0.1, 1.1)
        axes[1].grid(True, alpha=0.3)

        axes[2].clear()
        axes[2].plot(t, list(rx.motion_hist), "r-", linewidth=1)
        axes[2].set_ylabel("Motion")
        axes[2].set_ylim(-0.1, 1.1)
        axes[2].grid(True, alpha=0.3)

        axes[3].clear()
        axes[3].plot(t, list(rx.bpm_hist), "c-", linewidth=1)
        axes[3].set_ylabel("BPM")
        axes[3].set_xlabel("Time (s)")
        axes[3].grid(True, alpha=0.3)

        axes[0].set_title(f"wiview Live  |  CSI pkts: {rx.csi_count}  |  INF pkts: {rx.inf_count}")
        plt.tight_layout()
        fig.canvas.draw_idle()
        fig.canvas.flush_events()

    try:
        while True:
            pkt = rx.recv()
            if pkt and pkt["type"] == "inference":
                print(f"  [{pkt['time']:.1f}s] presence={pkt['data'].get('presence')} "
                      f"motion={pkt['data'].get('motion',0):.2f} "
                      f"bpm={pkt['data'].get('bpm',0):.1f} "
                      f"csi={pkt['data'].get('csi_hz',0):.1f}Hz")
            # Update plot every 500ms
            if int(time.time() * 2) % 2 == 0:
                update_plot()
    except KeyboardInterrupt:
        print("\n[wiview-host] Shutting down...")
    finally:
        rx.close()
        plt.close("all")


if __name__ == "__main__":
    main()
