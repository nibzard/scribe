# Build + Flash (Windows / PowerShell)

These steps are derived from `AGENTS.md` and `LESSONS_LEARNED.md` for this repo.

## Prereqs
- ESP-IDF installed at `C:\esp\v5.5.2\esp-idf`.
- Python env at `C:\Users\korisnik\.espressif\python_env\idf5.5_py3.11_env\Scripts\python.exe`.
- Device connected (COM5 has been the correct port on this machine).

## Export the ESP-IDF environment

### Option A (activate.py)
```powershell
$python="C:\Users\korisnik\.espressif\python_env\idf5.5_py3.11_env\Scripts\python.exe"
$idf_path="C:\esp\v5.5.2\esp-idf"
$idf_exports=& $python "$idf_path\tools\activate.py" --export
. $idf_exports
```

### Option B (export.ps1)
```powershell
$env:IDF_PATH="C:\esp\v5.5.2\esp-idf"
$env:PATH="C:\Users\korisnik\.espressif\python_env\idf5.5_py3.11_env\Scripts;$env:PATH"
& "$env:IDF_PATH\export.ps1"
```

## Build
```powershell
idf.py build
```

If this is a fresh build tree and ESP-IDF asks for a target, run:
```powershell
idf.py set-target esp32p4
```
then re-run the build.

## Flash + Monitor
```powershell
idf.py -p COM5 flash monitor
```
Or do it all in one step:
```powershell
idf.py -p COM5 build flash monitor
```

## Troubleshooting
- If flash fails due to a locked COM port, close any IDE/serial monitors using COM5.
- To find a stale process holding COM5:
  ```powershell
  Get-CimInstance Win32_Process | Where-Object { $_.CommandLine -match 'COM5' }
  ```
  Terminate the stale `idf.py`/`esp_idf_monitor` Python process, then replug the device.
- If the old binary still runs, verify the new build flashed by checking the boot log and app SHA.
