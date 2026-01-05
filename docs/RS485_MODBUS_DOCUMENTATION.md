# Documentation: Communication RS485 Modbus ESP32 ↔ Capteurs SHT20

## Table des Matières
1. [Introduction Théorique](#introduction-théorique)
2. [Architecture Physique](#architecture-physique)
3. [Protocole Modbus RTU](#protocole-modbus-rtu)
4. [Flux de Communication](#flux-de-communication)
5. [Implémentation détaillée](#implémentation-détaillée)
6. [Exemple Complet avec Diagrammes](#exemple-complet-avec-diagrammes)

---

## Introduction Théorique

### Qu'est-ce que RS485 ?

**RS485** (Recommended Standard 485) est une norme de communication série industrielle permettant la transmission de données sur longues distances avec plusieurs appareils.

#### Caractéristiques principales :

| Caractéristique | Valeur |
|---|---|
| **Distance maximale** | 1200 m (à bas débit) |
| **Nombre d'appareils** | 32 à 128 (dépend du driver) |
| **Débit maximal** | 10 Mbps |
| **Tension de fonctionnement** | -7V à +12V |
| **Type de transmission** | Différentielle (2 fils: A, B) |
| **Vitesse de liaison (notre projet)** | 9600 bauds |

### Transmission Différentielle

RS485 utilise une **transmission différentielle** sur 2 fils (A et B) :

```
ESP32 ─────────────────[A(+)]───────────── Capteur 1
       │                                      │
       └───────────[B(-)]────────────────────┘
       
La tension différentielle = V(A) - V(B)
- Si V(A) - V(B) > 200mV → Bit = 1 (Mark)
- Si V(A) - V(B) < -200mV → Bit = 0 (Space)
```


## Architecture Physique

### Schéma de Connexion

```
┌─────────────────────────────────────────────────────────────┐
│                          ESP32                              │
│  ┌──────────────────────────────────────────────────────┐   │
│  │ GPIO16 (RX) ──────────────┐                         │   │
│  │ GPIO17 (TX) ──────────────┤── RS485 Driver Module   │   │
│  │ GPIO4 (DE/RE) ────────────┘  (Max485 ou SN75176)    │   │
│  └──────────────────────────────────────────────────────┘   │
│                         │                                    │
│                    ┌────┴─────┐                             │
│                    │ A (Non-inv) ─────────────┐              │
│                    │ B (Inv)     ─────────────┤              │
│                    └────────────┘             │              │
│                                               │              │
└───────────────────────────────────────────────┼──────────────┘
                                                │
                    ┌───────────────────────────┴───────────────┐
                    │                                           │
                 ┌──┴─────┐                               ┌──┴─────┐
                 │ Capteur │                               │ Capteur │
                 │ SHT20 #1│                               │ SHT20 #2│
                 │ Addr: 1 │                               │ Addr: 2 │
                 └────┬────┘                               └────┬────┘
                      │                                        │
                   ┌──┴──┐                                  ┌──┴──┐
                   │ A/B │  Résistance de    REMARQUE:    │ A/B │
                   │ 120Ω│  terminaison ⟵─ Une résistance │120Ω │
                   │     │  Optionnel      par extrémité   │     │
                   └─────┘                                 └─────┘
```

### Pins ESP32 utilisés :

| Pin | Rôle | Description |
|-----|------|---|
| **GPIO16** | RX (Réception) | Reçoit les données du RS485 |
| **GPIO17** | TX (Émission) | Envoie les données sur RS485 |
| **GPIO4** | DE/RE (Direction) | Contrôle le mode TX/RX du driver |

### Module RS485 (Max485/SN75176)

Le module RS485 convertit les niveaux logiques TTL (0-3.3V) en signaux différentiels RS485 (-12V à +12V) :

```
ESP32 (TTL)              RS485 Module                  Bus RS485
────────────────────────────────────────────────────────────────
       TX (3.3V)   ────►  DI (Input)   ───► A(+) ──────────────
       RX         ◄────  RO (Output)   ───► B(-) ──────────────
       DE/RE      ────►  RE/DE Control
                         (Contrôle direction)
```

**Brochage Max485 typique :**
```
Pin 1 : A (Non-inversé)      → vers Bus A
Pin 2 : B (Inversé)          → vers Bus B  
Pin 3 : GND
Pin 4 : RE (Receive Enable)  ← GPIO4 (LOW = Réception)
Pin 5 : DE (Drive Enable)    ← GPIO4 (HIGH = Émission)
Pin 6 : DI (Data Input)      ← GPIO17 (TX)
Pin 7 : RO (Receive Output)  → GPIO16 (RX)
Pin 8 : VCC (5V ou 3.3V)
```

---

## Protocole Modbus RTU

### Qu'est-ce que Modbus RTU ?

**Modbus RTU** est un protocole de communication client-serveur utilisant RS485. Les capteurs SHT20 communiquent via ce protocole.

### Structure de la Trame Modbus

Chaque trame Modbus contient :

```
┌─────────┬──────────┬──────────┬──────────┬──────────┬─────────────┐
│ Address │ Function │  Data    │   Data   │   CRC    │   CRC       │
│ (1 octet)│(1 octet)│(Variable)│          │  (LSB)   │   (MSB)     │
└─────────┴──────────┴──────────┴──────────┴─────────────┬─────────┘
│◄─────── Données d'entrée du CRC ──────────────────────►│
│◄────────────────── Trame complète ──────────────────────►│
```

### Exemple Concret : Lecture de Température

Pour lire la température du capteur à l'adresse 1, la trame est :

```
┌─────────┬──────────┬──────────┬──────────┬──────────┬──────────┐
│ 0x01    │ 0x04     │ 0x00     │ 0x01     │ 0x00     │ 0x01     │
│ (1 octet)│(1 octet)│(1 octet) │(1 octet) │(1 octet) │(1 octet) │
└─────────┴──────────┴──────────┴──────────┴──────────┴──────────┘
Adresse   Fonction   Registre MSB Registre LSB Quantité MSB Quantité LSB
          FC04       Address High Address Low  Num Regs H  Num Regs L

│◄────────────── 6 octets pour CRC ──────────────────►│
```

**Signification des champs :**

| Champ | Hex | Signification |
|-------|-----|---|
| **Adresse** | 0x01 | Capteur n°1 |
| **Fonction** | 0x04 | "Read Input Registers" (FC04) |
| **Reg MSB** | 0x00 | Registre = 0x0001 (température) |
| **Reg LSB** | 0x01 | (MSB et LSB forment l'adresse) |
| **Quantité MSB** | 0x00 | Lire 1 registre = 0x0001 |
| **Quantité LSB** | 0x01 | (MSB et LSB forment la quantité) |

### Codes Fonction Modbus Courants

| Code | Nom | Rôle |
|------|-----|------|
| **0x03** | Read Holding Registers | Lire registres d'accumulateurs |
| **0x04** | Read Input Registers | **← Utilisé pour SHT20** |
| **0x06** | Write Single Register | Écrire 1 registre |
| **0x10** | Write Multiple Registers | Écrire plusieurs registres |

### Registres SHT20

| Registre | Adresse | Données | Description |
|----------|---------|---------|---|
| **Température** | 0x0001 | 2 octets | Température en 0.1°C (signée) |
| **Humidité** | 0x0002 | 2 octets | Humidité en 0.1%RH (non-signée) |
| **Status** | 0x0003 | 2 octets | État du capteur |

---

## CRC16 Modbus

### Pourquoi le CRC ?

Le **CRC16** (Cyclic Redundancy Check) détecte les erreurs de transmission :
- Polynôme utilisé : **0xA001**
- Valeur initiale : **0xFFFF**
- Envoyé en **Little-Endian** (LSB d'abord)

### Calcul du CRC - Explication Mathématique

Le CRC16 Modbus utilise l'algorithme **LRC réfléchi** :

```
Étape 1 : Initialiser CRC = 0xFFFF
Étape 2 : Pour chaque octet des données
  - XOR le CRC avec l'octet
  - Pour chaque bit (8 bits) :
    * Si le bit LSB du CRC = 1 :
      - Décaler CRC à droite
      - XOR avec le polynôme 0xA001
    * Sinon :
      - Décaler CRC à droite
Étape 3 : Retourner le CRC
```

### Exemple Numérique

Pour les données : **[0x01, 0x04, 0x00, 0x01, 0x00, 0x01]**

```
Initialisation : CRC = 0xFFFF (1111111111111111 en binaire)

Octet 1 : 0x01 (00000001)
├─ CRC XOR 0x01 → 0xFFFE
├─ 8 itérations de décalage/XOR
└─ Résultat : 0x30FD

Octet 2 : 0x04
└─ Après traitement : 0x0CDD

... (et ainsi de suite pour tous les octets)

CRC Final : 0x41C6
Envoyé comme : [0xC6] [0x41]  (LSB d'abord, puis MSB)
```

### Implémentation Code

```cpp
uint16_t calculateModbusCRC(byte *data, int length) {
  uint16_t crc = 0xFFFF;  // ← Initialisation
  
  for (int i = 0; i < length; i++) {
    crc ^= data[i];  // ← XOR avec l'octet courant
    
    for (int bit = 0; bit < 8; bit++) {  // ← Traiter 8 bits
      if (crc & 1) {  // ← Si LSB = 1
        crc = (crc >> 1) ^ 0xA001;  // ← Décaler + XOR polynôme
      } else {
        crc = crc >> 1;  // ← Sinon juste décaler
      }
    }
  }
  
  return crc;
}
```

---

## Flux de Communication

### Séquence Complète Maître → Esclave → Maître

```
┌──────────────────────────────────────────────────────────────┐
│                    ESP32 (Maître)                            │
│              readRS485Register(addr, reg)                    │
└──────────────────────┬───────────────────────────────────────┘
                       │
        ┌──────────────┴──────────────┐
        │                             │
        ▼                             │
   ┌─────────────────────────┐        │
   │ 1. CONSTRUCTION TRAME   │        │
   │ [Addr|Func|Reg|Qty|CRC]│        │
   └──────────┬──────────────┘        │
              │                       │
        ┌─────┴──────┐                │
        │ Vider RX   │                │
        │ buffer     │                │
        └─────┬──────┘                │
              │                       │
        ┌─────▼──────────────┐        │
        │ 2. ÉMISSION        │        │
        │ GPIO4 = HIGH       │        │
        │ (Mode TX)          │        │
        │ Envoyer 8 octets   │        │
        └─────┬──────────────┘        │
              │                       │
        ┌─────▼──────────────┐        │
        │ GPIO4 = LOW        │        │
        │ (Mode RX)          │        │
        └─────┬──────────────┘        │
              │                       │
        ┌─────▼──────────────────┐    │
        │ 3. ATTENDRE RÉPONSE    │    │
        │ Timeout = 200ms        │    │
        │ Attendre 7 octets      │    │
        └─────┬──────────────────┘    │
              │                       │
        ┌─────▼──────────────┐        │
        │ 4. RÉCEPTION       │        │
        │ Lire 7 octets      │        │
        │ du buffer          │        │
        └─────┬──────────────┘        │
              │                       │
        ┌─────▼──────────────┐        │
        │ 5. PARSING         │        │
        │ Vérifier:          │        │
        │ - Adresse OK?      │        │
        │ - Fonction OK?     │        │
        │ - Extraire données │        │
        └─────┬──────────────┘        │
              │                       │
        ┌─────▼──────────────┐        │
        │ 6. CONVERSION      │        │
        │ value / 10.0       │        │
        │ Retourner résultat │        │
        └────────┬───────────┘        │
                 │                    │
                 └────────────────────┘
```

### Timeline Chronologique

```
t=0ms    : GPIO4 = HIGH (Mode TX)
t=0-5ms  : Envoi de 8 octets @ 9600 baud (≈ 10.4ms pour 8 octets)
t=5ms    : GPIO4 = LOW (Mode RX)
t=5-10ms : Délai propagation réseau
t=10-15ms: Capteur traite et prépare réponse
t=15-25ms: Capteur envoie réponse (7 octets)
t=25-30ms: ESP32 reçoit les 7 octets
t=30-50ms: Traitement et parsing
t=50ms   : Retour du résultat
```

---

## Implémentation Détaillée

### 1️⃣ Initialisation du RS485

```cpp
void setup() {
  // === Configuration série USB (pour debug) ===
  Serial.begin(115200);  // Vitesse rapide pour affichage
  
  // === Configuration RS485 ===
  // Définir GPIO4 comme sortie pour contrôler DE/RE
  pinMode(RS485_DE_RE, OUTPUT);
  digitalWrite(RS485_DE_RE, LOW);  // Mode récepteur par défaut
  
  // === Initialisation UART2 ===
  // RS485Serial = HardwareSerial(2)  ← UART2 de l'ESP32
  RS485Serial.begin(
    9600,        // Débit 9600 bauds
    SERIAL_8N1,  // 8 bits, pas de parité, 1 stop bit
    16,          // RX sur GPIO16
    17           // TX sur GPIO17
  );
  
  Wire.begin();  // I2C pour O2
}
```

**Explication des paramètres :**
- **9600 bauds** = 9600 bits/seconde
- **8N1** = 8 Data, No Parity, 1 Stop
- **GPIO16/17** = Pins UART2 de l'ESP32

### 2️⃣ Construction de la Trame

```cpp
float readRS485Register(uint8_t address, uint16_t reg) {
  byte frame[8];  // Trame Modbus FC04
  
  // === CONSTRUCTION ===
  frame[0] = address;           // [0] Adresse esclave (1, 2 ou 3)
  frame[1] = MODBUS_FUNC_READ_REG;  // [1] Fonction = 0x04
  frame[2] = reg >> 8;          // [2] Adresse registre MSB
  frame[3] = reg & 0xFF;        // [3] Adresse registre LSB
  frame[4] = 0x00;              // [4] Quantité MSB
  frame[5] = 0x01;              // [5] Quantité LSB (lire 1)
  
  // Exemple si address=1, reg=0x0001 (température) :
  // frame = [0x01, 0x04, 0x00, 0x01, 0x00, 0x01]
  //          ├─ Adresse: 1
  //          ├─ Fonction: 4 (Read Input Registers)
  //          ├─ Registre: 0x0001 (température)
  //          └─ Quantité: 1 registre
  
  // === CALCUL CRC ===
  uint16_t crc = calculateModbusCRC(frame, 6);  // CRC sur 6 octets
  frame[6] = crc & 0xFF;        // CRC LSB (envoyé en premier)
  frame[7] = crc >> 8;          // CRC MSB (envoyé en deuxième)
  
  // Résultat : [0x01, 0x04, 0x00, 0x01, 0x00, 0x01, 0xC6, 0x41]
  // Les 2 derniers octets sont le CRC calculé
```

### 3️⃣ Émission du Paquet

```cpp
void sendRS485Frame(uint8_t address, byte *frame, int length) {
  // === VIDER LE BUFFER RX ===
  while (RS485Serial.available()) {
    RS485Serial.read();  // Nettoyer les vieilles données
  }
  
  // === MODE ÉMETTEUR ===
  digitalWrite(RS485_DE_RE, HIGH);  // [DE/RE = 1] → Mode TX
                                     // Driver RS485 active A/B
  
  // === ENVOI ===
  RS485Serial.write(frame, length);  // Envoyer les 8 octets
  RS485Serial.flush();                // Attendre fin transmission
  
  // === RETOUR MODE RÉCEPTEUR ===
  digitalWrite(RS485_DE_RE, LOW);   // [DE/RE = 0] → Mode RX
                                     // Driver RS485 écoute A/B
  
  // === TIMELINE ===
  // t=0µs : GPIO4 = HIGH
  // t=0-10ms : Transmission série (8 octets @ 9600 baud)
  // t=10ms : GPIO4 = LOW
  // Le bus est maintenant en écoute
}
```

### 4️⃣ Réception et Timeout

```cpp
  // === ATTENDRE LA RÉPONSE ===
  unsigned long startTime = millis();  // Mémoriser t=0
  
  // Boucler jusqu'à avoir 7 octets OU timeout
  while (RS485Serial.available() < MODBUS_RESPONSE_SIZE && 
         millis() - startTime < RS485_TIMEOUT_MS) {
    delay(1);  // Laisser respirer le µC
  }
  
  // === VÉRIFICATION DISPONIBILITÉ ===
  if (RS485Serial.available() >= MODBUS_RESPONSE_SIZE) {
    // Assez de données disponibles
    byte response[MODBUS_RESPONSE_SIZE];  // Array de 7 octets
    RS485Serial.readBytes(response, MODBUS_RESPONSE_SIZE);
    
    // === TIMELINE DE RÉCEPTION ===
    // t=0ms : Attendre réponse
    // t=15-25ms : Capteur envoie 7 octets
    //             [0x01][0x04][0x02][data_h][data_l][crc_l][crc_h]
    // t=25-30ms : ESP32 reçoit tout
    // t=30-50ms : Traitement
```

### 5️⃣ Parsing de la Réponse

```cpp
    // === STRUCTURE DE LA RÉPONSE ===
    // [0] = Adresse echo
    // [1] = Fonction echo (0x04)
    // [2] = Nombre d'octets de données (0x02 pour 1 registre)
    // [3] = Donnée MSB
    // [4] = Donnée LSB
    // [5] = CRC LSB
    // [6] = CRC MSB
    
    // === VALIDATION ===
    if (response[0] != address || response[1] != MODBUS_FUNC_READ_REG) {
      return NAN;  // Adresse ou fonction incorrecte
    }
    
    // === EXTRACTION DE LA VALEUR ===
    // Combiner les 2 octets en format Big-Endian (MSB d'abord)
    int16_t rawValue = (response[3] << 8) | response[4];
    
    // Exemple réel :
    // Si température = 23.5°C
    // Valeur codée = 235 (en 0.1°C)
    // response[3] = 0x00
    // response[4] = 0xEB (235 en hexa)
    // rawValue = (0x00 << 8) | 0xEB = 0x00EB = 235
    
    // === CONVERSION EN VALEUR PHYSIQUE ===
    return rawValue / 10.0;  // 235 / 10.0 = 23.5°C
  }
  
  return NAN;  // Timeout : aucune réponse
}
```

---

## Exemple Complet avec Diagrammes

### Scénario : Lire Température du Capteur 1

```
┌─────────────────────────────────────────────────────────────┐
│                                                             │
│  ESP32                          Capteur SHT20 (Addr 1)    │
│  ─────                          ──────────────────────────  │
│                                                             │
│  [Appel fonction]                                           │
│  readSHT20Temperature(1);                                   │
│    ↓                                                        │
│  readRS485Register(1, 0x0001)                              │
│    ↓                                                        │
│  ╔════════════════════════════════════════════════════╗    │
│  ║ ÉTAPE 1 : CONSTRUCTION TRAME                      ║    │
│  ║ frame[8] = [0x01, 0x04, 0x00, 0x01, 0x00, 0x01,  ║    │
│  ║             0xC6, 0x41]                           ║    │
│  ║                                                    ║    │
│  ║ Décodage :                                        ║    │
│  ║   Adresse    = 0x01 (Capteur 1)                  ║    │
│  ║   Fonction   = 0x04 (Read Input Registers)       ║    │
│  ║   Registre   = 0x0001 (Température)              ║    │
│  ║   Quantité   = 0x0001 (Lire 1 registre)          ║    │
│  ║   CRC        = 0x41C6 (Contrôle d'intégrité)     ║    │
│  ╚════════════════════════════════════════════════════╝    │
│    ↓                                                        │
│  ╔════════════════════════════════════════════════════╗    │
│  ║ ÉTAPE 2 : ÉMISSION (Mode TX)                      ║    │
│  ║ GPIO4 = HIGH → Driver RS485 en TX                ║    │
│  ║                                                    ║    │
│  ║ Temps d'émission :                               ║    │
│  ║ 8 octets × (10 bits/octet) / 9600 bps ≈ 8.3ms  ║    │
│  ╚════════════════════════════════════════════════════╝    │
│    ↓                                                    ↓   │
│    ↓ [RS485 Bus]                                     ↓   │
│    └─────────────────────────────────────────────────┘   │
│         Signal différentiel A-B (±12V)                    │
│                                                       │    │
│         Capteur reçoit et décode :                   │    │
│         ✓ Vérifie adresse = 1                        │    │
│         ✓ Vérifie fonction = 0x04                    │    │
│         ✓ Valide CRC                                 │    │
│         ✓ Accède registre 0x0001 (Température)      │    │
│    ↓                                                   ↓   │
│    ↓ [GPIO4 = LOW → Mode RX]                         ↓   │
│  ╔════════════════════════════════════════════════════╗   │
│  ║ ÉTAPE 3 : CAPTEUR RÉPOND                          ║   │
│  ║                                                    ║   │
│  ║ Préparation réponse (temps de traitement)         ║   │
│  ║ Si T = 23.5°C → valeur = 235 (0xEB)             ║   │
│  ║                                                    ║   │
│  ║ Trame réponse : [0x01, 0x04, 0x02, 0x00, 0xEB,  ║   │
│  ║                  0x.., 0x..]                      ║   │
│  ║                 (CRC calculé automatiquement)     ║   │
│  ╚════════════════════════════════════════════════════╝   │
│    ↑                                                   │   │
│    │ [RS485 Bus]                                     │   │
│    └──────────────────────────────────────────────────┘   │
│         Signal différentiel A-B (±12V)                    │
│                                                            │
│  ╔════════════════════════════════════════════════════╗   │
│  ║ ÉTAPE 4 : RÉCEPTION RÉPONSE                       ║   │
│  ║                                                    ║   │
│  ║ Temps total d'attente : ~30-40ms                  ║   │
│  ║   - Émission : ~8.3ms                             ║   │
│  ║   - Propagation : ~1ms                            ║   │
│  ║   - Traitement capteur : ~10-20ms                 ║   │
│  ║   - Réponse : ~8.3ms                              ║   │
│  ║   - Propagation retour : ~1ms                     ║   │
│  ║   - Réception : ~5ms                              ║   │
│  ╚════════════════════════════════════════════════════╝   │
│    ↓                                                        │
│  ╔════════════════════════════════════════════════════╗    │
│  ║ ÉTAPE 5 : PARSING RÉPONSE                         ║    │
│  ║                                                    ║    │
│  ║ response[0] = 0x01 ✓ (Adresse correcte)          ║    │
│  ║ response[1] = 0x04 ✓ (Fonction correcte)         ║    │
│  ║ response[2] = 0x02 (2 octets de données)         ║    │
│  ║ response[3] = 0x00 ┐                             ║    │
│  ║ response[4] = 0xEB ├─ Valeur = 0x00EB = 235    ║    │
│  ║ response[5] = CRC L                              ║    │
│  ║ response[6] = CRC H                              ║    │
│  ║                                                    ║    │
│  ║ Conversion : 235 / 10.0 = 23.5°C                ║    │
│  ╚════════════════════════════════════════════════════╝    │
│    ↓                                                        │
│  return 23.5;                                              │
│                                                            │
└─────────────────────────────────────────────────────────────┘
```

### Trace Hexadécimale Complète

```
╔═════════════════════════════════════════════════════════════╗
║                    TRAME ENVOYÉE                            ║
╠═════════╦═════════╦═════════╦═════════╦═════════╦═════════╗
║ Index   ║   0    ║    1    ║    2    ║    3    ║    4    ║
║ Valeur  ║  0x01  ║  0x04   ║  0x00   ║  0x01   ║  0x00   ║
║ Binaire ║00000001║00000100 ║00000000 ║00000001 ║00000000 ║
║ Sens    ║Adresse ║Fonction ║ Reg MSB ║Reg LSB  ║Qty MSB  ║
╠═════════╬═════════╬═════════╬═════════╬═════════╬═════════╣
║ Index   ║   5    ║    6    ║    7    ║         ║         ║
║ Valeur  ║  0x01  ║  0xC6   ║  0x41   ║         ║         ║
║ Binaire ║00000001║11000110 ║01000001 ║         ║         ║
║ Sens    ║Qty LSB ║CRC LSB  ║CRC MSB  ║         ║         ║
╚═════════╩═════════╩═════════╩═════════╩═════════╩═════════╝

CRC Détail :
  Données : [0x01, 0x04, 0x00, 0x01, 0x00, 0x01]
  CRC Calcul : 0x41C6
  Transmission : LSB d'abord = [0xC6, 0x41]

╔═════════════════════════════════════════════════════════════╗
║                    TRAME REÇUE                              ║
╠═════════╦═════════╦═════════╦═════════╦═════════╦═════════╗
║ Index   ║   0    ║    1    ║    2    ║    3    ║    4    ║
║ Valeur  ║  0x01  ║  0x04   ║  0x02   ║  0x00   ║  0xEB   ║
║ Binaire ║00000001║00000100 ║00000010 ║00000000 ║11101011 ║
║ Sens    ║Adresse ║Fonction ║Nb Octets║Data MSB ║Data LSB ║
╠═════════╬═════════╬═════════╬═════════╬═════════╬═════════╣
║ Index   ║   5    ║    6    ║         ║         ║         ║
║ Valeur  ║  0x.?  ║  0x.?   ║         ║         ║         ║
║ Binaire ║????????║???????? ║         ║         ║         ║
║ Sens    ║CRC LSB ║CRC MSB  ║         ║         ║         ║
╚═════════╩═════════╩═════════╩═════════╩═════════╩═════════╝

Extraction de la valeur :
  response[3] = 0x00 (byte MSB) = 00000000
  response[4] = 0xEB (byte LSB) = 11101011
  
  Combinaison : (0x00 << 8) | 0xEB
               = 0x00EB
               = 235 décimal
  
  Conversion : 235 / 10.0 = 23.5°C

  👆 C'est la valeur finale retournée !
```

---

## Résumé Visuel du Cycle Complet

```
                          CYCLE D'ACQUISITION
    
    ┌─────────────┐
    │    SETUP    │ Initialise GPIO4, UART2, I2C
    └────┬────────┘
         │
    ┌────▼──────────────┐
    │  BOUCLE PRINCIPALE │
    └────┬───────────────┘
         │
    ┌────▼──────────────────────────────────┐
    │ for (i = 0 to SENSOR_COUNT)            │
    │   address = SENSOR_ADDRESSES[i]        │
    └────┬───────────────────────────────────┘
         │
    ┌────▼────────────────────┐
    │ readSHT20Temperature()   │
    └────┬────────────────────┘
         │
    ┌────▼─────────────────────────┐
    │ readRS485Register()           │ ← FONCTION PRINCIPALE
    │ ┌────────────────────────────┐│
    │ │ 1. Construction trame      ││
    │ │ 2. Calcul CRC16            ││
    │ │ 3. Émission (Mode TX)      ││
    │ │ 4. Attente réponse         ││
    │ │ 5. Réception (Mode RX)     ││
    │ │ 6. Parsing réponse         ││
    │ │ 7. Conversion en °C        ││
    │ │ 8. Retour résultat         ││
    │ └────────────────────────────┘│
    └────┬─────────────────────────┘
         │
    ┌────▼────────────────────┐
    │ readSHT20Humidity()      │ ← Même fonction
    │ (registre 0x0002)        │
    └────┬────────────────────┘
         │
    ┌────▼──────────────────────┐
    │ Affichage résultats       │
    │ Serial.println(temp, °C)  │
    │ Serial.println(hum, %RH)  │
    └────┬─────────────────────┘
         │
    ┌────▼──────────────────────┐
    │ readOxygenLevel() (I2C)   │
    └────┬─────────────────────┘
         │
    ┌────▼──────────────────────┐
    │ delay(2000ms)             │
    │ Retour BOUCLE             │
    └──────────────────────────┘
```

---

## Points Clés à Retenir

### ✅ Points Importants

1. **RS485 vs UART** : RS485 permet plusieurs appareils, UART ne permet que 1
2. **GPIO4 (DE/RE)** : Contrôle la direction (TX = HIGH, RX = LOW)
3. **Timeout** : 200ms max pour recevoir une réponse
4. **CRC16** : Détecte les erreurs, polynôme = 0xA001
5. **Modbus RTU** : Protocole client-serveur avec adressage
6. **Big-Endian** : Les données sont en format MSB d'abord
7. **Vitesse** : 9600 baud = ~8.3ms pour 8 octets

### ⚠️ Points d'Erreurs Courants

| Erreur | Cause | Solution |
|--------|-------|----------|
| **Pas de réponse** | GPIO4 ne change pas de direction | Vérifier broche GPIO4 |
| **CRC invalide** | Calcul de CRC incorrect | Vérifier polynôme 0xA001 |
| **Réception partielle** | Buffer trop petit | Augmenter MODBUS_RESPONSE_SIZE |
| **Adresse mauvaise** | Capteur à mauvaise adresse | Vérifier avec commande AT |
| **Données corrompues** | Problème électrique RS485 | Vérifier câblage, résistances |

---

## Ressources Additionnelles

### Documents de Référence

- **Modbus Application Protocol V1.1b3** : Spécification complète
- **SHT20 Datasheet** : Registres et protocole
- **Max485 Datasheet** : Timing et électrique
- **RS485 Tutorials** : DifferentialSignaling.com

### Outils de Test

```cpp
// Debug Helper - Afficher une trame en hexadécimal
void printFrame(byte *frame, int length) {
  for (int i = 0; i < length; i++) {
    Serial.print("0x");
    if (frame[i] < 0x10) Serial.print("0");
    Serial.print(frame[i], HEX);
    Serial.print(" ");
  }
  Serial.println();
}

// Utilisation
byte myFrame[8] = {0x01, 0x04, 0x00, 0x01, 0x00, 0x01, 0xC6, 0x41};
printFrame(myFrame, 8);  // Affiche : 0x01 0x04 0x00 0x01 0x00 0x01 0xC6 0x41
```

---

## Conclusion

La communication RS485 Modbus RTU est un **système robuste et industriel** permettant la lecture simultanée de plusieurs capteurs. Le flux est déterministe :

1. **Émission** → Construction + Envoi trame
2. **Propagation** → Signal sur le bus RS485
3. **Traitement** → Capteur décode et prépare réponse
4. **Réception** → Écoute réponse avec timeout
5. **Parsing** → Extraction et validation données
6. **Conversion** → Transformation en valeur physique

Chaque étape est critique pour une communication fiable ! 🎯

