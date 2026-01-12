#include "web_app.h"
#include <SPIFFS.h>

// ⚠️ IMPORTANT : ici je suppose que tu as déjà dans web_app.cpp :
// void pushHistory(const Sample3& s);
// Si ta fonction a un autre nom, dis-moi lequel et je te l’adapte.

static bool parseCsvLineToSample(const String& line, Sample3 &s, uint32_t t) {
  char buf[240];
  line.toCharArray(buf, sizeof(buf));

  // Format attendu :
  // timestamp,temperature_bac1,humidity_bac1,oxygen_bac1,temperature_bac2,humidity_bac2,temperature_bac3,humidity_bac3

  char* tok = strtok(buf, ","); // timestamp (ignoré)
  if (!tok) return false;

  tok = strtok(NULL, ","); if(!tok) return false; s.b1Temp = atof(tok);
  tok = strtok(NULL, ","); if(!tok) return false; s.b1Hum  = atof(tok);
  tok = strtok(NULL, ","); if(!tok) return false; s.b1O2   = atof(tok);
  tok = strtok(NULL, ","); if(!tok) return false; s.b2Temp = atof(tok);
  tok = strtok(NULL, ","); if(!tok) return false; s.b2Hum  = atof(tok);
  tok = strtok(NULL, ","); if(!tok) return false; s.b3Temp = atof(tok);
  tok = strtok(NULL, ","); if(!tok) return false; s.b3Hum  = atof(tok);

  s.t = t; // temps fictif pour l’axe X (comme dans ton code)
  return true;
}

bool webLoadCsvToHistory(const char* path) {
  if (!SPIFFS.begin(true)) {
    Serial.println("❌ SPIFFS mount FAILED");
    return false;
  }

  if (!SPIFFS.exists(path)) {
    Serial.printf("❌ CSV not found: %s\n", path);
    return false;
  }

  File f = SPIFFS.open(path, FILE_READ);
  if (!f) {
    Serial.println("❌ Failed to open CSV");
    return false;
  }

  // Skip header
  f.readStringUntil('\n');

  uint32_t t = 0;
  uint32_t loaded = 0;

  while (f.available()) {
    String line = f.readStringUntil('\n');
    line.trim();
    if (line.length() == 0) continue;

    Sample3 s{};
    if (parseCsvLineToSample(line, s, t)) {
      // ✅ Remplir juste l’historique RAM
      pushHistory(s);   // <-- si ta fonction interne s’appelle comme ça
      loaded++;
      t += 10;
    }
  }

  f.close();
  Serial.printf("✅ CSV loaded to history: %lu lines\n", (unsigned long)loaded);
  return true;
}
