# Scribe - Distraction-Free Writing Device

Firmware for the Tab5-based writing device. The focus is instant boot, safe autosave, and a minimal UI that keeps writers in flow.

## Highlights

- Instant boot to the last open document and cursor.
- Piece table editor for fast edits and safe background saves.
- Autosave with atomic writes and local snapshots.
- "Send to Computer" export via USB HID typing.
- Optional cloud backup and AI assistance (opt-in only).

## Requirements

- ESP-IDF 5.5+ with target `esp32p4`
- RISC-V toolchain (`idf_tools.py install riscv32-esp-elf`)
- Python 3 and ESP-IDF environment initialized

## Build (hardware)

```bash
# Set target once per workspace
idf.py set-target esp32p4

# Build / flash / monitor
idf.py build
idf.py flash
idf.py monitor
```

## Wokwi simulation (ESP32-P4)

This repo includes `wokwi.toml`, `diagram.json`, and `sdkconfig.wokwi` to run in Wokwi with an ILI9341 SPI TFT.

```bash
# Install Wokwi CLI (one-time)
curl -L https://wokwi.com/ci/install.sh | sh

# Set your token (do not commit this)
export WOKWI_CLI_TOKEN=...

# Build Wokwi firmware
idf.py -B build-wokwi -D SDKCONFIG_DEFAULTS="sdkconfig.defaults;sdkconfig.wokwi" build

# Run simulator
wokwi-cli .
```

Notes:
- `sdkconfig.wokwi` enables the Wokwi display pin mapping in `components/scribe_ui/ui_app.cpp`.
- If the display is blank, verify the ESP32-P4 pin labels in Wokwi and update `diagram.json` accordingly.
- Serial output is forwarded on RFC2217 port 4000 (see `wokwi.toml`).

## Configuration

- UI strings live in `assets/strings/en.json` and can be overridden at `/sdcard/Scribe/strings/en.json`.
- Product spec lives in `SPECS.md`.

## Project Structure

- `main/` - Entry point and task initialization
- `components/scribe_ui/` - LVGL UI screens and widgets
- `components/scribe_input/` - USB HID keyboard input
- `components/scribe_editor/` - Piece table editor core
- `components/scribe_storage/` - SD card storage and autosave
- `components/scribe_export/` - "Send to Computer" USB device mode
- `components/scribe_services/` - Power, battery, time, Wi-Fi, AI
- `components/scribe_secrets/` - NVS-based token/key storage
- `assets/` - String resources, fonts, icons

## License

Apache License 2.0. See `LICENSE`.

## Contributing

See `CONTRIBUTING.md`.

## Security

See `SECURITY.md`.
