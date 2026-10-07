#include <Bluepad32.h>
#include <Update.h>
#include <WebServer.h>
#include <WiFi.h>
#include "TimeoutProbe.h"

namespace {

constexpr char AP_SSID[] = "GalaxyRVR-DualSense";
constexpr char AP_PASSWORD[] = "12345678";
constexpr char FIRMWARE_VERSION[] = "0.3.0-timeout-probe";
constexpr uint8_t STOP_FRAME[] = {0xA0, 0x03, 0x01, 0x01, 0x00, 0x00, 0xA1};
constexpr unsigned long STOP_INTERVAL_MS = 100;

WebServer server(80);
ControllerPtr connectedController = nullptr;
bool uploadFailed = false;
bool uploadComplete = false;
String uploadError;
String uartLine;
String uartError;
String lastR3Command;
bool awaitingStartAck = false;
bool r3Initialized = false;
unsigned long lastStopTime = 0;
uint32_t stopFramesSent = 0;
uint32_t sensorFramesReceived = 0;
unsigned long lastSensorTime = 0;
uint8_t sensorFrame[32];
size_t sensorBytes = 0;
size_t sensorExpected = 0;
bool readingSensorFrame = false;
unsigned long lastUartByteTime = 0;
TimeoutProbe timeoutProbe;
bool otaStarted = false;
String testError;
bool timeoutObserved = false;
uint32_t timeoutObservedAfterMs = 0;

bool controllerConnected() {
  return connectedController != nullptr && connectedController->isConnected();
}

const char* testState() {
  switch (timeoutProbe.state()) {
    case TimeoutProbe::State::Idle: return "idle - manual start required";
    case TimeoutProbe::State::Preparing: return "preparing zero command";
    case TimeoutProbe::State::Silent: return "motor packets withheld";
    case TimeoutProbe::State::Completed: return "completed - physical result unverified";
    case TimeoutProbe::State::Aborted: return "aborted - result invalid";
  }
  return "invalid";
}

void sendStop() {
  Serial.print("WSB+");
  Serial.write(STOP_FRAME, sizeof(STOP_FRAME));
  Serial.println();
  ++stopFramesSent;
  lastStopTime = millis();
}

void handleR3Line() {
  int start = uartLine.indexOf("SET+");
  if (start >= 0) {
    String command = uartLine.substring(start + 4);
    lastR3Command = command;
    if (command == "RESET") {
      r3Initialized = false;
      awaitingStartAck = false;
      uartError = "";
      // The stock R3 checks this protocol version, not our probe version.
      Serial.println("[OK] 1.5.4");
    } else if (command == "START") {
      Serial.print("[OK] ");
      Serial.println(WiFi.softAPIP());
      awaitingStartAck = true;
    } else if (command == "NAMEGalaxyRVR" || command == "TYPEGalaxyRVR" ||
               command == "APSSIDGalaxyRVR" || command == "APPSK12345678" ||
               command == "PORT30102" || command == "LAMP0") {
      // Preserve the probe AP; acknowledge the known stock R3 defaults only.
      Serial.println("[OK]");
    } else {
      uartError = "Unsupported R3 command: " + command;
      Serial.println("[ERR] Unsupported command");
    }
  } else if (uartLine == "[R3] Motor command timeout; zero required" &&
             timeoutProbe.state() == TimeoutProbe::State::Silent) {
    if (!timeoutObserved) {
      timeoutObserved = true;
      timeoutObservedAfterMs = millis() - timeoutProbe.driveTime();
    }
  } else if (awaitingStartAck && uartLine.endsWith("[OK]")) {
    awaitingStartAck = false;
    r3Initialized = true;
    sendStop();
  }
  uartLine = "";
}

void pollR3() {
  // UART0 is also the R3 data channel; never write diagnostic logs to it.
  if ((readingSensorFrame || uartLine.length() > 0) &&
      millis() - lastUartByteTime > 100) {
    if (readingSensorFrame) uartError = "Incomplete R3 sensor frame timed out";
    readingSensorFrame = false;
    uartLine = "";
  }
  size_t budget = 256;
  while (Serial.available() && budget-- > 0) {
    uint8_t value = static_cast<uint8_t>(Serial.read());
    lastUartByteTime = millis();
    if (readingSensorFrame) {
      sensorFrame[sensorBytes++] = value;
      if (sensorBytes == 1 && value != 0xA0) {
        uartError = "Invalid R3 sensor frame start";
        readingSensorFrame = false;
      } else if (sensorBytes == 2) {
        sensorExpected = static_cast<size_t>(value) + 4;
        if (sensorExpected != 11) {
          uartError = "Unexpected R3 sensor frame length";
          readingSensorFrame = false;
        }
      } else if (sensorExpected != 0 && sensorBytes == sensorExpected) {
        uint8_t checksum = 0;
        for (size_t i = 0; i < sensorExpected - 1; ++i) {
          if (i != 2) checksum ^= sensorFrame[i];
        }
        if (value == 0xA1 && checksum == sensorFrame[2] &&
            sensorFrame[3] == 0x81 && sensorFrame[6] == 0x82 &&
            sensorFrame[8] == 0x83) {
          ++sensorFramesReceived;
          lastSensorTime = millis();
        } else {
          uartError = "Invalid R3 sensor frame checksum or payload";
        }
        readingSensorFrame = false;
      }
      continue;
    }
    if (value == '\n') {
      handleR3Line();
    } else if (value != '\r' && value >= 32 && value <= 126) {
      uartLine += static_cast<char>(value);
      if (uartLine.endsWith("WSB+")) {
        uartLine = "";
        sensorBytes = 0;
        sensorExpected = 0;
        readingSensorFrame = true;
      } else if (uartLine.length() > 160) {
        uartError = "R3 text line exceeded 160 bytes";
        uartLine = "";
      }
    }
  }
}

const char PAGE[] PROGMEM = R"HTML(
<!doctype html>
<html lang="en">
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>GalaxyRVR DualSense setup</title>
  <style>
    body { font: 16px system-ui, sans-serif; max-width: 42rem; margin: 2rem auto; padding: 0 1rem; }
    pre { padding: 1rem; background: #f2f2f2; white-space: pre-wrap; }
    progress { width: 100%; }
  </style>
</head>
<body>
  <h1>GalaxyRVR DualSense setup</h1>
  <p>Firmware: 0.3.0-timeout-probe</p>
  <p>Manual timeout test: one motor command at 30/100, then 2 seconds without motor packets.
     Guarded R3 firmware required. Support the rover with ALL wheels free; keep hands clear.
     Turn off DualSense. No automatic movement on boot.</p>
  <p>After installation, reset the R3 in Run mode to start its initialization dialog.</p>
  <h2>Timeout test (one attempt per boot)</h2>
  <form method="post" action="/timeout-test">
    <label><input type="checkbox" name="confirm" value="wheels-free-guarded-r3" required>
      Guarded R3 installed, wheels free, power switch accessible.</label>
    <button type="submit">Start limited movement test</button>
  </form>
  <form method="post" action="/test-stop"><button type="submit">Send stop / abort test</button></form>
  <p>The web stop button is not a reliable emergency stop. Use the physical power switch if needed.</p>
  <h2>Controller</h2>
  <pre id="status">Waiting for status...</pre>
  <h2>Firmware update</h2>
  <form id="firmware-form">
    <input name="firmware" type="file" accept=".bin" required>
    <button type="submit">Upload OTA image</button>
  </form>
  <progress id="progress" value="0" max="100" hidden></progress>
  <pre id="upload-status" aria-live="polite"></pre>
  <script>
    const statusBox = document.getElementById('status');
    const uploadBox = document.getElementById('upload-status');
    const progress = document.getElementById('progress');
    const form = document.getElementById('firmware-form');

    async function refreshStatus() {
      try {
        const response = await fetch('/status', { cache: 'no-store' });
        if (!response.ok) throw new Error(`HTTP ${response.status}`);
        statusBox.textContent = await response.text();
      } catch (error) {
        statusBox.textContent = `Status unavailable: ${error.message}`;
      }
    }

    form.addEventListener('submit', event => {
      event.preventDefault();
      const file = form.elements.firmware.files[0];
      if (!file || !file.name.toLowerCase().endsWith('-ota.bin')) {
        uploadBox.textContent = 'Select a firmware image whose filename ends in -ota.bin.';
        return;
      }

      const request = new XMLHttpRequest();
      const data = new FormData();
      data.append('firmware', file);
      progress.hidden = false;
      progress.value = 0;
      uploadBox.textContent = 'Uploading...';
      request.upload.addEventListener('progress', event => {
        if (event.lengthComputable) progress.value = event.loaded * 100 / event.total;
      });
      request.addEventListener('load', () => {
        uploadBox.textContent = request.responseText || `HTTP ${request.status}`;
      });
      request.addEventListener('error', () => {
        uploadBox.textContent = 'Upload failed: network error.';
      });
      request.open('POST', '/update');
      request.send(data);
    });

    refreshStatus();
    setInterval(refreshStatus, 1000);
  </script>
</body>
</html>
)HTML";

void onConnectedController(ControllerPtr controller) {
  if (connectedController == nullptr) {
    connectedController = controller;
  }
}

void onDisconnectedController(ControllerPtr controller) {
  if (connectedController == controller) {
    connectedController = nullptr;
  }
}

void handleStatus() {
  String status = "Firmware: " + String(FIRMWARE_VERSION) + "\n";
  status += "R3 initialization: " + String(r3Initialized ? "START acknowledged" : "waiting - reset R3 in Run mode") + "\n";
  status += "Last R3 command: " + lastR3Command + "\n";
  status += "Stop frames sent: " + String(stopFramesSent) + "\n";
  status += "Valid R3 sensor frames: " + String(sensorFramesReceived) + "\n";
  if (sensorFramesReceived > 0) {
    status += "Last sensor frame age (ms): " + String(millis() - lastSensorTime) + "\n";
  }
  status += "UART error (last): " + (uartError.length() ? uartError : String("none")) + "\n";
  status += "Timeout test: " + String(testState()) + "\n";
  status += "Test error: " + (testError.length() ? testError : String("none")) + "\n";
  status += "R3 timeout message observed: " + String(timeoutObserved ? "yes" : "no") + "\n";
  if (timeoutObserved) {
    status += "Timeout message receive delay (ms, not physical stop time): " +
        String(timeoutObservedAfterMs) + "\n";
  }
  ControllerPtr controller = connectedController;
  if (controller == nullptr || !controller->isConnected()) {
    status += "DualSense: not connected";
    server.send(200, "text/plain; charset=utf-8", status);
    return;
  }

  status += "DualSense: connected\n";
  status += "Model: " + controller->getModelName() + "\n";
  status += "Left stick: X=" + String(controller->axisX());
  status += " Y=" + String(controller->axisY()) + "\n";
  status += "Right stick: X=" + String(controller->axisRX());
  status += " Y=" + String(controller->axisRY()) + "\n";
  status += "Brake raw: " + String(controller->brake());
  status += " | Throttle raw: " + String(controller->throttle()) + "\n";
  status += "Buttons: 0x" + String(controller->buttons(), HEX);
  status += " | Misc: 0x" + String(controller->miscButtons(), HEX);
  server.send(200, "text/plain; charset=utf-8", status);
}

void handleUpdateUpload() {
  HTTPUpload &upload = server.upload();

  if (upload.status == UPLOAD_FILE_START) {
    otaStarted = true;
    timeoutProbe.abort();
    testError = "OTA started; movement test disabled until reboot";
    if (r3Initialized) sendStop();
    uploadFailed = false;
    uploadComplete = false;
    uploadError = "";

    if (!upload.filename.endsWith("-ota.bin")) {
      uploadFailed = true;
      uploadError = "Only OTA application images ending in -ota.bin are accepted.";
      return;
    }
    if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {
      uploadFailed = true;
      uploadError = "Could not start OTA update: " + String(Update.errorString());
    }
  } else if (upload.status == UPLOAD_FILE_WRITE && !uploadFailed) {
    if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
      uploadFailed = true;
      uploadError = "Could not write OTA image: " + String(Update.errorString());
    }
  } else if (upload.status == UPLOAD_FILE_END && !uploadFailed) {
    if (Update.end(true)) {
      uploadComplete = true;
    } else {
      uploadFailed = true;
      uploadError = "OTA image was rejected: " + String(Update.errorString());
    }
  } else if (upload.status == UPLOAD_FILE_ABORTED) {
    Update.abort();
    uploadFailed = true;
    uploadError = "OTA upload was aborted.";
  }
}

