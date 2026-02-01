
#include "esp_timer.h"

esp_timer_handle_t display_timer;

/*
Anzeigetreiber: MAX6921
Es werden 6 VFD IV-11 mit je 7 Segmenten und einem Punkt angrsteuert.
Röhre 1: Stunden  10er
Röhre 2: Stunden  1er
Röhre 3: Minuten  10er
Röhre 4: Minuten  1er
Röhre 5: Sekunden 10er
Röhre 6: Sekunden 1er

Pin-Belegung MAX6921

Gitter Röhre 1: OUT9
Gitter Röhre 2: OUT8
Gitter Röhre 3: OUT7
Gitter Röhre 4: OUT2
Gitter Röhre 5: OUT1
Gitter Röhre 6: OUT0

Die Segmente aller Röhren sind parallel geschaltet und diesen Pins zugeordnet:
A: OUT14
B: OUT17
C: OUT10
D: OUT19
E: OUT18
F: OUT16
G: OUT12
P: OUT11

Die Röhren werden im Multipex-Betrieb angesteuert.
Um eine optisch stabile Anzeige zu bekommen soll die komplette Anzeige ca. 40x pro Sekunde aktualisiert werden.
Der Punkt kann zu verschiedenen Zwecken an allen Röhren angezeigt werden.
Der MAX6921 wird per SPI angesteuert.
Um Konflikte z.B. mit der Ansteuerung der NeoPixel-LEDs zu vermeiden wird eine eigene Instanz der SPI benutzt.
Es wird ein 20bit-Datenwort mit MSBfirst gesendet.
Um für den Anfang stabile Funktion zu haben, wird ein Takt von 250kHz verwendet.
Die Funktion sendToMax6921(???? data) sendet die Daten (20bit) zum MAX6921.
(???? sind noch durch geeignete Definition zu ersetztn)
Die Funktion setDisplayTime(time_t now) bereitet die übergebenen Daten auf und ruft zyklisch die Funktion sendToMax6921(???? data) auf.
Um die Anzeige 40x pro Sekunde aktualisieren zu können soll dies Interrupt-getrieben im Hintergrund, also nicht in "loop" passieren.

*/
#include "display.h"
#include <SPI.h>
#include <time.h>
#include <Arduino.h>
#include "clocktime.h"
#include "settings.h"
#include "leds.h"



// Interne Variablen für Punktsteuerung
static uint8_t dotMode = 0;
static uint8_t dotState = 0;
static uint8_t dotIndex = 0;
static TickType_t lastDotUpdateTick = 0;



//Segment-Zeilen-Masken für Animation der Ziffern
#define SEG_A 0x01
#define SEG_B 0x02
#define SEG_C 0x04
#define SEG_D 0x08
#define SEG_E 0x10
#define SEG_F 0x20
#define SEG_G 0x40
#define DOT_SEGMENT 0x80

/*
const uint16_t lineMasks[5] = {
  SEG_A,              // Zeile 1
  SEG_F | SEG_B,      // Zeile 2
  SEG_G,              // Zeile 3
  SEG_E | SEG_C,      // Zeile 4
  SEG_D               // Zeile 5
};
*/

const uint32_t lineMasks[5] = {
  (1 << 14),                         // Zeile 1: Segment A
  (1 << 16) | (1 << 17),             // Zeile 2: Segmente F + B
  (1 << 12),                         // Zeile 3: Segment G
  (1 << 10) | (1 << 18),             // Zeile 4: Segmente C + E
  (1 << 19)                          // Zeile 5: Segment D
};

//volatile bool animating = false; //für Animation


//#define VSPI_HOST 2
#define VSPI_HOST 1

SPIClass mySPI(VSPI_HOST); // oder HSPI, je nach freiem SPI-Controller

// MAX6921 Pins (an ESP32-S3-Zero)
#define DIN_PIN   2
#define BLANK_PIN 3
#define CLK_PIN   4
#define LOAD_PIN  5

// Segment-Pin-Zuordnung am MAX6921
// A: OUT14, B: OUT17, C: OUT10, D: OUT19, E: OUT18, F: OUT16, G: OUT12, P: OUT11

