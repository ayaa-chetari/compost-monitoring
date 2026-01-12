#pragma once
#include <Arduino.h>
#include "sensors.h"

void webInit();
void webLoop();                 
void webSetAccess(bool ok);     // NFC -> autorise/stoppe l’accès logique
void webPushSample(const Sample3 &s); // push vers SSE + RAM
