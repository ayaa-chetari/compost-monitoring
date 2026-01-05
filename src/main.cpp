/**
 * @file main.cpp
 * @brief Système de monitoring pour compostière avec capteurs SHT20 (RS485) et O2 (I2C)
 * @author PolyGreen
 * @date 2025-12-11
 */

#include <Arduino.h>
#include <stdint.h>
#include <HardwareSerial.h>
#include <Wire.h>

#include <SPI.h>
#include <SD.h>
#include <time.h>



// ==================== CONFIGURATION MATÉRIELLE ====================

/** UART2 de l'ESP32 pour la communication RS485 */
HardwareSerial RS485Serial(2);

/** Broche de contrôle direction RS485 (DE/RE) */
const int RS485_DE_RE = 4;

/** Nombre de capteurs SHT20 connectés */
const int SENSOR_COUNT = 3;

/** Adresses Modbus des capteurs SHT20 */
const uint8_t SENSOR_ADDRESSES[SENSOR_COUNT] = {1, 2, 3};

/** Adresse I2C du capteur d'oxygène (SEN0322) */
const uint8_t OXYGEN_I2C_ADDR = 0x73;

// ==================== CONFIGURATION DE COMMUNICATION ====================

/** Débit série RS485 (bauds) */
const int RS485_BAUD = 9600;

/** RX pour RS485 (GPIO16 sur ESP32) */
const int RS485_RX = 16;

/** TX pour RS485 (GPIO17 sur ESP32) */
const int RS485_TX = 17;

/** Timeout de lecture RS485 (millisecondes) */
const int RS485_TIMEOUT_MS = 200;

/** Taille de la trame Modbus réponse */
const int MODBUS_RESPONSE_SIZE = 7;

// ==================== REGISTRES MODBUS ====================

/** Registre de température SHT20 */
const uint16_t SHT20_REG_TEMP = 0x0001;

/** Registre d'humidité SHT20 */
const uint16_t SHT20_REG_HUMIDITY = 0x0002;

/** Code fonction Modbus pour lecture registres */
const uint8_t MODBUS_FUNC_READ_REG = 0x04;

/** Broche CS pour la carte SD */
const int SD_CS_PIN = 5;   // Chip Select carte SD (à adapter si besoin)
const char *CSV_FILENAME = "/data.csv";



// ==================== DÉCLARATIONS DE FONCTIONS ====================

/**
 * @brief Envoie une trame via RS485
 * @param address Adresse Modbus du capteur
 * @param frame Pointeur sur la trame à envoyer
 * @param length Longueur de la trame (octets)
 */
void sendRS485Frame(uint8_t address, byte *frame, int length);

/**
 * @brief Calcule le CRC16 pour une trame Modbus
 * @param data Pointeur sur les données
 * @param length Longueur des données
 * @return Valeur CRC16 calculée
 */
uint16_t calculateModbusCRC(byte *data, int length);

/**
 * @brief Lit un registre Modbus via RS485
 * @param address Adresse Modbus du capteur
 * @param reg Adresse du registre à lire
 * @return Valeur lue ou NAN en cas d'erreur
 */
float readRS485Register(uint8_t address, uint16_t reg);

/**
 * @brief Lit la valeur d'oxygène via I2C
 * @return Pourcentage d'oxygène (%Vol) ou NAN en cas d'erreur
 */
float readOxygenLevel();

/**
 * @brief Lit la température d'un capteur SHT20
 * @param address Adresse Modbus du capteur
 * @return Température en °C ou NAN en cas d'erreur
 */
float readSHT20Temperature(uint8_t address);

/**
 * @brief Lit l'humidité d'un capteur SHT20
 * @param address Adresse Modbus du capteur
 * @return Humidité en %RH ou NAN en cas d'erreur
 */
float readSHT20Humidity(uint8_t address);

// ==================== FONCTIONS DE COMMUNICATION RS485 ====================

/**
 * @brief Envoie une trame via RS485 avec contrôle de direction
 */
void sendRS485Frame(uint8_t address, byte *frame, int length) {
  digitalWrite(RS485_DE_RE, HIGH);  // Mode émetteur
  RS485Serial.write(frame, length);
  RS485Serial.flush();
  digitalWrite(RS485_DE_RE, LOW);   // Mode récepteur
}


/**
 * @brief Écrit une ligne de données dans le fichier CSV sur la carte SD
 */
