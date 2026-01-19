#include <Arduino.h>
#include <stdint.h>
#include <HardwareSerial.h>
#include <Wire.h>  // Pour I2C
#include <SPIFFS.h>  // Système de fichiers
#include <time.h>  // Pour l'horodatage
#include <WiFi.h>  // Pour NTP
#include "web_app.h"

HardwareSerial RS485Serial(2);   // UART2 de l'ESP32

const int DE_RE = 4;             // RS485 direction
const int CAPTEUR_COUNT = 3;
const uint8_t addresses[CAPTEUR_COUNT] = {1, 2, 3};

// ==== Module GERUI MOSFET PWM ====
const int MOSFET_RS485_PIN = 25;   // PWM pour 3 capteurs RS485 (temp/humidité)
const int MOSFET_O2_PIN = 26;      // PWM pour capteur O2 I2C
const int PWM_FREQUENCY = 5000;    // 5 kHz
const int PWM_RESOLUTION = 8;      // 8-bit (0-255)
const int PWM_CHANNEL_RS485 = 0;   // Canal PWM 0
const int PWM_CHANNEL_O2 = 1;      // Canal PWM 1
const int PWM_MAX = 255;           // Valeur max pour ON

// ==== Timing gestion consommation ====
const long READING_INTERVAL = 3600000;  // 1 heure en ms (3600000)
const long ACTIVE_DURATION = 120000;    // 2 minutes en ms (120000)
static unsigned long lastReadingTime = 0;
static bool sensorsActive = false;
static unsigned long activationStartTime = 0;

// ==== Capteur O2 I2C ====
const uint8_t OXYGEN_I2C_ADDR = 0x73; // à ajuster selon ton capteur
float readOxygen();

// ==== Déclarations forward des fonctions MOSFET ====
void activateSensors();
void deactivateSensors();
void updateSensorPowerState();

// ==== Contrôle PWM des modules MOSFET ====
void initMOSFET() {
  ledcSetup(PWM_CHANNEL_RS485, PWM_FREQUENCY, PWM_RESOLUTION);
  ledcSetup(PWM_CHANNEL_O2, PWM_FREQUENCY, PWM_RESOLUTION);
  
  ledcAttachPin(MOSFET_RS485_PIN, PWM_CHANNEL_RS485);
  ledcAttachPin(MOSFET_O2_PIN, PWM_CHANNEL_O2);
  
  // Démarrer en OFF
  deactivateSensors();
  
  Serial.println("[MOSFET] Modules MOSFET initialisés");
}

void activateSensors() {
  // Allumer tous les capteurs via PWM
  ledcWrite(PWM_CHANNEL_RS485, PWM_MAX);  // ON (255)
  ledcWrite(PWM_CHANNEL_O2, PWM_MAX);     // ON (255)
  
  sensorsActive = true;
  activationStartTime = millis();
  
  Serial.println("[MOSFET] Capteurs ACTIVÉS");
  delay(500);  // Attendre stabilisation
}

void deactivateSensors() {
  // Éteindre tous les capteurs
  ledcWrite(PWM_CHANNEL_RS485, 0);  // OFF
  ledcWrite(PWM_CHANNEL_O2, 0);     // OFF
  
  sensorsActive = false;
  
  Serial.println("[MOSFET] Capteurs DÉSACTIVÉS");
}

// Vérifier si les capteurs doivent rester actifs
void updateSensorPowerState() {
  unsigned long currentTime = millis();
  
  // Vérifier si c'est le moment de réactiver
  if (!sensorsActive && (currentTime - lastReadingTime >= READING_INTERVAL)) {
    activateSensors();
    lastReadingTime = currentTime;
  }
  
  // Vérifier si les capteurs ont été actifs assez longtemps
  if (sensorsActive && (currentTime - activationStartTime >= ACTIVE_DURATION)) {
    deactivateSensors();
  }
}

