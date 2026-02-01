#ifndef LEDS_H
#define LEDS_H

#include <Arduino.h>

// Initialisiert die LED-Hardware und -Effekte
void initLEDs();
// Setzt den gewünschten LED-Effektmodus
void setLEDEffect(uint8_t mode);     // z. B. 0 = aus, 1 = Regenbogen, 2 = Lauflicht
// Aktualisiert den aktuellen LED-Effekt (Animation)
void updateLEDEffect();              // aufrufen in loop() oder per Timer
// Setzt die LED-Helligkeit (0.0 bis 1.0)
void setLEDBrightness(float value);  // Helligkeit der LEDs einstellen

uint8_t getLEDEffect();

uint8_t getLEDEffectLast();

void saveLEDEffectLast(uint8_t mode);

bool areLEDsReady();

#endif
