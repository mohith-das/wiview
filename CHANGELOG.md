# Changelog

All notable changes to the wiview project.

## [0.1.0] — Unreleased

### Added
- Phase 0: Repository bootstrap, hello-world for Cardputer-Adv
- Phase 1: ESP32 CSI capture, WiFi STA, packet generator, live CSI health display
- Phase 2: DSP pipeline (preprocessing, calibration, presence, motion), IMU gate
- Phase 3: Breathing detection, multi-screen UI (Home, Waterfall, Breathing)
- Phase 4: WiFi provisioning (NVS + keyboard), UDP streaming protocol, host companion
- Phase 5: Polished documentation, architecture guide, sensing notes, CI release workflow
- 25 native unit tests for all DSP modules
- PlatformIO CI (build cardputer + run native tests)
- MIT license

### Technical Notes
- Target hardware: M5Stack Cardputer-Adv (ESP32-S3FN8, no PSRAM, BMI270 IMU)
- M5Unified 0.2.17 + M5Cardputer 1.1.1
- Memory: ~15% RAM (50 KB / 320 KB), ~27% Flash (886 KB / 3.3 MB)
- Header-only DSP for host-testable signal processing
- Independent protocol (not RuView-compatible; bridge planned as stretch goal)
