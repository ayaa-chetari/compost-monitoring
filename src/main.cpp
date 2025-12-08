#include <Arduino.h>
#include <stdint.h>
#include <HardwareSerial.h>

HardwareSerial RS485Serial(2);   // UART2 de l'ESP32

const int DE_RE = 4;  // Pin qui contrôle émission/réception du MAX485
const int slaveAddress = 1;




void sendFrame(byte *frame, int length) {
  digitalWrite(DE_RE, HIGH); // Mode émission
  RS485Serial.write(frame, length);
  RS485Serial.flush();       // Attendre la fin de l'envoi
  digitalWrite(DE_RE, LOW);  // Retour réception
}

float readRegister(uint16_t reg) {
  // Construction de la trame
  byte frame[8];
  frame[0] = slaveAddress;
  frame[1] = 0x04; // Fonction : Read Input Register
  frame[2] = reg >> 8;
  frame[3] = reg & 0xFF;
  frame[4] = 0x00;
  frame[5] = 0x01;

  // CRC16 (Modbus)
  uint16_t crc = 0xFFFF;
  for (int i = 0; i < 6; i++) {
    crc ^= frame[i];
    for (int b = 0; b < 8; b++) {
      crc = (crc >> 1) ^ ((crc & 1) ? 0xA001 : 0);
    }
  }
  frame[6] = crc & 0xFF;
  frame[7] = crc >> 8;

  // Vider le buffer série avant d’envoyer
  while (RS485Serial.available()) RS485Serial.read();

  sendFrame(frame, 8);

  unsigned long start = millis();
  while (RS485Serial.available() < 7 && millis() - start < 200) {
    delay(1);
  }

  if (RS485Serial.available() >= 7) {
    byte resp[7];
    RS485Serial.readBytes(resp, 7);

    if (resp[0] != slaveAddress || resp[1] != 0x04) {
      return NAN; // mauvaise trame
    }

    int16_t rawValue = (resp[3] << 8) | resp[4];
    return rawValue / 10.0;
  }

  return NAN;
}

void setup() {
  Serial.begin(115200);
  pinMode(DE_RE, OUTPUT);
  digitalWrite(DE_RE, LOW);
  RS485Serial.begin(9600, SERIAL_8N1, 16, 17);
  Serial.println("Lecture capteur SHT20 RS485 (Modbus)");
}

void loop() {
  float temperature = readRegister(0x0001);
  delay(100); // Laisse respirer le capteur
  float humidity = readRegister(0x0002);

  Serial.print("Température : ");
  Serial.print(temperature);
  Serial.println(" °C");

  Serial.print("Humidité : ");
  Serial.print(humidity);
  Serial.println(" %RH");

  Serial.println("---------------------------");
  delay(2000);
}