// Gitter für 6 Röhren: OUT9, OUT8, OUT7, OUT2, OUT1, OUT0
const uint32_t gridMap[6] = {
  (1<<9), (1<<8), (1<<7), (1<<2), (1<<1), (1<<0)
};

// Zahlenmuster 0–9 für die Segmente (A–G)
const uint32_t segmentMap[10] = {
  (1<<14)|(1<<17)|(1<<10)|(1<<19)|(1<<18)|(1<<16),           // 0
  (1<<17)|(1<<10),                                           // 1
  (1<<14)|(1<<17)|(1<<12)|(1<<18)|(1<<19),                   // 2
  (1<<14)|(1<<17)|(1<<10)|(1<<12)|(1<<19),                   // 3
  (1<<16)|(1<<12)|(1<<17)|(1<<10),                           // 4
  (1<<14)|(1<<16)|(1<<12)|(1<<10)|(1<<19),                   // 5
  (1<<14)|(1<<16)|(1<<12)|(1<<10)|(1<<18)|(1<<19),           // 6
  (1<<14)|(1<<17)|(1<<10),                                   // 7
  (1<<14)|(1<<17)|(1<<10)|(1<<12)|(1<<16)|(1<<18)|(1<<19),   // 8
  (1<<14)|(1<<17)|(1<<10)|(1<<12)|(1<<16)|(1<<19)            // 9
};

// Punktsegment (optional)
#define DOT_SEGMENT (1<<11)

// Zustände der Punkte
static bool dotStates[6] = {false, false, false, false, false, false};

//Zwischenspeicher für Animationen
int oldTimeDigits[6] = {0, 0, 0, 0, 0, 0}; // alter Zustand der Segmente der Ziffern bei Zeitanzeige
int oldDateDigits[6] = {0, 0, 0, 0, 0, 0}; // alter Zustand der Segmente der Ziffern bei Datumsanzeige

// Anzeige-Daten für jede Röhre
uint32_t displayData[6] = {0};

// Multiplex-Zähler
volatile int currentTube = 0;

//Pin-Zuordnungen Röhren Ein- und Aus-schalten
#define Radar_PIN   13
#define Pwr_on_PIN 1

static unsigned long tubeSleepLastUpdate = 0;
static unsigned long tubeSleepStartDelayUntil = 0; // Ende der Startphase
static int sleepCounter = 0;



// Initialisiert die Röhrenanzeige
void setupDisplay() {
  const esp_timer_create_args_t timer_args = {
    .callback = onTimer,
    .arg = nullptr,
    .dispatch_method = ESP_TIMER_TASK,
    .name = "display_timer"
  };

  esp_err_t err = esp_timer_create(&timer_args, &display_timer);
  if (err != ESP_OK) {
    Serial.println("Fehler beim Erstellen des Timers!");
    return;
  }

  err = esp_timer_start_periodic(display_timer, 3000); // alle 3ms
  if (err != ESP_OK) {
    Serial.println("Fehler beim Starten des Timers!");
  }

  pinMode(DIN_PIN, OUTPUT);
  pinMode(BLANK_PIN, OUTPUT);
  pinMode(CLK_PIN, OUTPUT);
  pinMode(LOAD_PIN, OUTPUT);

  digitalWrite(BLANK_PIN, LOW); // Anzeige aktivieren
  mySPI.begin(CLK_PIN, -1, DIN_PIN, LOAD_PIN); // CLK, MISO, MOSI, SS

  // GPIO13 als Eingang für Radar-Bewegungssensor konfigurieren
  pinMode(Radar_PIN, INPUT); // Radar-Sensor an GPIO6
  // GPIO1 als Ausgang für Röhren-Stromversorgung konfigurieren
  pinMode(Pwr_on_PIN, OUTPUT); // Röhren-Stromversorgung

  setTubes(true);  // Röhren initial einschalten

  // Warten, bis LED-Subsystem bereit
  unsigned long start = millis();
  while (!areLEDsReady() && millis() - start < 2000) {
      vTaskDelay(pdMS_TO_TICKS(50));
  }

  setLEDEffect(getLEDEffectLast());
  sleepCounter = getTimeToSleep();
  tubeSleepLastUpdate = millis();

  }

