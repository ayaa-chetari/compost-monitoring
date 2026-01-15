#ifndef WEB_APP_H
#define WEB_APP_H

#include <time.h>

// ===== Structure pour les données capteurs =====
struct Sample3 {
  time_t t;        // timestamp
  float b1Temp, b1Hum, b1O2;  // Bac 1
  float b2Temp, b2Hum;         // Bac 2
  float b3Temp, b3Hum;         // Bac 3
};

// ===== API Web =====
void webInit();           // Initialiser WiFi AP + serveur web
void webPushSample(const Sample3 &s);  // Ajouter données + CSV + SSE
void webSetAccess(bool ok);  // Définir accès (pour NFC)
void webLoop();           // Boucle web (optionnel)

#endif