void writeCSV(
  float t1, float h1, float o1,
  float t2, float h2,
  float t3, float h3
) {
  File file = SD.open(CSV_FILENAME, FILE_APPEND);
  if (!file) {
    Serial.println("Impossible d’ouvrir le fichier CSV");
    return;
  }

  // Timestamp ISO 8601
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    file.print("1970-01-01T00:00:00");
  } else {
    char timestamp[25];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%dT%H:%M:%S", &timeinfo);
    file.print(timestamp);
  }

  file.print(",");
  file.print(t1); file.print(",");
  file.print(h1); file.print(",");
  file.print(o1); file.print(",");
  file.print(t2); file.print(",");
  file.print(h2); file.print(",");
  file.print(t3); file.print(",");
  file.println(h3);

  file.close();
}


/**
 * @brief Calcule le CRC16 Modbus (polynôme 0xA001)
 */
uint16_t calculateModbusCRC(byte *data, int length) {
  uint16_t crc = 0xFFFF;
  
  for (int i = 0; i < length; i++) {
    crc ^= data[i];
    for (int bit = 0; bit < 8; bit++) {
      if (crc & 1) {
        crc = (crc >> 1) ^ 0xA001;
      } else {
        crc = crc >> 1;
      }
    }
  }
  
  return crc;
}

/**
 * @brief Lit un registre Modbus via RS485
 * Crée une trame Modbus FC04, envoie et traite la réponse
 */
float readRS485Register(uint8_t address, uint16_t reg) {
  byte frame[8];
  
  // Construction de la trame Modbus FC04
  frame[0] = address;
  frame[1] = MODBUS_FUNC_READ_REG;  // Fonction 04 (Read Input Registers)
  frame[2] = reg >> 8;               // Adresse registre (MSB)
  frame[3] = reg & 0xFF;             // Adresse registre (LSB)
  frame[4] = 0x00;                   // Quantité (MSB)
  frame[5] = 0x01;                   // Quantité (LSB) = 1 registre

  // Calcul et ajout du CRC
  uint16_t crc = calculateModbusCRC(frame, 6);
  frame[6] = crc & 0xFF;
  frame[7] = crc >> 8;

  // Vider le buffer d'entrée
  while (RS485Serial.available()) {
    RS485Serial.read();
  }

  // Envoyer la trame
  sendRS485Frame(address, frame, 8);

  // Attendre la réponse
  unsigned long startTime = millis();
  while (RS485Serial.available() < MODBUS_RESPONSE_SIZE && 
         millis() - startTime < RS485_TIMEOUT_MS) {
    delay(1);
  }

  // Traiter la réponse
  if (RS485Serial.available() >= MODBUS_RESPONSE_SIZE) {
    byte response[MODBUS_RESPONSE_SIZE];
    RS485Serial.readBytes(response, MODBUS_RESPONSE_SIZE);

    // Vérifier l'adresse et la fonction
    if (response[0] != address || response[1] != MODBUS_FUNC_READ_REG) {
      return NAN;
    }

    // Extraire la valeur (registre 16-bit, format big-endian)
    int16_t rawValue = (response[3] << 8) | response[4];
    
    // Conversion en valeur physique (divisé par 10)
    return rawValue / 10.0;
  }

  return NAN;  // Timeout ou pas de réponse
}

// ==================== FONCTIONS DE COMMUNICATION I2C ====================

/**
 * @brief Lit la concentration d'oxygène via le capteur I2C SEN0322
 * @return Pourcentage d'oxygène (%Vol) ou NAN en cas d'erreur
 */
float readOxygenLevel() {
  // Initier la transmission
  Wire.beginTransmission(OXYGEN_I2C_ADDR);
  Wire.write(0x03);  // Registre de données (0x03)
  
  if (Wire.endTransmission(false) != 0) {
    return NAN;  // Erreur de transmission
  }

  // Demander 2 octets de données
  Wire.requestFrom(OXYGEN_I2C_ADDR, 2);
  
  if (Wire.available() < 2) {
    return NAN;  // Pas assez de données
  }

  // Lire les données (LSB en premier, puis MSB)
  uint8_t lo = Wire.read();
  uint8_t hi = Wire.read();
  uint16_t rawValue = (hi << 8) | lo;

  // Conversion en pourcentage (divisé par 100)
  return rawValue / 100.0;
}

// ==================== FONCTIONS DE CAPTEURS SPÉCIFIQUES ====================

/**
 * @brief Lit la température d'un capteur SHT20
 */
float readSHT20Temperature(uint8_t address) {
  return readRS485Register(address, SHT20_REG_TEMP);
}

