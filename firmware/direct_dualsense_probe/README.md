# Direct DualSense probe

This isolated ESP32-CAM sketch advertises a Wi-Fi access point, provides controller telemetry, and accepts application OTA images. Version `0.2.0-stop-probe` adds the stock R3 initialization dialog and sends **only zero-power motor frames**. Trigger and button values never generate driving commands.

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
- Firmware version: `0.2.0-stop-probe`
- Hold **Create + PS** on the DualSense to make it discoverable.
- The status page reports whether a controller connected and shows raw axes/button data.
- OTA upload accepts only an application image with a filename ending in `-ota.bin`; a merged factory image is not an OTA image.

## Stop-only R3 test (0.2.0)

The build produces `dist/galaxyrvr-direct-dualsense-probe-0.2.0-ota.bin`. Version 0.1.0 remains a previously tested telemetry-only image; 0.2.0 has not yet been hardware-tested.

1. Safely raise the wheels, keep people/animals clear, and close RoboPilot.
2. Upload the 0.2.0 OTA application image through the current probe webpage.
3. Reconnect to `GalaxyRVR-DualSense` and open `http://192.168.4.1`.
4. Put the rover switch in **Run** and press the **R3 Reset** button. The stock R3 can remain blocked after an earlier failed camera initialization; restarting only the ESP32 does not unblock it.
5. Check for `R3 initialization: START acknowledged`, an increasing stop-frame count and an increasing valid sensor-frame count. Observe that the motors remain stopped.
6. Test L1/L2/R2: controller readings should change but wheels must not move. Report the status text and any UART errors before proceeding.

UART0 runs at 115200 baud. The probe answers only the known stock initialization commands. It reports protocol version `1.5.4` to the R3 version check, but its real firmware version remains `0.2.0-stop-probe` on the webpage. `RESET` resets the dialogue state, not the ESP32. R3 AP defaults are acknowledged without changing the probe's Wi-Fi settings. `START` returns the actual AP IP; the following R3 `[OK]` enables periodic stops at 100 ms intervals.

The transmitted frame is `WSB+` followed by `A0 03 01 01 00 00 A1` and CRLF. Sensor frames use the stock R3's separate checksum convention, checked with the expected sensor entity IDs. A stop-send count proves transmission only. Valid sensor frames are evidence of R3 return traffic, not a motor-position or physical-stop acknowledgement. Initialization status is historical; sensor age helps detect stale return traffic.

Before OTA writing/restart the probe sends a stop if initialized. This does not provide an independent R3 watchdog against a crashed ESP32. Driving remains disabled until disconnection, stale-input and update behavior are designed and verified.

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

The user approved local preparation of a separately versioned R3 build with a 500-ms command deadline and a zero-command rearm condition, but no flashing. The [guarded R3 build](../r3_command_guard/README.md) now compiles and passes native guard/parser/integration tests. Only valid motor commands refresh the deadline, expiry clears both stored powers and stops the motors, and stale powers cannot be revived by unrelated traffic. This variant rejects stock non-motor app commands and uses a bounded, validating UART parser. Preserve the official R3 HEX for restoration. R3 flashing uses USB-B/COM3, not the ESP32 OTA route; no guarded R3 firmware has been installed or hardware-tested yet.

Do not use `COM3` as an ESP32 upload port: that USB-B connection is the R3 board. OTA cannot recover a firmware that no longer starts its Wi-Fi/web server; a serial ESP32 recovery route remains unverified.
