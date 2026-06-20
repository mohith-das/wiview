# wiview — WiFi Sensing on the M5Stack Cardputer-Adv

[![CI](https://github.com/mohith-das/wiview/actions/workflows/ci.yml/badge.svg)](https://github.com/mohith-das/wiview/actions/workflows/ci.yml)
[![License: MIT](https://img.shields.io/badge/License-MIT-green.svg)](LICENSE)

**Turn your Cardputer into a contactless WiFi sensor.** wiview captures WiFi
Channel State Information (CSI) on-device, runs signal processing for
presence/motion/breathing detection, and displays results live on the
built-in screen. No external hardware needed.

> **Inspired by [RuView / WiFi-DensePose](https://github.com/ruvnet/RuView)** (MIT) —
> an independent, clean-room implementation. This project shares RuView's
> commitment to honest claims and open-source engineering.

## What It Does

| Capability          | How It Works                                              | Honest Notes |
|---------------------|-----------------------------------------------------------|--------------|
| **Presence**        | CSI amplitude variance vs. calibrated baseline (z-score)  | Reliable within ~2–3 m of the device |
| **Motion**          | Temporal variance of CSI amplitude over 32-sample window  | Distinguishes still vs. walking |
| **Breathing**       | Bandpass-filtered CSI amplitude → zero-crossing BPM       | Experimental. Subject must sit still ~2 m away. Range: 6–30 BPM |
| **Device-motion gate** | BMI270 IMU suppresses CSI inferences when the device is bumped or moved | Prevents false triggers |

## Quick Flash

1. **Download** the latest `wiview-full.bin` from [Releases](https://github.com/mohith-das/wiview/releases)
2. Put your Cardputer-Adv in download mode:
   - Power switch **OFF**
   - Hold **G0** button (left side)
   - Power switch **ON**, then release G0
3. Flash with esptool:

```bash
# macOS / Linux
esptool.py --chip esp32s3 --port /dev/cu.usbmodem* write_flash 0x0 wiview-full.bin

# Windows
esptool.py --chip esp32s3 --port COMx write_flash 0x0 wiview-full.bin
```

4. Cycle power. On first boot, use the keyboard to enter your WiFi SSID and password.

## Keyboard Controls

| Key  | Action                        |
|------|-------------------------------|
| `1`  | Home view (presence + motion) |
| `2`  | Waterfall (CSI heatmap)       |
| `3`  | Breathing (waveform + BPM)    |
| `G0` | Cycle views                   |
| `s`  | Toggle UDP streaming          |
| `r`  | Recalibrate baseline          |
| `h`  | Set stream host IP (manual)   |
| `u`  | Toggle output: wiview / RuView |
| `w`  | WiFi manager (add/switch/delete) |

## Build From Source

```bash
# Install PlatformIO
pip install platformio

# Clone and build
git clone https://github.com/mohith-das/wiview.git
cd wiview
pio run -e cardputer

# Flash
pio run -e cardputer -t upload

# Run native tests
pio test -e native
```

## Host Companion (Optional)

Stream CSI data and inferences to your computer for logging and live plotting:

```bash
pip install -r host/requirements.txt
python host/wiview_host.py --csv data.csv          # live plot + CSV log
python host/wiview_host.py --headless              # terminal only, no GUI
```

**Zero-config:** the host companion broadcasts an announce beacon on the LAN, so
the Cardputer **auto-discovers it** — just run the host, then press `s` on the
device. No IP to look up. The discovered host is cached in NVS for next time.

If your network blocks broadcast (AP "client isolation"), set the host manually:
press `h` on the Cardputer and type your computer's IP. As a build-time override
you can also set `-DWIVIEW_STREAM_HOST=\"192.168.1.50\"` in `platformio.ini`.
Resolution order: discovered/saved host → build flag → gateway.

### RuView integration

[RuView](https://github.com/ruvnet/RuView) is a passive UDP listener for its own
ADR-018 CSI format. Two ways to feed it:

1. **Direct (no host process):** press `u` on the Cardputer to switch its output
   to RuView's ADR-018 format, then stream straight to the RuView sensing-server.
   Set RuView's host with `h` (or discovery), and run RuView with its UDP port
   published on the LAN (`-p 5005:5005/udp -e CSI_SOURCE=esp32`).
2. **Bridge:** keep the Cardputer in native mode and run
   `wiview_host.py --ruview-compat`, which translates frames to ADR-018 and adds
   auto-discovery + the wiview dashboard.

See [`docs/protocol.md`](docs/protocol.md) for the ADR-018 wire format.

## Hardware Requirements

- **M5Stack Cardputer-Adv** (Stamp-S3A / ESP32-S3FN8) — the supported, tested target
  - 8 MB flash, no PSRAM
  - BMI270 IMU, ES8311 audio, ST7789V2 display
- **Original Cardputer:** *untested.* The radio-based sensing (presence/motion/
  breathing) should work, but the build targets the Adv and the device-motion
  gate uses the BMI270, which the original lacks — expect to need small changes.
- USB-C cable for flashing (the device runs untethered on its battery afterward)

## Repository

```
wiview/
├── firmware/src/
│   ├── main.cpp          # Entry point
│   ├── csi/              # ESP32 CSI capture + packet generator
│   ├── dsp/              # Preprocessing, calibration, presence, motion, breathing
│   ├── sensors/          # BMI270 IMU gate
│   ├── net/              # WiFi, NVS provisioning, UDP streamer
│   ├── ui/               # Home, waterfall, breathing screens
│   └── app/              # App controller + state machine
├── test/test_dsp/        # 27 native unit tests (PlatformIO native env)
├── host/                 # Python companion (UDP receiver + live plot)
├── docs/                 # Flashing guide, architecture, protocol spec
└── .github/workflows/    # CI: build + tests + release-on-tag
```

## Honest Claims

wiview is a **research-grade sensing prototype**, not a medical device or
security product. The algorithms work under controlled conditions:
- Presence detection requires a relatively static RF environment
- Breathing detection requires a still subject within 2–3 meters
- CSI quality depends on WiFi channel congestion and AP behavior
- The IMU gate suppresses false triggers from device movement, not
  all environmental artifacts

All outputs are labeled as **estimates**. No accuracy guarantees are made.
Always validate against ground truth for your specific environment.

## Credits

This project is inspired by [RuView / WiFi-DensePose](https://github.com/ruvnet/RuView)
by [@ruvnet](https://github.com/ruvnet), an open-source ESP32-S3 WiFi sensing
platform (MIT licensed). wiview is an independent, clean-room implementation
that shares the same algorithms and engineering culture but targets the
M5Stack Cardputer form factor with an Arduino + PlatformIO stack.

## License

MIT — see [LICENSE](LICENSE).
