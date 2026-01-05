# Guide Détaillé : Réception et Parsing RS485

## Flux Complet de Réception

### Phase 1 : Configuration Initiale

```cpp
void setup() {
  Serial.begin(115200);  // Communication debug
  
  // === CONFIGURATION RS485 ===
  // Pin GPIO4 = Contrôle de direction
  pinMode(RS485_DE_RE, OUTPUT);
  digitalWrite(RS485_DE_RE, LOW);  // Par défaut : mode récepteur
  
  // === UART2 INITIALIZATION ===
  // ESP32 a 3 UART : UART0 (USB), UART1 (libre), UART2 (utilisé)
  RS485Serial.begin(
    9600,          // Débit en bauds (bits/seconde)
    SERIAL_8N1,    // 8 bits data, No parity, 1 Stop bit
    16,            // RX pin (GPIO16)
    17             // TX pin (GPIO17)
  );
  
  // === CONFIGURATION I2C ===
  Wire.begin();  // SDA=GPIO21, SCL=GPIO22 (défaut ESP32)
  
  delay(500);
}

/**
 * TIMELINE DE SETUP :
 * 
 * t=0ms : GPIO4 configuration
 *         └─ De-asserting (LOW) = En écoute
 *
 * t=1ms : UART2 initialized at 9600 baud
 *         └─ 9600 bps = ~1.04ms pour transmettre 10 bits
 *         └─ Chaque octet = 10 bits (1 start + 8 data + 1 stop)
 *         └─ Temps par octet = 10 / 9600 = 1.04 ms
 *         └─ 8 octets = ~8.3 ms
 *
 * t=2ms : I2C initialized
 *
 * t=500ms : Prêt à communiquer
 */
```

### Phase 2 : Construction et Envoi de Trame

