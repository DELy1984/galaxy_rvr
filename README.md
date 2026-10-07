# GalaxyRVR mit PlayStation-Controller

Ziel dieses Projekts ist es, den SunFounder GalaxyRVR mit einem PlayStation-Controller zu bedienen. Diese README ist der Einstieg in die projektspezifische Wissensbasis.

## Wissensbasis

Die nächsten Themen werden Schritt für Schritt ergänzt:

- **Firmware**: installierte Versionen prüfen und bei Bedarf aktualisieren
- **ESP32-CAM-Update**: [Schritt-für-Schritt-Anleitung](manuals/update_ESP32-cam.md)
- **Hardware**: Rover, Controller und Verbindungsmöglichkeiten erfassen
- **Steuerung**: Befehle und Zuordnung der Controller-Tasten festlegen
- **Tests**: Verbindung, Fahrverhalten und Sicherheit überprüfen

## Projektplan: PS5-DualSense-Steuerung

### Startansatz: Computer als Vermittler

Zuerst soll ein Windows-Programm die Controller-Eingaben lesen und über das Rover-WLAN an den GalaxyRVR weitergeben:

```text
PS5-DualSense ──Bluetooth──> Windows-PC ──WLAN──> GalaxyRVR ──> Motoren
```

Der PC übernimmt zwei gleichzeitige Verbindungen: Bluetooth zum DualSense und WLAN zum Rover (`GalaxyRVR-6959E0`, Standardpasswort `12345678`). Für diese lokale Steuerung wird keine Internetverbindung benötigt. Die vorhandene RoboPilot-Firmware bleibt zunächst installiert; der PC soll dieselbe Steuerungsschnittstelle wie die App verwenden und keine R3-Firmwareänderung erfordern.

**Controller-Eingaben am 2026-10-07:** Der DualSense wurde zunächst per USB-Kabel mit dem PC verbunden. Windows erkannte ihn als `HID-konformer Gamecontroller` mit Status `OK` (Sony USB-Kennung `VID_054C&PID_0CE6`); Pygame meldete ein `DualSense Wireless Controller`-Gerät mit **6 Achsen und 17 Tasten**. Der Nutzer prüfte Sticks und Tasten in `joy.cpl`. Der [Eingabe-Tester](tools/read_dualsense.py) zeigt Achsen und Tasten an, ohne Rover-Befehle zu senden. Gemessene Triggerachsen: im Ruhezustand **A5 (L2) = -1.00**, **A6 (R2) = -1.00**; L2 halb gedrückt: **A5 = -0.50**; L2 und R2 jeweils ganz gedrückt: **A5/A6 = +1.00**. Bei getrenntem Drücken blieb die jeweils andere Achse bei -1.00. Damit ist für beide Trigger der Bereich **-1.00 bis +1.00** bestätigt. Danach funktionierten sowohl Buttontest als auch Fahrtest mit DualSense über **Bluetooth zum Windows-PC**. Der PC vermittelt die Befehle per WLAN an den Rover.

**Bedienung (bestätigt am 2026-10-07):** Tanksteuerung mit L2 für die linke Radseite und R2 für die rechte Radseite; beide Trigger zusammen fahren geradeaus, ein einzelner Trigger dreht den Rover. L1 (pygame-Button 10 / Index 9) schaltet per Tastendruck zwischen Vorwärts und Rückwärts um; auch bei gedrücktem L2/R2 wirkt der Richtungswechsel sofort. Gehaltenes L1 toggelt nur einmal; zum erneuten Umschalten muss es erst losgelassen und wieder gedrückt werden. `joy.cpl` zeigt L2/R2 als Bedienelemente 7/8; Pygame liest sie als Achsen A5/A6. Beide Triggerachsen laufen von -1.00 (losgelassen) bis +1.00 (ganz gedrückt).

Gewünschte maximale Leistung für den ersten Trigger-Fahrtest: **100 %** (vom Nutzer bestätigt am 2026-10-07). Der [PC-Steuerungsprototyp](tools/control_galaxyrvr.py) setzt diesen Maximalwert um, fragt vor jeder Verbindung durch Eingabe von `DRIVE` ausdrücklich nach, hält die Motoren beim Start auf null, bis L1 und beide Trigger losgelassen sind, und sendet beim normalen Beenden einen Null-Leistungsbefehl. Zum Starten: `python tools/control_galaxyrvr.py`. Vorher den PC mit dem Rover-WLAN verbinden, DualSense anschließen, RoboPilot schließen und die Räder des Rovers sicher aufbocken. Die Leistung folgt dem Triggerweg; ganz gedrückt kann 100 % anfordern. Eine kleine 2-%-Trigger-Deadzone filtert Ruhewert-Rauschen. L1 wechselt zwischen Vorwärts/Rückwärts; beim Umschalten während gedrücktem Trigger ändert sich die Drehrichtung sofort. Der Arduino-Motortreiber setzt jeden positiven oder negativen Befehl intern auf mindestens 28/255 PWM, daher ist die physische Geschwindigkeit nicht exakt linear zum Prozentwert. Bei WLAN-Ausfall kann der firmwareseitige Timeout etwa 3 Sekunden benötigen. Für die ersten Tests Personen und Tiere aus dem Bewegungsbereich halten.

