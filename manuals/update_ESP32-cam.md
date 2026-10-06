# ESP32-CAM-Firmware des GalaxyRVR aktualisieren

Diese Anleitung beschreibt das Update der ESP32-CAM-Firmware über WLAN und die OTA-Webseite des GalaxyRVR. Das USB-B-Kabel und der Windows-COM-Port werden dafür nicht verwendet; sie sind für die Verbindung zum R3-Board relevant.

## Ausgangslage und Zieldatei

Die am Rover angezeigte ESP32-CAM-Version ist **1.4.0**. SunFounder empfiehlt ein Update bei Version **1.5.1 oder niedriger**.

Zum Prüfzeitpunkt **2026-10-05** war `2.0.0-fix2` der neueste offizielle GalaxyRVR-Release. Das Archiv enthält die ESP32-CAM-Datei:

```text
output/ai-camera-firmware.v1.5.4-ota.bin
```

Die zugehörige R3-Datei heißt `galaxy-rvr.ino.2.0.0.hex`. **Diese `.hex`-Datei nicht für das ESP32-CAM-OTA-Update auswählen.**

Release-Archiv: [galaxy-rvr.ino.zip für `2.0.0-fix2`](https://github.com/sunfounder/galaxy-rvr/releases/download/2.0.0-fix2/galaxy-rvr.ino.zip)

> Der Release-Stand kann sich ändern. Vor einem späteren Update den [neuesten offiziellen Release](https://github.com/sunfounder/galaxy-rvr/releases/latest) und die dort enthaltene ESP32-CAM-OTA-Datei prüfen.

## Voraussetzungen

- Rover-Akku ausreichend laden.
- Computer oder Mobilgerät mit WLAN; die Datei `.bin` muss auf dem Gerät verfügbar sein, mit dem die OTA-Seite geöffnet wird.
- Die offizielle OTA-Datei `ai-camera-firmware.v1.5.4-ota.bin` aus dem oben genannten Archiv.
- Eine stabile Stromversorgung während des Updates.

## Update Schritt für Schritt

1. Das Release-Archiv herunterladen und entpacken.
2. Prüfen, dass die Datei `output/ai-camera-firmware.v1.5.4-ota.bin` vorhanden ist. Nicht die R3-`.hex`-Datei verwenden.
3. Die `.bin`-Datei auf den Computer übertragen, falls sie nicht bereits dort liegt. Falls du die OTA-Seite mit dem Mobilgerät aufrufst, die Datei auch auf dieses Mobilgerät übertragen.
4. Den Rover einschalten, den Modusschalter auf **Run** stellen und die **Reset**-Taste am R3-Board drücken.
5. Mit dem WLAN des Rovers verbinden:
   - Standardnetzwerk: `GalaxyRVR`
   - Standardpasswort: `12345678`
   - Falls das Gerät „Kein Internet“ meldet, trotzdem mit dem Rover-WLAN verbunden bleiben.
6. Im Browser `http://192.168.4.1` öffnen.
7. Auf der OTA-Seite kontrollieren, dass sie zum ESP32-CAM-Update gehört und die vorhandene Version **1.4.0** anzeigt, sofern die Seite eine Versionsnummer darstellt.
8. Die passende Aktion für die angezeigte OTA-Oberfläche wählen:
   - **Ansicht mit „Upgrade Firmware“**: Diese Schaltfläche wählen und `ai-camera-firmware.v1.5.4-ota.bin` auswählen.
   - **Ältere Ansicht mit Dateiauswahl („Add“ / „Update“)**: Die OTA-Datei `ai-camera-firmware.v1.5.4-ota.bin` hinzufügen und danach **Update** wählen.
9. Das Update starten und warten. Laut SunFounder dauert es typischerweise **1–2 Minuten**. Den Rover währenddessen **nicht ausschalten** und die Seite nicht schließen.
10. Wenn die Seite nach Abschluss **CONFIRM** anbietet, diese Schaltfläche wählen und den Neustart abwarten.

## Update kontrollieren

1. Nach dem Neustart erneut mit dem Rover-WLAN verbinden. Der Netzwerkname kann sich zu `AI Camera-xxxxxx` ändern; laut SunFounder bleibt das Passwort `12345678`.
2. `http://192.168.4.1` erneut öffnen und die angezeigte Firmwareversion prüfen. Erwartet wird die aktualisierte Version **1.5.4** (oder eine höhere Version, falls inzwischen ein neueres Release verwendet wurde).
3. Den unteren LED-Streifen beobachten. Laut SunFounder zeigt grün blinkendes Licht an, dass die ESP32-Firmware aktuell ist.
4. Den Rover und seine WLAN-Verbindung kurz auf normalen Start prüfen.
5. Das Prüfergebnis unten und in der [README](../README.md) festhalten.

| Zeitpunkt | ESP32-CAM-Version | LED / WLAN | Ergebnis |
| --- | --- | --- | --- |
| Vor dem Update | 1.4.0 | Andere Farbe blinkt; Rover-WLAN sichtbar | Update erforderlich |
| Nach dem Update | Noch nicht geprüft | Noch nicht geprüft | Offen |

## Wichtig: R3-Board und Fehlerfall

- Diese Anleitung aktualisiert **nur die ESP32-CAM**. Sie flasht nicht das R3-Board.
- Für das R3-Board beschreibt SunFounder ein separates Update per USB-A-auf-USB-B und Upload-Schalter. Die R3-Firmwaredatei ist eine `.hex`-Datei.
- SunFounder nennt für Updates die Reihenfolge **ESP32-CAM zuerst, R3 danach**.
- Bei **dauerhaft orangefarbener** Unterbodenbeleuchtung und gleichzeitig fehlendem Rover-WLAN beschreibt SunFounder einen speziellen Wiederherstellungsablauf wegen möglicher Firmware-Inkompatibilität. In diesem Fehlerfall nicht blind mit dieser Anleitung fortfahren, sondern zuerst die [SunFounder-FAQ](https://docs.sunfounder.com/projects/galaxy-rvr/en/latest/faq.html) und die offizielle Anleitung prüfen.

## Quellen

- [SunFounder: Firmware aktualisieren](https://docs.sunfounder.com/projects/galaxy-rvr/en/latest/update_firmware.html)
- [SunFounder: FAQ und Fehlerbehebung](https://docs.sunfounder.com/projects/galaxy-rvr/en/latest/faq.html)
- [Offizieller Release `2.0.0-fix2`](https://github.com/sunfounder/galaxy-rvr/releases/tag/2.0.0-fix2)
- [Offizieller jeweils neuester Release](https://github.com/sunfounder/galaxy-rvr/releases/latest)
