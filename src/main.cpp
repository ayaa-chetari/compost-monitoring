#include <Arduino.h>
#include "web_app.h"

void setup() {
  Serial.begin(115200);
  delay(200);

  webInit();                 // démarre WiFi AP + serveur + routes + page
  webSetAccess(true);         // pas de NFC pour le test
  webLoadCsvToHistory("/data.csv");  // lit le CSV existant et remplit la RAM

  Serial.println("✅ TEST: Web + CSV chargé");
}

void loop() {
  webLoop(); // souvent vide avec AsyncWebServer, mais tu peux le laisser
}
