#include <Arduino.h>
#include <HardwareSerial.h>

HardwareSerial RS485Serial(2);  // UART2 sur l'ESP32
const int DE_RE = 4;            // Broche de contrôle HW-97

const uint8_t OLD_ADDR = 1;     // Adresse actuelle du capteur
const uint8_t NEW_ADDR = 3;     // Nouvelle adresse qu’on veut programmer

void sendFrame(byte *frame, int length) {
  digitalWrite(DE_RE, HIGH);
  RS485Serial.write(frame, length);
  RS485Serial.flush();
  digitalWrite(DE_RE, LOW);
}

void setup() {
  Serial.begin(115200);
  pinMode(DE_RE, OUTPUT);
  digitalWrite(DE_RE, LOW);
  RS485Serial.begin(9600, SERIAL_8N1, 16, 17);

  Serial.print("Envoi : changement d'adresse ");
  Serial.print(OLD_ADDR);
  Serial.print(" → ");
  Serial.println(NEW_ADDR);

  byte frame[8];
  frame[0] = OLD_ADDR;        // Adresse actuelle du capteur
  frame[1] = 0x06;            // Fonction Modbus : Write Single Register
  frame[2] = 0x01;            // Registre HIGH : 0x0101
  frame[3] = 0x01;            // Registre LOW
  frame[4] = 0x00;            // Valeur HIGH (nouvelle adresse)
  frame[5] = NEW_ADDR;        // Valeur LOW

  // Calcul CRC16
  uint16_t crc = 0xFFFF;
  for (int i = 0; i < 6; i++) {
    crc ^= frame[i];
    for (int b = 0; b < 8; b++) {
      crc = (crc >> 1) ^ ((crc & 1) ? 0xA001 : 0);
    }
  }
  frame[6] = crc & 0xFF;
  frame[7] = crc >> 8;

  // Envoyer
  sendFrame(frame, 8);
  Serial.println("Trame envoyée.");

  // Attente de réponse
  delay(200);
  if (RS485Serial.available() >= 8) {
    byte response[8];
    RS485Serial.readBytes(response, 8);
    Serial.println("Réponse reçue :");
    for (int i = 0; i < 8; i++) {
      Serial.print("0x");
      Serial.print(response[i], HEX);
      Serial.print(" ");
    }
    Serial.println();
  } else {
    Serial.println("Pas de réponse reçue.");
  }
}

void loop() {
}
