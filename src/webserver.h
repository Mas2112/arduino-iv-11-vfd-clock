#pragma once

#include <ESPAsyncWebServer.h>

extern AsyncWebServer server;

void setupWebserver();
void loopRadarCheck();

// Initialisiert den Webserver und richtet alle Routen ein
void setupWebServer();
// Verarbeitet DNS-Anfragen für das Captive Portal
void processDNS();  // <--- NEU hinzugefügt