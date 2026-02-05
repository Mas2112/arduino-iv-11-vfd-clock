#include <ESPAsyncWebServer.h>
#include <DNSServer.h>
#include <WiFi.h>
#include "index_html.h"
#include "leds.h"
#include "settings.h"
#include "clocktime.h"
#include "webserver.h"
#include "display.h"
#include "ntptask.h"

#define RADAR_PIN 13   //Radar-Sensor GPIO

const byte DNS_PORT = 53;
DNSServer dnsServer;
AsyncWebServer server(80);

AsyncEventSource events("/events");   // für Server-Sent Events
int lastRadarState = -1;

void handleRedirectToRoot(AsyncWebServerRequest *request) {
  request->redirect("/");
}

void handleNCSI(AsyncWebServerRequest *request) {
  request->send(200, "text/plain", "Microsoft NCSI");
}

void handleMainPage(AsyncWebServerRequest *request) {
  String html = MAIN_page;
  html.replace("%CLOCKNAME%", getNameOfClock());
  request->send(200, "text/html", html);
}

void handleSetLEDBrightness(AsyncWebServerRequest *request) {
  if (request->hasParam("value", true)) {
    float value = request->getParam("value", true)->value().toFloat();
    updateLEDBrightness(value);
    request->send(200, "text/plain", "OK");
  } else {
    request->send(400, "text/plain", "Missing 'value'");
  }
}

void handleSetDarkMode(AsyncWebServerRequest *request) {
  if (request->hasParam("mode", true)) {
    String mode = request->getParam("mode", true)->value();
    setDarkMode(mode == "dark");
    request->send(200, "text/plain", "OK");
  } else {
    request->send(400, "text/plain", "Missing 'mode'");
  }
}

void handleSetClockName(AsyncWebServerRequest *request) {
  if (request->hasParam("name")) {
      String newName = request->getParam("name")->value();
      setNameOfClock(newName);
      request->send(200, "text/plain", "OK");
      delay(500);
      ESP.restart();  // Neustart nach Speichern
  } else {
      request->send(400, "text/plain", "Missing parameter");
  }
}

void handleGetSettings(AsyncWebServerRequest *request) {
  String json = "{";
  json += "\"brightness\":" + String(getLEDBrightness(), 2);
  json += ",\"theme\":\"" + String(isDarkModeEnabled() ? "dark" : "light") + "\"";
  json += "}";
  request->send(200, "application/json", json);
}

void handleScan(AsyncWebServerRequest *request) {
  int scanStatus = WiFi.scanComplete();
  Serial.print("WiFi.scanComplete() = ");
  Serial.println(scanStatus);

  if (scanStatus == WIFI_SCAN_RUNNING) {
    request->send(202, "application/json", "[]");
    return;
  }

  if (scanStatus == WIFI_SCAN_FAILED || scanStatus == -2) {
    Serial.println("Starte WLAN-Scan...");
    WiFi.scanNetworks(true);
    request->send(202, "application/json", "[]");
    return;
  }

  if (scanStatus >= 0) {
    Serial.printf("Gefundene Netzwerke: %d\n", scanStatus);
    String json = "[";
    for (int i = 0; i < scanStatus; ++i) {
      String ssid = WiFi.SSID(i);
      Serial.printf("  %d: %s (RSSI: %d)\n", i + 1, ssid.c_str(), WiFi.RSSI(i));
      json += "\"" + ssid + "\"";
      if (i < scanStatus - 1) json += ",";
    }
    json += "]";
    WiFi.scanDelete();
    request->send(200, "application/json", json);
    return;
  }

  request->send(500, "application/json", "[]");
}

void handleSetTime(AsyncWebServerRequest *request) {
  if (request->hasParam("hours", true) &&
      request->hasParam("minutes", true) &&
      request->hasParam("seconds", true)) {

    int hours   = request->getParam("hours", true)->value().toInt();
    int minutes = request->getParam("minutes", true)->value().toInt();
    int seconds = request->getParam("seconds", true)->value().toInt();

    setRTCtime(hours, minutes, seconds);

    request->send(200, "text/plain", "Time set successfully");
  } else {
    request->send(400, "text/plain", "Missing parameters");
  }
}

