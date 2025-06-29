#include <SPI.h>
#include <MFRC522.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <BluetoothSerial.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1

// Pins SPI pour les 3 lecteurs RFID
#define RST_PIN 0
#define SS_PIN_1 5   // CS Lecteur 1
#define SS_PIN_2 4   // CS Lecteur 2
#define SS_PIN_3 2   // CS Lecteur 3

// Création des instances MFRC522
MFRC522 rfid1(SS_PIN_1, RST_PIN);
MFRC522 rfid2(SS_PIN_2, RST_PIN);
MFRC522 rfid3(SS_PIN_3, RST_PIN);

// Clé d'authentification
MFRC522::MIFARE_Key key;

// Écran OLED
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
 
// Bluetooth
BluetoothSerial SerialBT;

// Variables pour le timeout de détection
unsigned long lastDetectionTime = 0;
const unsigned long TIMEOUT_DETECTION = 7000; // 7 secondes en millisecondes
bool timeoutSignalSent = false;

void setup() {
    Serial.begin(115200);
    delay(1000);

    // Initialisation I2C pour écran OLED (GPIO 16/17)
    Wire.begin(16, 17);

    // Initialisation écran OLED
    if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
        if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3D)) {
            Serial.println("Erreur: Écran OLED non détecté");
            for(;;);
        }
    }

    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    afficherTexteCentre("Initialisation...");
    delay(2000);

    // Initialisation SPI
    SPI.begin();

    // Initialisation des 3 lecteurs RFID
    rfid1.PCD_Init();
    rfid2.PCD_Init();
    rfid3.PCD_Init();

    // Configuration du gain d'antenne (pour capter le plus loin possible)
    rfid1.PCD_SetAntennaGain(rfid1.RxGain_max);
    rfid2.PCD_SetAntennaGain(rfid2.RxGain_max);
    rfid3.PCD_SetAntennaGain(rfid3.RxGain_max);

    // Clé par défaut
    for (byte i = 0; i < 6; i++) key.keyByte[i] = 0xFF;

    // Initialisation Bluetooth
    SerialBT.begin("ESP32-RFID-Server", false);
    delay(1000);

    // Initialiser le temps de dernière détection
    lastDetectionTime = millis();

    Serial.println("=== 3 LECTEURS RFID + TIMEOUT + BLUETOOTH PRÊTS ===");
    Serial.println("Lecteur 1 : GPIO 5");
    Serial.println("Lecteur 2 : GPIO 4");
    Serial.println("Lecteur 3 : GPIO 2");
    Serial.println("Timeout : 7 secondes -> Signal 0");
    Serial.println("Bluetooth: ESP32-RFID-Server");
    
    afficherTexteCentre("Pret");
}

void loop() {
    // Traitement des messages Bluetooth
    if (SerialBT.available()) {
        String message = "";
        while (SerialBT.available()) {
            char c = SerialBT.read();
            message += c;
            delay(1);
        }
        message.trim();
        if (message == "MOTEUR_CONNECT") {
            Serial.println("Moteur connecté !");
            SerialBT.println("MOTEUR_OK");
        }
    }

    // Variable pour détecter si un tag a été trouvé dans cette boucle
    bool tagDetected = false;

    // Vérifier le lecteur 1
    if (rfid1.PICC_IsNewCardPresent() && rfid1.PICC_ReadCardSerial()) {
        handleTag(rfid1, "Lecteur 1");
        tagDetected = true;
    }
    
    // Vérifier le lecteur 2
    if (!tagDetected && rfid2.PICC_IsNewCardPresent() && rfid2.PICC_ReadCardSerial()) {
        handleTag(rfid2, "Lecteur 2");
        tagDetected = true;
    }
    
    // Vérifier le lecteur 3
    if (!tagDetected && rfid3.PICC_IsNewCardPresent() && rfid3.PICC_ReadCardSerial()) {
        handleTag(rfid3, "Lecteur 3");
        tagDetected = true;
    }

    // Si un tag a été détecté, mettre à jour le temps
    if (tagDetected) {
        lastDetectionTime = millis();
        timeoutSignalSent = false;
    }

    // Vérifier le timeout (7 secondes sans détection)
    checkTimeoutAndSendSignal();
    
    delay(100);
}

