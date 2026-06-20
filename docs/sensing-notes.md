# wiview Sensing Notes

Practical guidance for getting reliable presence, motion, and breathing
detection from the Cardputer-Adv.

## Setup

1. **Placement matters.** Mount the Cardputer on a stable, non-metallic
   surface. The magnetic base helps. Avoid placing it near large metal
   objects, running microwaves, or active Bluetooth devices.

2. **WiFi channel.** Use a relatively uncongested 2.4 GHz channel.
   Channels 1, 6, and 11 are typical in the US. The CSI rate depends on
   AP responsiveness — some APs are "chattier" than others.

3. **Calibration.** Let the device calibrate for ~30 seconds in an empty
   room. The screen shows a progress percentage. Do not walk in front of
   the device during calibration.

4. **Recalibrate** when the RF environment changes significantly (people
   leaving/entering the room, furniture moved) by pressing `r`.

## Presence Detection

- **Works best** when a person walks between the Cardputer and the AP.
- **Range:** typically 2–3 meters in an open room. Walls reduce range.
- **Latency:** ~1–2 seconds for the z-score to cross the threshold.
- **False positives:** can occur from WiFi channel switching, AP
  reconfiguration, or large environmental changes. Recalibrate if needed.

## Motion Detection

- **Sensitive** to walking and arm movements within ~3 meters.
- The motion level is a continuous 0.0–1.0 value, not a binary trigger.
- Motion and presence can co-occur (person present + moving) or be
  independent (person present but still).

## Breathing Detection

- **Experimental.** Results vary significantly with subject position,
  distance, and RF environment.
- **Conditions for best results:**
  - Subject sits still within 2 meters of the device
  - Subject faces the device or is perpendicular to the WiFi path
  - The device is stationary (IMU gate reads STILL)
  - Calibration was performed in the empty room
- **Expected BPM range:** 6–30 (typical adult at rest: 12–20)
- **The CSI subcarrier used** matters — the system selects the carrier
  with highest variance, which varies by environment. This is automatic.
- **Validation:** compare against a manual count (count breaths for 30
  seconds, multiply by 2).

## IMU Gate

The BMI270 accelerometer and gyroscope detect when the device itself is
moved or bumped. When the device moves:
- Presence and motion outputs are **flagged but not suppressed** — the
  screen shows MOVING in red.
- Breathing BPM is **unreliable** while MOVING is shown.
- The gate resets within ~500 ms after the device becomes still again.

Thresholds (configurable in `imu_gate.h`):
- Acceleration deviation from 1g: 0.05g
- Gyroscope rate: 3.0 degrees/second

## Troubleshooting

| Symptom                     | Likely Cause                    | Fix                            |
|-----------------------------|---------------------------------|--------------------------------|
| CSI rate is 0 Hz            | Not connected to WiFi           | Re-enter WiFi credentials     |
| CSI rate is < 5 Hz          | AP not responding to UDP pings  | Try a different AP or channel |
| Always shows CLEAR          | Calibration failed              | Press `r` to recalibrate      |
| Always shows PRESENCE       | RF environment changed          | Press `r` to recalibrate      |
| BPM shows --                | Not enough data or device moving| Wait ~15s, ensure device still|
| Device keeps showing MOVING | Cardputer is vibrating          | Place on stable surface       |

## Advanced: Subcarrier Selection

wiview uses the mean amplitude across all usable subcarriers for
presence/motion. The breathing detector operates on the same signal.
For research purposes, individual subcarrier data is available via
the UDP streaming protocol (see [`protocol.md`](protocol.md)).
