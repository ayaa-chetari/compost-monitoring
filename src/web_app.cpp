#include "web_app.h"
#include "web_page.h"

#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <SPIFFS.h>
#include <time.h>

// ===== WiFi AP =====
static const char* ap_ssid = "PolyGreen";
static const char* ap_password = "compost123";

static IPAddress local_IP(192, 168, 10, 1);
static IPAddress gateway(192, 168, 10, 1);
static IPAddress subnet(255, 255, 255, 0);

// ===== Auth =====
static const char* auth_user = "admin";
static const char* auth_pass = "compost123";

// ===== Server =====
static AsyncWebServer server(80);
static AsyncEventSource events("/events");

// ===== Access flag (NFC) =====
static volatile bool g_accessOk = true;  // true par défaut (pas de NFC pour l'instant)

// ===== CSV single file =====
static const char* CSV_DATA = "/data.csv";

// ===== History RAM =====
static const size_t HISTORY_SIZE = 300;
static Sample3 historyBuf[HISTORY_SIZE];
static size_t histCount = 0;
static size_t histHead = 0;
static bool timeSynced = false;  // Flag: heure synchronisée?
static time_t timeOffsetSeconds = 0;  // Offset appliqué aux anciennes données

// ---------- Helpers ----------
static void pushHistory(const Sample3& s) {
  historyBuf[histHead] = s;
  histHead = (histHead + 1) % HISTORY_SIZE;
  if (histCount < HISTORY_SIZE) histCount++;
}

static void loadHistoryFromCSV() {
  // Charger les dernières lignes du CSV dans l'historique RAM
  if (!SPIFFS.exists(CSV_DATA)) return;
  
  File file = SPIFFS.open(CSV_DATA, FILE_READ);
  if (!file) return;
  
  // Lire toutes les lignes
  while (file.available()) {
    String line = file.readStringUntil('\n');
    if (line.length() == 0) continue;
    
    // Skip header
    if (line.indexOf("timestamp") != -1 || line.indexOf("date_time") != -1) continue;
    
    // Parser: timestamp/date_time,t1,h1,o2,t2,h2,t3,h3
    int idx = 0;
    float vals[8] = {0, 0, 0, 0, 0, 0, 0, 0};
    
    // Premier champ: peut être "YYYY-MM-DD HH:MM:SS" ou un nombre (old format)
    int comma = line.indexOf(',', idx);
    if (comma == -1) comma = line.length();
    
    String firstField = line.substring(idx, comma);
    
    // Vérifier si c'est un format date ou timestamp
    time_t t = 0;
    if (firstField.indexOf(':') != -1) {
      // Format date/heure: YYYY-MM-DD HH:MM:SS (ignorer, utiliser offset)
      t = 0;  // Will be set later
    } else {
      // Ancien format: timestamp en secondes
      t = (time_t)firstField.toInt();
    }
    
    idx = comma + 1;
    
    // Parser le reste des valeurs
    for (int i = 0; i < 7; i++) {
      comma = line.indexOf(',', idx);
      if (comma == -1) comma = line.length();
      
      String val = line.substring(idx, comma);
      if (val != "NAN" && val.length() > 0) {
        vals[i+1] = val.toFloat();
      } else {
        vals[i+1] = NAN;
      }
      idx = comma + 1;
    }
    
    Sample3 s;
    s.t = t;
    s.b1Temp = vals[1];
    s.b1Hum = vals[2];
    s.b1O2 = vals[3];
    s.b2Temp = vals[4];
    s.b2Hum = vals[5];
    s.b3Temp = vals[6];
    s.b3Hum = vals[7];
    
    pushHistory(s);
  }
  
  file.close();
  Serial.print("[WEB] Historique charge: ");
  Serial.print(histCount);
  Serial.println(" donnees");
}

static String safeNum(float v) {
  if (!isfinite(v)) return "null";
  String s = String(v, 2);
  return s;
}

static String latestJson() {
  if (histCount == 0) return "{}";
  size_t idx = (histHead + HISTORY_SIZE - 1) % HISTORY_SIZE;
  const Sample3 &s = historyBuf[idx];

  String j = "{";
  j += "\"t\":" + String(s.t + timeOffsetSeconds) + ",";
  j += "\"b1\":{\"tempC\":" + safeNum(s.b1Temp) + ",\"humPct\":" + safeNum(s.b1Hum) + ",\"o2Pct\":" + safeNum(s.b1O2) + "},";
  j += "\"b2\":{\"tempC\":" + safeNum(s.b2Temp) + ",\"humPct\":" + safeNum(s.b2Hum) + "},";
  j += "\"b3\":{\"tempC\":" + safeNum(s.b3Temp) + ",\"humPct\":" + safeNum(s.b3Hum) + "}";
  j += "}";
  return j;
}

