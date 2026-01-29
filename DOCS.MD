# Scribe Documentation

This is a living document that evolves with the firmware. It is written for both
users (how to use the device) and developers (how it works and where to change it).
For local agent workflow rules, see `AGENTS.md`.

## Table of Contents
1. Introduction
2. User Guide
   2.1 Editor Basics
   2.1.1 Editor Shortcuts (Reference)
   2.2 Projects
   2.3 Settings
   2.4 Wi-Fi
   2.5 Export
   2.6 Send to Computer (USB HID)
   2.7 GitHub Backup
   2.8 Help + Diagnostics
   2.9 Recovery + First Run
   2.10 Advanced
3. Developer Guide
   3.1 Build + Flash (PowerShell)
   3.2 AI Prompt + Magic Bar Flow (Jan 2026)
   3.3 Warnings + LVGL API Fixes
   3.4 UI and Settings Fixes (Jan 2026)
   3.5 Editor Font Pipeline + Text Rendering (Jan 2026)
   3.6 Codebase Map (Full Coverage)
   3.7 Strings + Localization
   3.8 Power, Audio, and IMU Services
4. Chapter 1: USB Storage + .env Import

## 1. Introduction
Scribe is a distraction-free writing device built for the M5Stack Tab5.
This document captures the current behavior and implementation details
so onboarding and handoff are easy.

## 2. User Guide
This section collects end-user flows and UI behaviors.

### 2.1 Editor Basics
- The editor screen is the default writing surface with a TextView-backed document.
- Cursor movement, selection, undo/redo, and find are handled through keybindings.
- The Magic Bar can insert AI results into the document (preview only).
- Find/search opens an overlay and highlights matches.

Key files:
- Editor UI: `components/scribe_ui/ui_screens/screen_editor.cpp`
- TextView widget: `components/scribe_ui/widgets/text_view.cpp`
- Keybindings: `components/scribe_input/keybinding.cpp`
- Selection model: `components/scribe_editor/selection.cpp`
- Editor core: `components/scribe_editor/editor_core.cpp`
- Find screen: `components/scribe_ui/ui_screens/screen_find.cpp`

### 2.1.1 Editor Shortcuts (Reference)
- Canonical shortcut list: `SPECS.md` (see the shortcuts section).
- Keep this doc in sync with `SPECS.md` when keybindings change.

Common shortcuts (see `components/scribe_input/keybinding.cpp`):
- Find: Ctrl+F
- Undo: Ctrl+Z
- Redo: Ctrl+Shift+Z
- AI prompt: Meta/Cmd/Win+P
- Brightness: Ctrl+Shift+Up/Down
- Export: Ctrl+E
- Projects: Ctrl+P
- Settings: Ctrl+,

### 2.2 Projects
- Projects screen lists available documents and supports rename/delete (archive).
- New Project flow prompts for name and creates a new document entry.

Key files:
- Projects screen: `components/scribe_ui/ui_screens/screen_projects.cpp`
- New Project screen: `components/scribe_ui/ui_screens/screen_new_project.cpp`
- Project library: `components/scribe_storage/project_library.cpp`

### 2.3 Settings
- Settings screen controls editor preferences, keyboard layout, and device options.
- Margin size, font size, and brightness are applied immediately to the editor.

Key files:
- Settings screen: `components/scribe_ui/ui_screens/screen_settings.cpp`
- Settings storage: `components/scribe_storage/settings_store.cpp`
- Editor screen application: `components/scribe_ui/ui_screens/screen_editor.cpp`

### 2.4 Wi-Fi
- Wi-Fi screen scans for networks, prompts for credentials, and shows status.
- Connect flow uses the Wi-Fi manager to start/stop connections.

Key files:
- Wi-Fi screen: `components/scribe_ui/ui_screens/screen_wifi.cpp`
- Wi-Fi manager: `components/scribe_services/wifi_manager.cpp`
- UI wiring: `components/scribe_ui/ui_app.cpp`

