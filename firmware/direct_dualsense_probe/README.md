# Direct DualSense probe

This ESP32-CAM sketch provides direct DualSense control, a Wi-Fi status page,
and application OTA. The installed version is `0.3.0-timeout-probe`; the current
source builds `0.4.0-direct-drive`, locally tested but **not installed or
hardware-tested**. Unlike earlier probes, 0.4.0 generates motor commands from
L2/R2 once the controls have been released to arm. Guarded R3 firmware is required.

## Build environment

Installed packages on the development PC:

- Arduino IDE 2.3.10
- Espressif Arduino-ESP32 core 2.0.17
- Bluepad32 Arduino core 4.1.0

The Bluepad32 package includes the AI Thinker ESP32-CAM board definition. Use:

- Board: `AI Thinker ESP32-CAM` under `ESP32 + Bluepad32 Arduino`
- Partition scheme: `Minimal SPIFFS (1.9MB APP with OTA/190KB SPIFFS)`

The OTA partition scheme is required; the board package's default `Huge APP` scheme has no OTA slot.

Compile from the repository root in PowerShell and produce an OTA-named application image:

```powershell
& ".\firmware\direct_dualsense_probe\build.ps1"
```

The resulting image is written to `firmware/direct_dualsense_probe/dist/`.

## Probe behavior

- Wi-Fi SSID: `GalaxyRVR-DualSense`
- Wi-Fi password: `12345678`
- Status page: `http://192.168.4.1`
- Current source version: `0.4.0-direct-drive`; installed version: `0.3.0-timeout-probe`
- Hold **Create + PS** on the DualSense to make it discoverable.
- The status page reports whether a controller connected and shows raw axes/button data.
- OTA upload accepts only an application image with a filename ending in `-ota.bin`; a merged factory image is not an OTA image.

## Stop-only R3 test (0.2.0)

The retained image is `dist/galaxyrvr-direct-dualsense-probe-0.2.0-ota.bin`. Versions 0.1.0 and 0.2.0 have been hardware-tested as recorded below. The current build script produces 0.4.0 without overwriting earlier images.

1. Safely raise the wheels, keep people/animals clear, and close RoboPilot.
2. Upload the 0.2.0 OTA application image through the current probe webpage.
3. Reconnect to `GalaxyRVR-DualSense` and open `http://192.168.4.1`.
4. Put the rover switch in **Run** and press the **R3 Reset** button. The stock R3 can remain blocked after an earlier failed camera initialization; restarting only the ESP32 does not unblock it.
5. Check for `R3 initialization: START acknowledged`, an increasing stop-frame count and an increasing valid sensor-frame count. Observe that the motors remain stopped.
6. Test L1/L2/R2: controller readings should change but wheels must not move. Report the status text and any UART errors before proceeding.

UART0 runs at 115200 baud. The probe answers only the known stock initialization commands. It reports protocol version `1.5.4` to the R3 version check, but its real firmware version remains `0.2.0-stop-probe` on the webpage. `RESET` resets the dialogue state, not the ESP32. R3 AP defaults are acknowledged without changing the probe's Wi-Fi settings. `START` returns the actual AP IP; the following R3 `[OK]` enables periodic stops at 100 ms intervals.

The transmitted frame is `WSB+` followed by `A0 03 01 01 00 00 A1` and CRLF. Sensor frames use the stock R3's separate checksum convention, checked with the expected sensor entity IDs. A stop-send count proves transmission only. Valid sensor frames are evidence of R3 return traffic, not a motor-position or physical-stop acknowledgement. Initialization status is historical; sensor age helps detect stale return traffic.

Before OTA writing/restart the probe sends a stop if initialized. The ESP32 alone cannot provide an independent R3 watchdog against its own crash. General controller driving remains disabled until disconnection, stale-input and update behavior are designed and verified.

## Timeout movement test (0.3.0, subsequently installed)

The user approved local preparation only on 2026-10-07. Native tests and the
ESP32 build passed: 1,182,849 / 1,966,080 program bytes, 106,412 static RAM bytes.
The output is `dist/galaxyrvr-direct-dualsense-probe-0.3.0-ota.bin`.
This section records the original test design; the later phone upload and
user-reported physical result are recorded below.