void handleUpdateResult() {
  if (uploadFailed) {
    server.send(400, "text/plain; charset=utf-8", uploadError);
    return;
  }
  if (!uploadComplete || Update.hasError()) {
    server.send(400, "text/plain; charset=utf-8", "No valid OTA image was installed.");
    return;
  }

  server.send(200, "text/plain; charset=utf-8", "OTA image accepted. Restarting.");
  if (r3Initialized) sendStop();
  delay(750);
  ESP.restart();
}

void handleTimeoutTest() {
  if (server.arg("confirm") != "wheels-free-guarded-r3") {
    server.send(400, "text/plain", "Explicit guarded-R3 / wheels-free confirmation required.");
    return;
  }
  if (otaStarted || timeoutProbe.state() != TimeoutProbe::State::Idle ||
      !r3Initialized || sensorFramesReceived == 0 ||
      millis() - lastSensorTime >= 500 || controllerConnected() ||
      uartError.length() != 0) {
    server.send(409, "text/plain", "Test blocked: require unused test, no OTA, fresh R3 telemetry, no UART error and disconnected controller.");
    return;
  }
  if (!timeoutProbe.start(millis())) {
    server.send(409, "text/plain", "Test already used this boot.");
    return;
  }
  sendStop();
  server.send(202, "text/plain", "Test scheduled once. Observe wheels and /status. Use physical power switch if wheels keep moving.");
}

