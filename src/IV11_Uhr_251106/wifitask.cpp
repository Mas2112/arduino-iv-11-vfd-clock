#include <Arduino.h>
#include <WiFi.h>
#include "wifitask.h"
#include "settings.h"
#include "webserver.h"
#include "ntptask.h"

const char* ap_password = "12345678";  // Optional

void startAccessPointMode() {
  // Starte Access Point
  // WLAN-Modus: gleichzeitiger Access Point und Station-Modus
  WiFi.mode(WIFI_AP_STA);               //macht parallel zum AP das Scannen nach WLA's möglich

  WiFi.softAP(getNameOfClock().c_str(), ap_password);    // Access Point mit gespeicherter SSID und Passwort starten

  Serial.println("Access Point gestartet");
  Serial.print("AP IP-Adresse: ");
  Serial.println(WiFi.softAPIP());
}

void startStationMode(const String& ssid, const String& password) {
  // Both SSID and password are set, try to connect in STA mode
  Serial.println("Attempting to connect to WiFi network in STA mode...");
  Serial.print("SSID: ");
  Serial.println(ssid);

  WiFi.setHostname(getNameOfClock().c_str());
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid.c_str(), password.c_str());

  // Try to connect for 30 seconds
  int attempts = 0;
  int maxAttempts = 30; // 30 seconds (1 second per attempt)
  
  while (WiFi.status() != WL_CONNECTED && attempts < maxAttempts) {
    Serial.print(".");
    delay(1000);
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    // Connection successfully established
    Serial.println();
    Serial.println("WiFi connected!");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    initTime();
  } else {
    // No connection could be established in 30 seconds, switch to AP mode
    Serial.println();
    Serial.println("Failed to connect to WiFi. Switching to Access Point mode...");

    startAccessPointMode();
  }
}

void wifiTask(void *pvParameters) {
  // Check if SSID and password are set in settings
  String ssid = getSSID();
  String password = getWlanPW();

  if (ssid.isEmpty() || password.isEmpty()) {
    // If SSID or password are not set, setup WiFi as AP mode
    startAccessPointMode();
  } else {
    // Both SSID and password are set, try to connect in STA mode
    startStationMode(ssid, password);
  }

  setupWebServer();

  while (true) {
    // Check if connection is still active
    if (WiFi.status() != WL_CONNECTED) {
      Serial.println("WiFi connection lost!");
    }
    
    processDNS();
    vTaskDelay(pdMS_TO_TICKS(100));
    loopRadarCheck();
  }
}