### 2.5 Export
- Export screen lets you export documents to SD card and list/delete exports.
- Exports are written into the Scribe exports directory on the SD card.

Key files:
- Export screen: `components/scribe_ui/ui_screens/screen_export.cpp`
- Export logic: `components/scribe_export/export_sd.cpp`
- Storage paths: `components/scribe_storage/storage_manager.cpp`

### 2.6 Send to Computer (USB HID)
- Send to Computer types the current document over USB HID to the host.
- The mode uses a HID-only descriptor distinct from USB storage mode.

Key files:
- HID typing flow: `components/scribe_export/send_to_computer.cpp`
- HID device setup: `components/scribe_export/usb_hid_device.cpp`
- Export screen entry point: `components/scribe_ui/ui_screens/screen_export.cpp`

### 2.7 GitHub Backup
- Backup screen queues uploads to GitHub/Gist using stored tokens.
- Tokens are imported from `.env` or stored via secrets.

Key files:
- Backup screen: `components/scribe_ui/ui_screens/screen_backup.cpp`
- Backup service: `components/scribe_services/github_backup.cpp`
- Token import: `components/scribe_services/env_importer.cpp`
- Secrets store: `components/scribe_secrets/secrets_nvs.cpp`

### 2.8 Help + Diagnostics
- Help screen surfaces usage tips and shortcuts.
- Diagnostics screen shows system status and device information.

Key files:
- Help screen: `components/scribe_ui/ui_screens/screen_help.cpp`
- Diagnostics screen: `components/scribe_ui/ui_screens/screen_diagnostics.cpp`

### 2.9 Recovery + First Run
- Recovery screen offers repair actions and recovery options after failures.
- First Run screen introduces initial setup steps on fresh devices.

Key files:
- Recovery screen: `components/scribe_ui/ui_screens/screen_recovery.cpp`
- Recovery logic: `components/scribe_storage/recovery.cpp`
- First Run screen: `components/scribe_ui/ui_screens/screen_first_run.cpp`

### 2.10 Advanced
- Advanced screen exposes USB storage, diagnostics, and device-level controls.

Key files:
- Advanced screen: `components/scribe_ui/ui_screens/screen_advanced.cpp`
- USB storage screen: `components/scribe_ui/ui_screens/screen_usb_storage.cpp`

## 3. Developer Guide
This section collects system architecture notes, key files, and config.

### 3.1 Build + Flash (PowerShell)
Use the ESP-IDF environment described in `LESSONS_LEARNED.md`. The following pattern
is the known-good flow on this machine:

```powershell
$python="C:\Users\korisnik\.espressif\python_env\idf5.5_py3.11_env\Scripts\python.exe"
$idf_path="C:\esp\v5.5.2\esp-idf"
$idf_exports=& $python "$idf_path\tools\activate.py" --export
. $idf_exports
idf.py -p COM5 build flash
```

If COM5 is busy, stop any `idf.py monitor` sessions and retry.

### 3.2 AI Prompt + Magic Bar Flow (Jan 2026)
Refactor summary:
- Removed the always-visible AI input field from the Magic Bar UI.
- Added a centered AI prompt popup that appears on Cmd+P / Win+P.
- Prompt submission sends: `PROMPT`, newline, `# SELECTED LINE OF TEXT`, newline,
  `# FULL PAGE FOR CONTEXT`.
- If no selection exists, the "selected line" is the line at the cursor.
- AI responses stream into the Magic Bar preview; Enter inserts, Esc discards.

Key behavior details:
- Key combo: Meta/Cmd/Win + P opens the prompt (AI must be enabled).
- Prompt UI handles: Enter to send, Esc to cancel, Backspace to delete, printable input.
- Magic Bar is now preview-only (no input field or style selector).
- AI settings copy updated to reflect sending prompt + selection + full page.

