# wiview Architecture

## System Overview

```
┌─────────────────────────────────────────────────────────┐
│                    Cardputer-Adv                         │
│                                                         │
│  ┌─────────┐   ┌──────────┐   ┌────────────────────┐   │
│  │  WiFi   │   │ CSI      │   │ DSP Pipeline       │   │
│  │  STA    │──▶│ Capture  │──▶│ Preprocess → Cal   │   │
│  │         │   │ (IRAM CB)│   │ → Presence → Motion │   │
│  └─────────┘   └──────────┘   │ → Breathing         │   │
│                               └─────────┬──────────┘   │
│                                         │               │
│  ┌─────────┐   ┌──────────┐   ┌────────▼──────────┐   │
│  │ BMI270  │──▶│ IMU Gate │──▶│ App Controller    │   │
│  │ IMU     │   │          │   │ (state machine)   │   │
│  └─────────┘   └──────────┘   └────────┬──────────┘   │
│                                         │               │
│                               ┌────────▼──────────┐   │
│                               │ UI Screens        │   │
│                               │ Home / Waterfall  │   │
│                               │ Breathing         │   │
│                               └────────┬──────────┘   │
│                                         │               │
│  ┌─────────┐                  ┌────────▼──────────┐   │
│  │ Keyboard│─────────────────▶│ Input Dispatch    │   │
│  └─────────┘                  └───────────────────┘   │
│                                                         │
│  ┌──────────────────────────────────────────────────┐   │
│  │ UDP Streamer ───────────────────────────▶ Host   │   │
│  └──────────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────┘
```

## Data Flow

1. **WiFi STA** connects to an access point. A **packet generator** sends
   UDP datagrams to the gateway at ~50 ms intervals, triggering AP responses.

2. **CSI Capture** registers an `esp_wifi_set_csi_rx_cb()` IRAM callback.
   Each received packet produces a `wifi_csi_info_t` containing raw I/Q
   subcarrier data. The callback copies data into a volatile ring buffer
   and increments a counter. No processing happens in the callback.

3. **DSP Pipeline** runs in the main loop (Arduino `loop()`):
   - `dsp_preprocess`: extracts amplitude (`sqrt(I²+Q²)`) and phase
     (`atan2(Q,I)`), unwraps phase, subtracts linear trend (CFO correction),
     and filters pilot/null subcarriers.
   - `dsp_calibration`: Welford online algorithm computes running mean and
     variance over ~600 samples (~30 seconds). This establishes the
     "empty room" baseline.
   - `dsp_presence`: compares current amplitude variance to baseline via
     z-score. A z-score > 3.0 triggers presence detection.
   - `dsp_motion`: 32-sample ring buffer temporal variance → normalized
     motion level 0.0–1.0.
   - `dsp_breathing`: DC removal + moving average smoothing → zero-crossing
     rate → EMA-smoothed BPM estimate (4–35 range).

4. **IMU Gate** reads the BMI270 accelerometer/gyroscope. If the device's
   acceleration deviates from 1g by > 0.05g or angular rate exceeds 3 dps,
   the device is marked "moving" and all CSI inferences are flagged as
   potentially unreliable.

5. **App Controller** aggregates all sensor data into a `SensorData` struct
   and routes it to the active UI screen. It also handles keyboard input
   dispatch and view switching.

6. **UDP Streamer** (optional, toggled with `s` key) sends CSI raw packets
   (binary, type 0x00) and inference packets (JSON, type 0x01) to a host
   on port 5005.

## CSI Primer

**Channel State Information (CSI)** describes how a WiFi signal propagates
from transmitter to receiver. Unlike RSSI (a single number), CSI captures
the amplitude and phase of each OFDM subcarrier — typically 52 data
subcarriers for 20 MHz 802.11n.

When a person moves through the WiFi field, they scatter and absorb the
signal, changing the CSI at the receiver. These changes are small but
detectable with statistical analysis.

The ESP32-S3's WiFi radio can be configured to report CSI for every
received packet. The LLTF (Legacy Long Training Field) mode provides
one complex sample (I + Q) per subcarrier per packet.

## Memory Budget

| Component          | RAM Usage       |
|-------------------|-----------------|
| Arduino + WiFi    | ~22 KB          |
| M5GFX framebuffer | ~65 KB (240×135×16bpp) |
| DSP state          | ~3 KB           |
| CSI ring buffer    | ~2 KB           |
| UI screens         | ~2 KB           |
| FreeRTOS overhead  | ~10 KB          |
| **Total**          | **~50 KB / 320 KB (15%)** |

No PSRAM is required — the entire firmware runs in the ESP32-S3's 512 KB
internal SRAM.

## Testing Strategy

All DSP algorithms are **header-only** and compile on the host via
PlatformIO's `native` environment. Unit tests use synthetic CSI data
and the Unity test framework. 25 tests cover:
- Preprocessing (amplitude, phase, unwrap, detrend)
- Calibration (Welford convergence, reset)
- Presence (z-score threshold, unready calibrator)
- Motion (temporal variance)
- Breathing (sine wave at 12 BPM, constant signal, reset)
- IMU gate (still, bumped, tilted, threshold configuration)
