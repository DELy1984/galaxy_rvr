#include <Bluepad32.h>
#include <Update.h>
#include <WebServer.h>
#include <WiFi.h>

namespace {

constexpr char AP_SSID[] = "GalaxyRVR-DualSense";
constexpr char AP_PASSWORD[] = "12345678";
constexpr char FIRMWARE_VERSION[] = "0.1.0-probe";

WebServer server(80);
ControllerPtr connectedController = nullptr;
bool uploadFailed = false;
bool uploadComplete = false;
String uploadError;

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
  <p>Firmware: 0.1.0-probe</p>
  <p>Controller telemetry only. This firmware does not send motor commands.</p>
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
  delay(750);
  ESP.restart();
}

}  // namespace

void setup() {
  BP32.setup(&onConnectedController, &onDisconnectedController);
  BP32.enableVirtualDevice(false);

  WiFi.mode(WIFI_AP);
  if (WiFi.softAP(AP_SSID, AP_PASSWORD)) {
    server.on("/", HTTP_GET, []() {
      server.send_P(200, "text/html; charset=utf-8", PAGE);
    });
    server.on("/status", HTTP_GET, handleStatus);
    server.on("/update", HTTP_POST, handleUpdateResult, handleUpdateUpload);
    server.onNotFound([]() {
      server.send(404, "text/plain; charset=utf-8", "Not found.");
    });
    server.begin();
  }
}

void loop() {
  BP32.update();
  server.handleClient();
  delay(5);
}
