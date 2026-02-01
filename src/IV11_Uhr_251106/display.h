
#pragma once
#include <Arduino.h>
#include <time.h>

// Initialisiert die Röhrenanzeige (z. B. MAX6921)
void setupDisplay();

// Zeigt die übergebene Zeit auf der Röhrenanzeige an
//void setDisplayTime(time_t now);

//Entscheiden, ob Uhrzeit, oder Datum angezeigt werden sollen
void timeOrDate();

//Uhrzeit anzeigen
void showTime(time_t now);
void showTimeAnimated(time_t now);

// Datum anzeigen
void showDate(time_t now);
void showDateAnimated(time_t now);

//Segment-Mixer für Animation (Übergang von der alten zur neuen Ziffer)
uint16_t mixSegmentsByLines(uint16_t fromSeg, uint16_t toSeg, int frame, int totalFrames);

//Animation innerhalb einer Ziffer
void animateDigitChange(int pos, int fromDigit, int toDigit);
void animateDigitsFramewise(int changedIndices[], int fromDigits[], int toDigits[], int count);

// Sendet Daten zum MAX6921
void sendToMax6921(uint16_t data);

// Timer-Interrupt-Funktion für Multiplexing
void IRAM_ATTR onTimer(void* arg);

// Setzt den Punktmodus (0–5)
void setDotsMode(uint8_t mode);

// Aktualisiert die Punkte entsprechend dem gewählten Modus
void updateDots();

// Setzt oder löscht einen einzelnen Punkt (0–5)
void setDot(uint8_t index, bool state);

// Setzt alle Punkte gleichzeitig (true = an, false = aus)
void setAllDots(bool state);

//Einschalten und Ausschalten der Röhren managen
void tubeSleepTask();

// Röhren Ein- Aus-schalten
void setTubes(bool on);


