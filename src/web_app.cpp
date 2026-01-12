#include "web_app.h"

#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <SPIFFS.h>

#include "web_page.h"

// ===== WiFi AP =====
static const char* ap_ssid = "ESP32_APP";
static const char* ap_password = "12345679";

static IPAddress local_IP(192,168,10,1);
static IPAddress gateway(192,168,10,1);
static IPAddress subnet(255,255,255,0);

// ===== Auth =====
static const char* auth_user = "admin";
static const char* auth_pass = "compost123";

// ===== Server =====
static AsyncWebServer server(80);
static AsyncEventSource events("/events");

// ===== Access flag (NFC) =====
static volatile bool g_accessOk = false;

// ===== CSV single file =====
static const char* CSV_DATA = "/data.csv";

// ===== History RAM =====
static const size_t HISTORY_SIZE = 300;
static Sample3 historyBuf[HISTORY_SIZE];
static size_t histCount = 0;
static size_t histHead  = 0;

// ---------- Helpers ----------
static void pushHistory(const Sample3& s){
  historyBuf[histHead] = s;
  histHead = (histHead + 1) % HISTORY_SIZE;
  if(histCount < HISTORY_SIZE) histCount++;
}

static String latestJson(){
  if(histCount == 0) return "{}";
  size_t idx = (histHead + HISTORY_SIZE - 1) % HISTORY_SIZE;
  const Sample3 &s = historyBuf[idx];

  String j = "{";
  j += "\"t\":" + String(s.t) + ",";
  j += "\"b1\":{\"tempC\":" + String(s.b1Temp,2) + ",\"humPct\":" + String(s.b1Hum,2) + ",\"o2Pct\":" + String(s.b1O2,2) + "},";
  j += "\"b2\":{\"tempC\":" + String(s.b2Temp,2) + ",\"humPct\":" + String(s.b2Hum,2) + "},";
  j += "\"b3\":{\"tempC\":" + String(s.b3Temp,2) + ",\"humPct\":" + String(s.b3Hum,2) + "}";
  j += "}";
  return j;
}

static String historyJson(){
  String j = "[";
  for(size_t i=0;i<histCount;i++){
    size_t idx = (histHead + HISTORY_SIZE - histCount + i) % HISTORY_SIZE;
    const Sample3 &s = historyBuf[idx];
    if(i) j += ",";
    j += "{";
    j += "\"t\":" + String(s.t) + ",";
    j += "\"b1\":{\"tempC\":" + String(s.b1Temp,2) + ",\"humPct\":" + String(s.b1Hum,2) + ",\"o2Pct\":" + String(s.b1O2,2) + "},";
    j += "\"b2\":{\"tempC\":" + String(s.b2Temp,2) + ",\"humPct\":" + String(s.b2Hum,2) + "},";
    j += "\"b3\":{\"tempC\":" + String(s.b3Temp,2) + ",\"humPct\":" + String(s.b3Hum,2) + "}";
    j += "}";
  }
  j += "]";
  return j;
}

static bool requireAuthAndAccess(AsyncWebServerRequest *request){
  if (!request->authenticate(auth_user, auth_pass)) {
    request->requestAuthentication();
    return false;
  }
  if(!g_accessOk){
    request->send(403, "text/plain", "Acces refuse (NFC requis)");
    return false;
  }
  return true;
}

static void ensureCsvHeader(){
  if(!SPIFFS.exists(CSV_DATA)){
    File f = SPIFFS.open(CSV_DATA, FILE_WRITE);
    if(f){
      f.println("timestamp,temperature_bac1,humidity_bac1,oxygen_bac1,temperature_bac2,humidity_bac2,temperature_bac3,humidity_bac3");
      f.close();
    }
  }
}

static void appendCsv(const Sample3 &s){
  File f = SPIFFS.open(CSV_DATA, FILE_APPEND);
  if(!f) return;

  // timestamp ici = s.t (tu peux mettre epoch/ISO plus tard)
  f.printf("%lu,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f\n",
    (unsigned long)s.t,
    s.b1Temp, s.b1Hum, s.b1O2,
    s.b2Temp, s.b2Hum,
    s.b3Temp, s.b3Hum
  );
  f.close();
}

// ---------- Public API ----------
void webSetAccess(bool ok){
  g_accessOk = ok;
}

void webInit(){
  if(!SPIFFS.begin(true)){
    Serial.println("SPIFFS mount FAILED");
    return;
  }
  ensureCsvHeader();

  WiFi.softAP(ap_ssid, ap_password);
  WiFi.softAPConfig(local_IP, gateway, subnet);

  // Page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    if (!requireAuthAndAccess(request)) return;
    request->send_P(200, "text/html", INDEX_HTML);
  });

  // API latest/history
  server.on("/api/latest", HTTP_GET, [](AsyncWebServerRequest *req){
    if (!requireAuthAndAccess(req)) return;
    req->send(200, "application/json", latestJson());
  });

  server.on("/api/history", HTTP_GET, [](AsyncWebServerRequest *req){
    if (!requireAuthAndAccess(req)) return;
    req->send(200, "application/json", historyJson());
  });

  // CSV download (single file)
  server.on("/csv/data", HTTP_GET, [](AsyncWebServerRequest *request){
    if (!requireAuthAndAccess(request)) return;
    if(!SPIFFS.exists(CSV_DATA)){
      request->send(404, "text/plain", "data.csv not found");
      return;
    }
    AsyncWebServerResponse *resp = request->beginResponse(SPIFFS, CSV_DATA, "text/csv");
    resp->addHeader("Content-Disposition", "attachment; filename=data.csv");
    request->send(resp);
  });

  server.addHandler(&events);
  server.begin();

  Serial.println("Web server started on AP 192.168.10.1");
}

void webPushSample(const Sample3 &s){
  // stocke RAM + écrit CSV + SSE
  pushHistory(s);
  appendCsv(s);

  String j = latestJson();
  events.send(j.c_str(), "sample", millis());
}

void webLoop(){
  // AsyncWebServer n’a pas besoin de loop
 // pour  ajouter d'autres trucs plus tard
}