```cpp
/**
 * ÉTAPE 1 : CONSTRUCTION DE LA TRAME MODBUS
 * 
 * Une trame Modbus FC04 (Read Input Registers) a cette structure :
 * 
 * ┌────────────────────────────────────────────────────────┐
 * │ Byte 0 : Slave Address                                 │
 * │ Byte 1 : Function Code (0x04 pour FC04)                │
 * │ Byte 2 : Starting Register Address (High Byte)         │
 * │ Byte 3 : Starting Register Address (Low Byte)          │
 * │ Byte 4 : Quantity of Input Registers (High Byte)       │
 * │ Byte 5 : Quantity of Input Registers (Low Byte)        │
 * │ Byte 6 : CRC (Low Byte) - Calculé                      │
 * │ Byte 7 : CRC (High Byte) - Calculé                     │
 * └────────────────────────────────────────────────────────┘
 * 
 * Total : 8 octets
 */

float readRS485Register(uint8_t address, uint16_t reg) {
  // ====================================================
  // ÉTAPE 1 : CONSTRUCTION TRAME
  // ====================================================
  
  byte frame[8];
  
  // Slave Address
  frame[0] = address;
  //   └─ Valeur : 1, 2 ou 3 (adresse Modbus du capteur)
  //   └─ Spécifie auquel des 3 capteurs envoyer la requête
  
  // Function Code
  frame[1] = MODBUS_FUNC_READ_REG;  // 0x04
  //   └─ 0x04 = Read Input Registers
  //   └─ Les SHT20 utilisent les "Input Registers" (lire seulement)
  
  // Starting Register Address
  frame[2] = reg >> 8;      // High byte de l'adresse registre
  frame[3] = reg & 0xFF;    // Low byte de l'adresse registre
  //   └─ Si reg = 0x0001 : frame[2]=0x00, frame[3]=0x01
  //   └─ Si reg = 0x0002 : frame[2]=0x00, frame[3]=0x02
  //   └─ Registres SHT20:
  //      • 0x0001 = Température
  //      • 0x0002 = Humidité
  //      • 0x0003 = Status
  
  // Quantity of Input Registers
  frame[4] = 0x00;          // High byte du nombre de registres
  frame[5] = 0x01;          // Low byte du nombre de registres
  //   └─ 0x0001 = Lire 1 registre (2 octets de données)
  //   └─ Toujours 0x0001 dans notre cas
  
  // EXEMPLE : Lire température du capteur 1
  // frame = [0x01, 0x04, 0x00, 0x01, 0x00, 0x01]
  //          │    │    └─ Registre 0x0001 (TEMP)
  //          │    └──── FC04 (Read Input Regs)
  //          └───────── Adresse 1
  
  
  // ====================================================
  // ÉTAPE 2 : CALCUL CRC16 MODBUS
  // ====================================================
  
  /**
   * CRC16 Modbus RTU Détail :
   * 
   * Polynôme : 0xA001 (réfléchi/inverse)
   * Valeur initiale : 0xFFFF
   * Format envoi : LSB d'abord, puis MSB
   * 
   * Algorithme (référence littéraire) :
   * 1. Initialiser CRC = 0xFFFF
   * 2. Pour chaque octet à traiter :
   *    a. XOR le CRC avec l'octet
   *    b. Pour chaque bit (0 à 7) :
   *       - Si LSB du CRC = 1 :
   *         → Décaler CRC à droite
   *         → XOR avec 0xA001
   *       - Sinon :
   *         → Juste décaler à droite
   * 3. Retourner le CRC calculé
   */
  
  uint16_t crc = calculateModbusCRC(frame, 6);
  //   └─ Passe les 6 premiers octets (pas le CRC lui-même)
  //   └─ Retourne 16 bits (2 octets)
  
  // Placement du CRC dans la trame
  frame[6] = crc & 0xFF;    // CRC Low Byte (envoyé en premier)
  frame[7] = crc >> 8;      // CRC High Byte (envoyé en second)
  //   └─ Format Little-Endian (inversé par rapport à la norme)
  //   └─ Cela fait partie de la spécification Modbus RTU
  
  // Exemple avec CRC calculé :
  // Si CRC = 0x41C6
  //   frame[6] = 0xC6 (bits 0-7)
  //   frame[7] = 0x41 (bits 8-15)
  // Trame complète = [0x01, 0x04, 0x00, 0x01, 0x00, 0x01, 0xC6, 0x41]
  
  
  // ====================================================
  // ÉTAPE 3 : NETTOYAGE DU BUFFER RX
  // ====================================================
  
  // Avant d'émettre, vider tout ce qui traîne dans le buffer RX
  // Cela peut inclure :
  //   - Réponses partielles des lectures précédentes
  //   - Bruits électriques décodés
  //   - Données reçues non lues
  
  while (RS485Serial.available()) {
    int unused = RS485Serial.read();
    // Serial.print("DEBUG: Octet ignoré = 0x");
    // Serial.println(unused, HEX);
  }
  
  
  // ====================================================
  // ÉTAPE 4 : SÉLECTION MODE ÉMETTEUR
  // ====================================================
  
  digitalWrite(RS485_DE_RE, HIGH);  // DE/RE = 1 (Mode TX)
  //   └─ GPIO4 passe à 3.3V
  //   └─ Le driver Max485 :
  //      • Active la sortie (A, B)
  //      • Désactive la réception
  //      • Commence à piloter le bus RS485
  //   └─ Timing : ~50µs après l'ordre
  
  // Dans le Max485 :
  // - Quand DE=1 : DI (GPIO17/TX) contrôle A/B
  // - Quand RE=1 : RO (GPIO16/RX) est à haute impédance
  
  
  // ====================================================
  // ÉTAPE 5 : TRANSMISSION SÉRIE
  // ====================================================
  
  RS485Serial.write(frame, 8);  // Envoyer les 8 octets
  //   └─ Chaque octet est envoyé séquentiellement
  //   └─ Format : 1 START + 8 DATA + 1 STOP = 10 bits
  //   └─ Temps par octet = 10 bits / 9600 bps = 1.04 ms
  //   └─ Pour 8 octets = 8 × 1.04 = 8.32 ms
  //   └─ La fonction ne bloque PAS (elle utilise un buffer interne)
  
  RS485Serial.flush();  // Attendre la fin de transmission
  //   └─ Bloque jusqu'à ce que tous les 8 octets soient envoyés
  //   └─ Dure ~8.3ms à 9600 baud
  //   └─ Avant cette ligne, les données peuvent être en transit
  //   └─ Après cette ligne, tout est envoyé physiquement
  
  
  // ====================================================
  // ÉTAPE 6 : RETOUR MODE RÉCEPTEUR
  // ====================================================
  
  digitalWrite(RS485_DE_RE, LOW);   // DE/RE = 0 (Mode RX)
  //   └─ GPIO4 passe à 0V
  //   └─ Le driver Max485 :
  //      • Désactive la sortie (A, B en haute impédance)
  //      • Active la réception (RO ← A-B)
  //      • Permet au ESP32 d'écouter le bus
  //   └─ Timing : ~50µs après l'ordre
  
  // IMPORTANT : 
  // Il doit y avoir un délai TRÈS COURT après flush() et avant LOW
  // Généralement négligeable car flush() prend déjà du temps
  // Mais certains systèmes rapides peuvent nécessiter un delay(1)
  
  
  // ====================================================
  // ÉTAPE 7 : ATTENTE DE LA RÉPONSE
  // ====================================================
  
  unsigned long startTime = millis();  // Mémoriser le temps de début
  //   └─ Returns : nombre de millisecondes depuis le démarrage
  //   └─ Type : unsigned long (32 bits)
  //   └─ Déborde après ~49 jours
  
  // Boucle d'attente
  while (RS485Serial.available() < MODBUS_RESPONSE_SIZE && 
         millis() - startTime < RS485_TIMEOUT_MS) {
    delay(1);  // Laisser respirer le µC
  }
  
  /**
   * TIMELINE D'ATTENTE :
   * 
   * t=0ms   : finish() complète, GPIO4 → LOW
   * t=0-1ms : Délai propagation réseau RS485
   * t=1-10ms: Capteur reçoit et décode trame
   * t=10-15ms: Capteur traite la requête
   *            └─ Accède au registre 0x0001
   *            └─ Prépare la réponse
   * t=15-25ms: Capteur envoie réponse (7 octets)
   * t=25-30ms: Propagation réseau
   * t=30-35ms: ESP32 reçoit tous les octets
   * 
   * TOTAL : ~30-40ms en conditions normales
   * TIMEOUT : 200ms (très généreux)
   */
  
  
  // ====================================================
  // ÉTAPE 8 : VÉRIFICATION RÉCEPTION
  // ====================================================
  
  if (RS485Serial.available() >= MODBUS_RESPONSE_SIZE) {
    // Au moins 7 octets reçus avant timeout
    
    byte response[MODBUS_RESPONSE_SIZE];  // Array de 7 octets
    RS485Serial.readBytes(response, MODBUS_RESPONSE_SIZE);
    //   └─ Lis exactement 7 octets du buffer RX
    //   └─ Bloque jusqu'à avoir les 7 octets OU timeout du UART
    
    /**
     * STRUCTURE DE LA RÉPONSE :
     * 
     * response[0] : Slave Address (echo)
     * response[1] : Function Code (echo) = 0x04
     * response[2] : Byte Count = 0x02 (toujours 2 pour 1 registre)
     * response[3] : Data High Byte
     * response[4] : Data Low Byte
     * response[5] : CRC Low Byte
     * response[6] : CRC High Byte
     */
    
    
    // ====================================================
    // ÉTAPE 9 : VALIDATION RÉPONSE
    // ====================================================
    
    // Vérifier que c'est la réponse au bon capteur et fonction
    if (response[0] != address || response[1] != MODBUS_FUNC_READ_REG) {
      // Mauvaise réponse
      Serial.print("DEBUG: Mauvaise réponse - ");
      Serial.print("Addr=0x");
      Serial.print(response[0], HEX);
      Serial.print(" Func=0x");
      Serial.println(response[1], HEX);
      return NAN;  // Retourner NAN (Not A Number)
    }
    
    
    // ====================================================
    // ÉTAPE 10 : EXTRACTION DES DONNÉES
    // ====================================================
    
    // Les données sont dans response[3] et response[4]
    // Format : Big-Endian (MSB d'abord)
    
    int16_t rawValue = (response[3] << 8) | response[4];
    
    /**
     * EXPLICATION DE L'EXTRACTION :
     * 
     * response[3] = 0x00 (byte MSB = bits 15-8)
     * response[4] = 0xEB (byte LSB = bits 7-0)
     * 
     * (response[3] << 8)  : Décale 0x00 de 8 bits à gauche
     *                       0x00 << 8 = 0x0000
     * 
     * | response[4]       : OU logique avec byte LSB
     *                       0x0000 | 0x00EB = 0x00EB
     * 
     * = 0x00EB = 235 décimal (en format signé 16-bit)
     * 
     * Pour température :
     *   - 235 est l'équivalent de 23.5°C (valeur × 0.1°C)
     *   
     * Pour humidité :
     *   - 450 est l'équivalent de 45.0%RH (valeur × 0.1%RH)
     */
    
    
    // ====================================================
    // ÉTAPE 11 : CONVERSION EN VALEUR PHYSIQUE
    // ====================================================
    
    // Les données SHT20 sont encodées en 0.1 unités
    // Diviser par 10.0 pour obtenir la vraie valeur
    
    float physicalValue = rawValue / 10.0;
    
    /**
     * EXEMPLES DE CONVERSION :
     * 
     * Température :
     *   rawValue = 235  → 235 / 10.0 = 23.5°C
     *   rawValue = 0    → 0 / 10.0 = 0.0°C
     *   rawValue = -50  → -50 / 10.0 = -5.0°C
     * 
     * Humidité :
     *   rawValue = 450  → 450 / 10.0 = 45.0%RH
     *   rawValue = 1000 → 1000 / 10.0 = 100.0%RH
     *   rawValue = 0    → 0 / 10.0 = 0.0%RH
     */
    
    return physicalValue;
    
  } else {
    // ====================================================
    // ERREUR : TIMEOUT
    // ====================================================
    
    Serial.print("DEBUG: Timeout - Octets reçus = ");
    Serial.print(RS485Serial.available());
    Serial.print(" (attendus ");
    Serial.print(MODBUS_RESPONSE_SIZE);
    Serial.println(")");
    
    return NAN;  // Aucune réponse reçue
  }
}
```

