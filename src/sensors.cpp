#include "sensors.h"
#include <HardwareSerial.h>
#include <Wire.h>
#include <math.h>

HardwareSerial RS485Serial(2);

static const int RS485_DE_RE = 4;
static const int RS485_BAUD = 9600;
static const int RS485_RX = 16;
static const int RS485_TX = 17;

static const int RS485_TIMEOUT_MS = 200;
static const int MODBUS_RESPONSE_SIZE = 7;

static const uint16_t SHT20_REG_TEMP = 0x0001;
static const uint16_t SHT20_REG_HUMIDITY = 0x0002;
static const uint8_t MODBUS_FUNC_READ_REG = 0x04;

static const uint8_t OXYGEN_I2C_ADDR = 0x73;

static const uint8_t SENSOR_ADDRESSES[3] = {1,2,3};

// ---------- RS485 helpers ----------
static void sendRS485Frame(byte *frame, int length) {
  digitalWrite(RS485_DE_RE, HIGH);
  RS485Serial.write(frame, length);
  RS485Serial.flush();
  digitalWrite(RS485_DE_RE, LOW);
}

static uint16_t calculateModbusCRC(byte *data, int length) {
  uint16_t crc = 0xFFFF;
  for (int i=0;i<length;i++){
    crc ^= data[i];
    for (int b=0;b<8;b++){
      crc = (crc & 1) ? ((crc>>1) ^ 0xA001) : (crc>>1);
    }
  }
  return crc;
}

static float readRS485Register(uint8_t address, uint16_t reg) {
  byte frame[8];
  frame[0] = address;
  frame[1] = MODBUS_FUNC_READ_REG;
  frame[2] = reg >> 8;
  frame[3] = reg & 0xFF;
  frame[4] = 0x00;
  frame[5] = 0x01;

  uint16_t crc = calculateModbusCRC(frame, 6);
  frame[6] = crc & 0xFF;
  frame[7] = crc >> 8;

  while (RS485Serial.available()) RS485Serial.read();

  sendRS485Frame(frame, 8);

  unsigned long start = millis();
  while (RS485Serial.available() < MODBUS_RESPONSE_SIZE && (millis()-start) < RS485_TIMEOUT_MS) {
    delay(1);
  }

  if (RS485Serial.available() >= MODBUS_RESPONSE_SIZE) {
    byte resp[MODBUS_RESPONSE_SIZE];
    RS485Serial.readBytes(resp, MODBUS_RESPONSE_SIZE);

    if (resp[0] != address || resp[1] != MODBUS_FUNC_READ_REG) return NAN;

    int16_t raw = (resp[3] << 8) | resp[4];
    return raw / 10.0f;
  }

  return NAN;
}

static float readSHT20Temperature(uint8_t addr){ return readRS485Register(addr, SHT20_REG_TEMP); }
static float readSHT20Humidity(uint8_t addr){ return readRS485Register(addr, SHT20_REG_HUMIDITY); }

// ---------- I2C O2 ----------
static float readOxygenLevel() {
  Wire.beginTransmission(OXYGEN_I2C_ADDR);
  Wire.write(0x03);
  if (Wire.endTransmission(false) != 0) return NAN;

  Wire.requestFrom(OXYGEN_I2C_ADDR, (uint8_t)2);
  if (Wire.available() < 2) return NAN;

  uint8_t lo = Wire.read();
  uint8_t hi = Wire.read();
  uint16_t raw = (hi << 8) | lo;

  return raw / 100.0f;
}

// ---------- Public API ----------
void sensorsInit() {
  pinMode(RS485_DE_RE, OUTPUT);
  digitalWrite(RS485_DE_RE, LOW);

  RS485Serial.begin(RS485_BAUD, SERIAL_8N1, RS485_RX, RS485_TX);
  Wire.begin();
}

bool sensorsRead(Sample3 &out) {
  // Bac1
  out.b1Temp = readSHT20Temperature(SENSOR_ADDRESSES[0]);
  delay(60);
  out.b1Hum  = readSHT20Humidity(SENSOR_ADDRESSES[0]);

  // Bac2
  delay(60);
  out.b2Temp = readSHT20Temperature(SENSOR_ADDRESSES[1]);
  delay(60);
  out.b2Hum  = readSHT20Humidity(SENSOR_ADDRESSES[1]);

  // Bac3
  delay(60);
  out.b3Temp = readSHT20Temperature(SENSOR_ADDRESSES[2]);
  delay(60);
  out.b3Hum  = readSHT20Humidity(SENSOR_ADDRESSES[2]);

  // O2 Bac1
  out.b1O2 = readOxygenLevel();

  // “true” même si NAN, tu peux décider sinon
  return true;
}