// Sendet ein 20-Bit-Datenwort an den MAX6921
void sendToMax6921(uint32_t data) {
  
  digitalWrite(LOAD_PIN, LOW);
  mySPI.beginTransaction(SPISettings(4000000, MSBFIRST, SPI_MODE0)); //4MHz

  mySPI.transfer((data >> 16) & 0xFF);
  mySPI.transfer((data >> 8) & 0xFF);
  mySPI.transfer(data & 0xFF);

  mySPI.endTransaction();
  delayMicroseconds(1);
  digitalWrite(LOAD_PIN, HIGH);
  delayMicroseconds(1);
  digitalWrite(LOAD_PIN, LOW);
}

// Timer-Interrupt-Funktion für Multiplexing
void IRAM_ATTR onTimer(void* arg) {
  // Segmentdaten + Grid-Maske für aktuelle Röhre senden
  sendToMax6921(displayData[currentTube] | gridMap[currentTube]);
  // Nächste Röhre vorbereiten
  currentTube = (currentTube + 1) % 6;
 }


//Entscheiden, ob Uhrzeit, oder Datum angezeigt werden sollen
void timeOrDate() {
  time_t now = getCurrentTime();
  struct tm *t = localtime(&now);
  int s = t->tm_sec;

  //Serial.printf("timeOrDate(): ziAniMode = %d\n", getziAniMode());

  int mode1 = getziAniMode();   // 0 = statisch, 1 = animiert

  bool showDateNow = (s >= 30 && s < 33);

  if (showDateNow) {
    if (mode1 == 1) {
      showDateAnimated(now);
    } else {
      showDate(now);
    }
  } else {
    if (mode1 == 1) {
      showTimeAnimated(now);
    } else {
      showTime(now);
    }
  }
}

//Uhrzeit anzeigen
void showTime(time_t now) {
  setDotsMode(4); // Die Punkte bei Stunden-Einer und bei Minuten-Einer blinken im Sekundentakt wechselseitig
  struct tm *t = localtime(&now);
  int h = t->tm_hour;
  int m = t->tm_min;
  int s = t->tm_sec;

  displayData[0] = segmentMap[h / 10] | (dotStates[0] ? DOT_SEGMENT : 0);
  displayData[1] = segmentMap[h % 10] | (dotStates[1] ? DOT_SEGMENT : 0);
  displayData[2] = segmentMap[m / 10] | (dotStates[2] ? DOT_SEGMENT : 0);
  displayData[3] = segmentMap[m % 10] | (dotStates[3] ? DOT_SEGMENT : 0);
  displayData[4] = segmentMap[s / 10] | (dotStates[4] ? DOT_SEGMENT : 0);
  displayData[5] = segmentMap[s % 10] | (dotStates[5] ? DOT_SEGMENT : 0);
}

//Datum anzeigen
void showDate(time_t now) {
  setDotsMode(2); // Es wird ein Punkt bei Stunden-Einer und ein Punkt bei Minuten-Einer dargestellt
  struct tm *t = localtime(&now);
  int d = t->tm_mday;
  int mo = t->tm_mon + 1;
  int y = t->tm_year % 100;

  displayData[0] = segmentMap[d / 10] | (dotStates[0] ? DOT_SEGMENT : 0);
  displayData[1] = segmentMap[d % 10] | (dotStates[1] ? DOT_SEGMENT : 0);
  displayData[2] = segmentMap[mo / 10] | (dotStates[2] ? DOT_SEGMENT : 0);
  displayData[3] = segmentMap[mo % 10] | (dotStates[3] ? DOT_SEGMENT : 0);
  displayData[4] = segmentMap[y / 10] | (dotStates[4] ? DOT_SEGMENT : 0);
  displayData[5] = segmentMap[y % 10] | (dotStates[5] ? DOT_SEGMENT : 0);
}

//Uhrzeit animiert anzeigen
void showTimeAnimated(time_t now) {
  setDotsMode(4);
  struct tm *t = localtime(&now);
  int newDigits[6] = {
    t->tm_hour / 10,
    t->tm_hour % 10,
    t->tm_min / 10,
    t->tm_min % 10,
    t->tm_sec / 10,
    t->tm_sec % 10
  };

  for (int i = 5; i >= 0; i--) { // von rechts nach links
    if (oldTimeDigits[i] != newDigits[i]) {
      animateDigitChange(i, oldTimeDigits[i], newDigits[i]);
      oldTimeDigits[i] = newDigits[i];
    } else {
      displayData[i] = segmentMap[newDigits[i]] | (dotStates[i] ? DOT_SEGMENT : 0);
    }
  }
}