void checkTimeoutAndSendSignal() {
    unsigned long currentTime = millis();
    
    // Vérifier si 7 secondes se sont écoulées sans détection
    if ((currentTime - lastDetectionTime >= TIMEOUT_DETECTION) && !timeoutSignalSent) {
        Serial.println("=== TIMEOUT 7 SECONDES ===");
        Serial.println("Aucune détection -> Envoi signal 0");
        
        // Envoyer signal 0 via Bluetooth
        if (SerialBT.hasClient()) {
            SerialBT.println("SIGNAL:0");
            Serial.println("Signal 0 envoyé via Bluetooth (timeout)");
        } else {
            Serial.println("Aucun client Bluetooth - Signal 0 non envoyé");
        }
        
        // Affichage sur écran
        afficherTimeout();
        
        // Marquer que le signal a été envoyé pour éviter les envois répétés
        timeoutSignalSent = true;
        
        // Attendre 2 secondes puis retour à "Pret"
        delay(2000);
        afficherTexteCentre("Pret");
    }
}

void afficherTimeout() {
    display.clearDisplay();
    display.setCursor(0, 0);
    display.setTextSize(1);
    
    display.println("TIMEOUT DETECTION");
    display.println();
    display.println("Aucun vetement");
    display.println("detecte depuis");
    display.println("7 secondes");
    display.println();
    display.println("Signal 0 envoye");
    
    // Affichage de l'état Bluetooth en bas à droite
    display.setTextSize(1);
    String btStatus = SerialBT.hasClient() ? "BT:OK" : "BT:--";
    int16_t x1, y1;
    uint16_t w, h;
    display.getTextBounds(btStatus, 0, 0, &x1, &y1, &w, &h);
    int x = SCREEN_WIDTH - w - 2;
    display.setCursor(x, 56);
    display.println(btStatus);
    
    display.display();
}

void handleTag(MFRC522 &rfid, String lecteur) {
    String uid = getUIDString(rfid);
    Serial.println(lecteur + " - UID : " + uid);

    // Vérifier le type de tag
    MFRC522::PICC_Type piccType = rfid.PICC_GetType(rfid.uid.sak);

    String couleur = "";
    String materiaux = "";

    if (piccType == MFRC522::PICC_TYPE_ISO_14443_4 ||
        piccType == MFRC522::PICC_TYPE_MIFARE_UL) {
        // Tag NTAG213
        couleur = lirePage(rfid, 4) + lirePage(rfid, 5);
        materiaux = lirePage(rfid, 6) + lirePage(rfid, 7);
    } else {
        // Tag MIFARE Classic
        couleur = lireBlocClassic(rfid, 5);
        materiaux = lireBlocClassic(rfid, 6);
    }

    couleur.trim();
    materiaux.trim();
    couleur.toUpperCase();

    // Affichage sur écran OLED
    afficherInfosRFID(lecteur, uid, couleur, materiaux);

    // Envoi du signal Bluetooth selon la couleur
    if (SerialBT.hasClient()) {
        int signalMoteur = 0;
        
        if (couleur == "BLANC" || couleur == "WHITE" || couleur == "BLANCHE") {
            signalMoteur = 1;
            Serial.println(couleur + " détectée -> Signal Bluetooth: 1");
        } else if (couleur.length() > 0) {
            signalMoteur = 2;
            Serial.println(couleur + " détectée -> Signal Bluetooth: 2");
        }
        
        if (signalMoteur > 0) {
            SerialBT.println("SIGNAL:" + String(signalMoteur));
            Serial.println("Signal envoyé via Bluetooth : " + String(signalMoteur));
        }
    }

    rfid.PICC_HaltA();
    rfid.PCD_StopCrypto1();
    delay(3000); // Afficher pendant 3 secondes
    
    // Retour à l'écran "Pret"
    afficherTexteCentre("Pret");
}

