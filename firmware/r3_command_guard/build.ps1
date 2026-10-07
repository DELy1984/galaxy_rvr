$ErrorActionPreference = 'Stop'

$cli = Join-Path $env:LOCALAPPDATA 'Programs\arduino-ide\resources\app\lib\backend\resources\arduino-cli.exe'
if (-not (Test-Path -LiteralPath $cli)) { throw "Arduino CLI not found: $cli" }
$dependencies = Get-Content -LiteralPath (Join-Path $PSScriptRoot 'dependencies.json') -Raw | ConvertFrom-Json
$cores = & $cli core list --format json
if ($LASTEXITCODE -ne 0) { throw 'Could not inspect Arduino cores' }
$coreSpec = $dependencies.core.Split('@')
$core = ($cores | ConvertFrom-Json).platforms | Where-Object { $_.id -eq $coreSpec[0] }
if ($core.installed_version -ne $coreSpec[1]) { throw "Install declared core: $($dependencies.core)" }
$libraries = & $cli lib list --format json
if ($LASTEXITCODE -ne 0) { throw 'Could not inspect Arduino libraries' }
$installed = ($libraries | ConvertFrom-Json).installed_libraries
foreach ($specification in $dependencies.libraries) {
    $spec = $specification.Split('@')
    $matches = @($installed | Where-Object { $_.library.name -eq $spec[0] })
    if ($matches.Count -ne 1 -or $matches[0].library.version -ne $spec[1]) {
        throw "Install declared library: $specification"
    }
}
$work = Join-Path $PSScriptRoot '.build'
$sketch = Join-Path $work 'galaxy-rvr'
$camera = Join-Path $work 'libraries\SunFounder_AI_Camera'
$output = Join-Path $work 'output'
$dist = Join-Path $PSScriptRoot 'dist'
New-Item -ItemType Directory -Path $work, $sketch, $camera, $output, $dist -Force | Out-Null

function Get-PinnedSource($name, $url, $commit) {
    $path = Join-Path $work $name
    if (-not (Test-Path -LiteralPath $path)) {
        & git clone --no-checkout $url $path
        if ($LASTEXITCODE -ne 0) { throw "Could not clone $url" }
    }
    & git -C $path checkout --detach $commit
    if ($LASTEXITCODE -ne 0) { throw "Could not select $commit" }
    $actual = & git -C $path rev-parse HEAD
    if ($LASTEXITCODE -ne 0 -or $actual -ne $commit) { throw "Unexpected revision in $path" }
    return $path
}

function Replace-Once($text, $pattern, $replacement) {
    $regex = New-Object System.Text.RegularExpressions.Regex($pattern)
    if ($regex.Matches($text).Count -ne 1) { throw "Expected one source anchor: $pattern" }
    return $regex.Replace($text, [System.Text.RegularExpressions.MatchEvaluator]{ param($match) $replacement })
}

$roverSource = Get-PinnedSource 'upstream-rover' 'https://github.com/sunfounder/galaxy-rvr.git' $dependencies.roverCommit
$cameraSource = Get-PinnedSource 'upstream-camera' 'https://github.com/sunfounder/SunFounder_AI_Camera.git' $dependencies.cameraCommit
Copy-Item -Path (Join-Path $roverSource 'galaxy-rvr\*') -Destination $sketch -Force
Copy-Item -Path (Join-Path $cameraSource 'src') -Destination $camera -Recurse -Force
Copy-Item -LiteralPath (Join-Path $cameraSource 'library.properties') -Destination $camera -Force
Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'MotorCommandGuard.h') -Destination $sketch -Force
Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'R3InputParser.h') -Destination (Join-Path $camera 'src') -Force
Copy-Item -LiteralPath (Join-Path $roverSource 'LICENSE') -Destination (Join-Path $dist 'UPSTREAM-LICENSE') -Force

$inoPath = Join-Path $sketch 'galaxy-rvr.ino'
$ino = Get-Content -LiteralPath $inoPath -Raw
$ino = Replace-Once $ino '#define VERSION "2\.0\.0"' '#define VERSION "2.0.0-direct-guard.1"'
$ino = Replace-Once $ino '#include "galaxy-rvr.h"' "#include `"galaxy-rvr.h`"`n#include `"MotorCommandGuard.h`"`nMotorCommandGuard motorGuard;"
$guardLoop = @'
void loop() {
  enforceMotorGuard();
'@
$ino = Replace-Once $ino 'void loop\(\) \{' $guardLoop
$ino = Replace-Once $ino '(?s)void onReceive\(\) \{.*?\r?\n\}\r?\n(?=\r?\nvoid handleSensorData)' (Get-Content -LiteralPath (Join-Path $PSScriptRoot 'on_receive.inc') -Raw)
[System.IO.File]::WriteAllText($inoPath, $ino)

$libraryPath = Join-Path $camera 'src\SunFounder_AI_Camera.cpp'
$library = Get-Content -LiteralPath $libraryPath -Raw
$library = Replace-Once $library '#include "SunFounder_AI_Camera.h"' "#include `"SunFounder_AI_Camera.h`"`n#include `"R3InputParser.h`""
$library = Replace-Once $library '(?s)void AiCamera::readInto\(char \*buffer\)\s*\{.*?\r?\n\}' (Get-Content -LiteralPath (Join-Path $PSScriptRoot 'read_into.inc') -Raw)
[System.IO.File]::WriteAllText($libraryPath, $library)
$libraryHeaderPath = Join-Path $camera 'src\SunFounder_AI_Camera.h'
$libraryHeader = Get-Content -LiteralPath $libraryHeaderPath -Raw
$libraryHeader = Replace-Once $libraryHeader 'void readInto\(char \*buffer\)' 'void readInto(uint8_t *buffer)'
[System.IO.File]::WriteAllText($libraryHeaderPath, $libraryHeader)

& $cli compile --fqbn 'arduino:avr:uno' --libraries (Join-Path $work 'libraries') --output-dir $output $sketch
if ($LASTEXITCODE -ne 0) { throw "R3 compilation failed: $LASTEXITCODE" }
$image = Join-Path $output 'galaxy-rvr.ino.hex'
if (-not (Test-Path -LiteralPath $image)) { throw "Missing compiled image: $image" }
$destination = Join-Path $dist 'galaxyrvr-r3-2.0.0-direct-guard.1.hex'
Copy-Item -LiteralPath $image -Destination $destination -Force
Write-Output "Local R3 image only (NOT flashed): $destination"
Get-FileHash -LiteralPath $destination -Algorithm SHA256