/**
 * @brief Lit l'humidité d'un capteur SHT20
 */

float readSHT20Humidity(uint8_t address) {
  return readRS485Register(address, SHT20_REG_HUMIDITY);
}

// ==================== INITIALISATION ====================

/**
 * @brief Configuration initiale du système
 * - Initialise la communication série USB (debug)
 * - Configure la broche RS485 DE/RE
 * - Initialise UART2 pour RS485
 * - Initialise le bus I2C
 */
void setup() {
  // Initialiser la communication série USB (115200 bauds pour debug)
  Serial.begin(115200);
  
  // Configurer la broche de direction RS485
  pinMode(RS485_DE_RE, OUTPUT);
  digitalWrite(RS485_DE_RE, LOW);  // Mode récepteur par défaut
  
  // Initialiser RS485 (UART2)
  RS485Serial.begin(RS485_BAUD, SERIAL_8N1, RS485_RX, RS485_TX);
  
  // Initialiser le bus I2C (SDA=GPIO21, SCL=GPIO22 par défaut sur ESP32)
  Wire.begin();
  
  // Message de démarrage
  Serial.println("\n================================");
  Serial.println("  SYSTÈME DE MONITORING");
  Serial.println("  Capteurs SHT20 (RS485)");
  Serial.println("  Capteur O2 (I2C)");
  Serial.println("================================\n");

    // ==================== INITIALISATION CARTE SD ====================
  if (!SD.begin(SD_CS_PIN)) {
    Serial.println("Erreur : Carte SD non détectée");
  } else {
    Serial.println("Carte SD initialisée");

    // Créer le fichier CSV avec en-tête si inexistant
    if (!SD.exists(CSV_FILENAME)) {
      File file = SD.open(CSV_FILENAME, FILE_WRITE);
      if (file) {
        file.println("timestamp,temperature_bac1,humidity_bac1,oxygen_bac1,temperature_bac2,humidity_bac2,temperature_bac3,humidity_bac3");
        file.close();
        Serial.println("📄 Fichier CSV créé avec en-tête");
      }
    }
  }

  // ==================== CONFIGURATION HORLOGE (timestamp) ====================
  configTime(0, 0, "pool.ntp.org");  // UTC (ou adapte si besoin)
}

// ==================== BOUCLE PRINCIPALE ====================

/**
 * @brief Boucle principale - Acquisition de données à intervalle régulier
 * - Lit tous les capteurs SHT20
 * - Lit le capteur d'oxygène
 * - Affiche les résultats via série
 */
void loop() {
  // Lecture des capteurs de température et humidité
  for (int i = 0; i < SENSOR_COUNT; i++) {
    uint8_t address = SENSOR_ADDRESSES[i];

    // Afficher l'en-tête du capteur
    Serial.print("\n[Capteur SHT20 - Adresse ");
    Serial.print(address);
    Serial.println("]");

    // Lire les données
    float temperature = readSHT20Temperature(address);
    delay(100);
    float humidity = readSHT20Humidity(address);

    // Afficher la température
    Serial.print("  Température: ");
    if (!isnan(temperature)) {
      Serial.print(temperature, 1);
      Serial.println(" °C");
    } else {
      Serial.println("Erreur de lecture");
    }

    // Afficher l'humidité
    Serial.print("  Humidité: ");
    if (!isnan(humidity)) {
      Serial.print(humidity, 1);
      Serial.println(" %RH");
    } else {
      Serial.println("Erreur de lecture");
    }

    delay(500);
  }

  // Lecture du capteur d'oxygène
  Serial.print("\n[Capteur d'Oxygène - Adresse I2C 0x");
  Serial.print(OXYGEN_I2C_ADDR, HEX);
  Serial.println("]");
  
  float oxygen = readOxygenLevel();
  Serial.print("  Oxygène: ");
  if (!isnan(oxygen)) {
    Serial.print(oxygen, 1);
    Serial.println(" %Vol");
  } else {
    Serial.println("Erreur de lecture");
  }

  // Fin du cycle
  Serial.println("\n================================");
  Serial.println("  Cycle d'acquisition terminé");
  Serial.println("================================\n");
  
    // ==================== ENREGISTREMENT CSV ====================
  writeCSV(
    readSHT20Temperature(1),
    readSHT20Humidity(1),
    oxygen,
    readSHT20Temperature(2),
    readSHT20Humidity(2),
    readSHT20Temperature(3),
    readSHT20Humidity(3)
  );

  delay(60000);  // Attendre avant le prochain cycle
}


