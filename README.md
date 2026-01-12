# Scribe - Distraction-Free Writing Device

Firmware for Tab5 dedicated writing device.

## Building

Requires ESP-IDF v5.0+:

```bash
idf.py build
idf.py flash
idf.py monitor
```

## Project Structure

- `main/` - Entry point and task initialization
- `components/scribe_ui/` - LVGL UI screens and widgets
- `components/scribe_input/` - USB HID keyboard input
- `components/scribe_editor/` - Piece table editor core
- `components/scribe_storage/` - SD card storage and autosave
- `components/scribe_export/` - "Send to Computer" USB device mode
- `components/scribe_services/` - Power, battery, time, Wi-Fi
- `components/scribe_secrets/` - NVS-based token/key storage
- `assets/` - String resources, fonts, icons

## Key Features

- Instant boot to last open document
- Piece table editor for efficient edits and safe background saves
- Autosave with atomic writes and snapshots
- "Send to Computer" - types document via USB HID
- Optional cloud backup and AI (opt-in, never intrusive)

## License

Apache License 2.0. See `LICENSE`.

## Contributing

See `CONTRIBUTING.md`.

## Security

See `SECURITY.md`.
