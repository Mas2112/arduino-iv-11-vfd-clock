/*
Für sichere Funktion erforderlich:
Zusätzliche Block-Kondensatoren an 5V und 3,3V (47uF)
Einstellungen Board-Manager:
Board: "Waveshare ESP32-S3-Zero" (Wichtig: Wirklich genau das auswählen!)
USB CDC On Boot: "Enable"
CPU Frequency "80MHz (WiFi)"
Partition Scheme: "Huge APP (3MB No OTA/1MB SPIFFS)"
PSRAM: "Disabled"
Mit diesen Einstellungen muss auch keine Taste am Board mehr gedückt werden, um die Firmware darauf zu laden.

Board-Packet "esp32 by Espressiv Systems" Version 3.2.1 funktioniert

zusätzliche Bibliotheken: 
https://github.com/me-no-dev/ESPAsyncWebServer (ESP Async Vebserver by ESP32Async (V.3.7.10))
https://github.com/me-no-dev/AsyncTCP (Async TCP by ESP32Async (V.3.4.6))

NeoPixelBus by Makuna (V.2.8.4) --> funktioniert NICHT mit Board-Packet "esp32 by Espressiv Systems" Version 3.3.0!!!
*/

//08.07.2025: In dieser Version funktioniert der AP mit einem asynchronen Webserver, mit dem auch Android klarkommt
//09.07.2025: Grundfunktion für 6 × SK6812MINI-LEDs an GPIO9 hinzu (Erkenntnis: Diese LED's funktionieren sicher mit Ub = 3,3V (getestet bis 2,3V))
//11.07.2025: leds.cpp und leds.h hinzu, um auch die Leuchteffekte separat zu halten + erste einfache Effekte
//12.07.2025: Umstellung auf aktuelles Board-Packet; erste Einstellung über Webside; persistentes Abspeichern hinzu.
//15.07.2025: Persistente Daten werden nun auch beim Aufruf der Webside übernommen.
//16.07.2025: WLAN-Scan und entsprechend Felder auf Webside hinzu, Scan liefert aber noch keine Ergebnisse.
//17.07.2025: WLAN-Scan funktioniert jetzt. (Achtung: Arbeitet natürlich nur mit 2,4GHz!)
//31.07.2025: Nach einem Irrweg mit C++-Syntax wird vom Stand (17.07.2025) aus neu gestartet.
//            Funktional heute keine Änderungen, aber umfängliche Kommentare dazu.
//05.08.2025: Radar-Pin von GPIO6 zu GPIO13
//            Erste Implementierung Ansteuerung Röhren (display.cpp / display.h); 
//            Initialisierung MAX6921 in setup() aufgenommen
//            setDisplayTime(now); mit Durchlauf von Zahlenmustern in loop() 
//06.08.2025: Da HW-SPI für den MAX6921 wahrscheinlich so auf dem ESP32-S3-Zero nicht funktioniert, wird eine eigene Instanz (SPIClass) einer SPI versucht
//            Damit kann ein SPI-Controller direkt ausgewählt werden um Konflikte zu vermeiden
//            In dieser Version scheint da ansich zu funktionieren. Daten kommen aus den Pns, die Anzeige stimmt aber noch nicht.
//15.08.2025: Erste lauffähige Ansteuerung der Röhren mit mySPI mit 4MHz und Multiplax-Ansteuerung der Röhren.
//            Es wird mit 500Hz multiplext, bei 6 Röhren ergeben sich ca. 86Hz Wiederholrate.
//17.08.2025: Feststellung, dass WLAN-AP nicht mehr funktioniert. Daher MUX auf 333Hz, was einer Wiederholrate von 55Hz entspricht reduziert.
//            Da die Anzeige immernoch stabil genug ist, und damit das Timing insgesammt entsopannter wird, so belassen.
//            Lösung für AP-Problem war die OPtion "Erase all Flash before Sketch upload..." 
//18.08.2025: RTC wurde in betrieb genommen. Dabei wurde die IIC-Geschwindigkeit auf 4MHz gesetzt und die Anzahl der RTC-Abdragen auf 1x pro Sekunde begrenzt.
//            Leider ist WLAN-AP noch sehr instabil. --> Als nächstes prüfen, ob es sinnvoll ist einen Rechenkern für WLAN zu reservieren
//19.08.2025: Umstellung auf Tasks, die explizied einzelnen Rechenkernen zugeteilt werden. Dafür kommen die Dateien "tasks.cpp" und "tasks.h" hinzu.
//            Core 0 ist jetzt ausschließlich für AP und WLAN zuständig.
//            Core 1 übernimmt alle anderen AUfgaben.(MAX6921 per Timer-Interrupt permanent beschicken (MUX..); RTC abfragen; LED-Effekt bedienen)
//            Es wird jeweils eine Task für Display, RTC, LED-Effekt und WLAN eingerichtet.
//            Teile des Setup werden auch in die Tasks verlsgert, damit die initialisierten Funktioen nachher auch tatsächlich auf dem richtigen Core laufen.
//            Compilieren und Fehlersuche steht noch aus.
//20.08.2025: Nach Hinzufügen diverser Includes in tasks.cpp funktioniert das Task Handling und damit die Zuordnung von Aufgaben auf spezielle Cores.
//            Leider ist das WLAN-Problem damit nicht behoben. Nach wie vor läuft WALN nur sporadisch, auch wenn alle anderen Aufgaben deaktiviert werden.
//            Ein Minimal-Test-Programm hat sich ähnlich verhalten. Wahrscheinlich sind die "Billig-Boards" unbrauchbar. Als nächstes wird Original "Waveshare" versucht.
//21.08.2025: Punktanimation hinzu, die jeweils im Timer-Interrupt mit aktualisiert wird. (Alle Funktionen in display.cpp)
//08.09.2025: Anpassung an den zeitscheiben der Tasks: multiplexTask kommt aller 40 Ticks dran. 40 ist bewusst gewählt, um ein Vielfaches für die Aufrufe anderer
//            Tasks und damit Wettlaufbedingungen / Jitter zu vermeiden
//            ledEffectTask kommt aller 100 Ticks dran. Das ist ausreichend für eine flüssige Animation.
//            rtcTask kommt aller 240 Ticks dran. Für die Aktualisierung der Zeit würde auch 1000 Ticks reichen, aber da die Punkt-Animation mit Schritten aller
//            0,5s hier enthalten ist, wurde dieser Wert gewählt.
//            wifiTask kommt aller 100 Ticks dran. Das ist ausreichend für eine flüssige bedienung der Webside. Es ist aber auch lang genug, um sich nicht selbst
//            "zu beißen". Mit anderen Tasks kann wifiTask nicht kollidieren, da es exklusiv auf Core 0 läuft.
//            In index_html.cpp Felder zum Stellen der Uhrzeit (HH:MMM:SS) ergänzt und auch die Übergabe-Variablen angelegt. Die Übernahme in webserver.cpp wurde noch
//            nicht umgesetzt.
//10.09.2025: Das Stellen von Uhrzeit und Datum ist vollständig implementiert. Beim Laden der Webside werden die entsprechenden Felder auch mit den aktuellen Werten
//            aus der RTC befüllt
//12.09.2025: Zustandsanzeige Radarpin auf Webside hinzu. Das soll beim Einrichten der Personenerkennung via Radar dienen. Eine Box auf der Webside zeigt an, ob eine
//            Person erkannt wurde, oder nicht. Bei jedem Pegelwechsel an dem pin wird die Webside geupdated.
//19.09.2025: Abwechselndes Anzeigen von Uhrzeit und Datum hinzu: Immer ab Sekunde 30 wird für 3 Sekunden das Datum angezeigt.
//27.10.2025: Animation "Rollendes Einblenden der Ziffern" hinzu. Noch keine Auswahl über Webinterface, Einfache Anzeigeroutinen bleiben Verfügbar.
//04.11.2025: Variablen in Settings (settings.cpp und settings.h) hinzu: 
//              useSleepMode (0=Röhren immer an; 1=Röhren nur an, wenn Person erkannt, bzw. timeToSleep noch nicht abgelaufen, kann im Webintewrface eingestellt werden)
//              timeToSleep (Zeit in Minuten, die ab dem Augenblick vergeht, wenn keine Person vom Radar mehr erkannt wird, kann im Webintewrface eingestellt werden)
//              ziAniMode (0=keine Animation; 1=Rollendes Einblenden;2=..., kann im Webintewrface eingestellt werden)
//              nameOfClock (Name der Uhr als Access Point und später im WLAN, kann im Webintewrface eingestellt werden)
//              ssID (SSID des gescannten und dann ausgewählten WLAN)
//              wlanPW (Passwort des ausgewählten WLAN, muss im Webinterface eingegeben werden)
//              timeShift (0=keine automatische Sommerzeit/Winterzeitumstellung; 1=automatische Sommerzeit/Winterzeitumstellung)
//              useNTP (0=keinen NTP-Server verwenden; 1=keinen NTP-Server verwenden)
//              ntpServer (IP-Adresse, oder URL des zu verwendenden NTP-Server)
//            Röhrenabschaltung hinzu:
//              Im Webinterface kann mit einer Checbox, die auf die Variable useSleepMode nicht gesetzt sein (0) für "Röhren leuchten immer", oder gesetzt sein (1) für 
//              Röhren schalten ab, wenn niemand im Raum ist.
//              Mit einer zweistelligen EInstellmaske ähnlich der Uhrzeit kann die Dauer in Minuten von dem Augenblick an, wo niemand vom Radar erkannt wird bis zum 
//              Abschalten der Röhren gewählt werden. (Variable timeToSleep)
//              DIeser Wert wird in eine RAM-Variable kopiert und läuft da als Count Down ab dem Augenblick wo vom Radar niemand mehr erkannt wird  bis zum Abschalten
//              der Röhren.
//              Sobald das Radar wieder eine Person erkennt, wird der Wert wieder auf den in der Variable timeToSleep gesetzt, und wenn nötig die Röhren wieder eingeschaltet.
//06.11.2025: Einstellbarer Name für den AP der Uhr und den Namen der Uhr in einem WLAN hinzu:
//              Nutzung der Variablen "nameOfClock"
//              Auf der Webside wird ein Eingabefeld generiert.
//              Es gibt einen Button "Name Speichern und Neustart"
//              Nach den Drücken des Buttons wird der Name persistent abgespeichert und die Uhr neu gestartet.
//            Auswahlmöglichkeit der Animation für die Ziffern auf der Webside hinzu:
//              Nutzung der Variablen ziAniMode (0=keine Animation; 1=Rollendes Einblenden von Segmenten)
//              Der einstellige Wert wird über eine Box mit der Ziffer und Up- und Down-Pfeilen eingestellt, genau so, wie bei der Uhrzeit
//              Es gibt einen Button "Speichern und anwenden"
//              Nach den Drücken des Buttons wird der der Animationseffekt persistent abgespeichert und angewendet.

#include <Arduino.h>
#include "settings.h"
#include "tasks.h"
#include "display.h"

unsigned long lastMotionTime = 0;
bool tubesEnabled = false;

// Initialisierung aller Module und Start des Access Points
void setup() {

  // Serielle Schnittstelle zur Debug-Ausgabe initialisieren
  Serial.begin(115200); //für Debug...
  // Kurze Pause zur Entlastung des Prozessors
  delay(1000);  // Warten, bis USB verbunden ist (hilfreich auf einigen Boards)

  // Persistente Einstellungen (z. B. LED-Helligkeit, Dark Mode) laden
  loadSettings();                    // Aus Flash laden
  //Serial.printf("Nach loadSettings(): ziAniMode = %d\n", getziAniMode());

  startAllTasks();  // Alle Tasks starten
}

// Hauptschleife:
void loop() {
// Das wird alles in tasks.cpp abgehandelt

}
