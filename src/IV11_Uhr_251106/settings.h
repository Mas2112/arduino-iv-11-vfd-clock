#pragma once

//#ifndef SETTINGS_H
//#define SETTINGS_H

#include <Arduino.h>

// Lade- und Speicherfunktionen
void loadSettings();
void saveSettings();

// LED-Helligkeit
void updateLEDBrightness(float value);
float getLEDBrightness();

// Dark Mode
void setDarkMode(bool enabled);
bool isDarkModeEnabled();

// Sleep-Mode
void setUseSleepMode(int mode);
int getUseSleepMode();

// Zeit bis Sleep
void setTimeToSleep(int minutes);
int getTimeToSleep();

// Animationsmodus
void setziAniMode(int mode);
int getziAniMode();

// Name der Uhr
void setNameOfClock(const String& name);
String getNameOfClock();

// WLAN SSID
void setSSID(const String& ssid);
String getSSID();

// WLAN Passwort
void setWlanPW(const String& pw);
String getWlanPW();

// Sommer-/Winterzeit
void setTimeShift(int shift);
int getTimeShift();

// NTP verwenden
void setUseNTP(int use);
int getUseNTP();

// NTP-Server
void setNtpServer(const String& server);
String getNtpServer();

void setTimeZone(const String& tz);
String getTimeZone();