// ==== Variables globales pour les données ====
struct SensorData {
  float temp_bac1, hum_bac1;
  float temp_bac2, hum_bac2;
  float temp_bac3, hum_bac3;
  float oxygen;
} currentData;

const char* CSV_FILE = "/data/sensors.csv";
const char* NTP_SERVER = "pool.ntp.org";
const long GMT_OFFSET_SEC = 3600;  // UTC+1 (France hiver) = 3600 sec
const int DAYLIGHT_OFFSET_SEC = 3600;  // UTC+2 (France été) = 3600 sec additionnel
// ------------------ Envoi d'une trame RS485 ------------------
void sendFrame(uint8_t address, byte *frame, int length) {
  digitalWrite(DE_RE, HIGH);
  RS485Serial.write(frame, length);
  RS485Serial.flush();
  digitalWrite(DE_RE, LOW);
}

// ------------------ Lecture registre RS485 ------------------
float readRegister(uint8_t address, uint16_t reg) {
  byte frame[8];
  frame[0] = address;
  frame[1] = 0x04;
  frame[2] = reg >> 8;
  frame[3] = reg & 0xFF;
  frame[4] = 0x00;
  frame[5] = 0x01;

  // CRC16
  uint16_t crc = 0xFFFF;
  for (int i = 0; i < 6; i++) {
    crc ^= frame[i];
    for (int b = 0; b < 8; b++) {
      crc = (crc >> 1) ^ ((crc & 1) ? 0xA001 : 0);
    }
  }
  frame[6] = crc & 0xFF;
  frame[7] = crc >> 8;

  while (RS485Serial.available()) RS485Serial.read();
  sendFrame(address, frame, 8);

  unsigned long start = millis();
  while (RS485Serial.available() < 7 && millis() - start < 200) {
    delay(1);
  }

  if (RS485Serial.available() >= 7) {
    byte resp[7];
    RS485Serial.readBytes(resp, 7);

    if (resp[0] != address || resp[1] != 0x04) return NAN;

    int16_t rawValue = (resp[3] << 8) | resp[4];
    return rawValue / 10.0;
  }

  return NAN;
}

// -------- Initialiser SPIFFS --------
bool initSPIFFS() {
  if (!SPIFFS.begin(true)) {
    Serial.println("Erreur initialisation SPIFFS");
    return false;
  }
  
  // Créer le répertoire /data s'il n'existe pas
  if (!SPIFFS.exists("/data")) {
    File dir = SPIFFS.open("/data", FILE_WRITE);
    dir.close();
  }
  
  Serial.println("SPIFFS initialisé");
  return true;
}
// -------- Synchroniser l'heure avec NTP --------
void syncTimeWithNTP() {
  Serial.println("Synchronisation avec NTP...");
  configTime(GMT_OFFSET_SEC, DAYLIGHT_OFFSET_SEC, NTP_SERVER);
  
  time_t now = time(nullptr);
  int attempts = 0;
  while (now < 24 * 3600 && attempts < 20) {
    delay(500);
    Serial.print(".");
    now = time(nullptr);
    attempts++;
  }
  
  Serial.println();
  Serial.print("Heure actuelle : ");
  Serial.println(ctime(&now));
}

// -------- Écrire les données dans le CSV --------
void writeDataToCSV(SensorData data) {
  time_t now = time(nullptr);
  struct tm* timeinfo = localtime(&now);
  char timestamp[25];
  strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", timeinfo);
  
  // Vérifier si le fichier existe, sinon créer avec header
  bool fileExists = SPIFFS.exists(CSV_FILE);
  
  File file = SPIFFS.open(CSV_FILE, FILE_APPEND);
  if (!file) {
    Serial.println("Erreur ouverture fichier CSV");
    return;
  }
  
  // Écrire le header si c'est la première fois
  if (!fileExists) {
    file.println("timestamp,temperature_bac1,humidity_bac1,oxygen,temperature_bac2,humidity_bac2,temperature_bac3,humidity_bac3");
  }
  
  // Écrire une ligne de données
  file.print(timestamp);
  file.print(",");
  file.print(data.temp_bac1, 2);
  file.print(",");
  file.print(data.hum_bac1, 2);
  file.print(",");
  file.print(data.oxygen, 2);
  file.print(",");
  file.print(data.temp_bac2, 2);
  file.print(",");
  file.print(data.hum_bac2, 2);
  file.print(",");
  file.print(data.temp_bac3, 2);
  file.print(",");
  file.println(data.hum_bac3, 2);
  
  file.close();
  
  Serial.print("[CSV] Données écrites : ");
  Serial.println(timestamp);
}