---

## Diagramme Temporel Détaillé

```
┌─────────────────────────────────────────────────────────────────┐
│                                                                 │
│  t=0µs    : GPIO4 = HIGH (Mode TX activé)                      │
│  │                                                              │
│  │         ┌──────────────────────────────────┐                │
│  │         │   ÉMISSION 8 OCTETS              │                │
│  │         │  @ 9600 bps = 8.3ms              │                │
│  │         │  [0x01][0x04][0x00][0x01]        │                │
│  │         │  [0x00][0x01][0xC6][0x41]        │                │
│  │         └──────────────────────────────────┘                │
│  │ │                                          │                │
│  │ ▼────────────────────────────────────────▼─────────────── │
│  │ 0    1    2    3    4    5    6    7    8     8.3           │
│  │[ms]                                                          │
│  │                                                              │
│  └─ GPIO4 = LOW (Mode RX)                                     │
│                                                                 │
│  t=8.3ms  : RS485 Bus libre (attente réponse)                │
│  │                                                              │
│  │         ┌─────────────────────────────────┐                │
│  │         │ Capteur reçoit et traite        │                │
│  │         │  +Propagation : 1-2ms           │                │
│  │         │  +Décodage : 2-5ms              │                │
│  │         │  +Traitement : 5-10ms           │                │
│  │         │  +Préparation réponse : 5-10ms  │                │
│  │         └─────────────────────────────────┘                │
│  │ │                                          │                │
│  │ ▼────────────────────────────────────────▼─────────────── │
│  │ 8    10    15    20    25    30    35    38.3 [ms]         │
│  │                                                              │
│  └─ Capteur commence émission réponse                         │
│                                                                 │
│  t=30ms   : ÉMISSION RÉPONSE 7 OCTETS                         │
│  │         @ 9600 bps = 7.3ms                                 │
│  │         [0x01][0x04][0x02][0x00][0xEB]                    │
│  │         [0x??][0x??]  (CRC)                               │
│  │ │                                          │                │
│  │ ▼────────────────────────────────────────▼─────────────── │
│  │ 30   31    32    33    34    35    36   37.3   38    40    │
│  │ [ms]                                                        │
│  │                                                              │
│  └─ ESP32 commence à recevoir                                │
│                                                                 │
│  t=37.3ms : RÉCEPTION COMPLÈTE                               │
│  │                                                              │
│  │         ┌────────────────────────────────┐                │
│  │         │ ESP32 Parse réponse            │                │
│  │         │ Validation, extraction, retour │                │
│  │         └────────────────────────────────┘                │
│  │ │                                          │                │
│  │ ▼────────────────────────────────────────▼─────────────── │
│  │ 37.3  38    39    40    41    42 [ms]                      │
│  │                                                              │
│  └─ Retour de la fonction                                    │
│                                                                 │
│  ╔════════════════════════════════════════════════════════════╗
│  ║ TEMPS TOTAL : 0 → 42 ms (en conditions normales)          ║
│  ║ TIMEOUT : 200ms (5x plus que nécessaire)                  ║
│  ║ MARGE DE SÉCURITÉ : Très bonne                            ║
│  ╚════════════════════════════════════════════════════════════╝
│                                                                 │
└─────────────────────────────────────────────────────────────────┘
```