Key files:
- Prompt UI: `components/scribe_ui/ui_screens/screen_ai_prompt.cpp`
- Prompt UI header: `components/scribe_ui/ui_screens/screen_ai_prompt.h`
- Prompt wiring + payload building: `components/scribe_ui/ui_app.cpp`
- Key event meta handling: `components/scribe_input/key_event.h`
- USB HID meta modifier mapping: `components/scribe_input/keyboard_host.cpp`
- Keybinding update: `components/scribe_input/keybinding.cpp`
- Magic Bar preview refactor: `components/scribe_ui/ui_screens/screen_magic_bar.cpp`
- Strings: `assets/strings/en.json`
- Shortcut docs: `SPECS.md`

### 3.3 Warnings + LVGL API Fixes
- TinyTTF font caches are now compiled only when `LV_USE_TINY_TTF` is enabled.
- Updated LVGL spinner usage to match current API: create first, then set params.
- Storage request initialization switched to explicit assignment to avoid build errors.

Key files:
- Fonts: `components/scribe_ui/theme/fonts.cpp`
- Spinner: `components/scribe_ui/ui_screens/screen_storage.cpp`
- Storage request init: `main/app_main.cpp`

### 3.4 UI and Settings Fixes (Jan 2026)
User-facing updates and the related code touched in this pass:
- Cursor drift fix: TextView now uses LVGL text measurement and wrapping to keep the caret aligned with proportional fonts and spaces.
- Editor margins: added three margin sizes in Settings and applied via TextView insets.
- Brightness setting + shortcut: settings entry plus Ctrl+Shift+Up/Down adjustment; brightness reapplies on wake.
- Font size picker first-open bug fixed; large font sizes now work by enabling TinyTTF.
- Recovery dialog buttons now respond to touch.
- Projects menu rename + delete (archive) shortcut; menu height now fits list content.
- HR keyboard layout enabled in Settings cycling (layout only; full UTF-8 input still pending).
- HUD "Saved" glyph replaced with plain text to avoid missing-glyph box.
- Wi-Fi UI now supports scan, list, password prompt, and connect status updates.
- Settings picker lists (font size/font family/etc.) now render immediately on first open; picker overlays force a layout refresh + invalidate after unhide to avoid the 1px line state.
- Editor viewport reflows on orientation change; the editor layout is resized after display rotation updates.

Key files:
- TextView + cursor metrics: `components/scribe_ui/widgets/text_view.cpp`
- Editor layout + margins: `components/scribe_ui/ui_screens/screen_editor.cpp`
- Settings UI: `components/scribe_ui/ui_screens/screen_settings.cpp`
- Settings storage: `components/scribe_storage/settings_store.h`
- HUD/toast strings: `assets/strings/en.json`
- Recovery dialog: `components/scribe_ui/ui_screens/screen_recovery.cpp`
- Projects/menu: `components/scribe_ui/ui_screens/screen_projects.cpp`, `components/scribe_ui/ui_screens/screen_menu.cpp`
- Wi-Fi screen + wiring: `components/scribe_ui/ui_screens/screen_wifi.cpp`, `components/scribe_ui/ui_app.cpp`

### 3.5 Editor Font Pipeline + Text Rendering (Jan 2026)
What changed in this pass:
- TinyTTF remains disabled for editor fonts (stability on ESP32-P4). Editor fonts now use precompiled LVGL fonts.
- Added a generator script to precompile fonts with `lv_font_conv` and include them in the build.
- Fixed TextView snapshot rendering: draw tasks outlive the callback, so text must be backed by stable storage (cache the snapshot text).

Generator script:
- `tools/generate_editor_fonts.ps1`
- Default sizes: 14,16,18,20,22,24,28,32,36,40,44,48,56,64,72
- Default range: ASCII 0x20-0x7F
- Outputs to `components/scribe_ui/theme/generated/`
- Current command used:

```powershell
powershell -ExecutionPolicy Bypass -File tools\generate_editor_fonts.ps1
```

