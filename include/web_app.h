/*#pragma once
#include <Arduino.h>
#include "sensors.h"

void webInit();
void webLoop();                 
void webSetAccess(bool ok);     // NFC -> autorise/stoppe l’accès logique
void webPushSample(const Sample3 &s); // push vers SSE + RAM*/

#pragma once
#include <Arduino.h>

struct Sample3 {
  uint32_t t;
  float b1Temp, b1Hum, b1O2;
  float b2Temp, b2Hum;
  float b3Temp, b3Hum;
};

void webInit();
void webLoop();
void webSetAccess(bool enable);

// Ajoute ça
bool webLoadCsvToHistory(const char* path);

