#include <Arduino.h>
#include <HardwareSerial.h>
#include <Wire.h>
#include <SPIFFS.h>
#include "esp_sleep.h"
#include "web_app.h"
#include <time.h>

// ================= RS485 =================
HardwareSerial RS485Serial(2);
const int DE_RE = 4;
const int RS485_RX = 16;
const int RS485_TX = 17;
const int MOSFET_SHT20 = 32;  // MOSFET pour SHT20

// ================= BOUTON =================
const int BUTTON_PIN = 27;
volatile bool buttonPressed = false;
volatile unsigned long buttonPressTime = 0;

// ================= CAPTEURS =================
const int CAPTEUR_COUNT = 3;
const uint8_t addresses[CAPTEUR_COUNT] = {1, 2, 3};
const uint8_t OXYGEN_I2C_ADDR = 0x73;

// ================= TIMING =================
const unsigned long SLEEP_TIME_US = 5 * 60 * 1000000;  // 5 minutes en µs
const unsigned long WIFI_TIMEOUT_MS = 5 * 60 * 1000;   // 5 min avant extinction WiFi (auto)
const char *CSV_FILE = "/data.csv";

// ================= FLAGS =================
bool wifiActive = false;
unsigned long wifiStartTime = 0;

// ================= UTILS MOSFET =================
inline void sht20On()  { digitalWrite(MOSFET_SHT20, LOW); }
inline void sht20Off() { digitalWrite(MOSFET_SHT20, HIGH); }

// ================= INTERRUPT BOUTON =================
void IRAM_ATTR handleButtonPress() {
  buttonPressed = true;
  buttonPressTime = millis();
  Serial.println("\n!!! BOUTON APPUYE !!!"); // Debug direct
}

// ================= SPIFFS CSV =================
void writeCSV(float t1, float h1, float o2_val, float t2, float h2, float t3, float h3) {
  // Créer structure Sample3 pour web_app
  Sample3 sample;
  sample.t = time(nullptr);  // Heure système (synchronisée via NTP quand WiFi actif)
  sample.b1Temp = t1;
  sample.b1Hum = h1;
  sample.b1O2 = o2_val;
  sample.b2Temp = t2;
  sample.b2Hum = h2;
  sample.b3Temp = t3;
  sample.b3Hum = h3;
  
  // Si WiFi actif, utiliser web_app, sinon écrire directement
  if (wifiActive) {
    webPushSample(sample);
  } else {
    // Écrire CSV directement
    File file = SPIFFS.open(CSV_FILE, FILE_APPEND);
    if (!file) {
      Serial.println("[CSV] Erreur ouverture");
      return;
    }

    file.print(sample.t);
    file.print(",");
    file.print(isnan(sample.b1Temp) ? "NAN" : String(sample.b1Temp, 2).c_str());
    file.print(",");
    file.print(isnan(sample.b1Hum) ? "NAN" : String(sample.b1Hum, 2).c_str());
    file.print(",");
    file.print(isnan(sample.b1O2) ? "NAN" : String(sample.b1O2, 2).c_str());
    file.print(",");
    file.print(isnan(sample.b2Temp) ? "NAN" : String(sample.b2Temp, 2).c_str());
    file.print(",");
    file.print(isnan(sample.b2Hum) ? "NAN" : String(sample.b2Hum, 2).c_str());
    file.print(",");
    file.print(isnan(sample.b3Temp) ? "NAN" : String(sample.b3Temp, 2).c_str());
    file.print(",");
    file.println(isnan(sample.b3Hum) ? "NAN" : String(sample.b3Hum, 2).c_str());

    file.close();
    Serial.println("[CSV] Donnees ecrites");
  }
}
uint16_t crc16(byte *data, int len) {
  uint16_t crc = 0xFFFF;
  for (int i = 0; i < len; i++) {
    crc ^= data[i];
    for (int b = 0; b < 8; b++)
      crc = (crc >> 1) ^ ((crc & 1) ? 0xA001 : 0);
  }
  return crc;
}

// ================= RS485 =================
void sendFrame(byte *frame, int len) {
  digitalWrite(DE_RE, HIGH);
  RS485Serial.write(frame, len);
  RS485Serial.flush();
  digitalWrite(DE_RE, LOW);
}

float readRegister(uint8_t addr, uint16_t reg) {
  byte frame[8];
  frame[0] = addr;
  frame[1] = 0x04;
  frame[2] = reg >> 8;
  frame[3] = reg & 0xFF;
  frame[4] = 0x00;
  frame[5] = 0x01;

  uint16_t crc = crc16(frame, 6);
  frame[6] = crc & 0xFF;
  frame[7] = crc >> 8;

  while (RS485Serial.available()) RS485Serial.read();
  sendFrame(frame, 8);
  delay(50);

  unsigned long t0 = millis();
  int available = 0;
  while ((available = RS485Serial.available()) < 7 && millis() - t0 < 200) delay(1);

  if (available >= 7) {
    byte resp[7];
    RS485Serial.readBytes(resp, 7);
    if (resp[0] == addr && resp[1] == 0x04) {
      int16_t raw = (resp[3] << 8) | resp[4];
      return raw / 10.0;
    }
  }
  return NAN;
}

// ================= OXYGENE I2C =================
float readOxygen() {
  Wire.beginTransmission(OXYGEN_I2C_ADDR);
  Wire.write(0x05);
  int err = Wire.endTransmission(true);
  
  if (err != 0) return NAN;
  
  delay(100);
  int bytes = Wire.requestFrom(OXYGEN_I2C_ADDR, (uint8_t)2);
  
  if (bytes < 2) return NAN;

  uint8_t byte1 = Wire.read();
  uint8_t byte2 = Wire.read();
  uint16_t raw = (byte1 << 8) | byte2;
  
  return raw / 100.0;
}