Important LVGL font note:
- `lv_font_conv` compresses bitmaps by default, but `CONFIG_LV_USE_FONT_COMPRESSED` is not enabled in this project.
- Fonts were regenerated with `--no-compress` in the generator script to ensure text renders.
- Alternative: enable `CONFIG_LV_USE_FONT_COMPRESSED=y` if you want compressed fonts.

Key files touched:
- Font selection and TinyTTF toggle: `components/scribe_ui/theme/fonts.cpp`
- Precompiled font headers and manifest: `components/scribe_ui/theme/generated/`
- Font generator: `tools/generate_editor_fonts.ps1`
- TextView draw cache for snapshot text: `components/scribe_ui/widgets/text_view.cpp`

### 3.6 Codebase Map (Full Coverage)
This section is a coverage map of the current codebase. Each area lists the main
responsibilities and the core files to start with.

#### 3.6.1 Application Entry + Tasks
- `main/app_main.cpp`:
  - Initializes NVS, storage, services, UI, editor, and task loops.
  - Owns the event queue (`g_event_queue`) and storage queue.
  - Runs three tasks: UI task (LVGL + editor), input task (keyboard), storage task (autosave + format).
- `main/app_event.h`: Central event types and payloads for inter-task routing.
- `main/app_queue.h` + `main/app_queue.cpp`: Legacy queue helpers (not used by current `app_main` flow).
- `main/app_build_info.h`: Build version macros embedded at compile time.

#### 3.6.2 Editor Core (Text Model)
- `components/scribe_editor/editor_core.*`: Cursor, selection, edit operations, undo/redo, find, snapshots.
- `components/scribe_editor/piece_table.*`: Piece-table backing store and snapshots.
- `components/scribe_editor/undo_stack.*`: Undo/redo stack with text ops.
- `components/scribe_editor/selection.*`: Selection range utilities.
- Tests: `test/test_editor_core.cpp`, `test/test_piece_table.cpp`, `test/test_undo_stack.cpp`.

#### 3.6.3 Storage + Recovery
- `components/scribe_storage/storage_manager.*`:
  - SD card mount/unmount, directory setup, format + verify, USB MSC handoff.
  - Defines Scribe path constants (Projects/Archive/Exports/etc).
- `components/scribe_storage/project_library.*`: `library.json` persistence, project list + metadata.
- `components/scribe_storage/autosave.*`: Background saves, snapshot rotation, atomic save path.
- `components/scribe_storage/session_state.*`: Cursor + scroll persistence per project.
- `components/scribe_storage/recovery.*`: Journaled recovery and autosave temp recovery.

#### 3.6.4 UI System (LVGL)
- `components/scribe_ui/ui_app.*`: Screen routing, editor binding, async queues, and app-level UI state.
- `components/scribe_ui/display_driver.*`: LVGL display setup, flush callback, backlight control.
- `components/scribe_ui/touch_controller.*`: Touch controller auto-detect (GT911/ST7123).
- `components/scribe_ui/theme/*`: Theme colors, fonts, and generated font assets.
- `components/scribe_ui/widgets/*`: TextView, HUD overlay, and toast UI elements.
- `components/scribe_ui/ui_screens/*`: Screen implementations (Editor, Menu, Projects, Settings, Export, AI, etc).
- `components/scribe_ui/Kconfig`: UI-related build-time settings.

#### 3.6.5 Input + Keyboard
- `components/scribe_input/key_event.*`: Normalized key event model + HID mapping.
- `components/scribe_input/keymap.*`: Layout mapping (e.g., HR layout support).
- `components/scribe_input/keybinding.*`: High-level shortcut mapping to app actions.
- `components/scribe_input/keyboard_host.*`: USB host keyboard driver.
- `components/scribe_input/keyboard_host_control.*`: USB host start/stop control.
- `components/scribe_input/space_hold_detector.*`: F1/HUD emulation via Space hold.

