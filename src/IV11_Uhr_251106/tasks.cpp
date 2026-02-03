#include <Arduino.h>
#include "tasks.h"
#include "display.h"
#include "leds.h"
#include "clocktime.h"
#include "settings.h"
#include "webserver.h"
#include <WiFi.h>
#include "wifitask.h"

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