The boot behavior remains periodic zero commands after R3 initialization.
Only an explicit POST to `/timeout-test` with confirmation
`confirm=wheels-free-guarded-r3` schedules a test. The page provides a confirmation
checkbox and start button. It requires an unused test this boot, disconnected
controller, fresh sensor traffic (age less than 500 ms), no recorded UART error,
and no OTA attempt. The confirmation is an operator assertion, not automatic
detection of the R3 firmware or physically supported wheels.

1. Keep the rover safely supported with ALL wheels free, hands/people/animals
   clear, and the physical power switch accessible. Guarded R3
   `2.0.0-direct-guard.1` must be installed. Turn the DualSense off.
2. After an approved OTA installation and R3 initialization, verify version,
   fresh sensor traffic, stationary wheels, and no UART error.
3. Only after separate permission, manually start the test once. The ESP32
   sends zero, waits at least 100 ms, sends exactly one forward motor packet
   `A0 03 01 01 1E 1E A1` (30/100 on both sides), then withholds motor
   packets for 2 seconds. **No continued movement packets are sent.**
4. The R3 should expire its command after 500 ms plus loop/driver latency.
   The ESP32 observes the R3 timeout text, if received, and reports its receive
   delay. This is **not a measurement of physical wheel stopping time**.
   Video is needed to measure wheel motion; a timeout message alone is
   insufficient, and no motion means the stop-from-motion test is inconclusive.
5. After the silent period, the ESP32 resumes zero packets as a fallback.
   Controller connection, stale telemetry, UART errors, or lost initialization
   abort the test and send zero; this invalidates the timeout result.
   A preparation delay of 500 ms or more aborts without sending movement.

The attempt is consumed even if aborted; repeat requests cannot extend or
restart it. Completion/abort cannot start another test until an explicit
ESP32 reboot, which itself never starts movement. `/test-stop` is a manual POST
that aborts and sends zero. Any OTA attempt aborts and disables tests until reboot.

The two-second fallback is loop-driven, **not guaranteed if the ESP32 hangs or
blocks in HTTP handling**. The web button is not an emergency stop. If wheels
continue moving, use the physical power switch immediately; do not wait for
network access. General DualSense driving remains disabled in 0.3.0.

### 0.3.0 upload attempts (2026-10-07)

The user authorized installing 0.3.0, but not starting the movement test.
The first curl upload failed with error 56 / HTTP 000 after 36.81 seconds and
327,481 uploaded multipart bytes. Status requests then timed out. After a
user-performed power cycle, `/status` confirmed 0.2.0 still running with R3
initialized, fresh sensors, and no controller connection.

The user closed browser tabs and separately approved one further attempt.
That upload also failed with error 56 / HTTP 000 after 53.75 seconds and
786,233 uploaded multipart bytes. Neither transfer received an acceptance
response or transmitted the complete image. No movement test was started.
0.3.0 installation is not verified; the last confirmed running version is
0.2.0. The previously successful curl procedure is therefore not a reliable
fix for the intermittent OTA failure. No third attempt was made.

### Read-only network checks after reboot (2026-10-07)

No upload or movement test was performed in this check. Windows was associated
with the rover AP at 99% signal, IPv4 `192.168.4.2`, with a direct
`192.168.4.0/24` WLAN route and a reachable ARP neighbor.
The initial HTTP GET returned curl error 52 (empty reply).
Six subsequent interface-bound `/status` reads produced four HTTP 200 replies
and two TCP connect timeouts (3-second connection deadline). Successful replies
confirmed 0.2.0, START acknowledged, disconnected controller, no recorded UART
error, sensor age 2 ms, and stop/sensor counters increasing from 959/10,023
to 1,139/11,907 without resetting.

The user closed browser tabs and temporarily selected Upload without reset
or flashing. Five of six further TCP connections timed out; one succeeded,
showing counters 2,231/23,546 and fresh sensor age 8 ms. Thus this switch-only
test did **not demonstrate isolation of UART sensor traffic** and cannot
establish whether UART load causes the network issue. Separate .NET ICMP
probes returned two successes (21 and 88 ms) and two timeouts out of four.
The user restored Run afterward.

Windows `Test-Connection` itself failed with a local resource error; the
alternative .NET probes above were used instead. Reading adapter power settings
also failed with Windows error 31, so no conclusion about power-save settings
was reached and no settings were changed. Native guard/parser/probe tests still
passed, and `git diff --check` was clean.