//Datum animiert anzeigen
void showDateAnimated(time_t now) {
  setDotsMode(2);
  struct tm *t = localtime(&now);
  int newDigits[6] = {
    t->tm_mday / 10,
    t->tm_mday % 10,
    (t->tm_mon + 1) / 10,
    (t->tm_mon + 1) % 10,
    (t->tm_year % 100) / 10,
    (t->tm_year % 100) % 10
  };

  for (int i = 5; i >= 0; i--) { // von rechts nach links
    if (oldDateDigits[i] != newDigits[i]) {
      animateDigitChange(i, oldDateDigits[i], newDigits[i]);
      oldDateDigits[i] = newDigits[i];
    } else {
      displayData[i] = segmentMap[newDigits[i]] | (dotStates[i] ? DOT_SEGMENT : 0);
    }
  }
}



//Segment-Mixer für Animation (Übergang von der alten zur neuen Ziffer)
uint32_t mixSegmentsByLines(uint32_t fromSeg, uint32_t toSeg, int frame, int totalFrames) {
  if (frame == totalFrames - 1) {
    // Letzter Frame: komplette neue Ziffer anzeigen
    return toSeg;
  }

  uint32_t result = 0;
  int half = totalFrames / 2;

  if (frame < half) {
    // Alte Ziffer wird zeilenweise ausgeblendet
    for (int i = frame; i < 5; i++) {
      result |= fromSeg & lineMasks[i];
    }
  } else {
    // Neue Ziffer wird zeilenweise eingeblendet
    for (int i = 0; i < frame - half + 1; i++) {
      result |= toSeg & lineMasks[i];
    }
  }

  return result;
}


void animateDigitChange(int pos, int fromDigit, int toDigit) {
  const int frameCount = 8;
  uint32_t tempDisplay[6];
  memcpy(tempDisplay, displayData, sizeof(displayData)); // Kopie erstellen

  for (int f = 0; f < frameCount; f++) {
    tempDisplay[pos] = mixSegmentsByLines(segmentMap[fromDigit], segmentMap[toDigit], f, frameCount)
                        | (dotStates[pos] ? DOT_SEGMENT : 0);
    memcpy(displayData, tempDisplay, sizeof(displayData)); // Anzeige aktualisieren
    delay(50);
  }
}

void animateDigitsFramewise(int changedIndices[], int fromDigits[], int toDigits[], int count) {
  const int frameCount = 8;
  for (int f = 0; f < frameCount; f++) {
    for (int i = 0; i < count; i++) {
      int pos = changedIndices[i];
      uint32_t frameData = mixSegmentsByLines(segmentMap[fromDigits[i]], segmentMap[toDigits[i]], f, frameCount);
      displayData[pos] = frameData | (dotStates[pos] ? (1 << 11) : 0);
    }
    delay(50); // Anzeige wird durch Interrupt aktualisiert
  }
}


// Funktionen für Punkt-Animation
// 0 --> kein Punkt zu sehen
// 1 --> Punkt läuft von links nach rechts im 0,5s-Takt durch
// 2 --> Es wird ein Punkt bei Stunden-Einer und ein Punkt bei Minuten-Einer dargestellt (für Datum)
// 3 --> die Punkte bei Stunden-Einer und bei Minuten-Einer blinken im Sekundentakt gleichzeitig
// 4 --> die Punkte bei Stunden-Einer und bei Minuten-Einer blinken im Sekundentakt wechselseitig
// 5 --> alle 6 Punkte blinken im Sekundentakt gleichzeitig.

// Setzt den Punktmodus (0–5)
void setDotsMode(uint8_t mode) {
    dotMode = mode;
}

