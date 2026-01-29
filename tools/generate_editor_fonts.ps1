param(
    [int[]]$Sizes = @(14,16,18,20,22,24,28,32,36,40,44,48,56,64,72),
    [string]$Range = "0x20-0x7F"
)

$ErrorActionPreference = "Stop"

$root = Resolve-Path "$PSScriptRoot\.."
$fontsDir = Join-Path $root "assets\fonts"
$outDir = Join-Path $root "components\scribe_ui\theme\generated"

New-Item -ItemType Directory -Force $outDir | Out-Null
Get-ChildItem $outDir -Filter "scribe_font_*.c" -ErrorAction SilentlyContinue | Remove-Item -Force
Get-ChildItem $outDir -Filter "scribe_fonts*.h" -ErrorAction SilentlyContinue | Remove-Item -Force
Get-ChildItem $outDir -Filter "scribe_fonts_manifest.c" -ErrorAction SilentlyContinue | Remove-Item -Force

$families = @(
    @{ Id = "montserrat"; Path = (Join-Path $fontsDir "montserrat.ttf") },
    @{ Id = "dejavu";      Path = (Join-Path $fontsDir "dejavu.ttf") },
    @{ Id = "ubuntu";      Path = (Join-Path $fontsDir "ubuntu.ttf") }
)

$header = @()
$header += "#pragma once"
$header += "#include <lvgl.h>"
$header += "#include <stddef.h>"
$header += ""
$header += "#ifdef __cplusplus"
$header += 'extern "C" {'
$header += "#endif"
$header += ""
$header += "typedef struct {"
$header += "    int size_px;"
$header += "    const lv_font_t* font;"
$header += "} ScribeFontEntry;"
$header += ""

foreach ($family in $families) {
    foreach ($size in $Sizes) {
        $fontName = "scribe_font_{0}_{1}" -f $family.Id, $size
        $header += "extern const lv_font_t $fontName;"
    }
    $header += ""
    $header += "extern const ScribeFontEntry scribe_{0}_fonts[];" -f $family.Id
    $header += "extern const size_t scribe_{0}_font_count;" -f $family.Id
    $header += ""
}

$header += "#ifdef __cplusplus"
$header += "}"
$header += "#endif"

$headerPath = Join-Path $outDir "scribe_fonts.h"
Set-Content -Path $headerPath -Value $header -Encoding ASCII

$manifest = @()
$manifest += '#include "scribe_fonts.h"'
$manifest += ""

foreach ($family in $families) {
    foreach ($size in $Sizes) {
        $fontName = "scribe_font_{0}_{1}" -f $family.Id, $size
        $outFile = Join-Path $outDir ("{0}.c" -f $fontName)
        & npx --yes lv_font_conv --size $size --bpp 4 --format lvgl --no-compress --lv-include lvgl.h --font $family.Path -r $Range --force-fast-kern-format --lv-font-name $fontName -o $outFile
    }

    $manifest += "const ScribeFontEntry scribe_{0}_fonts[] = {{" -f $family.Id
    foreach ($size in $Sizes) {
        $fontName = "scribe_font_{0}_{1}" -f $family.Id, $size
        $manifest += "    {{ {0}, &{1} }}," -f $size, $fontName
    }
    $manifest += "};"
    $manifest += "const size_t scribe_{0}_font_count = sizeof(scribe_{0}_fonts) / sizeof(scribe_{0}_fonts[0]);" -f $family.Id
    $manifest += ""
}

$manifestPath = Join-Path $outDir "scribe_fonts_manifest.c"
Set-Content -Path $manifestPath -Value $manifest -Encoding ASCII

Write-Host ("Generated fonts in {0}" -f $outDir)