These small samples show intermittent connectivity even without OTA flash
writes or a connected gamepad. They do not locate the fault in Windows, RF,
ESP32 Wi-Fi/BT coexistence, power supply, or application scheduling.
Continuous counters in successful responses argue against a reboot between
those readings, not against every possible crash. Next useful isolation is a
read-only comparison from a second Wi-Fi client before further upload attempts.

### Phone upload and physical timeout result (2026-10-07)

The user reported a slow but successful 0.3.0 OTA upload from a phone without
network errors. A PC GET subsequently confirmed `0.3.0-timeout-probe`,
START acknowledged, 15,350 valid sensor frames, sensor age 3 ms, and no UART
error. The user explicitly confirmed that the wheels moved briefly and stopped
by themselves after approximately half a second, rather than at the 2-second
fallback. This is a user-observed physical timeout result, not an accurately
measured stopping time. At the later GET, the test state was idle and the
timeout-message flag was false, so that response does not contain a preserved
record of the reported test.

The phone result strengthens the case for a PC/client/link-specific problem,
but does not prove a timeout setting is responsible. Previous curl POST failures
were connection resets after 36.81/53.75 seconds, before the configured
120-second limit; they were not curl's overall timeout error. Read-only TCP
connections also failed intermittently. Phone OTA is currently the preferred
observed update route; no Windows networking settings have been changed.

## Direct driving (0.4.0, local build only)

The user selected PC-style **automatic arming after releasing L1/L2/R2**, with
maximum power 30/100. No Options button is needed. Only a PS5-model gamepad is
accepted; additional controllers and other device models are disconnected.

- L2 (`brake`) controls the left wheels, R2 (`throttle`) the right wheels.
  Raw values 0-20 are zero; 20-1020 map linearly to 0-30 with rounding.
  API values 1021-1023 saturate at 30; out-of-range values stop and disarm.
- A new L1 press toggles forward/reverse once, including while triggers are
  held, matching the prior PC behavior. Holding L1 does not toggle repeatedly.
  Reversing while moving changes the requested sign immediately; first tests
  should switch direction with released triggers.
- Boot, controller reconnect, input gap, R3 reset, and safety stops clear
  arming and reset direction to forward. Fresh reports with L1 released and
  both triggers <=20 arm at zero; only a subsequent report may request motion.
- Newly delivered Bluepad32 controller data are consumed only when
  `BP32.update()` and the selected controller's `hasData()` indicate an update.
  No new report for **250 ms** stops/disarms. This measures delivery to the
  application, not the age of a radio packet inside the Bluetooth stack.
- Initialized R3, sensor age **less than 250 ms**, and no recorded UART error
  are required. R3 timeout/rearm messages stop, send zero, and skip controller
  input that loop. R3 parser errors stop and remain visible until R3 reset.
- A loop gap >=250 ms sends zero and skips that loop's controller report.
  Motor frames are transmitted every 50 ms, using signed payload bytes and
  XOR. The installed R3 guard independently expires at 500 ms if packets stop.
- Starting any OTA upload sends zero and locks driving until reboot, even if
  the upload fails. `POST /drive-stop` also locks until reboot. The timeout-test
  start/stop routes are no longer registered in this version.
- The status page shows arming/reason, direction, requested left/right power,
  input age, packet counts and R3 timeout/rearm-message counts. These values
  are requests/counters, not measured wheel speeds or motor acknowledgement.

The webpage avoids overlapping status fetches and pauses polling while its own
OTA upload runs. This is an improvement for the new page, **not a demonstrated
fix** for the old installed uploader or the Windows network failures.

Native tests passed for automatic arming, held-trigger blocking, disconnect,
R3 safety gates, lock state, stale-input boundaries, timer wraparound, L1
edge toggles with triggers held, and monotonic trigger scaling over 0-1023.
The existing R3 and timeout-probe regression tests also passed. The ESP32 build
uses 1,183,761 / 1,966,080 program bytes and 106,412 static RAM bytes.
Output: `dist/galaxyrvr-direct-dualsense-probe-0.4.0-ota.bin`.

**Not uploaded.** Install only after approval, with the controller off and all
wheels safely free. Then confirm actual 0.4.0 status and fresh R3 traffic.
Initial hardware tests must check each side, trigger release, L1 once/held,
controller disconnect/reconnect with triggers held, and rearming only after
release. Use the physical power switch for an emergency; unreliable WLAN makes
the web stop button unsuitable as an emergency stop. This software is not a
hardware-rated safety system and cannot protect against every R3/hardware fault.

