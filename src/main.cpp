#include <Arduino.h>
#include "sensors.h"
#include "web_app.h"

// ===== CONFIG =====
static const uint32_t SAMPLE_PERIOD_MS = 5000;   // lecture capteurs
static const int BUTTON_AP = 12;                 // bouton activation AP

// ===== VARIABLES =====
unsigned long lastSampleMs = 0;
bool webStarted = false;

void setup() {
  Serial.begin(115200);

  pinMode(BUTTON_AP, INPUT_PULLUP);

  // Init capteurs
  sensorsInit();

  Serial.println("System ready. Acquisition active.");
}

void loop() {
  unsigned long now = millis();

  // ===== LECTURE CAPTEURS =====
  if (now - lastSampleMs >= SAMPLE_PERIOD_MS) {
    lastSampleMs = now;

    Sample3 s;
    s.t = now;

    if (sensorsRead(s)) {
      // Enregistre CSV + RAM + push SSE si web actif
      webPushSample(s);

      Serial.println("Sample acquired & stored");
    } else {
      Serial.println("Sensor read failed");
    }
  }

  // ===== ACTIVATION WEB VIA BOUTON =====
  if (!webStarted && digitalRead(BUTTON_AP) == LOW) {
    delay(50); // anti-rebond simple
    if (digitalRead(BUTTON_AP) == LOW) {
      Serial.println("Starting Web AP...");

      webInit();
      webSetAccess(true);
      webLoadCsvToHistory("/data.csv");

      webStarted = true;
    }
  }

  // ===== WEB LOOP (vide mais propre) =====
  if (webStarted) {
    webLoop();
  }
}