**PC-WLAN-Test am 2026-10-07:** Der Nutzer hat den PC mit `GalaxyRVR-6959E0` verbunden und die Rover-Webseite unter `http://192.168.4.1` erfolgreich geöffnet. Eine zusätzliche HTTP-Abfrage vom PC bestätigte Status **200**. Damit ist die Web-Erreichbarkeit geprüft, noch nicht die WebSocket-Steuerung. Es wurden bei diesem Test keine Fahrbefehle gesendet.

**WebSocket-Verbindungstest am 2026-10-07:** Vom PC aus ist TCP-Port **30102** erreichbar; Port **8765** verweigert die Verbindung. Der WebSocket-Handshake mit `ws://192.168.4.1:30102/` war erfolgreich (Zustand `Open`). Anschließend wurde die Verbindung geschlossen. Es wurden keine Anwendungsdaten oder Motorbefehle gesendet. Das bestätigt den Verbindungsaufbau, noch nicht die Verarbeitung von Steuerbefehlen durch das R3-Board.

**Edge-Rückkanaltest am 2026-10-07:** Der Browser bestätigte einen erfolgreichen Verbindungsaufbau und ein normales Schließen (Code `1000`). Der Rover lieferte zuerst eine JSON-Geräteidentifikation mit `Name` und `Type` jeweils `GalaxyRVR`. Das bestätigt den Rückkanal; binäre Sensorpakete und die Verarbeitung eigener Motorbefehle sind noch nicht am Gerät getestet.

### Steuerprotokoll: Quellcodeprüfung

