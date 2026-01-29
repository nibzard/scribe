$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$stopFile  = Join-Path $ScriptDir ".STOP"

$promptFiles = @(".PROMPT.md", ".FLASH.md", ".MERGE.md") | ForEach-Object { Join-Path $ScriptDir $_ }

while ($true) {
    if (Test-Path $stopFile) { break }

    foreach ($file in $promptFiles) {
        if (-not (Test-Path $file)) { continue }

        $text = Get-Content -Raw $file
        if ([string]::IsNullOrWhiteSpace($text)) { continue }

        $text | codex exec --yolo -

        # If MERGE created STOP (or any step did), exit immediately.
        if (Test-Path $stopFile) { break }
    }

    if (Test-Path $stopFile) { break }
    Start-Sleep -Seconds 1
}
