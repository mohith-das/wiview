# Flashing wiview to the Cardputer-Adv

## Prerequisites

- [PlatformIO](https://platformio.org/install) (`pip install platformio`)
- USB-C cable
- macOS, Linux, or Windows

## Connection

| OS      | Port                   |
|---------|------------------------|
| macOS   | `/dev/cu.usbmodem*`    |
| Linux   | `/dev/ttyACM*`         |
| Windows | `COMx`                 |

### Entering Download Mode (Cardputer-Adv)

1. Turn the power switch **OFF**
2. Hold the **G0** button (left-side button behind the screen)
3. Turn the power switch **ON** while holding G0
4. Release G0
5. The device is now in download mode

## Build & Flash

```bash
# Build
pio run -e cardputer

# Flash
pio run -e cardputer -t upload

# Monitor serial output
pio run -e cardputer -t monitor
```

## Troubleshooting

- **Port not found:** Ensure the device is in download mode (see above).
- **Flash fails:** Try a different USB cable or port.
- **Screen blank after flash:** Press the reset button or cycle power.