void pollTimeoutTest() {
  if (timeoutProbe.active() &&
      (!r3Initialized || controllerConnected() ||
       millis() - lastSensorTime >= 500 || uartError.length() != 0)) {
    timeoutProbe.abort();
    testError = "Aborted: initialization, controller, sensor freshness or UART condition changed";
    sendStop();
    return;
  }
  const TimeoutProbe::Action action = timeoutProbe.step(millis());
  if (action == TimeoutProbe::Action::Drive) {
    constexpr uint8_t frame[] = {0xA0, 0x03, 0x01, 0x01, 30, 30, 0xA1};
    Serial.print("WSB+");
    Serial.write(frame, sizeof(frame));
    Serial.println();
  } else if (action == TimeoutProbe::Action::Stop) {
    if (timeoutProbe.state() == TimeoutProbe::State::Aborted) {
      testError = "Loop delayed before movement; test aborted";
    }
    sendStop();
  }
}

}  // namespace

void setup() {
  Serial.begin(115200);
  uartLine.reserve(164);
  BP32.setup(&onConnectedController, &onDisconnectedController);
  BP32.enableVirtualDevice(false);

  WiFi.mode(WIFI_AP);
  if (WiFi.softAP(AP_SSID, AP_PASSWORD)) {
    server.on("/", HTTP_GET, []() {
      server.send_P(200, "text/html; charset=utf-8", PAGE);
    });
    server.on("/status", HTTP_GET, handleStatus);
    server.on("/update", HTTP_POST, handleUpdateResult, handleUpdateUpload);
    server.on("/timeout-test", HTTP_POST, handleTimeoutTest);
    server.on("/test-stop", HTTP_POST, []() {
      timeoutProbe.abort();
      testError = "Manual stop; test disabled until reboot";
      if (r3Initialized) sendStop();
      server.send(200, "text/plain", "Stop sent if R3 initialized. Test aborted.");
    });
    server.onNotFound([]() {
      server.send(404, "text/plain; charset=utf-8", "Not found.");
    });
    server.begin();
  }
}

void loop() {
  BP32.update();
  pollR3();
  pollTimeoutTest();
  if (r3Initialized && !timeoutProbe.active() &&
      millis() - lastStopTime >= STOP_INTERVAL_MS) {
    sendStop();
  }
  server.handleClient();
  delay(5);
}