#### 3.6.6 Services (AI, Wi-Fi, Backup, Power, Audio, IMU)
- `components/scribe_services/ai_assist.*`: OpenAI Responses API streaming, prompt building, cancelation.
- `components/scribe_services/env_importer.*`: `.env` parsing for AI + GitHub tokens.
- `components/scribe_services/wifi_manager.*`: Wi-Fi scan/connect (local or remote ESP-WiFi-Remote).
- `components/scribe_services/github_backup.*`: GitHub/Gist backup queueing + conflict handling.
- `components/scribe_services/power_manager.*`: Sleep/awake, auto-sleep, battery callbacks.
- `components/scribe_services/battery.*`: Battery percent/charging status via INA226.
- `components/scribe_services/rtc_time.*`: SNTP + RTC time sync utilities.
- `components/scribe_services/audio_manager.*`: ES8388 speaker + ES7210 mic support.
- `components/scribe_services/audio_feedback.*`: Simple UI audio feedback cues.
- `components/scribe_services/imu_manager.*`: BMI270 IMU init + polling.

#### 3.6.7 Export + USB Device Modes
- `components/scribe_export/export_sd.*`: Save document exports to SD and list/delete exports.
- `components/scribe_export/send_to_computer.*`: USB HID "type out document" mode.
- `components/scribe_export/usb_hid_device.*`: USB HID device descriptors + init/deinit.
- `components/scribe_export/usb_msc_device.*`: USB mass storage device mode and callbacks.

#### 3.6.8 Hardware Abstractions
- `components/scribe_hw/tab5_i2c.*`: Shared I2C bus + device helpers.
- `components/scribe_hw/tab5_io_expander.*`: IO expander control for Tab5 peripherals.
- `components/scribe_hw/tab5_ina226.*`: INA226 power monitor interface.
- `components/scribe_hw/tab5_rx8130.*`: RX8130 RTC access.
- `components/sensor_bmi270/*`: Vendor BMI270 driver sources and headers.

#### 3.6.9 Secrets + Utilities
- `components/scribe_secrets/secrets_nvs.*`: NVS-backed storage for tokens/keys.
- `components/scribe_utils/strings.*`: String table loader for `assets/strings/en.json`.
- `components/scribe_utils/string_utils.h`: Small string helpers (trim, etc).

#### 3.6.10 Assets + Tooling
- `assets/strings/en.json`: UI copy and user-facing strings.
- `assets/fonts/*`: Source fonts for generator script.
- `components/scribe_ui/theme/generated/*`: Precompiled LVGL fonts.
- `tools/generate_editor_fonts.ps1`: Font build pipeline wrapper.
- `image.c`: LVGL image asset blob used in UI (generated).

#### 3.6.11 Build, Config, and Integration
- `CMakeLists.txt`: Project name and version (`PROJECT_VER`).
- `main/CMakeLists.txt` + component `CMakeLists.txt`: ESP-IDF component wiring.
- `sdkconfig.defaults` + `sdkconfig`: Build-time config defaults and current config.
- `partitions.csv`: Partition layout for flash.
- `wokwi.toml` + `sdkconfig.wokwi`: Wokwi simulation config.
- `dependencies.lock`: Managed component lockfile.

#### 3.6.12 Tests + CI + Meta
- Unit tests live in `test/` (CMake + Makefile included).
- Repo meta: `README.md`, `SPECS.md`, `DEVICE-SPECS.md`, `TODO.md`,
  `CONTRIBUTING.md`, `SECURITY.md`, `CODE_OF_CONDUCT.md`, `LICENSE`.
- Logs and local build output: `build/`, `idf_*_*.log` (local artifacts).

#### 3.6.13 Managed Components and Vendor Code
- `managed_components/`: ESP-IDF managed components (3rd-party).
  Keep edits here minimal; prefer upstream updates if fixes are needed.
- `M5Tab5-UserDemo/`: Upstream sample/demo code for Tab5 reference.

### 3.7 Strings + Localization
- UI strings live in `assets/strings/en.json`.
- The string table is loaded at runtime and accessed by UI screens.
- Update string keys in the JSON and the referring screen code together.