// Aktualisiert die Punkte entsprechend dem gewählten Modus
void updateDots() {
    TickType_t now = xTaskGetTickCount();

    switch (dotMode) {
        case 0:
            // Alle Punkte aus
            setAllDots(false);
            break;

        case 1:
            // Lauflicht von links nach rechts im 0,5s-Takt
            if (now - lastDotUpdateTick >= pdMS_TO_TICKS(500)) {
                lastDotUpdateTick = now;
                // Nur den aktuellen Punkt setzen, alle anderen bleiben wie sie sind
                for (uint8_t i = 0; i < 6; i++) {
                    setDot(i, i == dotIndex);
                }
                dotIndex = (dotIndex + 1) % 6;
            }
            break;

        case 2:
            // Punkte bei Stunden-Einer (1) und Minuten-Einer (3) dauerhaft an
            setDot(1, true);
            setDot(3, true);
            break;

        case 3:
            // Beide Punkte blinken gleichzeitig im Sekundentakt
            if (now - lastDotUpdateTick >= pdMS_TO_TICKS(1000)) {
                lastDotUpdateTick = now;
                dotState = !dotState;
                setDot(1, dotState);
                setDot(3, dotState);
            }
            break;

        case 4:
            // Punkte blinken wechselseitig im Sekundentakt
            if (now - lastDotUpdateTick >= pdMS_TO_TICKS(1000)) {
                lastDotUpdateTick = now;
                dotState = !dotState;
                setDot(1, dotState);
                setDot(3, !dotState);
            }
            break;

        case 5:
            // Alle Punkte blinken gleichzeitig im Sekundentakt
            if (now - lastDotUpdateTick >= pdMS_TO_TICKS(1000)) {
                lastDotUpdateTick = now;
                dotState = !dotState;
                setAllDots(dotState);
            }
            break;
    }
}

// Setzt oder löscht einen einzelnen Punkt (0–5)

void setDot(uint8_t index, bool state) {
    if (index >= 6) return;
    dotStates[index] = state;
}

// Setzt alle Punkte gleichzeitig

void setAllDots(bool state) {
    for (uint8_t i = 0; i < 6; i++) {
        dotStates[i] = state;
    }
}

//Einschalten und Ausschalten der Röhren managen
void tubeSleepTask() {
    //Serial.print("UseSleepMode: "); Serial.println(getUseSleepMode()); // Debug-Ausgabe um Funktion der Variablen zu prüfen
    //Serial.print("TimeToSleep: "); Serial.println(getTimeToSleep());   // Debug-Ausgabe um Funktion der Variablen zu prüfen
    static unsigned long lastUpdate = 0;
    static unsigned long startTime = millis();
    static bool tubesOn = false;
    static bool ledsOn = false;
    static bool initialized = false;

    unsigned long now = millis();
    bool radarDetected = digitalRead(Radar_PIN);

    // Softstart: 5 Sekunden nach Start keine Aktionen
    if (!initialized && now - startTime < 5000) {
        return;
    } else if (!initialized) {
        initialized = true;
        // Nach Softstart initial einschalten
        setTubes(true);
        setLEDEffect(getLEDEffectLast());
        tubesOn = true;
        ledsOn = true;
        sleepCounter = getTimeToSleep();
    }

    // Wenn Sleep-Modus deaktiviert ist → alles dauerhaft an
    if (!getUseSleepMode()) {
        if (!tubesOn) {
            setTubes(true);
            tubesOn = true;
        }
        if (!ledsOn) {
            setLEDEffect(getLEDEffectLast());
            ledsOn = true;
        }
        return;
    }

    // Sleep-Modus aktiv
    if (radarDetected) {
        sleepCounter = getTimeToSleep();

        if (!tubesOn) {
            setTubes(true);
            tubesOn = true;
        }

        if (!ledsOn) {
            setLEDEffect(getLEDEffectLast());
            ledsOn = true;
        }
    }

    // Nur einmal pro Minute prüfen
    if (now - lastUpdate >= 60000) {
        lastUpdate = now;

        if (!radarDetected) {
            if (sleepCounter > 0) {
                sleepCounter--;
            } else {
                if (tubesOn) {
                    setTubes(false);
                    tubesOn = false;
                }

                if (ledsOn) {
                    saveLEDEffectLast(getLEDEffect());
                    setLEDEffect(0);
                    ledsOn = false;
                }
            }
        }
    }
}

// Röhren ein- oder ausschalten
void setTubes(bool on) {
    digitalWrite(Pwr_on_PIN, on ? HIGH : LOW);
}
