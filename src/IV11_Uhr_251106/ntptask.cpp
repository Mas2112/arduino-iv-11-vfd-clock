#include <Arduino.h>
#include <time.h>
#include "ntptask.h"
#include "settings.h"
#include "clocktime.h"
#include "esp_sntp.h"

bool timeSynchronized = false;

void timeSyncCallback(struct timeval *tv) {
    Serial.println("Zeit synchronisiert");
    timeSynchronized = true;

    struct tm timeinfo;
    if (getLocalTime(&timeinfo)) {
        Serial.printf("Setting time: %s", asctime(&timeinfo));
        setRTCtime(timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
        updateRTCDate(timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday);
    } else {
        Serial.println("Fehler beim Abrufen der lokalen Zeit");
    }
}

void initTime() {
    sntp_set_time_sync_notification_cb(timeSyncCallback);

    // First connect to NTP server, with 0 TZ offset
    String ntpServer = getNtpServer();
    initNtpServer(ntpServer);
    String timeZone = getTimeZone();
    initTimeZone(timeZone);
}

void initNtpServer(const String& server) {
    // Don't know why this code does not work.
    //configTime(0, 0, server.c_str());
    configTime(0, 0, server.c_str(), "pool.ntp.org", "time.nist.gov");
    Serial.println("NTP-Server aktualisiert: " + server);
}

void initTimeZone(const String& tz) {
    // Now adjust the TZ. Clock settings are adjusted to show the new local time
    setenv("TZ", tz.c_str(), 1);  
    tzset();
    Serial.println("Zeitzone gesetzt: " + tz);
}

bool isTimeSynchronized() {
    return timeSynchronized;
}

tm getNtpTime() {
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) {
        timeinfo.tm_year = 0;
        timeinfo.tm_mon = 0;
        timeinfo.tm_mday = 0;
        timeinfo.tm_wday = 0;
        timeinfo.tm_yday = 0;
        timeinfo.tm_hour = 0;
        timeinfo.tm_min = 0;
        timeinfo.tm_sec = 0;
        timeinfo.tm_isdst = 0;
    }

    return timeinfo; // Uninitialized, but caller should check isTimeSynchronized() first
}