static String historyJson() {
  String j = "[";
  for (size_t i = 0; i < histCount; i++) {
    size_t idx = (histHead + HISTORY_SIZE - histCount + i) % HISTORY_SIZE;
    const Sample3 &s = historyBuf[idx];
    if (i) j += ",";
    j += "{";
    j += "\"t\":" + String(s.t + timeOffsetSeconds) + ",";
    j += "\"b1\":{\"tempC\":" + safeNum(s.b1Temp) + ",\"humPct\":" + safeNum(s.b1Hum) + ",\"o2Pct\":" + safeNum(s.b1O2) + "},";
    j += "\"b2\":{\"tempC\":" + safeNum(s.b2Temp) + ",\"humPct\":" + safeNum(s.b2Hum) + "},";
    j += "\"b3\":{\"tempC\":" + safeNum(s.b3Temp) + ",\"humPct\":" + safeNum(s.b3Hum) + "}";
    j += "}";
  }
  j += "]";
  return j;
}

static bool requireAuth(AsyncWebServerRequest *request) {
  if (!request->authenticate(auth_user, auth_pass)) {
    request->requestAuthentication();
    return false;
  }
  return true;
}

static void ensureCsvHeader() {
  if (!SPIFFS.exists(CSV_DATA)) {
    File f = SPIFFS.open(CSV_DATA, FILE_WRITE);
    if (f) {
      f.println("date_time,temperature_bac1,humidity_bac1,oxygen_bac1,temperature_bac2,humidity_bac2,temperature_bac3,humidity_bac3");
      f.close();
    }
  }
}

static void appendCsv(const Sample3 &s) {
  File f = SPIFFS.open(CSV_DATA, FILE_APPEND);
  if (!f) return;

  // Formater le timestamp en date/heure lisible
  time_t t = s.t + timeOffsetSeconds;
  struct tm* timeinfo = localtime(&t);
  char timeStr[20];
  strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", timeinfo);

  f.printf("%s,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f\n",
    timeStr,
    s.b1Temp, s.b1Hum, s.b1O2,
    s.b2Temp, s.b2Hum,
    s.b3Temp, s.b3Hum
  );
  f.close();
}

// ---------- Public API ----------
void webSetAccess(bool ok) {
  g_accessOk = ok;
}

static void syncNTP() {
  // Synchroniser l'heure via NTP
  Serial.println("[NTP] Synchronisation en cours...");
  
  // Utiliser pool.ntp.org
  configTime(0, 0, "pool.ntp.org", "time.nist.gov");
  
  // Attendre max 15s la synchronisation
  time_t now = time(nullptr);
  int attempts = 0;
  while (now < 24 * 3600 && attempts < 150) {  // 24*3600 = 1970-01-01 + 1 jour
    delay(100);
    now = time(nullptr);
    attempts++;
  }
  
  if (now > 24 * 3600) {
    Serial.print("[NTP] OK - Heure: ");
    Serial.println(ctime(&now));
  } else {
    Serial.println("[NTP] Timeout - Utilisant timestamp par défaut");
  }
}

static void setTimeFromClient(time_t clientTime) {
  if (timeSynced) {
    Serial.println("[TIME] Déjà synchronisée, skip");
    return;
  }
  
  // Récupérer l'heure actuelle de l'ESP (avant la synchro)
  time_t espTimeNow = time(nullptr);
  
  // Configurer timezone France (CET/CEST)
  setenv("TZ", "CET-1CEST,M3.5.0,M10.5.0/3", 1);
  tzset();
  
  // Mettre à jour l'heure de l'ESP une seule fois
  struct timeval tv;
  tv.tv_sec = clientTime;
  tv.tv_usec = 0;
  settimeofday(&tv, nullptr);
  
  // Calculer l'offset à appliquer aux anciennes données
  timeOffsetSeconds = clientTime - espTimeNow;
  
  timeSynced = true;
  
  Serial.print("[TIME] Synchro CLIENT: ");
  Serial.println(ctime(&clientTime));
  Serial.print("[TIME] Offset appliqué: ");
  Serial.print(timeOffsetSeconds);
  Serial.println(" secondes");
}

