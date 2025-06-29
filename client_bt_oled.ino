#include <Wire.h>
#include <BluetoothSerial.h> // librairie bluetooth
#include <Adafruit_GFX.h> 
#include <Adafruit_SSD1306.h> 

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1

BluetoothSerial SerialBT;
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

const char* scannerName = "ESP32-Scanner";

// Déclaration des fonctions avant setup()
void afficherTexteCentre(String texte);
void afficherInfosRFID(String message);

void setup() {
    Serial.begin(115200);
    
    // Initialisation I2C et écran OLED
    Wire.begin(21, 22); // SDA=21, SCL=22
    
    if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
        Serial.println("Erreur initialisation écran OLED");
        for(;;); // Arrêt si écran non détecté
    }
    
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    
    // Initialisation Bluetooth
    SerialBT.begin("ESP32-OLED-Client", true);
    
    afficherTexteCentre("Connexion...");
    
    // Connexion au serveur RFID
    bool connected = SerialBT.connect(scannerName);
    if (connected) {
        Serial.println("Connexion réussie !");
        afficherTexteCentre("Connecte");
        delay(1000);
        afficherTexteCentre("Pret");
    } else {
        Serial.println("Echec connexion");
        afficherTexteCentre("Echec");
    }
}

void loop() {
    static String messageBuffer = "";
    static unsigned long lastReceiveTime = 0;
    
    if (SerialBT.available()) {
        while (SerialBT.available()) {
            char c = SerialBT.read();
            messageBuffer += c;
            lastReceiveTime = millis();
        }
    }
    
    // Si on a reçu des données et qu'il n'y a plus rien depuis 100ms, traiter le message
    if (messageBuffer.length() > 0 && (millis() - lastReceiveTime > 100)) {
        messageBuffer.trim();
        Serial.println("=== MESSAGE COMPLET ===");
        Serial.println(messageBuffer);
        Serial.println("=== FIN MESSAGE ===");
        afficherInfosRFID(messageBuffer);
        messageBuffer = ""; // Vider le buffer
    }
    
    static bool wasConnected = false;
    bool isConnected = SerialBT.connected();
    
    if (!isConnected && wasConnected) {
        Serial.println("Connexion perdue !");
        afficherTexteCentre("Reconnexion...");
        wasConnected = false;
        // Tentative de reconnexion automatique
        while (!SerialBT.connected()) {
            Serial.println("Tentative de reconnexion...");
            afficherTexteCentre("Tentative reconnexion");
            if (SerialBT.connect(scannerName)) {
                Serial.println("Reconnexion réussie !");
                afficherTexteCentre("Connecte");
                delay(1000);
                afficherTexteCentre("Pret");
                wasConnected = true;
                break;
            }
            delay(3000); // Attendre 3 secondes avant nouvelle tentative
        }
    } else if (isConnected && !wasConnected) {
        Serial.println("Connexion établie !");
        afficherTexteCentre("Connecte");
        delay(1000);
        afficherTexteCentre("Pret");
        wasConnected = true;
    }
    
    delay(50);
}

void afficherInfosRFID(String message) {
    display.clearDisplay();
    display.setCursor(0, 0);
    display.setTextSize(1);
    
    // Titre principal
    display.println("Vetement detecte :");
    display.println();
    
    // Variables pour extraire les informations
    String uid = "";
    String couleur = "";
    String materiau = "";
    
    // Debug : afficher chaque ligne
    Serial.println("=== ANALYSE LIGNES ===");
    
    // Parser le message ligne par ligne
    int startPos = 0;
    int lineNumber = 0;
    
    while (startPos < message.length()) {
        int endPos = message.indexOf('\n', startPos);
        if (endPos == -1) endPos = message.length();
        
        String line = message.substring(startPos, endPos);
        line.trim();
        
        Serial.println("Ligne " + String(lineNumber) + " : [" + line + "]");
        
        if (line.length() > 0) {
            if (lineNumber == 0) {
                // Première ligne : "UID : XXXXXXXX"
                if (line.startsWith("UID :") || line.startsWith("UID:")) {
                    int colonPos = line.indexOf(':');
                    if (colonPos != -1) {
                        uid = line.substring(colonPos + 1);
                        uid.trim();
                    }
                }
            } else if (lineNumber == 1) {
                // Deuxième ligne : couleur
                if (line != "Tag inconnu" && line != "Erreur lecture" && line.length() > 0) {
                    couleur = line;
                }
            } else if (lineNumber == 2) {
                // Troisième ligne : matériaux
                if (line != "Tag inconnu" && line != "Erreur lecture" && line.length() > 0) {
                    materiau = line;
                }
            }
            lineNumber++;
        }
        
        startPos = endPos + 1;
    }
    
    Serial.println("UID final : [" + uid + "]");
    Serial.println("Couleur finale : [" + couleur + "]");
    Serial.println("Materiau final : [" + materiau + "]");
    
    // Affichage formaté
    int y = 20;
    
    // Afficher la couleur
    if (couleur.length() > 0) {
        display.setCursor(0, y);
        display.println("Couleur :");
        y += 10;
        display.setCursor(0, y);
        if (couleur.length() > 21) {
            couleur = couleur.substring(0, 18) + "...";
        }
        display.println(couleur);
        y += 15;
    } else {
        display.setCursor(0, y);
        display.println("Couleur : N/A");
        y += 15;
    }
    
    // Afficher le matériau
    if (materiau.length() > 0) {
        display.setCursor(0, y);
        display.println("Materiaux :");
        y += 10;
        display.setCursor(0, y);
        if (materiau.length() > 21) {
            materiau = materiau.substring(0, 18) + "...";
        }
        display.println(materiau);
    } else {
        display.setCursor(0, y);
        display.println("Materiaux : N/A");
    }
    
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
    display.display();
}
