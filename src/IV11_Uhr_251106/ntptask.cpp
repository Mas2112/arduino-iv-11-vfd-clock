#include <Arduino.h>
#include <time.h>
#include "ntptask.h"
#include "settings.h"
#include "clocktime.h"
#include "esp_sntp.h"

// https://github.com/nayarsystems/posix_tz_db/blob/master/zones.csv
// Europe/Berlin
const char * timeZone = "CET-1CEST,M3.5.0,M10.5.0/3";

void timeSyncCallback(struct timeval *tv) {
    Serial.println("Zeit synchronisiert");

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
    configTime(0, 0, getNtpServer().c_str());
    
    // Now adjust the TZ. Clock settings are adjusted to show the new local time
    setenv("TZ", timeZone, 1);  
    tzset();
}