// -------- Réinitialiser le bus I2C quand il est bloqué --------
void resetI2CBus() {
  Wire.end();
  delay(50);
  Wire.begin(21, 22);
  Wire.setClock(25000);  // Très basse fréquence : 25 kHz
  Serial.println("[I2C RESET]");
}

// ------------------ Scan I2C pour diagnostic ------------------
void scanI2C() {
  Serial.println("\n[I2C SCAN] Recherche des capteurs I2C...");
  int count = 0;
  for (byte i = 8; i < 120; i++) {
    Wire.beginTransmission(i);
    if (Wire.endTransmission() == 0) {
      Serial.print("Capteur trouvé à l'adresse 0x");
      Serial.println(i, HEX);
      count++;
    }
  }
  if (count == 0) {
    Serial.println("⚠️ AUCUN capteur I2C détecté ! Vérifiez les connexions SDA/SCL");
  }
  Serial.println();
}

// ------------------ SETUP ------------------
void setup() {
  Serial.begin(115200);
  delay(2000);
  
  Serial.println("\n=== SYSTÈME DÉMARRAGE ===\n");
  
  // Initialiser RS485
  pinMode(DE_RE, OUTPUT);
  digitalWrite(DE_RE, LOW);
  RS485Serial.begin(9600, SERIAL_8N1, 16, 17);
  
  // Initialiser I2C
  Wire.begin(21, 22);
  Wire.setClock(100000);  // 100 kHz
  
  // Initialiser MOSFET PWM
  initMOSFET();
  
  // Initialiser SPIFFS
  initSPIFFS();
  
  // Synchroniser l'heure (nécessite WiFi)
  // Pour tester sans WiFi : définir l'heure manuellement avec setTime()
  // syncTimeWithNTP();  // À décommenter si WiFi disponible
  
  // Définir l'heure manuellement pour test (lundi 15 janvier 2024, 12:00:00)
  time_t test_time = 1705317600;  // À adapter selon votre heure réelle
  struct timeval tv = {.tv_sec = test_time};
  settimeofday(&tv, NULL);
  
  // Initialiser le serveur web
  webInit();
  
  // Première activation immédiate pour la première lecture
  lastReadingTime = millis();
  
  Serial.println("RS485 initialisé à 9600 baud");
  Serial.println("I2C initialisé à 100 kHz");
  Serial.println("MOSFET PWM configuré (2 min active / 1H d'intervalle)");
  Serial.println("Attente de l'activation des capteurs...\n");
}