---

## Cas Spécial : Lecture Multiple (3 Capteurs)

```cpp
void readAllSensors() {
  Serial.println("\n=== LECTURE DES 3 CAPTEURS ===");
  
  for (int i = 0; i < SENSOR_COUNT; i++) {
    uint8_t addr = SENSOR_ADDRESSES[i];
    
    // Lire température
    float temp = readSHT20Temperature(addr);
    delay(100);  // Délai entre les requêtes
    
    // Lire humidité
    float humidity = readSHT20Humidity(addr);
    delay(500);  // Délai avant le prochain capteur
    
    // Afficher résultats
    Serial.print("Capteur ");
    Serial.print(addr);
    Serial.print(" : ");
    Serial.print(temp, 1);
    Serial.print("°C | ");
    Serial.print(humidity, 1);
    Serial.println("%RH");
  }
}

/**
 * TIMELINE COMPLÈTE (3 capteurs × 2 mesures + délais) :
 * 
 * Capteur 1:
 *   t=0ms    : Lire température    → 40ms
 *   t=100ms  : Lire humidité       → 40ms (au total 100+40=140ms)
 *   t=640ms  : Fin capteur 1, délai 500ms
 *
 * Capteur 2:
 *   t=640ms  : Lire température    → 40ms
 *   t=740ms  : Lire humidité       → 40ms
 *   t=1240ms : Fin capteur 2, délai 500ms
 *
 * Capteur 3:
 *   t=1240ms : Lire température    → 40ms
 *   t=1340ms : Lire humidité       → 40ms
 *   t=1840ms : Fin capteur 3
 *
 * TEMPS TOTAL : ~1.8 secondes pour tous les capteurs
 * BOUCLE principale : delay(2000) = 2 secondes total
 */
```

---

## Résumé en Table de Référence

| Phase | Temps | Action | Description |
|-------|-------|--------|---|
| **Préparation** | 0-10µs | GPIO4 = HIGH | Passer en mode émetteur |
| **Émission** | 10µs-8.3ms | TX 8 octets | 9600 baud = 1.04ms/octet |
| **Fin TX** | 8.3ms | GPIO4 = LOW | Mode récepteur |
| **Propagation** | 8.3-10ms | Bus libre | Signal se propage (+2ms) |
| **Réception capteur** | 10-15ms | RX + Décode | Capteur reçoit trame |
| **Traitement** | 15-25ms | Accès mémoire | Lit registre 0x0001/0x0002 |
| **Préparation réponse** | 25-30ms | Construit trame | Constructio réponse + CRC |
| **Émission réponse** | 30-37.3ms | TX 7 octets | Envoie réponse |
| **Propagation retour** | 37.3-38ms | Bus libre | Signal revient |
| **Réception ESP32** | 38-43ms | RX + Parse | Reçoit et valide |
| **Retour fonction** | ~40-50ms | Total | Retour float |

