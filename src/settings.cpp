#include "settings.h"
#include <Preferences.h>
#include "leds.h"



Preferences prefs;// globale Instanz bleibt dauerhaft bestehen

// --------------------
// Persistente Variablen
// --------------------

// LED-Helligkeit (0.0 = aus, 1.0 = maximale Helligkeit)
float ledBrightness = 0.5f;

// Dark Mode (true = aktiv, false = deaktiviert)
bool darkModeEnabled = true;

// useSleepMode:
// 0 = Röhren immer an
// 1 = Röhren nur an, wenn Person erkannt oder timeToSleep noch nicht abgelaufen
int useSleepMode = 0;

// timeToSleep:
// Zeit in Minuten, die vergeht, nachdem keine Person mehr erkannt wird
int timeToSleep = 5;

// ziAniMode:
// 0 = keine Animation
// 1 = Rollendes Einblenden
// 2 = weitere Animationen
int ziAniMode = 1;

// nameOfClock:
// Name der Uhr (Access Point und später im WLAN)
String nameOfClock = "VFDClock";

// ssID:
// SSID des ausgewählten WLAN
String ssID = "";

// wlanPW:
// Passwort des ausgewählten WLAN
String wlanPW = "";

// timeShift:
// 0 = keine automatische Sommer-/Winterzeitumstellung
// 1 = automatische Umstellung aktiv
int timeShift = 1;

// useNTP:
// 0 = keinen NTP-Server verwenden
// 1 = NTP-Server verwenden
int useNTP = 1;

// ntpServer:
// IP-Adresse oder URL des zu verwendenden NTP-Servers
String ntpServer = "pool.ntp.org";

// --------------------
// Lade gespeicherte Einstellungen
// --------------------
void loadSettings() {
    prefs.begin("vfdclock", true);  // readonly öffnen

    ledBrightness         = prefs.getFloat("ledBrightness", 0.5f);
    darkModeEnabled       = prefs.getBool("darkMode", true);
    useSleepMode          = prefs.getInt("useSleepMode", 0);
    timeToSleep           = prefs.getInt("timeToSleep", 5);
    ziAniMode             = prefs.getInt("ziAniMode", 1);
    nameOfClock           = prefs.getString("nameOfClock", "VFDClock");
    ssID                  = prefs.getString("ssID", "");
    wlanPW                = prefs.getString("wlanPW", "");
    timeShift             = prefs.getInt("timeShift", 1);
    useNTP                = prefs.getInt("useNTP", 1);
    ntpServer             = prefs.getString("ntpServer", "pool.ntp.org");

    prefs.end();

    setLEDBrightness(ledBrightness);
//    Serial.printf("[loadSettings] ziAniMode geladen: %d\n", ziAniMode);
}

// --------------------
// Speichere aktuelle Einstellungen
// --------------------

void saveSettings() {
    prefs.begin("vfdclock", false);  // Schreibmodus

    //prefs.remove("ziAniMode");

    prefs.putFloat("ledBrightness", ledBrightness);
    prefs.putBool("darkMode", darkModeEnabled);
    prefs.putInt("useSleepMode", useSleepMode);
    prefs.putInt("timeToSleep", timeToSleep);
    prefs.putInt("ziAniMode", ziAniMode);
    prefs.putString("nameOfClock", nameOfClock);
    prefs.putString("ssID", ssID);
    prefs.putString("wlanPW", wlanPW);
    prefs.putInt("timeShift", timeShift);
    prefs.putInt("useNTP", useNTP);
    prefs.putString("ntpServer", ntpServer);

    prefs.end();  // erst jetzt wirklich commit

//    Serial.printf("[saveSettings] ziAniMode gespeichert: %d\n", ziAniMode);
}


// --------------------
// Getter und Setter
// --------------------
void updateLEDBrightness(float value) {
    if (value < 0.0f) value = 0.0f;
    if (value > 1.0f) value = 1.0f;
    ledBrightness = value;
    setLEDBrightness(ledBrightness);
    saveSettings();
}

float getLEDBrightness() { return ledBrightness; }

void setDarkMode(bool enabled) { darkModeEnabled = enabled; saveSettings(); }
bool isDarkModeEnabled() { return darkModeEnabled; }

void setUseSleepMode(int mode) { useSleepMode = mode; saveSettings(); }
int getUseSleepMode() { return useSleepMode; }

void setTimeToSleep(int minutes) { timeToSleep = minutes; saveSettings(); }
int getTimeToSleep() { return timeToSleep; }

void setziAniMode(int mode) { ziAniMode = mode; saveSettings(); }
int getziAniMode() { return ziAniMode; }

void setNameOfClock(const String& name) { nameOfClock = name; saveSettings(); }
String getNameOfClock() { return nameOfClock; }

void setSSID(const String& ssid) { ssID = ssid; saveSettings(); }
String getSSID() { return ssID; }

void setWlanPW(const String& pw) { wlanPW = pw; saveSettings(); }
String getWlanPW() { return wlanPW; }

void setTimeShift(int shift) { timeShift = shift; saveSettings(); }
int getTimeShift() { return timeShift; }

void setUseNTP(int use) { useNTP = use; saveSettings(); }
int getUseNTP() { return useNTP; }

void setNtpServer(const String& server) { ntpServer = server; saveSettings(); }
String getNtpServer() { return ntpServer; }