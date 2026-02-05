# Uhr mit 6 Stück IV-11 VFD-Röhren
Arduino Code für die Steuerung von 6 IV-11 VFD-Röhren, angefangen von Kersten Große, weiterentwickelt von mir :)

Hardwareprojekt bei www.easyeda.com

# Arduino IDE Setup
* Arduino IDE installieren
* Esp32 by Espressif Systems Version 3.2.1 installieren
  * Tools => Boards => Boards Manager
  * esp32 suchen
  * 3.2.1 auswählen
  * INSTALL
* Folgende Bibliotheken im Library Manager von Arduino IDE installieren:
  * NeoPixelBus by Makuna (V.2.8.4)
  * NTPClient by Fabrice (V3.2.1)
  * RTClib by Adafruit (V2.1.4)
    * Adafruit BusIO by Adafruit (V1.17.2)
  * RTC by Makuna (V2.5.0)
  * Timezone by Jack Christensen (V1.2.5)
    * Time by Michael Margolis (1.6.1)
  * ESP Async WebServer by ESP32Async (V.3.7.10)
    * https://github.com/me-no-dev/ESPAsyncWebServer
  * Async TCP by ESP32Async (V.3.4.6)
    * https://github.com/me-no-dev/AsyncTCP
* Board Einstellungen:
  * Board: "Waveshare ESP32-S3-Zero" (Wichtig: Wirklich genau das auswählen!)
  * USB CDC On Boot: "Enable"CPU Frequency "80MHz (WiFi)"
  * Partition Scheme: "Huge APP (3MB No OTA/1MB SPIFFS)"
  * PSRAM: "Disabled"
  * Mit diesen Einstellungen muss auch keine Taste am Board mehr gedückt werden, um die Firmware darauf zu laden.