// ================= SETUP =================
void setup() {
  Serial.begin(115200);
  delay(1000);

  pinMode(DE_RE, OUTPUT);
  digitalWrite(DE_RE, LOW);
  
  pinMode(MOSFET_SHT20, OUTPUT);
  sht20Off();
  
  // Init bouton avec interrupt
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), handleButtonPress, FALLING);
  
  // Configurer GPIO27 comme source de wakeup (sortir du deep sleep)
  esp_sleep_enable_ext0_wakeup(GPIO_NUM_27, 0);  // 0 = LOW (bouton appuyé)

  RS485Serial.begin(9600, SERIAL_8N1, RS485_RX, RS485_TX);
  Wire.begin();
  Wire.setClock(100000);

  // Initialiser SPIFFS
  if (!SPIFFS.begin(true)) {
    Serial.println("[SPIFFS] Erreur init");
  } else {
    Serial.println("[SPIFFS] OK");
    
    // Créer header CSV s'il n'existe pas
    if (!SPIFFS.exists(CSV_FILE)) {
      File f = SPIFFS.open(CSV_FILE, FILE_WRITE);
      if (f) {
        f.println("timestamp,temperature_bac1,humidity_bac1,oxygen_bac1,temperature_bac2,humidity_bac2,temperature_bac3,humidity_bac3");
        f.close();
        Serial.println("[CSV] Header cree");
      }
    }
  }

  // Vérifier si bouton appuyé au démarrage
  if (buttonPressed || (esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_EXT0)) {
    Serial.println("[BOUTON] Détecté au wakeup - WiFi ON");
    buttonPressed = true;  // Sera traité dans loop()
  } else {
    Serial.println("\n=== DEMARRAGE - COLLECTE ===");
    Serial.println("Appuyer sur le BOUTON (GPIO27) pour activer le WiFi");
    Serial.println("SSID: PolyGreen | Pass: compost123");
    Serial.println("IP: 192.168.10.1");
  }
}

// ================= LOOP =================
void loop() {
  // Vérifier si bouton appuyé
  if (buttonPressed) {
    if (wifiActive) {
      // WiFi actif: 2e appui = arrêter WiFi
      Serial.println("[BOUTON] 2e appui - WiFi OFF");
      webStop();
      wifiActive = false;
      buttonPressed = false;
      delay(500);  // Debounce
      // Relancer deep sleep normal
      esp_sleep_enable_timer_wakeup(SLEEP_TIME_US);
      esp_sleep_enable_ext0_wakeup(GPIO_NUM_27, 0);
      esp_deep_sleep_start();
    } else {
      // WiFi inactif: 1er appui = démarrer WiFi
      Serial.println("[BOUTON] 1er appui - WiFi ON");
      wifiActive = true;
      wifiStartTime = millis();
      buttonPressed = false;
      delay(200);  // Debounce
      webInit();
      return;  // Rester en WiFi
    }
  }

  // Si WiFi actif
  if (wifiActive) {
    // Vérifier timeout WiFi (5 minutes)
    if (millis() - wifiStartTime > WIFI_TIMEOUT_MS) {
      Serial.println("[WiFi] Timeout (5min) - Arrêt AUTO");
      webStop();
      wifiActive = false;
      buttonPressed = false;
      delay(500);
      // Relancer deep sleep normal
      esp_sleep_enable_timer_wakeup(SLEEP_TIME_US);
      esp_sleep_enable_ext0_wakeup(GPIO_NUM_27, 0);
      esp_deep_sleep_start();
    }
    
    // Boucle WiFi légère
    delay(100);
    return;  // Ne pas faire de collecte pendant WiFi
  }

  // Mode collecte (sans WiFi)
  Serial.println("=== COLLECTE DE DONNEES ===");

  // -------- ACTIVATION SHT20 --------
  sht20On();
  delay(500);

  // -------- LECTURE RS485 SHT20 --------
  float t1 = readRegister(addresses[0], 0x0001);
  delay(100);
  float h1 = readRegister(addresses[0], 0x0002);
  delay(100);
  
  float t2 = readRegister(addresses[1], 0x0001);
  delay(100);
  float h2 = readRegister(addresses[1], 0x0002);
  delay(100);
  
  float t3 = readRegister(addresses[2], 0x0001);
  delay(100);
  float h3 = readRegister(addresses[2], 0x0002);

  // -------- DESACTIVATION SHT20 --------
  sht20Off();

  // -------- LECTURE OXYGENE I2C --------
  float o2 = readOxygen();

  // -------- AFFICHER RESULTATS --------
  Serial.print("SHT20-1: T=");
  Serial.print(t1, 1);
  Serial.print(" H=");
  Serial.println(h1, 1);
  
  Serial.print("SHT20-2: T=");
  Serial.print(t2, 1);
  Serial.print(" H=");
  Serial.println(h2, 1);
  
  Serial.print("SHT20-3: T=");
  Serial.print(t3, 1);
  Serial.print(" H=");
  Serial.println(h3, 1);
  
  Serial.print("O2: ");
  Serial.println(o2, 1);

  // -------- ENREGISTRER CSV --------
  writeCSV(t1, h1, o2, t2, h2, t3, h3);

  // -------- SLEEP 5 MIN --------
  Serial.println("[SLEEP] Deep sleep 5 min...");
  Serial.println("Appuyer BOUTON pour activer WiFi");
  delay(500);  // Temps pour que les prints passent
  
  esp_sleep_enable_timer_wakeup(SLEEP_TIME_US);
  esp_sleep_enable_ext0_wakeup(GPIO_NUM_27, 0);  // Réactiver bouton wakeup
  esp_deep_sleep_start();
}
