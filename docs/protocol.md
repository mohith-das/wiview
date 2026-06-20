# wiview Streaming Protocol v1

The wiview firmware streams CSI data and inferences over **UDP** on port **5005**
to a host computer on the same WiFi network.

## Transport

- **Protocol:** UDP (connectionless, low latency)
- **Default port:** 5005
- **Target:** Host IP on the same LAN (default: gateway IP)
- **Direction:** Cardputer → Host (unidirectional)

## Packet Format

Every packet has a **12-byte binary header** followed by a type-specific payload.

### Header (12 bytes, big-endian)

| Offset | Size | Field       | Description                          |
|--------|------|-------------|--------------------------------------|
| 0      | 4    | magic       | `0x77697669` ("wivi")                |
| 4      | 1    | version     | Protocol version (currently `1`)     |
| 5      | 1    | type        | `0x00` = CSI raw, `0x01` = inference |
| 6      | 2    | payload_len | Length of the payload in bytes       |
| 8      | 4    | sequence    | Monotonically increasing counter     |

### Type 0x00 — CSI Raw Data

| Offset | Size | Field            | Description                     |
|--------|------|------------------|---------------------------------|
| 0      | 4    | timestamp_ms     | Millisecond timestamp           |
| 4      | 2    | num_subcarriers  | Number of I/Q pairs             |
| 6      | 2    | flags            | Reserved (0)                    |
| 8      | N    | iq_data          | int8 I/Q pairs (N = 2 × num_subcarriers) |

Each subcarrier is represented as a pair of signed 8-bit integers:
`[I₀, Q₀, I₁, Q₁, ..., Iₙ₋₁, Qₙ₋₁]`

### Type 0x01 — Inference (JSON Lines)

The payload is a **null-terminated JSON object** (one per packet):

```json
{"t":1700000000,"presence":true,"conf":0.87,"motion":0.32,"bpm":15.2,"still":true,"csi_hz":18.5}
```

| Field      | Type    | Description                                    |
|------------|---------|------------------------------------------------|
| t          | uint32  | Millisecond timestamp                          |
| presence   | boolean | Human presence detected                        |
| conf       | float   | Confidence 0.0–1.0                             |
| motion     | float   | Motion level 0.0 (still) – 1.0 (active)        |
| bpm        | float   | Breathing rate in breaths per minute           |
| still      | boolean | Device physically stationary (IMU gate)        |
| csi_hz     | float   | Current CSI packet rate                        |

## Example

```python
import socket, struct, json

sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sock.bind(("0.0.0.0", 5005))

while True:
    data, addr = sock.recvfrom(2048)
    magic, ver, ptype, plen, seq = struct.unpack(">IBBHI", data[:12])
    if magic != 0x77697669:
        continue  # not a wiview packet

    payload = data[12:12+plen]
    if ptype == 0x00:
        ts, sc, flags = struct.unpack(">IHH", payload[:8])
        iq = payload[8:]
        print(f"CSI: seq={seq} ts={ts} sc={sc} len={len(iq)}")
    elif ptype == 0x01:
        obj = json.loads(payload[:-1])  # strip null terminator
        print(f"INF: {obj}")
```

## Compatibility

This is an **independent protocol**, not compatible with RuView's wire format.

A **`--ruview-compat` bridge** is included in the host companion. It re-encodes
wiview frames into RuView's ADR-018 wire format and forwards them over UDP to a
RuView sensing-server:

- wiview CSI (`0x00`) → RuView CSI frame, magic `0xC5110001` (little-endian:
  20-byte header — magic, node_id, n_antennas, n_subcarriers, freq_mhz, seq,
  rssi, noise_floor, ppdu_type, flags — followed by the int8 I/Q pairs).
- wiview inference (`0x01`) → RuView vitals packet, magic `0xC5110002` (32 bytes:
  presence/motion flags, breathing rate ×100, motion energy, presence score).

```bash
# RuView listens for ESP32 CSI on UDP 5005 inside the container; publish it on a
# different host port (this tool already binds host:5005) and bridge to it:
docker run -d --name ruview -e CSI_SOURCE=esp32 -e RUVIEW_ALLOW_UNAUTHENTICATED=1 \
    -e SENSING_BIND_ADDR=0.0.0.0 \
    -p 127.0.0.1:3000:3000 -p 127.0.0.1:3001:3001 -p 127.0.0.1:5006:5005/udp \
    ruvnet/wifi-densepose:latest

python host/wiview_host.py --headless --ruview-compat \
    --ruview-host 127.0.0.1 --ruview-port 5006
```
