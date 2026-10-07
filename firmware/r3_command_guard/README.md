# R3 command-age guard (local preparation only)

This separately versioned build was written to the R3 on 2026-10-07 with
explicit user approval. Flash verification and post-flash stop-only communication
passed. The user subsequently reported wheels stopping by themselves after
approximately half a second during the 0.3.0 ESP32 test; this is not an exact
stopping-time measurement. The user subsequently confirmed installing and
successfully testing ESP32 direct driving 0.4.0 on 2026-10-07.

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

Local compilation and native tests passed on 2026-10-07. The AVR build uses 13,754 of 32,256 flash
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

### Flash record (2026-10-07)

The user connected USB-B, selected Upload, and explicitly authorized flashing.
CH340 was detected on COM3; avrdude read ATmega328P signature `0x1e950f`,
wrote 13,754 bytes, and verified all 13,754 bytes. The image SHA256 was
`5DC0CD3C3F9A4F22CBE4679CBDB374A2D873312ADD7A396C7960009C2EF9300C`.
No bootloader or fuse update was requested.

The user then selected Run and pressed R3 reset. Initial HTTP checks could not
reach the ESP32 while the PC was disconnected from the rover WLAN. The user
reported that the rover had been off and then switched it on. After another
requested connection attempt, `/status` reported `START acknowledged`.
Stop counters increased from 1,944 to 2,014 and valid sensor counters from
24,630 to 25,492, with sensor ages 9 ms and 2 ms, no recorded UART error, and
the controller disconnected. The user confirmed that all wheels remained still.
This verifies post-flash communication and observed stationary wheels under
continuous zero commands. It does not verify timeout behavior or a stop from
motion. An initial HTTP request still failed before subsequent reads succeeded;
WLAN reliability remains unresolved.

On 2026-10-07 the user reported installing ESP32 0.3.0 via phone OTA and
confirmed actual wheel movement followed by a self-stop after approximately
half a second. A later GET confirmed 0.3.0 but did not preserve that test's
timeout flag/state. This adds a user-observed stop-from-motion result, not a
measured maximum stopping latency or proof of all failure scenarios.
