# ESP32-S3 Build (ESP-IDF)

The platform layer for ESP32-S3 drives a 128x64 SSD1306 OLED display over I2C.
The full math engine runs on the Xtensa LX7 dual-core processor.

## Prerequisites

- ESP-IDF v5.x installed and sourced
- `xtensa-esp32s3-elf-gcc` toolchain

## Wiring (SSD1306)

| SSD1306 Pin | ESP32-S3 Pin |
|---|---|
| VCC | 3.3V |
| GND | GND |
| SCL | GPIO 9 |
| SDA | GPIO 8 |

## Build and Flash

```bash
idf.py set-target esp32s3
idf.py build
idf.py -p /dev/ttyUSB0 flash monitor
```

## Display Layout

The OLED renders in 8x16 font (8 chars per row, 4 rows visible):
- Row 0: expression input
- Row 1-2: result
- Row 3: error / status flags

Scroll up with the side button to see history (last 16 entries).
