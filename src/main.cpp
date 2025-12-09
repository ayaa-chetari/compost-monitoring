#include <Arduino.h>
#include <stdint.h>
#include <HardwareSerial.h>
#include <Wire.h>  // Pour I2C

HardwareSerial RS485Serial(2);   // UART2 de l'ESP32

const int DE_RE = 4;             // RS485 direction
const int CAPTEUR_COUNT = 3;
const uint8_t addresses[CAPTEUR_COUNT] = {1, 2, 3};

// ==== Capteur O2 I2C ====
const uint8_t OXYGEN_I2C_ADDR = 0x73; // à ajuster selon ton capteur
float readOxygen();

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

// ------------------ SETUP ------------------
void setup() {
  Serial.begin(115200);
  pinMode(DE_RE, OUTPUT);
  digitalWrite(DE_RE, LOW);
  RS485Serial.begin(9600, SERIAL_8N1, 16, 17);
  Wire.begin(); // I2C (SDA, SCL) = GPIO21, GPIO22 par défaut sur ESP32
  Serial.println("Lecture des capteurs SHT20 (RS485) + Oxygène (I2C)");
}

// ------------------ LOOP ------------------
void loop() {
  for (int i = 0; i < CAPTEUR_COUNT; i++) {
    uint8_t addr = addresses[i];

    Serial.print("📟 Capteur SHT20 adresse ");
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
    delay(500);
  }

  // Lecture du capteur d'oxygène
  float oxygen = readOxygen();
  Serial.print("Oxygène : ");
  if (!isnan(oxygen)) {
    Serial.print(oxygen);
    Serial.println(" %Vol");
  } else {
    Serial.println("Lecture échouée");
  }

  Serial.println("==== Fin du cycle ====\n");
  delay(2000);
}

// ------------------ Lecture du capteur d'oxygène I2C ------------------
float readOxygen() {
  Wire.beginTransmission(0x73);       // Adresse du SEN0322
  Wire.write(0x03);                   // Registre de lecture (0x03)
  if (Wire.endTransmission(false) != 0) return NAN;

  Wire.requestFrom(0x73, 2);          // Lire 2 octets
  if (Wire.available() < 2) return NAN;

  uint8_t lo = Wire.read();           // ⚠️ LSB d'abord
  uint8_t hi = Wire.read();           // Puis MSB
  uint16_t raw = (hi << 8) | lo;

  return raw / 100.0;                 // Conversion en %Vol
}