void handleSetDate(AsyncWebServerRequest *request) {
  if (request->hasParam("day", true) &&
      request->hasParam("month", true) &&
      request->hasParam("year", true)) {

    int day   = request->getParam("day", true)->value().toInt();
    int month = request->getParam("month", true)->value().toInt();
    int year  = request->getParam("year", true)->value().toInt();

    updateRTCDate(year, month, day);

    request->send(200, "text/plain", "Date set successfully");
  } else {
    request->send(400, "text/plain", "Missing parameters");
  }
}

void handleGetDateTime(AsyncWebServerRequest *request) {
  DateTime now = getDateTime();

  String payload = "{";
  payload += "\"h10\":" + String(now.hour() / 10) + ",";
  payload += "\"h1\":" + String(now.hour() % 10) + ",";
  payload += "\"m10\":" + String(now.minute() / 10) + ",";
  payload += "\"m1\":" + String(now.minute() % 10) + ",";
  payload += "\"s10\":" + String(now.second() / 10) + ",";
  payload += "\"s1\":" + String(now.second() % 10) + ",";

  payload += "\"d10\":" + String(now.day() / 10) + ",";
  payload += "\"d1\":" + String(now.day() % 10) + ",";
  payload += "\"mo10\":" + String(now.month() / 10) + ",";
  payload += "\"mo1\":" + String(now.month() % 10) + ",";
  int year = now.year();
  payload += "\"y1000\":" + String(year / 1000) + ",";
  payload += "\"y100\":" + String((year / 100) % 10) + ",";
  payload += "\"y10\":" + String((year / 10) % 10) + ",";
  payload += "\"y1\":" + String(year % 10);
  payload += "}";

  request->send(200, "application/json", payload);
}

void handleGetSleepSettings(AsyncWebServerRequest *request) {
  String json = "{";
  json += "\"useSleepMode\":" + String(getUseSleepMode());
  json += ",\"timeToSleep\":" + String(getTimeToSleep());
  json += "}";
  request->send(200, "application/json", json);
}

void handleSetSleepSettings(AsyncWebServerRequest *request) {
  if (request->hasParam("useSleepMode", true) && request->hasParam("timeToSleep", true)) {
    int mode = request->getParam("useSleepMode", true)->value().toInt();
    int minutes = request->getParam("timeToSleep", true)->value().toInt();

    setUseSleepMode(mode);
    setTimeToSleep(minutes);

    request->send(200, "text/plain", "Sleep settings updated");
  } else {
    request->send(400, "text/plain", "Missing parameters");
  }
}

void handleRadar(AsyncWebServerRequest *request) {
  int state = digitalRead(RADAR_PIN);
  String json = "{\"radar\":" + String(state) + "}";
  request->send(200, "application/json", json);
}

void handleSetAnimMode(AsyncWebServerRequest *request) {
  if (request->hasParam("mode")) {
    int newMode = request->getParam("mode")->value().toInt();

    Serial.printf("Setze ziAniMode auf %d\n", newMode);

    setziAniMode(newMode);

    timeOrDate();

    request->send(200, "text/plain", "OK");
  } else {
    request->send(400, "text/plain", "Fehler: Parameter fehlt");
  }
}

void handleSetWifi(AsyncWebServerRequest *request) {
  if (request->hasParam("ssid", true) && request->hasParam("password", true)) {
    String ssid = request->getParam("ssid", true)->value();
    String password = request->getParam("password", true)->value();

    setSSID(ssid);
    setWlanPW(password);

    request->send(200, "text/plain", "WiFi credentials saved. Restarting...");
    
    // delay(1000);
    // Update Wifi
  } else {
    request->send(400, "text/plain", "Missing parameters");
  }
}

void handleGetNetworkInfo(AsyncWebServerRequest *request) {
  String json = "{";
  json += "\"ssid\":\"" + getSSID() + "\"";
  json += ",\"ntpServer\":\"" + getNtpServer() + "\"";
  json += ",\"timeZone\":\"" + getTimeZone() + "\"";
  json += "}";
  request->send(200, "application/json", json);
}

