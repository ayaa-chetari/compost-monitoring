#include <Arduino.h>
#include "web_app.h"

void setup() {
  Serial.begin(115200);

  // Init du web (WiFi AP + serveur + routes + page HTML)
  webInit();

  // Si ton web_app a un mode “accès protégé”, on l’ouvre pour les tests
  webSetAccess(true);

  // Charger le CSV existant (SPIFFS) vers l'historique RAM
  // (fonction à ajouter dans web_app si elle n'existe pas encore)
  webLoadCsvToHistory("/data.csv");

  Serial.println("✅ Mode TEST : web only (CSV existant)");
}

void loop() {
  webLoop();   // optionnel (souvent vide si AsyncWebServer)
}
