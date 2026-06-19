# wiview — WiFi Sensing on the M5Stack Cardputer-Adv

[![CI](https://github.com/mohith-das/wiview/actions/workflows/ci.yml/badge.svg)](https://github.com/mohith-das/wiview/actions/workflows/ci.yml)

A WiFi Channel State Information (CSI) sensing device that runs on the
M5Stack Cardputer-Adv (ESP32-S3FN8 / Stamp-S3A). Captures CSI on-device,
runs signal processing for presence/motion/breathing detection, and displays
results live on the built-in screen.

**Inspired by [RuView / WiFi-DensePose](https://github.com/ruvnet/RuView)** (MIT) —
an independent, clean-room implementation.

> ⚠️ **Early development.** Phase 0: hardware bootstrap. Not yet functional for sensing.

## Status

- [x] Phase 0 — Bootstrap (hello-world on Cardputer-Adv)
- [ ] Phase 1 — CSI capture
- [ ] Phase 2 — DSP core + presence
- [ ] Phase 3 — Breathing + UI
- [ ] Phase 4 — Streaming + host companion
- [ ] Phase 5 — Productize

## Quick Start

See [`docs/flashing.md`](docs/flashing.md) for flashing instructions.

## License

MIT — see [LICENSE](LICENSE).
