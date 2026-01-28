# Lessons Learned (Scribe on M5Stack Tab5)

## Hardware + BSP
- Target board is M5Stack Tab5 (ESP32-P4 + ESP32-C6 module). Core resources: 16 MB Flash, 32 MB PSRAM.
- Display is a 5-inch 1280x720 IPS TFT driven over MIPI-DSI (Tab5 uses ST7123 integrated display/touch driver). Touch controller is GT911 over I2C.
- Audio path uses ES8388 + ES7210; camera is SC2356 over MIPI-CSI; BMI270 IMU; power via MP4560 + IP2326; RTC RX8130CE.
- BSP component in use: espp/m5stack-tab5 (v1.0.32). It centralizes display, touch, audio, camera, sensors, power, etc.

## Build Environment / Workflow
- ESP-IDF is installed at `C:\esp\v5.5.2\esp-idf`.
- Use the Python env under `C:\Users\korisnik\.espressif\python_env\idf5.5_py3.11_env\Scripts\python.exe` and `tools\activate.py` to export env vars before running `idf.py`.
- PowerShell pattern that consistently works:

```powershell
$python="C:\Users\korisnik\.espressif\python_env\idf5.5_py3.11_env\Scripts\python.exe"
$idf_path="C:\esp\v5.5.2\esp-idf"
$idf_exports=& $python "$idf_path\tools\activate.py" --export
. $idf_exports
idf.py build
```

- Alternate PowerShell pattern (also works on this machine) using `export.ps1`:

```powershell
$env:IDF_PATH="C:\esp\v5.5.2\esp-idf"
$env:PATH="C:\Users\korisnik\.espressif\python_env\idf5.5_py3.11_env\Scripts;$env:PATH"
& "$env:IDF_PATH\export.ps1"
idf.py -p COM5 build flash
```

## Flashing + Monitor
- Use `idf.py flash monitor` from the repo root.
- COM5 has been the correct port for the device here.
- If the flash fails with a port lock, close any other serial monitors, IDEs, or processes that are holding COM5.
- If the old binary is still running, verify the new build actually flashed by checking the monitor log (boot log will show app SHA) and comparing with the new build output.
- To find and free a stuck COM port on Windows, list processes with `Get-CimInstance Win32_Process | Where-Object { $_.CommandLine -match 'COM5' }` and terminate the stale `esp_idf_monitor`/`idf.py` python processes; then replug the device.

## Logging / Debugging Practices
- If `idf.py monitor` does not show logs, use a direct Python serial read (toggle DTR/RTS once) to capture the boot log and confirm runtime behavior.
- A store access fault inside LVGL drawing (`lv_memset` / `lv_draw_sw_fill`) often indicates a null draw buffer pointer. Added logging around `lv_display_get_buf_active()` helps confirm buffer pointer and size.
- For UI sizing issues, ensure LVGL objects have their layout updated before querying sizes; clamp viewports to valid positive sizes.
- If storage is not mounted, avoid auto-creating projects and handle `mkdir` / `fopen` errors explicitly; fail gracefully instead of crashing.

## Device Bring-up Notes
- Tab5 BSP and Scribe both need I2C. The BSP uses the new I2C driver. Share the same I2C bus by adopting the BSP bus handle rather than re-initializing I2C.
- Audio I2C must be on I2C0 to match the shared bus.
- Main task stack was increased to 8192 to avoid early stack issues.

## Useful Checks When Screen Is Black
- Confirm the new binary is running (boot log + app SHA).
- Verify LVGL draw buffer is non-null and has reasonable size.
- If still black, temporarily bypass DSI init to isolate display vs. app issues.

