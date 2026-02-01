#include "clocktime.h"

#include <Wire.h>
#include "RTClib.h"

RTC_DS3231 rtc;

// RTC_DS3231 rtc;
static time_t currentTime = 0;
static uint32_t lastRtcReadMs = 0;
static const uint32_t RTC_READ_INTERVAL_MS = 1000; // 1 Hz reicht völlig
unsigned long tubeOnDuration = 10000;

void setupTime() {
  Wire.begin(8, 7);         // SDA=GPIO8, SCL=GPIO7
  Wire.setClock(400000);    // schneller -> kürzere Blockzeit
  if (!rtc.begin()) {
    Serial.println("❌ RTC nicht gefunden!");
    return;
  }
  if (rtc.lostPower()) {
    Serial.println("⚠️ RTC hat Strom verloren, setze Compile-Time");
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }
  currentTime = rtc.now().unixtime();
  lastRtcReadMs = millis();
}

// Nur bei Bedarf (1×/s) von der RTC lesen:
void updateTime() {
  uint32_t nowMs = millis();
  if (nowMs - lastRtcReadMs >= RTC_READ_INTERVAL_MS) {
    currentTime = rtc.now().unixtime();
    lastRtcReadMs = nowMs;
  }
}

// Gibt die aktuelle Zeit als time_t zurück
time_t getCurrentTime() {
  return currentTime;
}

// Gibt die aktuelle Zeit als DateTime zurück (praktisch für Debug/Web)
DateTime getDateTime() {
  return rtc.now();
}

// Gibt die Dauer zurück, wie lange die Röhren aktiv bleiben
unsigned long getTubeOnDuration() {
  return tubeOnDuration;
}

// Ermöglicht das Setzen der Röhren-Anzeigedauer
void setTubeOnDuration(unsigned long duration) {
  tubeOnDuration = duration;
}

//Hilfsfunktion zum Stellen der uhrzeit
void setRTCtime(int hours, int minutes, int seconds) {
  // Hole das aktuelle Datum, damit wir es beim Setzen nicht verlieren
  DateTime now = rtc.now();
  rtc.adjust(DateTime(now.year(), now.month(), now.day(),
                      hours, minutes, seconds));
}

//Hilfsfunktion zum Stellen des datums
void updateRTCDate(int year, int month, int day) {
  DateTime now = rtc.now();
  rtc.adjust(DateTime(year, month, day,
                      now.hour(), now.minute(), now.second()));
}
