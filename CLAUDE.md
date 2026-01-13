# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build

ESP-IDF is at `/home/niko/esp/esp-idf`. Source the export script first:

```bash
source /home/niko/esp/esp-idf/export.sh
```

**Build for hardware:**
```bash
idf.py set-target esp32p4  # once per workspace
idf.py build
idf.py flash
idf.py monitor
```

**Build for Wokwi simulation:**
```bash
idf.py -B build-wokwi -D SDKCONFIG_DEFAULTS="sdkconfig.defaults;sdkconfig.wokwi" build
wokwi-cli .
```

**Tests:**
```bash
idf.py --test-component all
```

## Architecture

Scribe is firmware for a distraction-free writing device on ESP32-P4 + M5Tab5 hardware.

### Task Model (FreeRTOS)

- **ui_task**: Owns LVGL and EditorCore. Processes queued input events, updates editor, renders UI. LVGL calls ONLY here.
- **input_task**: USB HID host keyboard read → KeyEvent → ui_queue.
- **storage_task**: Receives save requests, writes to SD atomically (tmp + rename), never blocks UI.
- **main**: Initializes all services, then loops with idle checks.

Key queues: `g_event_queue` (ui events), `g_storage_queue` (save requests).

### Component Ownership

| Component | Responsibility | Dependencies |
|-----------|---------------|--------------|
| `scribe_editor` | Piece table editor core, undo/redo, selection. Pure logic, no LVGL/IO. | None |
| `scribe_ui` | LVGL screens (editor, menu, settings, etc.), TextView renderer, HUD, toasts. | lvgl, scribe_editor |
| `scribe_input` | USB HID keyboard → normalized KeyEvent, keybinding dispatcher. | usb/hid |
| `scribe_storage` | SD card mount, autosave (atomic tmp+rename), snapshots, recovery journal. | sdcard, fatfs |
| `scribe_export` | USB HID device mode ("Send to Computer"), SD export helpers. | tinyusb |
| `scribe_services` | Power, battery, RTC, WiFi (ESP32-C6), AI (OpenAI streaming), audio, IMU. | Various |
| `scribe_secrets` | NVS token/key storage with redaction support. | nvs |
| `scribe_utils` | String helpers, localization (Strings from `assets/strings/en.json`). | json |
| `scribe_hw` | M5Tab5 I2C devices (INA226, RX8130), IO expander. | i2c |

### Data Flow

1. USB key press → input_task → KeyEvent → g_event_queue
2. ui_task receives KeyEvent → KeybindingDispatcher → EditorCore mutation → UIApp.updateEditor() → LVGL invalidate
3. On edit (or 5s idle): EditorCore.createSnapshot() → StorageRequest → g_storage_queue
4. storage_task writes autosave.tmp → atomic rename → updates library.json + session state

### Important Design Constraints

- **LVGL threading**: All LVGL calls must happen on ui_task. Never call LVGL from storage_task or input_task.
- **Non-blocking I/O**: Storage never blocks UI. Use queues for cross-task communication.
- **Piece table snapshots**: EditorSnapshot copies only piece list (not buffers), enabling safe concurrent saves.
- **Recovery journal**: `/Scribe/{project}/journal/*.rec` files record edits for crash recovery.

### Storage Layout

```
/Scribe/
  library.json          # project list, last_opened
  settings.json         # theme, font size, wifi, AI config
  Projects/{id}/
    project.json        # metadata, backup state
    manuscript.md       # main doc
    autosave.tmp        # atomic write target
    journal/*.rec       # recovery journal
    snapshots/*.md.~N   # rotated snapshots
  Archive/{id}/...      # soft-deleted projects
```

### Key Files

- `main/app_main.cpp`: Entry point, task creation, keybinding callback wiring.
- `components/scribe_ui/ui_app.cpp`: Screen manager, LVGL lifecycle.
- `components/scribe_editor/piece_table.cpp`: Core data structure.
- `components/scribe_storage/autosave.cpp`: Atomic save logic.
- `SPECS.md`: Product spec (UX copy, keybindings, data model). Use for strings/behavior questions.

### Localization

UI strings in `assets/strings/en.json`. Use `Strings::getInstance().get("key.id")`. Overrideable at `/sdcard/Scribe/strings/en.json`.

### Wokwi Simulation

- `wokwi.toml`: Simulator config (RFC2217 serial on port 4000).
- `diagram.json`: ESP32-P4 + ILI9341 SPI TFT circuit.
- `sdkconfig.wokwi`: Overrides display pin mapping in `scribe_ui/ui_app.cpp`.
- Build with `-B build-wokwi -D SDKCONFIG_DEFAULTS="sdkconfig.defaults;sdkconfig.wokwi"`.

When display is blank in Wokwi: verify ESP32-P4 pin labels match `diagram.json`. Wokwi's ESP32-P4 support is evolving.

### Configuration

- `sdkconfig.defaults`: Base ESP32-P4 config.
- `sdkconfig.wokwi`: Wokwi display override.
- `partitions.csv`: 12MB factory, 3MB storage (for projects).
- `dependencies.lock`: Component manager locks (do NOT hand-edit).

### Common Patterns

**Creating a new screen:**
1. Add `screen_*.{cpp,h}` to `scribe_ui/ui_screens/`.
2. Register in `UIApp::showScreenName()` and `ui_app.h` enum.
3. Wire in keybinding callback in `app_main.cpp`.
4. Add strings to `en.json` per SPECS.md.

**Adding a component dependency:**
Edit component's `CMakeLists.txt` `REQUIRES` or `PRIV_REQUIRES` list.

**Threading rule**: If you need to update UI from a non-ui_task context, send an Event to `g_event_queue` and handle in ui_task's event loop.