## Display Driver + LVGL Port Integration
- M5 notes that the Tab5 screen driver changed to ST7123 (from ILI9881C) starting Oct 14, 2025. Check the rear label to confirm the panel/driver and match the init sequence accordingly.
- ST7123 is an integrated display/touch driver over MIPI-DSI on Tab5; keep DSI init aligned with the BSP for this panel.
- When using `esp_lvgl_port` with DSI: software rotation (`sw_rotate=1`) allocates an extra rotation buffer; if internal RAM is tight, `lvgl_port_add_disp_dsi` can fail with “Not enough memory for LVGL buffer (rotation buffer) allocation!” (this matches current crash logs).
- DSI underrun errors like “can’t fetch data from external memory fast enough” can happen when LVGL draw buffers live in PSRAM; prefer internal DMA-capable RAM for DSI buffers when possible.
- Long-term direction: use `esp_lvgl_port` + BSP DSI handles, but avoid software rotation unless you can afford the extra buffer; rely on hardware rotation in `esp_lcd` instead.

## Session Notes (2026-01-28)
- Confirmed Tab5 panel/driver is ST7123 (MIPI-DSI).
- DSI stability: reducing DPI clock from 100 to 70 MHz in the BSP (`managed_components/espp__m5stack-tab5/src/video.cpp`) eliminated underrun/blank-screen behavior on this unit.
- LVGL + DSI buffers: stable config uses a small DMA-capable internal buffer (width * 50 lines), `sw_rotate=1`, `double_buffer=false`, and no PSRAM for the draw buffer.
- Rotation handling: IMU orientation mapping was inverted on this device; swap portrait/landscape mapping and re-evaluate periodically (250 ms) to get correct physical rotation.
- Never revert auto-rotation to `abs_x > abs_y => LANDSCAPE`. That mapping is wrong on this unit; keep the swapped mapping.
- UI strings: embed `assets/strings/en.json` via CMake `EMBED_TXTFILES` as a fallback so the UI has strings even if SD is missing.
- SD card: FATFS long file names must be enabled (LFN heap + max 255) or writes like `library.json` fail with `EINVAL`.
- Storage: call a shared `ensureDirectories()` before saving; log `errno` + `strerror` for `mkdir`/`fopen` to pinpoint failures quickly.
- Port lock: COM5 is frequently locked by lingering `idf.py monitor`; close it or kill the process before flashing.
- Boot log capture: when `idf.py monitor` is stuck, a minimal pyserial reader that toggles RTS once can capture the boot log reliably.
- ST7123 touch: reset timing matters. Using ~100 ms low/high on the touch reset line (GPIO 23 via IO expander) makes the 0x55 probe succeed; short 5 ms pulses failed. Avoid `esp_lcd_touch_exit_sleep` for ST7123 (not supported).
- USB keyboard (host): requires adding `espressif/usb_host_hid` (>=1.0.3) and using its HID host callbacks; `usb_host_install` + `usb_host_lib_handle_events` must run. Do not include TinyUSB HID headers in the same translation unit as `usb/hid_host.h` to avoid `hid_report_type_t` conflicts.
- USB keyboard debug: log HID VID/PID + subclass/proto on connect, accept HID keyboards even if not Boot subclass, and warn once on short (<8-byte) reports; handle `USB_HOST_LIB_EVENT_FLAGS_NO_CLIENTS/ALL_FREE` in the USB host event loop.
- Menu navigation: HID arrow keys require mapping usages 0x4F-0x52 (right/left/down/up) plus home/end/page/insert; without this, ESC works but arrows don't. Menu list items also need LVGL click handlers on list buttons; otherwise touch taps on menu items do nothing.
- UI ghosting lines: avoid semi-transparent screen backgrounds (for menu/new-project/dialog screens). Use `LV_OPA_COVER` unless LVGL screen transparency is explicitly enabled; otherwise faint horizontal banding can appear on Tab5.
- Theme settings now use `theme_id` with a registry of named themes (Scribe, Dracula, Catppuccin, Solarized); settings load migrates legacy `dark_theme` to the closest match.
- Default theme is Dracula, backlight defaults to 50%, and font sizing now uses pixel sizes (12-28 in 2px steps).

