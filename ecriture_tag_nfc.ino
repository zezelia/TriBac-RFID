#include <Wire.h>
#include <MFRC522_I2C.h>

#define MFRC522_I2C_ADDRESS 0x28
MFRC522_I2C mfrc522(MFRC522_I2C_ADDRESS, 0xFF, &Wire);

const int I2C_SDA = 21;
const int I2C_SCL = 22;

void setup() {
    Serial.begin(115200);
    Wire.begin(I2C_SDA, I2C_SCL);
    mfrc522.PCD_Init();
    mfrc522.PCD_SetAntennaGain(mfrc522.RxGain_max);

    Serial.println("=== ÉCRITURE NTAG213 ===");
    Serial.println("Approcher le tag NTAG213");
}

void loop() {
    if (!mfrc522.PICC_IsNewCardPresent() || !mfrc522.PICC_ReadCardSerial()) {
        delay(500);
        return;
    }

    Serial.print("UID : ");
    for (byte i = 0; i < mfrc522.uid.size; i++) {
        Serial.print(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " ");
        Serial.print(mfrc522.uid.uidByte[i], HEX);
    }
    Serial.println();

    // Vérifier le type de tag
    byte piccType = mfrc522.PICC_GetType(mfrc522.uid.sak);
    Serial.print("Type : ");
    Serial.println(mfrc522.PICC_GetTypeName(piccType));

    if (piccType == MFRC522_I2C::PICC_TYPE_ISO_14443_4 || 
        piccType == MFRC522_I2C::PICC_TYPE_MIFARE_UL) {
        
        Serial.println("Tag NTAG détecté");
        
        // Données à écrire (4 octets par page pour NTAG) à modifier en fonction des données souhaitées sur le tag
        byte page4[4] = {'C', 'O', 'U', 'L'};  // Page 4 : Couleur
        byte page5[4] = {'E', 'U', 'R', ' '};  // Suite couleur
        byte page6[4] = {'C', 'O', 'T', 'O'};  // Page 6 : Matériau  
        byte page7[4] = {'N', ' ', ' ', ' '};  // Suite matériau

        // Écriture sur les pages 4, 5, 6, 7 (zone utilisateur NTAG213)
        if (ecrirePage(4, page4) && ecrirePage(5, page5) && 
            ecrirePage(6, page6) && ecrirePage(7, page7)) {
            Serial.println("Écriture réussie sur NTAG213 !");
            
            // Vérification en lisant
            lirePage(4);
            lirePage(5);
            lirePage(6);
            lirePage(7);
        } else {
            Serial.println("Écriture échouée");
        }
    } else {
        Serial.println("Ce tag n'est pas un NTAG");
    }

    mfrc522.PICC_HaltA();
    delay(3000);
}

bool ecrirePage(byte page, byte* data) {
    // Pour NTAG, utiliser MIFARE_Ultralight_Write
    if (mfrc522.MIFARE_Ultralight_Write(page, data, 4) == MFRC522_I2C::STATUS_OK) {
        Serial.print("Page ");
        Serial.print(page);
        Serial.println(" écrite");
        return true;
    } else {
        Serial.print("Échec page ");
        Serial.println(page);
        return false;
    }
}

void lirePage(byte page) {
    byte buffer[18];
    byte size = sizeof(buffer);
    
    if (mfrc522.MIFARE_Read(page, buffer, &size) == MFRC522_I2C::STATUS_OK) {
        Serial.print("Page ");
        Serial.print(page);
        Serial.print(" : ");
        for (byte i = 0; i < 4; i++) {
            if (buffer[i] >= 32 && buffer[i] <= 126) {
                Serial.print((char)buffer[i]);
            } else {
                Serial.print(".");
            }
        }
        Serial.println();
    }
}