Key files:
- String table: `assets/strings/en.json`
- String loader: `components/scribe_utils/strings.cpp`
- String API: `components/scribe_utils/strings.h`

### 3.8 Power, Audio, and IMU Services
- Power manager handles sleep/wake, auto-sleep, and backlight reapply.
- Audio manager initializes codecs and handles feedback tones.
- IMU manager wraps BMI270 init and polling for motion events.

Key files:
- Power manager: `components/scribe_services/power_manager.cpp`
- Audio manager: `components/scribe_services/audio_manager.cpp`
- Audio feedback: `components/scribe_services/audio_feedback.cpp`
- IMU manager: `components/scribe_services/imu_manager.cpp`
- BMI270 driver: `components/sensor_bmi270/`
## 4. Chapter 1: USB Storage + .env Import

### 4.1 User Flow
1. Go to Settings -> Advanced -> USB storage.
2. Scribe unmounts `/sdcard`, starts USB MSC, and shows the USB storage screen.
3. Connect USB-C to your PC; the SD card appears as a removable drive.
4. Copy files as needed. Optionally place a `.env` file in `/Scribe` on the SD card.
5. Press Esc to exit, or unplug the USB cable to auto-exit.
6. Scribe remounts the SD card and imports keys from `/sdcard/Scribe/.env`.

### 4.2 .env Import (User and Dev)
- Path: `/sdcard/Scribe/.env`
- Behavior: file is read-only; it is parsed but never altered.
- Supported keys:
  - `OPENAI_API_KEY` -> stored via `AIAssist::setAPIKey`
  - `GITHUB_TOKEN` or `GITHUB_API_TOKEN` -> stored via `GitHubBackup::setToken`
- Parsing notes: ignores blank lines and `#` comments; supports optional
  `export KEY=...`; trims whitespace; strips surrounding single/double quotes.

### 4.3 Safety and Handoff Behavior
- While USB MSC is active:
  - Storage writes are suspended (autosave and manual save requests are dropped safely).
  - SD card is unmounted from the app before MSC starts.
  - Keyboard host is stopped; it restarts when MSC stops.
- On USB detach:
  - MSC stops, SD card remounts, keyboard host resumes.

### 4.4 Descriptor Handling
- HID device uses a custom HID-only device descriptor.
- MSC mode uses a custom MSC-only device descriptor.
- This avoids PID/interface mismatches when switching between HID and MSC.

### 4.5 Config Updates
- `CONFIG_TINYUSB_MSC_ENABLED=y`
- `CONFIG_TINYUSB_DESC_MSC_STRING="Scribe USB Storage"`
- `CONFIG_TINYUSB_MSC_BUFSIZE=8192`

### 4.6 Key Files (Developer Reference)
- USB MSC device wrapper:
  - `components/scribe_export/usb_msc_device.cpp`
  - `components/scribe_export/usb_msc_device.h`
- HID init/deinit:
  - `components/scribe_export/usb_hid_device.cpp`
  - `components/scribe_export/usb_hid_device.h`
- Storage suspend/remount helpers:
  - `components/scribe_storage/storage_manager.cpp`
  - `components/scribe_storage/storage_manager.h`
- .env importer:
  - `components/scribe_services/env_importer.cpp`
  - `components/scribe_services/env_importer.h`
- UI wiring + screen:
  - `components/scribe_ui/ui_app.cpp`
  - `components/scribe_ui/ui_app.h`
  - `components/scribe_ui/ui_screens/screen_usb_storage.cpp`
  - `components/scribe_ui/ui_screens/screen_usb_storage.h`
- Advanced menu entry:
  - `components/scribe_ui/ui_screens/screen_advanced.cpp`
- Events:
  - `main/app_event.h`
  - `main/app_main.cpp`
- Strings:
  - `assets/strings/en.json`
- Config:
  - `sdkconfig.defaults`
  - `sdkconfig`













