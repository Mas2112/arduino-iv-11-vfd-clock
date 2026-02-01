#include "leds.h"
#include <NeoPixelBus.h>

#define LED_PIN     9
#define LED_COUNT   6

NeoPixelBus<NeoGrbFeature, Neo800KbpsMethod> strip(LED_COUNT, LED_PIN);

static uint8_t effectMode = 0; //aktueller LED-Effekt
static uint8_t effectModeLast = 0; //letzter LED-Effekt
static uint16_t rainbowOffset = 0;
static uint8_t runnerPos = 0;
static float ledBrightness = 1.0f;  // Bereich: 0.0 bis 1.0
static bool ledsReady = false;

// Initialisiert die LED-Hardware und -Effekte
void initLEDs() {
    strip.Begin();
    strip.Show();  // Alle LEDs aus
    ledsReady = true;   // Signal: LEDs betriebsbereit
}


// Setzt die LED-Helligkeit (0.0 bis 1.0)
void setLEDBrightness(float value) {
    if (value < 0.0f) value = 0.0f;
    if (value > 1.0f) value = 1.0f;
    ledBrightness = value;
}

RgbColor scaleColor(const RgbColor& color, float brightness) {
    uint8_t r = static_cast<uint8_t>(color.R * brightness);
    uint8_t g = static_cast<uint8_t>(color.G * brightness);
    uint8_t b = static_cast<uint8_t>(color.B * brightness);
    return RgbColor(r, g, b);
}

// Setzt den gewünschten LED-Effektmodus
void setLEDEffect(uint8_t mode) {
    if (mode == 0 && millis() < 8000) return; // nicht gleich beim Start ausschalten
    effectMode = mode;
    rainbowOffset = 0;
    runnerPos = 0;
}

// Aktualisiert den aktuellen LED-Effekt (Animation)
void updateLEDEffect() {
    if (effectMode == 0) {
        // Alle aus
        for (int i = 0; i < LED_COUNT; i++) {
            strip.SetPixelColor(i, RgbColor(0, 0, 0));
        }
        strip.Show();
    }
    else if (effectMode == 1) {
        // Regenbogen-Farbverlauf
        for (int i = 0; i < LED_COUNT; i++) {
            float hue = fmod((rainbowOffset + i * 60.0f), 360.0f);
            RgbColor color = HslColor(hue / 360.0f, 1.0f, 0.5f);
            strip.SetPixelColor(i, scaleColor(color, ledBrightness));
        }
        strip.Show();
        rainbowOffset = (rainbowOffset + 5) % 360;
    }
    else if (effectMode == 2) {
        // Lauflicht
        for (int i = 0; i < LED_COUNT; i++) {
            RgbColor color = (i == runnerPos) ? RgbColor(255, 50, 0) : RgbColor(0, 0, 0);
            strip.SetPixelColor(i, scaleColor(color, ledBrightness));
        }
        strip.Show();
        runnerPos = (runnerPos + 1) % LED_COUNT;
    }
}


uint8_t getLEDEffect() {
    return effectMode;
}

uint8_t getLEDEffectLast() {
    return effectModeLast;
}

void saveLEDEffectLast(uint8_t mode) {
    effectModeLast = mode;
}

bool areLEDsReady() {
    return ledsReady;
}
