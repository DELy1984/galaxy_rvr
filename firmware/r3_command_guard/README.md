# R3 command-age guard (local preparation only)

This separately versioned build is **not installed on the rover**. The R3 still
runs the official 2.0.0 HEX, while the ESP32 runs the tested 0.2.0 stop-only probe.
Do not enable driving until the guarded R3 has been installed and hardware-tested.

## Agreed behavior

- Deadline: **500 ms** since the last accepted motor command; expiry occurs at
  `age >= 500`, using unsigned subtraction across the `millis()` wraparound.
- On expiry or an explicit camera disconnect, clear both stored powers and call
  `carStop()`. A new packet cannot revive stale values.
- Startup, disconnect, and expiry require a valid **zero/zero command** before
  a later nonzero command is accepted.
- Only the exact three-byte motor payload `01 | left | right` refreshes the
  deadline. Both signed values must be within -100 to 100. Invalid commands are
  logged and do not refresh the deadline.
- The UART parser checks `WSB+`, start/end bytes, length 3, and payload XOR.
  Partial packets persist across calls but expire after 100 ms without a byte.
  Malformed packets are rejected and logged. Parsing is bounded per loop.
- This is a **direct-control-only variant**. Stock RGB, servo, lamp, speech, and
  autonomous-mode commands are deliberately not supported; sensor telemetry and
  camera initialization remain. This prevents an autonomous mode from bypassing
  the motor-command deadline.

The 500 ms deadline is checked in the R3 loop and before accepting a command,
not in an interrupt. Actual stopping time also includes loop/driver latency.
This is not a hardware emergency stop or proof against a hung R3. The stock
AVR loop watchdog remains disabled. A zero command is a protocol rearm condition,
not an operator driving permission; the future ESP32 driving firmware must still
require fresh input and deliberate arming.

## Sources and build

[dependencies.json](dependencies.json) pins the source commits and toolchain:

- SunFounder GalaxyRVR `e1fc5c8e082599e7f577adad1fb4ce2a1a709131`
  (tag `2.0.0-fix2`).
- SunFounder AI Camera `7ef221853b02ba268d70b17b7fbedf41e5e48ff2`.
- Arduino AVR Boards 1.8.6, SoftPWM 1.0.1, ArduinoJson 6.21.5.

The [build script](build.ps1) clones pinned upstream sources into ignored
`.build`, copies them into an isolated sketch/library, and applies checked,
single-match replacements using our guard and parser. Upstream repositories
and global installed camera libraries are not edited. SunFounder's project is
GPL-3.0; preserve its license and corresponding generated source if distributing
the modified HEX. The script copies the upstream license beside the output.

Install declared dependencies with the Arduino IDE bundled CLI if needed:

```powershell
$cli = Join-Path $env:LOCALAPPDATA 'Programs\arduino-ide\resources\app\lib\backend\resources\arduino-cli.exe'
& $cli core install arduino:avr@1.8.6
& $cli lib install SoftPWM@1.0.1 ArduinoJson@6.21.5
```

From the repository root:

```powershell
& .\firmware\r3_command_guard\test.ps1
& .\firmware\r3_command_guard\build.ps1
```

The native tests use Visual Studio 2022 Community's C++ compiler, warnings as
errors, and cover exact timeout boundaries, periodic refresh, malformed/range
errors, zero rearming, explicit disconnect, wraparound, UART fragmentation and
recovery, and the actual sketch integration functions. The generated HEX is
`dist\galaxyrvr-r3-2.0.0-direct-guard.1.hex`, **not an ESP32 OTA image**.
Neither script uploads firmware or opens COM3.

## Validation and next hardware gate

Local compilation and native tests passed on 2026-10-07. Hardware has not been
changed or tested with this build. The AVR build uses 13,754 of 32,256 flash
bytes and 1,169 of 2,048 static RAM bytes (879 bytes remain for stack/locals).
This memory report is not a runtime stack-usage test. The original restore image remains
`misc\galaxy-rvr.ino\output\galaxy-rvr.ino.2.0.0.hex`; USB-B/COM3 is the R3
flash route, unlike the ESP32 WLAN OTA route.

Before flashing, obtain separate approval and confirm the R3 port and restore
image. First test the guarded R3 against the existing ESP32 stop-only probe.
Then verify rejection of nonzero commands before zero rearming and measured
stopping after missing commands on a safely supported rover, using a separately
approved test setup. Do not treat native tests or stationary wheels under
continuous zero packets as proof of a physical stop from motion.