void webInit() {
  if (!SPIFFS.begin(true)) {
    Serial.println("SPIFFS mount FAILED");
    return;
  }
  ensureCsvHeader();
  
  // Charger les données existantes du CSV
  loadHistoryFromCSV();

  // Configurer WiFi AP
  WiFi.mode(WIFI_AP);
  WiFi.softAP(ap_ssid, ap_password);
  WiFi.softAPConfig(local_IP, gateway, subnet);

  Serial.println("\n===== WiFi AP Started =====");
  Serial.print("SSID: ");
  Serial.println(ap_ssid);
  Serial.print("Password: ");
  Serial.println(ap_password);
  Serial.println("IP: 192.168.10.1");
  Serial.println("============================\n");
  
  // Synchroniser l'heure via NTP
  syncNTP();

  // Page HTML
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    // if (!requireAuth(request)) return;  // Auth disabled for testing
    request->send_P(200, "text/html", INDEX_HTML);
  });

  // API latest/history
  server.on("/api/latest", HTTP_GET, [](AsyncWebServerRequest *req) {
    // if (!requireAuth(req)) return;  // Auth disabled
    req->send(200, "application/json", latestJson());
  });

  server.on("/api/history", HTTP_GET, [](AsyncWebServerRequest *req) {
    // if (!requireAuth(req)) return;  // Auth disabled
    req->send(200, "application/json", historyJson());
  });

  // Endpoint pour mettre à jour l'heure depuis le client
  server.on("/api/settime", HTTP_POST, [](AsyncWebServerRequest *req) {
    time_t clientTime = 0;
    
    if (req->hasParam("time")) {
      String timeStr = req->getParam("time")->value();
      clientTime = (time_t)timeStr.toInt();
      Serial.print("[SETTIME] Reçu: ");
      Serial.print(timeStr);
      Serial.print(" -> ");
      Serial.println(clientTime);
    }
    
    if (clientTime > 0) {
      setTimeFromClient(clientTime);
      req->send(200, "application/json", "{\"status\":\"ok\"}");
    } else {
      req->send(400, "application/json", "{\"error\":\"invalid or missing time parameter\"}");
    }
  });
  
  server.on("/api/settime", HTTP_GET, [](AsyncWebServerRequest *req) {
    time_t clientTime = 0;
    
    if (req->hasParam("time")) {
      String timeStr = req->getParam("time")->value();
      clientTime = (time_t)timeStr.toInt();
      Serial.print("[SETTIME] Reçu: ");
      Serial.print(timeStr);
      Serial.print(" -> ");
      Serial.println(clientTime);
    }
    
    if (clientTime > 0) {
      setTimeFromClient(clientTime);
      req->send(200, "application/json", "{\"status\":\"ok\"}");
    } else {
      req->send(400, "application/json", "{\"error\":\"invalid or missing time parameter\"}");
    }
  });

  // CSV download (généré depuis l'historique RAM avec bon format)
  server.on("/csv/data", HTTP_GET, [](AsyncWebServerRequest *request) {
    // if (!requireAuth(request)) return;  // Auth disabled
    
    // Générer le CSV à la volée
    String csvContent = "date_time,temperature_bac1,humidity_bac1,oxygen_bac1,temperature_bac2,humidity_bac2,temperature_bac3,humidity_bac3\n";
    
    for (size_t i = 0; i < histCount; i++) {
      size_t idx = (histHead + HISTORY_SIZE - histCount + i) % HISTORY_SIZE;
      const Sample3 &s = historyBuf[idx];
      
      // Formater la date/heure
      time_t t = s.t + timeOffsetSeconds;
      struct tm* timeinfo = localtime(&t);
      char timeStr[20];
      strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", timeinfo);
      
      // Ajouter la ligne
      char line[256];
      snprintf(line, sizeof(line), "%s,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f\n",
        timeStr,
        s.b1Temp, s.b1Hum, s.b1O2,
        s.b2Temp, s.b2Hum,
        s.b3Temp, s.b3Hum
      );
      csvContent += line;
    }
    
    // Envoyer le CSV
    AsyncWebServerResponse *response = request->beginResponse(200, "text/csv", csvContent);
    response->addHeader("Content-Disposition", "attachment; filename=data.csv");
    request->send(response);
  });

  server.addHandler(&events);
  server.begin();

  Serial.println("Web server started !");
}

void webStop() {
  server.end();
  WiFi.softAPdisconnect(true);  // Arrêter le WiFi AP
  WiFi.mode(WIFI_OFF);
  Serial.println("\n===== WiFi AP Stopped =====\n");
}

void webPushSample(const Sample3 &s) {
  // Stocker en RAM + écrire CSV + SSE
  pushHistory(s);
  appendCsv(s);

  String j = latestJson();
  events.send(j.c_str(), "sample", millis());
  
  Serial.println("[WEB] Sample pushed");
}

void webLoop() {
  // AsyncWebServer n'a pas besoin de loop
}
