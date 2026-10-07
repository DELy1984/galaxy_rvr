$ErrorActionPreference = 'Stop'

$cli = Join-Path $env:LOCALAPPDATA 'Programs\arduino-ide\resources\app\lib\backend\resources\arduino-cli.exe'
if (-not (Test-Path -LiteralPath $cli)) {
    throw "Arduino CLI not found at '$cli'. Install Arduino IDE 2.3.10 first."
}

$buildDir = Join-Path $env:TEMP 'galaxyrvr-direct-dualsense-probe-build'
$distDir = Join-Path $PSScriptRoot 'dist'
New-Item -ItemType Directory -Path $buildDir, $distDir -Force | Out-Null

& $cli compile --fqbn 'esp32-bluepad32:esp32:esp32cam:PartitionScheme=min_spiffs' --output-dir $buildDir $PSScriptRoot
if ($LASTEXITCODE -ne 0) {
    throw "Arduino CLI compilation failed with exit code $LASTEXITCODE."
}

$compiledImage = Join-Path $buildDir 'direct_dualsense_probe.ino.bin'
if (-not (Test-Path -LiteralPath $compiledImage)) {
    throw "Expected application image was not created at '$compiledImage'."
}

$otaImage = Join-Path $distDir 'galaxyrvr-direct-dualsense-probe-0.6.0-ota.bin'
Copy-Item -LiteralPath $compiledImage -Destination $otaImage -Force
Write-Output "OTA application image: $otaImage"