Quellen: [R3-Firmware 2.0.0-fix2](https://github.com/sunfounder/galaxy-rvr/blob/2.0.0-fix2/galaxy-rvr/galaxy-rvr.ino), [ESP32-CAM-WebSocket-Code v1.5.4](https://github.com/sunfounder/ai-camera-firmware/blob/v1.5.4/src/ws_server.cpp) und [Arduino-Kommunikationsbibliothek](https://github.com/sunfounder/SunFounder_AI_Camera/blob/7ef221853b02ba268d70b17b7fbedf41e5e48ff2/src/SunFounder_AI_Camera.cpp). Die Bibliotheksquelle erklärt den Parser; ihre genaue beim Bau des Release-Images verwendete Version ist noch nicht bestätigt.

- Der ESP32 leitet binäre WebSocket-Nachrichten an das R3 weiter.
- Der untersuchte Arduino-Parser erwartet `A0 | Nutzdatenlänge | XOR der Nutzdaten | Nutzdaten | A1`. Kein JSON-Motorbefehl und kein zusätzliches `WSB+` im WebSocket-Paket; dieses Präfix fügt der ESP32 intern hinzu.
- Motor-Nutzdaten sind `01 | links | rechts`. Die R3-Firmware interpretiert die Motorwerte als vorzeichenbehaftete 8-Bit-Werte; der vorgesehene Leistungsbereich ist -100 bis +100.
- Ein ausschließliches Stopp-Paket ergibt sich daraus als `A0 03 01 01 00 00 A1` (Hex); das Senden und Stoppen nach dem kurzen Motorimpuls wurde am Rover bestätigt. Der Stopp aus längerer Bewegung bzw. beim WLAN-Abbruch ist separat nicht geprüft.
- Das Prüfsummenverfahren eingehender Motorpakete nicht ungeprüft auf ausgehende Sensorpakete übertragen: Der Sensorpaket-Aufbau im R3-Code verwendet eine andere XOR-Berechnung.

**Stopp-Verhalten laut Quellen:** Der ESP32-Code v1.5.4 enthält Daten- und Ping/Pong-Timeouts von jeweils 3000 ms. Ein Daten-Timeout meldet intern `APPSTOP`; die Arduino-Bibliothek setzt darauf den Verbindungsstatus auf getrennt. Beim Übergang in den Idle-Zustand stoppt die R3-Firmware die Motoren. Das ist kein garantierter sofortiger Stopp bei Funkabbruch und muss unter kontrollierten Bedingungen am Gerät geprüft werden. Ein PC kann über eine bereits ausgefallene WLAN-Verbindung keinen Stoppbefehl zustellen.

**Stopp-Test am 2026-10-07:** Nach Bestätigung der sicheren Aufstellung mit frei schwebenden Rädern und geschlossener RoboPilot-App wurde ausschließlich `A0 03 01 01 00 00 A1` binär gesendet. Der PC empfing die JSON-Geräteidentifikation und ein vollständiges binäres Sensorpaket (`A0 07 E5 81 FF F6 82 00 83 CB A1`); anschließend wurde die Verbindung geschlossen. Das bestätigt den binären Rückkanal, ist aber keine explizite Motorbefehls-Bestätigung und kein Nachweis des Stopps aus einer Bewegung heraus. Es wurden keine Fahrbefehle gesendet.

**Erster Motortest am 2026-10-07:** Mit freischwebenden Rädern und geschlossener RoboPilot-App wurde der Motorbefehl `A0 03 01 01 0A 0A A1` (beide Seiten Sollwert 10/100 vorwärts) 500 ms gesendet, unmittelbar gefolgt vom Stoppbefehl `A0 03 01 01 00 00 A1`. Beide Räder liefen laut Nutzer kurz vorwärts und stoppten danach. Der PC bestätigte das Senden beider Pakete; die physische Beobachtung kam vom Nutzer. Dies ist ein erfolgreicher erster Hardwaretest mit fester kleiner Sollvorgabe, aber noch kein Test des DualSense-Eingangs oder des Funkabbruch-Timeouts. Die Firmware blieb unverändert.

**PC-Steuerungsprototyp implementiert und am Rover erfolgreich getestet:** `tools/control_galaxyrvr.py` liest L1/L2/R2 und sendet Motorframes fortlaufend an den getesteten WebSocket. Am **2026-10-07** bestätigte der Nutzer Vorwärts- und Rückwärtsfahrt sowie Buttontest sowohl über USB als auch über **Bluetooth zwischen DualSense und PC**; der PC verbindet sich per WLAN mit dem Rover. L1 schaltet die Richtung; L2/R2 steuern weiter die linke/rechte Radseite. Die volle Triggerposition kann die vom Nutzer gewählten 100 % anfordern; eine 2-%-Deadzone filtert Ruhewertrauschen. Beim Start wird erst nach expliziter `DRIVE`-Bestätigung verbunden, Nullleistung bleibt aktiv, bis L1 und beide Trigger losgelassen sind, und Ctrl+C sendet einen Stopp. Nur die spätere direkte Bluetooth-Verbindung Controller → Rover ist noch offen. Tests decken Trigger-Skalierung, Richtungs-Toggle und signierte Motorframes ab.

### Nächster Entwicklungsschritt: DualSense direkt am Rover

Zielarchitektur:

```text
DualSense ──Bluetooth──> ESP32-CAM ──serielle Steuerbefehle──> R3-Board ──> Motoren
                                         └──WLAN-Webseite für Status und OTA-Update
```

- Die PC-vermittelte Steuerung ist erfolgreich getestet (USB und Bluetooth zum PC); sie bleibt als funktionierender Vergleichs- und Rückfallweg verfügbar.
- SunFounders passendes ESP32-CAM-Quellprojekt ist öffentlich unter [ai-camera-firmware, Tag v1.5.4](https://github.com/sunfounder/ai-camera-firmware/tree/v1.5.4) verfügbar. Das heruntergeladene Release-Archiv in `misc/galaxy-rvr.ino.zip` enthält nur Firmware-Ausgaben, darunter `ai-camera-firmware.v1.5.4-ota.bin`; der Quellcode liegt separat im Repository.
- Die Original-Firmware bietet WLAN, eine Web-Einstellungsseite und den HTTP-OTA-Upload-Endpunkt `/update` ([settings.cpp](https://github.com/sunfounder/ai-camera-firmware/blob/v1.5.4/src/settings.cpp)). Ihr Hauptprogramm initialisiert WLAN und den Webserver und verarbeitet die serielle Schnittstelle; aktuell startet es zusätzlich die Kamera ([main.cpp](https://github.com/sunfounder/ai-camera-firmware/blob/v1.5.4/src/main.cpp)).
- Die Build-Konfiguration zielt auf `esp32cam` und verwendet `min_spiffs.csv` ([platformio.ini](https://github.com/sunfounder/ai-camera-firmware/blob/v1.5.4/platformio.ini)). SunFounders [Build-Skript](https://github.com/sunfounder/ai-camera-firmware/blob/v1.5.4/tools/build.py) erzeugt ein OTA-Image und zusätzlich ein zusammengefügtes Factory-Image aus App, Bootloader, Partitionstabelle und `boot_app0`. Das Factory-Image ist im lokalen Release-Archiv nicht enthalten; seine Erzeugung ist noch nicht lokal verifiziert und schafft für sich allein noch keinen physischen Zugang zum ESP32.
- [Bluepad32](https://github.com/ricardoquesada/bluepad32) unterstützt den DualSense und den originalen ESP32-Chip. Gewählt wurde **Arduino IDE mit dem offiziellen „ESP32 + Bluepad32“-Boardpaket**. Auf dem Entwicklungs-PC sind Arduino IDE 2.3.10, ESP32 Arduino 2.0.17 und Bluepad32 4.1.0 installiert; der offizielle Controller-Beispielsketch wurde für AI Thinker ESP32-CAM mit OTA-fähiger Partitionierung erfolgreich kompiliert.
- Ein getrenntes [DualSense-Probeprojekt](firmware/direct_dualsense_probe/README.md) ist vorbereitet. Es startet einen eigenen WLAN-Access-Point, zeigt Firmwareversion und Bluetooth-Status/-Messwerte im Browser und bietet einen OTA-Upload für `-ota.bin`-Anwendungsimages. Es enthält absichtlich keinen Motorsteuerungs- oder R3-Sendecode. Der Probe-Build wurde erfolgreich kompiliert: **1,153,805 Byte von 1,966,080 Byte** möglicher OTA-App-Größe (58 %). Das Image wurde noch nicht auf den Rover geladen und die DualSense-Verbindung daher noch nicht am Rover getestet.
- Für den Probe-Build ist `Minimal SPIFFS (1.9MB APP with OTA/190KB SPIFFS)` fest ausgewählt. SunFounders Originalquelle und unser Boardpaket verwenden dieselben OTA-Partitionsgrenzen; die Probe-App passt in einen OTA-App-Slot. Die Partitionsübereinstimmung beweist noch nicht, dass Bootloader und installierte Firmware jeden Rückweg unterstützen.
- Die serielle Verbindung läuft laut Firmware mit **115200 Baud**. ESP32 und R3 verpacken WebSocket-Binärdaten als `WSB+`, danach folgen die unveränderten Binärbytes und ein Zeilenende. Die R3-seitige [SunFounder_AI_Camera-Bibliothek](https://github.com/sunfounder/SunFounder_AI_Camera/blob/7ef221853b02ba268d70b17b7fbedf41e5e48ff2/src/SunFounder_AI_Camera.cpp) erkennt danach den Binärframe `A0 | Länge | XOR | Nutzdaten | A1`. Für die Rover-Steuerung bleibt die geprüfte Nutzlast `01 | links | rechts`; die PC-WebSocket-Schnittstelle darf also nicht mit dem UART-Wrapper verwechselt werden.
- Die vorhandene OTA-Route nimmt ein App-Update entgegen und startet danach neu. OTA ist nur verfügbar, solange der ESP32 noch WLAN und Webserver startet; es ist deshalb ein Update-/Rollbackweg, aber kein garantierter Notfallweg bei nicht startender Firmware. Ob Bootloader und Partitionierung nach einem fehlgeschlagenen Start automatisch zurückrollen, ist noch nicht belegt. Ein serieller ESP32-Recovery-Flash ist am Gerät ebenfalls nicht verifiziert.
- Die R3-Firmware bleibt zunächst SunFounders Kommunikationsfirmware 2.0.0. Der Probe-Sketch lässt Kamera und Stream weg, behält WLAN/OTA bei und stellt die geforderte Statusseite bereit.
- Nächster Hardware-Meilenstein: OTA-Risiko und Wiederherstellung gemeinsam abwägen, dann – erst nach ausdrücklicher Freigabe – den Probe-Sketch über die vorhandene OTA-Seite installieren und ausschließlich Bluetooth-Verbindung/Eingaben ohne Motoraktivierung prüfen. Erst danach kommen R3-Steuerung, Startfreigabe und Stopplogik hinzu. Einen garantierten seriellen ESP32-Notfall-Flash gibt es weiterhin nicht.

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

Die aktuelle Dokumentation beschreibt das Wiederherstellen/Aktualisieren der R3-Kommunikationsfirmware über das offizielle Update-Skript. Sie nennt keinen vergleichbaren LED- oder OTA-Check, der die R3-Version nur ausliest. Deshalb lässt sich der R3-Stand nicht allein aus der ESP32-Anzeige ableiten. Vor dem Update wurde bestätigt, dass kein eigener Arduino-Code auf das R3-Board geladen worden war.

SunFounder weist darauf hin, dass eigener Arduino-Code die R3-Kommunikationsfirmware überschreibt. Die Dokumentation verlangt bei der Firmware-Aktualisierung die Reihenfolge **ESP32-CAM zuerst, R3 danach**. Bei einer Versionskombination mit solid-orangefarbener Unterbodenbeleuchtung und fehlendem WLAN nennt SunFounder einen speziellen Wiederherstellungsablauf; dann nicht einfach die Reihenfolge ändern.

**USB-Verbindung und Update:** Windows erkannte den USB-Seriell-Adapter `USB-SERIAL CH340` auf **COM3**. Das offizielle SunFounder-Skript lud am **2026-10-06** erfolgreich `galaxy-rvr.ino.2.0.0.hex` auf das R3-Board. Anschließend wurde der Schalter zurück auf **Run** gestellt. Danach meldete der Nutzer die WLAN-SSID `GalaxyRVR-6959E0`. Das Telefon verbindet sich mit diesem WLAN, `http://192.168.4.1` ist erreichbar und die korrekte **RoboPilot-App** verbindet sich mit dem Rover. Zunächst wurde versehentlich eine andere App verwendet. Die Fahrsteuerung wurde getestet; der Rover reagiert.

### Geprüfter aktueller Release

Am **2026-10-05** ist der neueste offizielle GitHub-Release `2.0.0-fix2` (veröffentlicht am 2026-09-08). Das Release-Archiv enthält:

- ESP32-CAM: `ai-camera-firmware.v1.5.4-ota.bin`
- R3: `galaxy-rvr.ino.2.0.0.hex`

Die ESP32-CAM-Version 1.5.4 liegt über der in der SunFounder-Anleitung genannten Schwelle 1.5.1. Vor dem Update meldete der Rover Version 1.4.0; das Update auf 1.5.4 wurde am **2026-10-06** erfolgreich durchgeführt.

### Schritt 2: ESP32-CAM-Update

Das Update wurde am **2026-10-06** erfolgreich abgeschlossen. Siehe [Schritt-für-Schritt-Anleitung](manuals/update_ESP32-cam.md). Das R3-Board wurde in diesem Schritt nicht aktualisiert; das separate R3-Update auf Version 2.0.0 wurde danach ebenfalls erfolgreich durchgeführt.

### Prüfergebnis

Ergebnis nach dem ESP32-CAM-Update:

| Komponente | Beobachtung / angezeigte Version | Ergebnis |
| --- | --- | --- |
| ESP32-CAM | Vorher **1.4.0**, nachher **1.5.4** (OTA-Seite; Screenshot vom 2026-10-06); WLAN-SSID war zunächst `AI Camera-6959E0` | Update erfolgreich |
| R3-Board | `galaxy-rvr.ino.2.0.0.hex` erfolgreich über `USB-SERIAL CH340 (COM3)` geflasht; danach SSID `GalaxyRVR-6959E0`; OTA-Webseite und RoboPilot-App erreichbar | Flash und Fahrsteuerung mit RoboPilot erfolgreich getestet |

## Quellen

- [GalaxyRVR-Dokumentation](https://docs.sunfounder.com/projects/galaxy-rvr/en/latest/index.html)
- [Firmware aktualisieren](https://docs.sunfounder.com/projects/galaxy-rvr/en/latest/update_firmware.html)
- [FAQ und Fehlerbehebung](https://docs.sunfounder.com/projects/galaxy-rvr/en/latest/faq.html)
- [SunFounder GalaxyRVR-Steuercode](https://github.com/sunfounder/galaxy-rvr/tree/2.0.0-fix2/galaxy-rvr)
- [SunFounder AI Camera Arduino-Bibliothek](https://github.com/sunfounder/SunFounder_AI_Camera)
- [Sony: DualSense per Bluetooth koppeln](https://www.playstation.com/en-gb/support/hardware/pair-dualsense-controller-bluetooth/?country-selector=true)
- [Offizieller GalaxyRVR-Firmware-Download (jeweils neuester Release)](https://github.com/sunfounder/galaxy-rvr/releases/latest/download/galaxy-rvr.ino.zip)
- [Geprüfter Release `2.0.0-fix2`](https://github.com/sunfounder/galaxy-rvr/releases/tag/2.0.0-fix2)

Dokumentationsstand geprüft am **2026-10-05**. Der Latest-Download-Link kann künftig auf einen anderen Release zeigen; der oben genannte Versionsstand bezieht sich auf den am Prüftag neuesten Release. Die tatsächliche installierte Firmware muss am Rover geprüft werden.
