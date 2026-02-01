#include <ESPAsyncWebServer.h>
#include <DNSServer.h>
#include <WiFi.h>
#include "index_html.h"
#include "leds.h"
#include "settings.h"
#include "clocktime.h"
#include "webserver.h"
#include "display.h"

#define RADAR_PIN 13   //Radar-Sensor GPIO

const byte DNS_PORT = 53;
DNSServer dnsServer;
AsyncWebServer server(80);

AsyncEventSource events("/events");   // für Server-Sent Events
int lastRadarState = -1;

// Initialisiert den Webserver und richtet alle Routen ein
void setupWebServer() {
  // DNS-Server für Captive Portal
  dnsServer.start(DNS_PORT, "*", WiFi.softAPIP());

  // Captive-Portal-Routen für Android/iOS/Windows
  server.on("/generate_204", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->redirect("/");
  });

  server.on("/hotspot-detect.html", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->redirect("/");
  });

  server.on("/ncsi.txt", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/plain", "Microsoft NCSI");
  });

  server.on("/connecttest.txt", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->redirect("/");
  });

  server.on("/redirect", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->redirect("/");
  });

  // Hauptseite
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
      String html = MAIN_page;
      html.replace("%CLOCKNAME%", getNameOfClock());
      request->send(200, "text/html", html);
  });

  // POST: LED-Helligkeit
  server.on("/setLEDBrightness", HTTP_POST, [](AsyncWebServerRequest *request) {
    if (request->hasParam("value", true)) {
      float value = request->getParam("value", true)->value().toFloat();
      updateLEDBrightness(value);
      request->send(200, "text/plain", "OK");
    } else {
      request->send(400, "text/plain", "Missing 'value'");
    }
  });

  // POST: Dark Mode
  server.on("/setDarkMode", HTTP_POST, [](AsyncWebServerRequest *request) {
    if (request->hasParam("mode", true)) {
      String mode = request->getParam("mode", true)->value();
      setDarkMode(mode == "dark");
      request->send(200, "text/plain", "OK");
    } else {
      request->send(400, "text/plain", "Missing 'mode'");
    }
  });

  server.on("/setClockName", HTTP_GET, [](AsyncWebServerRequest *request) {
      if (request->hasParam("name")) {
          String newName = request->getParam("name")->value();
          setNameOfClock(newName);
          request->send(200, "text/plain", "OK");
          delay(500);
          ESP.restart();  // Neustart nach Speichern
      } else {
          request->send(400, "text/plain", "Missing parameter");
      }
  });

  // GET: Settings
  server.on("/getSettings", HTTP_GET, [](AsyncWebServerRequest *request) {
    String json = "{";
    json += "\"brightness\":" + String(getLEDBrightness(), 2);
    json += ",\"theme\":\"" + String(isDarkModeEnabled() ? "dark" : "light") + "\"";
    json += "}";
    request->send(200, "application/json", json);
  });

  // WLAN-Scan
    server.on("/scan", HTTP_GET, [](AsyncWebServerRequest *request) {
    int scanStatus = WiFi.scanComplete();
    Serial.print("WiFi.scanComplete() = ");
    Serial.println(scanStatus);

    if (scanStatus == WIFI_SCAN_RUNNING) {
      request->send(202, "application/json", "[]");
      return;
    }

    if (scanStatus == WIFI_SCAN_FAILED || scanStatus == -2) {
      Serial.println("Starte WLAN-Scan...");
      WiFi.scanNetworks(true);  // Asynchron starten
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

    request->send(500, "application/json", "[]"); // Unerwarteter Zustand
  });

  server.on("/setTime", HTTP_POST, [](AsyncWebServerRequest *request) {
    if (request->hasParam("hours", true) &&
        request->hasParam("minutes", true) &&
        request->hasParam("seconds", true)) {

      int hours   = request->getParam("hours", true)->value().toInt();
      int minutes = request->getParam("minutes", true)->value().toInt();
      int seconds = request->getParam("seconds", true)->value().toInt();

      // Übergabe an clocktime.cpp
      setRTCtime(hours, minutes, seconds);

      request->send(200, "text/plain", "Time set successfully");
    } else {
      request->send(400, "text/plain", "Missing parameters");
    }
  });

  server.on("/setDate", HTTP_POST, [](AsyncWebServerRequest *request) {
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
  });

  server.on("/getDateTime", HTTP_GET, [](AsyncWebServerRequest *request) {
    DateTime now = getDateTime();   // vorhandene Funktion aus clocktime.cpp

    // JSON mit Ziffern aufbereiten
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
  });

  server.on("/getSleepSettings", HTTP_GET, [](AsyncWebServerRequest *request) {
    String json = "{";
    json += "\"useSleepMode\":" + String(getUseSleepMode());
    json += ",\"timeToSleep\":" + String(getTimeToSleep());
    json += "}";
    request->send(200, "application/json", json);
  });

  server.on("/setSleepSettings", HTTP_POST, [](AsyncWebServerRequest *request) {
    if (request->hasParam("useSleepMode", true) && request->hasParam("timeToSleep", true)) {
      int mode = request->getParam("useSleepMode", true)->value().toInt();
      int minutes = request->getParam("timeToSleep", true)->value().toInt();

      setUseSleepMode(mode);
      setTimeToSleep(minutes);

      request->send(200, "text/plain", "Sleep settings updated");
    } else {
      request->send(400, "text/plain", "Missing parameters");
    }
  });

    pinMode(RADAR_PIN, INPUT);

    // Events-Handler (SSE) registrieren
    server.addHandler(&events);

    // Falls du zusätzlich noch eine JSON-Abfrage willst:
    server.on("/radar", HTTP_GET, [](AsyncWebServerRequest *request){
      int state = digitalRead(RADAR_PIN);
      String json = "{\"radar\":" + String(state) + "}";
      request->send(200, "application/json", json);
    });

    // Route: Ziffern-Animationsmodus setzen
    server.on("/setAnimMode", HTTP_GET, [](AsyncWebServerRequest *request) {
        if (request->hasParam("mode")) {
            int newMode = request->getParam("mode")->value().toInt();

            // Debug-Ausgabe
            Serial.printf("Setze ziAniMode auf %d\n", newMode);

            // Speichern in RAM und NVS
            setziAniMode(newMode);

            // Direkt anwenden: alle Tasks, die Animation nutzen, lesen nun getziAniMode()
            // Optional: Live-Anzeige aktualisieren
            timeOrDate(); // einmalige Aktualisierung nach Änderung

            request->send(200, "text/plain", "OK");
        } else {
            request->send(400, "text/plain", "Fehler: Parameter fehlt");
        }
    });

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
