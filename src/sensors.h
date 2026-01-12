#pragma once
#include <Arduino.h>

struct Sample3 {
  uint32_t t;     // temps (ex: secondes ou epoch)
  float b1Temp, b1Hum, b1O2;
  float b2Temp, b2Hum;
  float b3Temp, b3Hum;
};

void sensorsInit();
bool sensorsRead(Sample3 &out);   // lit tous les capteurs (1 acquisition)