void handleSetNtpServer(AsyncWebServerRequest *request) {
  if (request->hasParam("ntpServer", true)) {
    String ntpServer = request->getParam("ntpServer", true)->value();
    setNtpServer(ntpServer);
    request->send(200, "text/plain", "NTP server updated");
    initNtpServer(ntpServer);  // NTP-Server sofort anwenden
  } else {
    request->send(400, "text/plain", "Missing 'ntpServer' parameter");
  }
}

void handleSetTimeZone(AsyncWebServerRequest *request) {
  if (request->hasParam("timeZone", true)) {
    String timeZone = request->getParam("timeZone", true)->value();
    setTimeZone(timeZone);
    request->send(200, "text/plain", "Time zone updated");
    initTimeZone(timeZone);  // Zeitzone sofort anwenden
  } else {
    request->send(400, "text/plain", "Missing 'timeZone' parameter");
  }
}

void setupWebServerHandlers(AsyncWebServer& server)
{
  // Captive-Portal-Routen für Android/iOS/Windows
  server.on("/generate_204", HTTP_GET, handleRedirectToRoot);
  server.on("/hotspot-detect.html", HTTP_GET, handleRedirectToRoot);
  server.on("/ncsi.txt", HTTP_GET, handleNCSI);
  server.on("/connecttest.txt", HTTP_GET, handleRedirectToRoot);
  server.on("/redirect", HTTP_GET, handleRedirectToRoot);

  // Hauptseite
  server.on("/", HTTP_GET, handleMainPage);

  // POST: LED-Helligkeit
  server.on("/setLEDBrightness", HTTP_POST, handleSetLEDBrightness);

  // POST: Dark Mode
  server.on("/setDarkMode", HTTP_POST, handleSetDarkMode);

  server.on("/setClockName", HTTP_GET, handleSetClockName);

  // GET: Settings
  server.on("/getSettings", HTTP_GET, handleGetSettings);

  // WLAN-Scan
  server.on("/scan", HTTP_GET, handleScan);

  server.on("/setTime", HTTP_POST, handleSetTime);

  server.on("/setDate", HTTP_POST, handleSetDate);

  server.on("/getDateTime", HTTP_GET, handleGetDateTime);

  server.on("/getSleepSettings", HTTP_GET, handleGetSleepSettings);

  server.on("/setSleepSettings", HTTP_POST, handleSetSleepSettings);

  // Falls du zusätzlich noch eine JSON-Abfrage willst:
  server.on("/radar", HTTP_GET, handleRadar);

  // Route: Ziffern-Animationsmodus setzen
  server.on("/setAnimMode", HTTP_GET, handleSetAnimMode);

  server.on("/setWifi", HTTP_POST, handleSetWifi);
  server.on("/getNetworkInfo", HTTP_GET, handleGetNetworkInfo);
  server.on("/setNtpServer", HTTP_POST, handleSetNtpServer);
  server.on("/setTimeZone", HTTP_POST, handleSetTimeZone);

  // Events-Handler (SSE) registrieren
  server.addHandler(&events);
}

// Initialisiert den Webserver und richtet alle Routen ein
void setupWebServer() {
  // DNS-Server für Captive Portal
  dnsServer.start(DNS_PORT, "*", WiFi.softAPIP());

  setupWebServerHandlers(server);

  pinMode(RADAR_PIN, INPUT);

  server.begin();
}

// Verarbeitet DNS-Anfragen für das Captive Portal
void processDNS() {
  dnsServer.processNextRequest();
}

void loopRadarCheck() {
  int current = digitalRead(RADAR_PIN);
  if (current != lastRadarState) {
    lastRadarState = current;
    events.send(String(current).c_str(), "radar", millis());
  }
}

void printLocalTime(){
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)){
    Serial.println("Failed to obtain time 1");
    return;
  }
  Serial.print("getLocalTime: ");
  Serial.println(&timeinfo, "%Y-%m-%d %H:%M:%S");
}

void sendTimeEvent() {
  DateTime now = getDateTime();
  
  char buffer[20];
  snprintf(buffer, sizeof(buffer), "%04d-%02d-%02d %02d:%02d:%02d",
           now.year(), now.month(), now.day(),
           now.hour(), now.minute(), now.second());
  String payload = String(buffer);
  events.send(payload.c_str(), "time", millis());
  
  Serial.println("RTC Time: " + payload);
  printLocalTime();
}
