# GalaxyRVR mit PlayStation-Controller

Ziel dieses Projekts ist es, den SunFounder GalaxyRVR mit einem PlayStation-Controller zu bedienen. Diese README ist der Einstieg in die projektspezifische Wissensbasis.

## Wissensbasis

Die nächsten Themen werden Schritt für Schritt ergänzt:

- **Firmware**: installierte Versionen prüfen und bei Bedarf aktualisieren
- **ESP32-CAM-Update**: [Schritt-für-Schritt-Anleitung](manuals/update_ESP32-cam.md)
- **Hardware**: Rover, Controller und Verbindungsmöglichkeiten erfassen
- **Steuerung**: Befehle und Zuordnung der Controller-Tasten festlegen
- **Tests**: Verbindung, Fahrverhalten und Sicherheit überprüfen

## Schritt 1: Firmwarestand prüfen

Der Rover wurde über USB-B mit diesem Computer verbunden und wird von Windows als `USB-SERIAL CH340 (COM3)` erkannt. Die Verbindung allein zeigt nicht, welche Firmware-Version installiert ist. Bitte die folgenden Beobachtungen am Gerät erfassen, bevor Firmware aktualisiert oder eigener Code auf das R3-Board geladen wird.

### ESP32-CAM: Startanzeige und Versionsnummer

1. Den Rover ausreichend laden und einschalten.
2. Den Modusschalter auf **Run** stellen und die **Reset**-Taste am R3-Board drücken.
3. Den unteren LED-Streifen beobachten:
   - **Grün blinkend**: Laut SunFounder ist die ESP32-Firmware aktuell.
   - **Andere blinkende Farbe**: Die Firmware sollte anhand der offiziellen Update-Anleitung geprüft bzw. aktualisiert werden.
4. Für eine genauere Versionsprüfung das Gerät per WLAN verbinden:
   - Standard-SSID: `GalaxyRVR`
   - Standardpasswort: `12345678`
   - Im Browser `http://192.168.4.1` öffnen.
5. Falls die OTA-Seite eine Versionsnummer anzeigt (Oberfläche **Version B**), diese notieren. Laut SunFounder ist bei einer Version **höher als 1.5.1** kein ESP32-Update erforderlich; bei **1.5.1 oder niedriger** ist ein Update vorgesehen.

> Die blinkende grüne LED bestätigt laut Anleitung den ESP32-Stand, nicht automatisch die Firmware des R3-Boards.

### R3-Board: getrennt prüfen

Die aktuelle Dokumentation beschreibt das Wiederherstellen/Aktualisieren der R3-Kommunikationsfirmware über das offizielle Update-Skript. Sie nennt keinen vergleichbaren LED- oder OTA-Check, der die R3-Version nur ausliest. Deshalb den R3-Stand zunächst als **ungeprüft** notieren und nicht allein aus der ESP32-Anzeige auf ihn schließen.

SunFounder weist darauf hin, dass eigener Arduino-Code die R3-Kommunikationsfirmware überschreibt. Das Update-Skript sollte nur nach Klärung des bisherigen Zustands und anhand der offiziellen Anleitung verwendet werden. Die Dokumentation verlangt bei der Firmware-Aktualisierung die Reihenfolge **ESP32-CAM zuerst, R3 danach**. Bei einer Versionskombination mit solid-orangefarbener Unterbodenbeleuchtung und fehlendem WLAN nennt SunFounder einen speziellen Wiederherstellungsablauf; dann nicht einfach die Reihenfolge ändern.

**USB-Verbindung erkannt:** Windows zeigt den USB-Seriell-Adapter `USB-SERIAL CH340` auf **COM3**. Das bestätigt, dass der Computer den Adapter erkennt; es liest nicht die installierte R3-Firmware aus und startet keinen Upload.

### Geprüfter aktueller Release

Am **2026-10-05** ist der neueste offizielle GitHub-Release `2.0.0-fix2` (veröffentlicht am 2026-09-08). Das Release-Archiv enthält:

- ESP32-CAM: `ai-camera-firmware.v1.5.4-ota.bin`
- R3: `galaxy-rvr.ino.2.0.0.hex`

Die ESP32-CAM-Version 1.5.4 liegt über der in der SunFounder-Anleitung genannten Schwelle 1.5.1. Der Rover meldet 1.4.0; sein ESP32-CAM-Update ist daher erforderlich.

### Schritt 2: ESP32-CAM-Update

Das Update wurde noch **nicht** ausgeführt. Siehe [Schritt-für-Schritt-Anleitung](manuals/update_ESP32-cam.md). Das R3-Board wird in diesem Schritt nicht aktualisiert; sein Firmwarestand bleibt separat ungeprüft.

### Prüfergebnis

Nach dem Prüfen hier ergänzen:

| Komponente | Beobachtung / angezeigte Version | Ergebnis |
| --- | --- | --- |
| ESP32-CAM | OTA-Seite meldet Version **1.4.0**; LED blinkt in anderer Farbe | **Veraltet**; SunFounder empfiehlt ein Update bei Version 1.5.1 oder niedriger |
| R3-Board | USB-B erkannt als `USB-SERIAL CH340 (COM3)` | Firmwarestand weiterhin ungeprüft |

## Quellen

- [GalaxyRVR-Dokumentation](https://docs.sunfounder.com/projects/galaxy-rvr/en/latest/index.html)
- [Firmware aktualisieren](https://docs.sunfounder.com/projects/galaxy-rvr/en/latest/update_firmware.html)
- [FAQ und Fehlerbehebung](https://docs.sunfounder.com/projects/galaxy-rvr/en/latest/faq.html)
- [Offizieller GalaxyRVR-Firmware-Download (jeweils neuester Release)](https://github.com/sunfounder/galaxy-rvr/releases/latest/download/galaxy-rvr.ino.zip)
- [Geprüfter Release `2.0.0-fix2`](https://github.com/sunfounder/galaxy-rvr/releases/tag/2.0.0-fix2)

Dokumentationsstand geprüft am **2026-10-05**. Der Latest-Download-Link kann künftig auf einen anderen Release zeigen; der oben genannte Versionsstand bezieht sich auf den am Prüftag neuesten Release. Die tatsächliche installierte Firmware muss am Rover geprüft werden.
