#pragma once
#include <Arduino.h>
#include <time.h>
#include "RTClib.h"

// Initialisiert die RTC und setzt die Startzeit
void setupTime();

// Aktualisiert die aktuelle Zeit von der RTC
void updateTime();

// Gibt die aktuelle Zeit als time_t zurück
time_t getCurrentTime();

// Gibt die aktuelle Zeit als DateTime zurück (praktisch für Debug/Web)
DateTime getDateTime();

// Gibt die Dauer zurück, wie lange die Röhren aktiv bleiben
unsigned long getTubeOnDuration();

// Setzt die Dauer, wie lange die Röhren aktiv bleiben
void setTubeOnDuration(unsigned long duration);

//Hilfsfunktion zum Stellen der uhrzeit
void setRTCtime(int hours, int minutes, int seconds);

//Hilfsfunktion zum Stellen des Datums
void updateRTCDate(int year, int month, int day);

// Setzt die aktuelle Zeit als UTC-Zeit (z. B. von NTP)
void setTimeUtc(time_t utc);