void afficherInfosRFID(String lecteur, String uid, String couleur, String materiau) {
    display.clearDisplay();
    display.setCursor(0, 0);
    display.setTextSize(1);
    
    // Titre avec numéro de lecteur
    display.println("Vetement detecte :");
    display.println(lecteur);
    display.println();
    
    // Affichage de la couleur
    if (couleur.length() > 0) {
        display.println("Couleur :");
        display.setTextSize(1);
        if (couleur.length() > 21) {
            couleur = couleur.substring(0, 18) + "...";
        }
        display.println(couleur);
        display.println();
    } else {
        display.println("Couleur : N/A");
        display.println();
    }
    
    // Affichage du matériau
    if (materiau.length() > 0) {
        display.println("Materiaux :");
        if (materiau.length() > 21) {
            materiau = materiau.substring(0, 18) + "...";
        }
        display.println(materiau);
    } else {
        display.println("Materiaux : N/A");
    }
    
    // Affichage de l'état Bluetooth en bas à droite
    display.setTextSize(1);
    String btStatus = SerialBT.hasClient() ? "BT:OK" : "BT:--";
    int16_t x1, y1;
    uint16_t w, h;
    display.getTextBounds(btStatus, 0, 0, &x1, &y1, &w, &h);
    int x = SCREEN_WIDTH - w - 2;
    display.setCursor(x, 56);
    display.println(btStatus);
    
    display.display();
}

void afficherTexteCentre(String texte) {
    display.clearDisplay();
    display.setTextSize(1);
    
    // Calculer la position pour centrer le texte
    int16_t x1, y1;
    uint16_t w, h;
    display.getTextBounds(texte, 0, 0, &x1, &y1, &w, &h);
    
    int x = (SCREEN_WIDTH - w) / 2;
    int y = (SCREEN_HEIGHT - h) / 2;
    
    display.setCursor(x, y);
    display.println(texte);
    
    // Affichage de l'état Bluetooth en bas à droite
    display.setTextSize(1);
    String btStatus = SerialBT.hasClient() ? "BT:OK" : "BT:--";
    display.getTextBounds(btStatus, 0, 0, &x1, &y1, &w, &h);
    int btX = SCREEN_WIDTH - w - 2;
    display.setCursor(btX, 56);
    display.println(btStatus);
    
    display.display();
}

String getUIDString(MFRC522 &rfid) {
    String uidStr;
    for (byte i = 0; i < rfid.uid.size; i++) {
        uidStr += (rfid.uid.uidByte[i] < 0x10 ? "0" : "");
        uidStr += String(rfid.uid.uidByte[i], HEX);
        uidStr += " ";
    }
    uidStr.trim();
    uidStr.toUpperCase();
    return uidStr;
}

String lirePage(MFRC522 &rfid, byte page) {
    byte buffer[18];
    byte size = sizeof(buffer);

    if (rfid.MIFARE_Read(page, buffer, &size) == MFRC522::STATUS_OK) {
        String data = "";
        for (byte i = 0; i < 4; i++) {
            if (buffer[i] >= 32 && buffer[i] <= 126) {
                data += (char)buffer[i];
            }
        }
        return data;
    }
    return "";
}

String lireBlocClassic(MFRC522 &rfid, byte blockAddr) {
    MFRC522::MIFARE_Key key;
    for (byte i = 0; i < 6; i++) key.keyByte[i] = 0xFF;

    byte buffer[18];
    byte size = sizeof(buffer);

    if (rfid.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, blockAddr, &key, &(rfid.uid)) != MFRC522::STATUS_OK) {
        return "";
    }

    if (rfid.MIFARE_Read(blockAddr, buffer, &size) != MFRC522::STATUS_OK) {
        return "";
    }

    String data = "";
    for (byte i = 0; i < 16; i++) {
        if (buffer[i] >= 32 && buffer[i] <= 126) {
            data += (char)buffer[i];
        }
    }
    data.trim();
    return data;
}
