#include <Arduino.h>
#include "sensors.h"
#include "web_app.h"

// 1 heure
static const uint32_t LOG_PERIOD_MS = 3600UL * 1000UL;

static uint32_t lastLogMs = 0;

// ======= Stub NFC (à remplacer) =======
static bool nfcIsValid(){
  // TODO: mettre ton code NFC ici (lecture tag)
  // return true si le badge/valise est détecté
  return false;
}

void setup(){
  Serial.begin(115200);

  sensorsInit();
  webInit();

  // au démarrage : pas d’accès tant que NFC non validé
  webSetAccess(false);

  lastLogMs = millis();
}

void loop(){
  // 1) NFC en continu (rapide)
  static uint32_t lastNfcPoll = 0;
  if(millis() - lastNfcPoll >= 200){ // check NFC 5 fois/sec
    lastNfcPoll = millis();
    bool ok = nfcIsValid();
    webSetAccess(ok);
  }

  // 2) toutes les 1 heure : acquisition + sauvegarde CSV + SSE
  uint32_t now = millis();
  if(now - lastLogMs >= LOG_PERIOD_MS){
    lastLogMs = now;

    Sample3 s{};
    s.t = (uint32_t)(now / 1000); // timestamp en secondes depuis boot (simple)
    sensorsRead(s);

    // pousse vers web : RAM + CSV + SSE
    webPushSample(s);

    Serial.println("Sample logged + pushed to web");
  }

  // 3) boucle web (pas obligatoire)
  webLoop();
}