// ------------------ LOOP ------------------
void loop() {
  // Mettre à jour l'état d'alimentation des capteurs
  updateSensorPowerState();
  
  // Si les capteurs ne sont pas actifs, attendre
  if (!sensorsActive) {
    unsigned long timeUntilNextReading = READING_INTERVAL - (millis() - lastReadingTime);
    Serial.print("[SLEEP MODE] Prochaine lecture dans : ");
    Serial.print(timeUntilNextReading / 1000);
    Serial.println(" secondes");
    delay(10000);  // Vérifier l'état tous les 10 secondes
    return;
  }

  // ===== LECTURE RS485 CAPTEURS SHT20 =====
  Serial.println("\n[ACQUISITION] Lecture des capteurs RS485...");
  for (int i = 0; i < CAPTEUR_COUNT; i++) {
    uint8_t addr = addresses[i];

    Serial.print("📟 SHT20 adresse ");
    Serial.println(addr);

    float temperature = readRegister(addr, 0x0001);
    delay(100);
    float humidity = readRegister(addr, 0x0002);

    Serial.print("Température : ");
    Serial.print(temperature);
    Serial.println(" °C");

    Serial.print("Humidité : ");
    Serial.print(humidity);
    Serial.println(" %RH");

    Serial.println("---------------------------");
    
    // Stocker les données dans la structure
    if (i == 0) {
      currentData.temp_bac1 = temperature;
      currentData.hum_bac1 = humidity;
    } else if (i == 1) {
      currentData.temp_bac2 = temperature;
      currentData.hum_bac2 = humidity;
    } else if (i == 2) {
      currentData.temp_bac3 = temperature;
      currentData.hum_bac3 = humidity;
    }
    
    delay(500);
  }

  // ===== LECTURE I2C CAPTEUR O2 =====
  Serial.println("[ACQUISITION] Lecture du capteur O2 I2C...");
  int bytesReceived = Wire.requestFrom((uint8_t)0x73, (size_t)2, (bool)true);
  
  if (bytesReceived >= 2) {
    uint8_t byte1 = Wire.read();
    uint8_t byte2 = Wire.read();
    uint16_t raw = (byte2 << 8) | byte1;
    
    float oxygen = raw / 100.0;
    currentData.oxygen = oxygen;
    
    Serial.print("Oxygène : ");
    Serial.print(oxygen);
    Serial.println(" %Vol");
  } else {
    Serial.println("Oxygène : Erreur lecture");
    currentData.oxygen = NAN;
  }
  
  // ===== ÉCRIRE DANS LE CSV =====
  writeDataToCSV(currentData);
  
  // ===== ENVOYER VER LE WEB =====
  Sample3 sample;
  sample.t = time(nullptr);
  sample.b1Temp = currentData.temp_bac1;
  sample.b1Hum = currentData.hum_bac1;
  sample.b1O2 = currentData.oxygen;
  sample.b2Temp = currentData.temp_bac2;
  sample.b2Hum = currentData.hum_bac2;
  sample.b3Temp = currentData.temp_bac3;
  sample.b3Hum = currentData.hum_bac3;
  
  webPushSample(sample);
  
  Serial.println("==== Fin du cycle de lecture ====\n");
  
  // Boucle rapide pendant la fenêtre active pour éventuelles mises à jour
  delay(5000);
}

// ------------------ Lecture du capteur d'oxygène I2C ------------------
float readOxygen() {
  static unsigned long lastReadTime = 0;
  unsigned long now = millis();
  
  // Lire seulement tous les 30 secondes
  if (now - lastReadTime < 30000) {
    return NAN;
  }
  
  lastReadTime = now;
  Serial.println("[O2] Tentative de lecture...");
  
  // Attendre un peu avant de lire
  delay(100);
  
  // SIMPLE : lire 2 octets sans rien écrire
  int bytesReceived = Wire.requestFrom((uint8_t)0x73, (size_t)2, (bool)true);
  
  Serial.print("[O2] Bytes reçus: ");
  Serial.println(bytesReceived);
  
  if (bytesReceived >= 2) {
    uint8_t byte1 = Wire.read();
    uint8_t byte2 = Wire.read();
    uint16_t raw = (byte2 << 8) | byte1;
    
    Serial.print("[O2] Raw: ");
    Serial.println(raw);
    
    // Valider (0-21000 = 0-210%)
    if (raw > 0 && raw <= 21000) {
      float oxygen = raw / 100.0;
      Serial.print("[O2 SUCCESS] ");
      Serial.print(oxygen);
      Serial.println(" %Vol");
      return oxygen;
    }
  }
  
  Serial.println("[O2] Pas de réponse");
  return NAN;
}