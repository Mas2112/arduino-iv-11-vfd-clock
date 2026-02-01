#include <Arduino.h>
#include "tasks.h"
#include "display.h"
#include "leds.h"
#include "clocktime.h"
#include "settings.h"
#include "webserver.h"
#include <WiFi.h>

//const char* ap_ssid = "IV11-Uhr";
const char* ap_password = "12345678";  // Optional


// === Task-Funktionen ===
void multiplexTask(void *pvParameters) {
  // Anzeige (z. B. Röhren über MAX6921) initialisieren
  //initLEDs();
  //setLEDEffect(1); // Regenbogen z. B.
  setupDisplay(); // Initialisiere die Anzeige
  while (true) {
    // Die Multiplex-Ansteuerunge des MAX6921 ist Interrupt-getrieben und braucht daher keine zyklischen Aufrufe.
    updateDots(); 
    tubeSleepTask(); //Handling Röhren Ein- und Aus-schalten
    vTaskDelay(pdMS_TO_TICKS(5)); //War ursprünglich auf 40, muss wahrscheinlich für animierte Anzeige schneller laufen
  }
}

void ledEffectTask(void *pvParameters) {
  // LED-Streifen initialisieren
  initLEDs();
  // LED-Effektmodus setzen (z. B. Regenbogen)
  setLEDEffect(1); // Regenbogen z. B.
  // LED-Helligkeit gemäß gespeicherter Einstellung setzen
  setLEDBrightness(getLEDBrightness()); // Helligkeit setzen
  while (true) {
    updateLEDEffect();                  // LED-Effekt aktualisieren (z. B. Animation fortsetzen)
    //delay(100);                         // alle 0,1 Sekunde aktualisieren
    vTaskDelay(pdMS_TO_TICKS(100));
  }
}

void rtcTask(void *pvParameters) {
  setupTime();                          //RTC initialisieren
  while (true) {
    updateTime();                       // holt aktuelle Zeit von der RTC
    time_t nowTime = getCurrentTime();  // liefert als time_t
    //setDisplayTime(nowTime);            // an deine Display-Funktion übergeben
    timeOrDate();                       // Entscheiden, ob Zeit, oder Datum angezeigt werden sollen, und das dann auch machen
    vTaskDelay(pdMS_TO_TICKS(5));      //War ursprünglich auf 250, muss für Animationen schneller laufen
  }
}

void wifiTask(void *pvParameters) {
  // Starte Access Point
  // WLAN-Modus: gleichzeitiger Access Point und Station-Modus
  WiFi.mode(WIFI_AP_STA);               //macht parallel zum AP das Scannen nach WLA's möglich

  WiFi.softAP(getNameOfClock().c_str(), ap_password);    // Access Point mit gespeicherter SSID und Passwort starten

  Serial.println("Access Point gestartet");
  Serial.print("AP IP-Adresse: ");
  Serial.println(WiFi.softAPIP());

  delay(1000);                          // Zeit für Access Point Initialisierung
  setupWebServer();                     // Asynchronen Webserver und Captive Portal initialisieren
  while (true) {
    // DNS-Anfragen für Captive Portal bearbeiten
    processDNS();  // Wichtig für Android Captive Portal
    vTaskDelay(pdMS_TO_TICKS(100));
    loopRadarCheck(); //fragt das Radar-Pin ab
  }
}

// === Task-Startfunktion ===
 void startAllTasks() {
  xTaskCreatePinnedToCore(multiplexTask, "Multiplex", 2048, NULL, 2, NULL, 1);
  xTaskCreatePinnedToCore(ledEffectTask, "LEDEffect", 2048, NULL, 1, NULL, 1);
  xTaskCreatePinnedToCore(rtcTask, "RTC", 2048, NULL, 1, NULL, 1);
  xTaskCreatePinnedToCore(wifiTask, "WiFi", 4096, NULL, 3, NULL, 0);
}

/*
void startAllTasks() {
  // LED zuerst!
  xTaskCreatePinnedToCore(ledEffectTask, "LEDEffect", 2048, NULL, 2, NULL, 1);
  vTaskDelay(pdMS_TO_TICKS(200)); // kurze Wartezeit, bis initLEDs() fertig ist

  xTaskCreatePinnedToCore(multiplexTask, "Multiplex", 2048, NULL, 2, NULL, 1);
  xTaskCreatePinnedToCore(rtcTask, "RTC", 2048, NULL, 1, NULL, 1);
  xTaskCreatePinnedToCore(wifiTask, "WiFi", 4096, NULL, 3, NULL, 0);
}
*/
/*
void startAllTasks() {
  xTaskCreatePinnedToCore(multiplexTask, "Multiplex", 2048, NULL, 2, NULL, 1);
  delay(500); // <--- kurze Pause, damit Display & Röhren init fertig
  xTaskCreatePinnedToCore(ledEffectTask, "LEDEffect", 2048, NULL, 1, NULL, 1);
  xTaskCreatePinnedToCore(rtcTask, "RTC", 2048, NULL, 1, NULL, 1);
  xTaskCreatePinnedToCore(wifiTask, "WiFi", 4096, NULL, 3, NULL, 0);
}
*/