## Flashing safety

The user reported a successful rover test on 2026-10-07 after accepting the unresolved recovery risk. The screenshot confirms firmware `0.1.0-probe`, an accessible web page, and a connected controller identified as `DualSense`. Resting readings were left stick X=4/Y=4, right stick X=-4/Y=4, brake/throttle=0, and buttons/misc=0.

Additional screenshots on 2026-10-07 confirmed these individual inputs:

| Input | Brake raw | Throttle raw | Buttons | Misc |
| --- | ---: | ---: | --- | --- |
| Released | 0 | 0 | `0x0` | `0x0` |
| L2 fully pressed | 1020 | 0 | `0x40` | `0x0` |
| R2 fully pressed | 0 | 1020 | `0x80` | `0x0` |
| L1 pressed | 0 | 0 | `0x10` | `0x0` |

These are observed endpoints, not a measurement of every intermediate trigger position. L1 must be tested with a bitmask rather than equality, so simultaneous trigger/button presses do not hide it.

On 2026-10-07 the user confirmed that uploading the probe OTA image again through the probe's web interface succeeded. OTA updating is therefore hardware-tested for this image. Restoring the original SunFounder firmware and automatic boot rollback have not been verified.

Later on 2026-10-07, repeated attempts to install 0.2.0 through the running 0.1.0 webpage ended with a network error at roughly 40-50% browser upload progress. After restarting, the device still reported 0.1.0; a direct HTTP GET to `/status` also confirmed this. The earlier same-image test does not establish reliable updating to a different image or identify whether the new application ever booted. The cause of the transfer failure is not yet established. Do not treat the browser progress bar as the number of bytes committed to flash.

A user-authorized diagnostic upload of the same 0.2.0 image used `curl` with a 40 KiB/s limit, browser tabs closed, and no connected controller. It failed after approximately 82.6 seconds with `curl` error 56 (connection reset), HTTP code 000, and 122,880 bytes reported uploaded out of the 1,186,128-byte image plus multipart overhead. No successful OTA response was received. Subsequent `/status` requests timed out, although Windows remained associated with `GalaxyRVR-DualSense` at 99% signal. This reproduces a failure without browser status polling or an active controller connection, but does not prove the reset cause, an ESP32 reboot, or the number of bytes written to flash. No second upload was attempted.

After the user power-cycled the rover, a subsequent upload on 2026-10-07 succeeded using an explicit WLAN source address, no rate limit, and an empty `Expect` header. The server returned HTTP 200, `OTA image accepted. Restarting.`, after 47.27 seconds and 1,186,382 multipart bytes uploaded. Windows disconnected from the AP during restart; reconnecting the existing WLAN profile restored access. `/status` then confirmed `Firmware: 0.2.0-stop-probe`, waiting for R3 initialization, zero stop frames, zero sensor frames, no UART error, and no connected controller. This verifies the version-changing OTA and new application startup, not the root cause of earlier failures or the R3 hardware integration.

### Successfully tested Windows OTA procedure

Close rover browser tabs, leave the controller disconnected, connect to the rover WLAN, and check the current WLAN IPv4 address. The address was `192.168.4.2` in this test; replace it if different. From the repository root, use PowerShell:

```powershell
curl.exe --noproxy "*" --interface 192.168.4.2 --connect-timeout 5 --max-time 120 -H "Expect:" -i -F "firmware=@firmware\direct_dualsense_probe\dist\galaxyrvr-direct-dualsense-probe-0.2.0-ota.bin" -w "`nHTTP=%{http_code} Uploaded=%{size_upload} Duration=%{time_total}s`n" http://192.168.4.1/update
```

Require HTTP 200 with the acceptance response. After the automatic restart, reconnect to `GalaxyRVR-DualSense` if Windows disconnected, then verify the actual running version:

```powershell
curl.exe --noproxy "*" --interface 192.168.4.2 --connect-timeout 5 --max-time 10 -sS http://192.168.4.1/status
```

Do not automatically retry failed POST requests. A successful transfer alone does not prove the new firmware started. This procedure changed several conditions together and followed a power cycle; it does not establish that the interface binding, removal of throttling, or `Expect` header individually fixed the failure. Browser OTA reliability remains unresolved.

