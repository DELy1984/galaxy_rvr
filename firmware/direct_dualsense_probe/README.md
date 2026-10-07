# Direct DualSense probe

This isolated ESP32-CAM sketch is a first-stage probe. It advertises a Wi-Fi access point, provides a status page with controller telemetry, and accepts application OTA images. It intentionally sends **no** bytes to the R3 and contains **no** motor-control code.

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
- Firmware version: `0.1.0-probe`
- Hold **Create + PS** on the DualSense to make it discoverable.
- The status page reports whether a controller connected and shows raw axes/button data. Trigger naming and range must be confirmed on the actual ESP32 before steering is implemented.
- OTA upload accepts only an application image with a filename ending in `-ota.bin`; a merged factory image is not an OTA image.

## Flashing safety

This sketch has only been compiled; it has not been uploaded to the rover. Do not use `COM3` as an ESP32 upload port: that USB-B connection is the R3 board. Do not flash until the original OTA path and partition compatibility have been checked on the exact device and a recovery path is available. OTA cannot recover a firmware that no longer starts its Wi-Fi/web server.