### R3 stop-only hardware result

On 2026-10-07 the user confirmed that the rover was supported with wheels free, Run mode selected, and the R3 reset button pressed. `/status` reported `START acknowledged`, last command `START`, stop frames increasing from 439 to 592, and valid sensor frames increasing from 4,204 to 5,667. Both successful readings showed sensor age 2 ms, no recorded UART error, and the controller disconnected. One intervening HTTP request timed out; the next succeeded without another reset, so web connectivity is not yet demonstrated to be consistently reliable.

The user confirmed that all wheels remained still. This verifies the initialization dialogue, outgoing stop-frame counter, incoming valid sensor traffic, and observed stationary wheels. It does not verify stopping from motion, motor-command acknowledgement, controller handling during R3 traffic, or an independent R3 watchdog.

### DualSense and R3 coexistence result

On 2026-10-07, with 0.2.0 and the R3 already initialized, the user connected the DualSense. `/status` simultaneously reported the DualSense model, fresh R3 sensor data, and increasing stop/sensor counters. Holding L2, R2, and L1 together produced Brake 1020, Throttle 1020, and Buttons `0xd0` (the combined `0x40`, `0x80`, and `0x10` masks). At that reading, stop frames were 3,169, valid sensor frames 32,721, sensor age 3 ms, and no UART error was recorded.

After the user switched off the controller without restarting the rover, `/status` reported `DualSense: not connected`, R3 still `START acknowledged`, 3,615 stop frames, 37,508 valid sensor frames, sensor age 2 ms, and no recorded UART error. This confirms connection, combined input, and disconnect detection alongside ongoing R3 traffic in the stop-only firmware. It does not measure disconnect latency or verify stopping from motion, input freshness protection, or an independent R3 watchdog.

Several HTTP reads timed out during this test; later reads succeeded without a reboot. Wi-Fi/web reliability remains an unresolved issue and must not be inferred from the successful Bluetooth/UART readings.

### R3 command-loss protection review

On 2026-10-07 the upstream [R3 sketch at tag 2.0.0-fix2](https://github.com/sunfounder/galaxy-rvr/blob/2.0.0-fix2/galaxy-rvr/galaxy-rvr.ino), its [configuration](https://github.com/sunfounder/galaxy-rvr/blob/2.0.0-fix2/galaxy-rvr/galaxy-rvr.h), and the [camera library at the inspected revision](https://github.com/sunfounder/SunFounder_AI_Camera/blob/7ef221853b02ba268d70b17b7fbedf41e5e48ff2/src/SunFounder_AI_Camera.cpp) were reviewed.

In this source, motor commands store left/right power and APP mode reapplies those stored values. The library sets `ws_connected` when it receives control frames; explicit disconnect, APP_STOP, or camera initialization messages clear it. No elapsed-time check clears it when UART control packets simply stop arriving. The R3 sketch stops on entry to IDLE, but silent ESP32 failure does not itself cause that transition in the inspected source. The AVR watchdog is disabled (`WATCH_DOG 0`); merely enabling it would monitor a stuck R3 loop, not missing ESP32 commands while that loop continues running.

This is a source-level finding, not a measured motor-runaway test or proof that the installed HEX uses exactly this library revision. Do not send nonzero motor commands to test it on the stock firmware.

The user initially approved local preparation of a separately versioned R3 build with a 500-ms command deadline and a zero-command rearm condition. The [guarded R3 build](../r3_command_guard/README.md) compiles and passes native guard/parser/integration tests. Only valid motor commands refresh the deadline, expiry clears both stored powers and stops the motors, and stale powers cannot be revived by unrelated traffic. This variant rejects stock non-motor app commands and uses a bounded, validating UART parser. Preserve the official R3 HEX for restoration. After separate approval on 2026-10-07, it was flashed via USB-B/COM3 and all 13,754 bytes verified. Following WLAN reconnection, START acknowledgement and increasing stop/sensor counters were confirmed; the user reported stationary wheels. Hardware timeout and stopping-from-motion tests remain pending. ESP32 0.2.0 remains unchanged.

Do not use `COM3` as an ESP32 upload port: that USB-B connection is the R3 board. OTA cannot recover a firmware that no longer starts its Wi-Fi/web server; a serial ESP32 recovery route remains unverified.
