#include <SPI.h>
#include <MFRC522.h>
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>
#include <Wire.h>
#include <LiquidCrystal_PCF8574.h>

ESP8266WiFiMulti WiFiMulti;

#define SS_PIN 4
#define RST_PIN 5

LiquidCrystal_PCF8574 lcd(0x27);  

MFRC522 rfid(SS_PIN, RST_PIN); 
MFRC522::MIFARE_Key key;

byte nuidPICC[4];
unsigned char i;
int show;

void setup() {
  int error;
  Serial.begin(9600);
  // Buzzer Tanımlama
  pinMode (16, OUTPUT) ;

  //Wifi
  for (uint8_t t = 4; t > 0; t--) {
    Serial.printf("[SETUP] WAIT %d...\n", t);
    Serial.flush();
    delay(1000);
  }
  WiFi.mode(WIFI_STA);
  WiFiMulti.addAP("SSID", "PASS");


  SPI.begin(); 
  rfid.PCD_Init(); 

  for (byte i = 0; i < 6; i++) {
    key.keyByte[i] = 0xFF;
  }

  Wire.begin(2, 0);
  Wire.beginTransmission(0x27); 
  error = Wire.endTransmission();
  Serial.print("Error: ");
  Serial.print(error);

  if (error == 0) {
    Serial.println(": LCD found.");

  } else {
    Serial.println(": LCD not found.");
  } // if

  lcd.begin(16, 2); 
  show = 0;
  lcd.setBacklight(255);
  lcd.home();
  lcd.clear();
  Serial.println(F("This code scan the MIFARE Classsic NUID."));
  Serial.print(F("Using the following key:"));
  printHex(key.keyByte, MFRC522::MF_KEY_SIZE);
}

void loop() {

  // Reset the loop if no new card present on the sensor/reader. This saves the entire process when idle.
  if ( ! rfid.PICC_IsNewCardPresent())
  {
    lcd.setCursor(0, 0);
    lcd.print(" KART  OKUTUNUZ");
    lcd.setCursor(0, 1);
    lcd.print("     WEBTECH    ");
    return;
  }


  // Verify if the NUID has been readed
  if ( ! rfid.PICC_ReadCardSerial())
    return;

  Serial.print(F("PICC type: "));
  MFRC522::PICC_Type piccType = rfid.PICC_GetType(rfid.uid.sak);
  Serial.println(rfid.PICC_GetTypeName(piccType));

  // Check is the PICC of Classic MIFARE type
  if (piccType != MFRC522::PICC_TYPE_MIFARE_MINI &&
      piccType != MFRC522::PICC_TYPE_MIFARE_1K &&
      piccType != MFRC522::PICC_TYPE_MIFARE_4K) {
    Serial.println(F("Your tag is not of type MIFARE Classic."));
    return;
  }

  if (rfid.uid.uidByte[0] != nuidPICC[0] ||
      rfid.uid.uidByte[1] != nuidPICC[1] ||
      rfid.uid.uidByte[2] != nuidPICC[2] ||
      rfid.uid.uidByte[3] != nuidPICC[3] ) {
    Serial.println(F("A new card has been detected."));

    // Store NUID into nuidPICC array
    for (byte i = 0; i < 4; i++) {
      nuidPICC[i] = rfid.uid.uidByte[i];
    }

    Serial.println(F("The NUID tag is:"));
    Serial.print(F("In hex: "));
    printHex(rfid.uid.uidByte, rfid.uid.size);

    if ((WiFiMulti.run() == WL_CONNECTED)) {

      WiFiClient client;

      HTTPClient http;

      Serial.print("[HTTP] begin...\n");
      if (http.begin(client, "URL" + String(rfid.uid.uidByte[0]) + String(rfid.uid.uidByte[1]) + String(rfid.uid.uidByte[2])) + String(rfid.uid.uidByte[3])) { // HTTP


        // start connection and send HTTP header
        int httpCode = http.GET();

        // httpCode will be negative on error
        if (httpCode > 0) {
          // HTTP header has been send and Server response header has been handled
          Serial.printf("[HTTP] GET... code: %d\n", httpCode);

          // file found at server
          if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
            String payload = http.getString();
            Serial.println(payload.substring(0, 1));
            


            if (payload.substring(0, 1) == "G")
            {
              for (i = 0; i < 160; i++) // When a frequency sound
              {
                digitalWrite (16, HIGH) ; //send tone
                delay (1) ;
                digitalWrite (16, LOW) ; //no tone
                delay (1) ;
              }
            }
            if (payload.substring(0, 1) == "C")
            {
              for (i = 0; i < 160; i++) // When a frequency sound
              {
                digitalWrite (16, HIGH) ; //send tone
                delay (2) ;
                digitalWrite (16, LOW) ; //no tone
                delay (2) ;
              }
            }
            if (payload.substring(0, 1) == "H")
            {
              for (i = 0; i < 320; i++) // When a frequency sound
              {
                digitalWrite (16, HIGH) ; //send tone
                delay (2) ;
                digitalWrite (16, LOW) ; //no tone
                delay (2) ;
              }
            }


            lcd.setBacklight(255);
            lcd.home();
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print("  KART  OKUNDU  ");
            lcd.setCursor(0, 1);
            lcd.print(payload);

          }
        } else {
          for (i = 0; i < 320; i++) // When a frequency sound
          {
            digitalWrite (16, HIGH) ; //send tone
            delay (2) ;
            digitalWrite (16, LOW) ; //no tone
            delay (2) ;
          }
          Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
          lcd.setCursor(0, 0);
          lcd.print("    BASARISIZ   ");
          lcd.setCursor(0, 1);
          lcd.print(" Sunucu  Hatasi ");
        }

        http.end();
      } else {
        for (i = 0; i < 320; i++) // When a frequency sound
        {
          digitalWrite (16, HIGH) ; //send tone
          delay (2) ;
          digitalWrite (16, LOW) ; //no tone
          delay (2) ;
        }
        Serial.printf("[HTTP} Unable to connect\n");
        lcd.setCursor(0, 0);
        lcd.print("    BASARISIZ   ");
        lcd.setCursor(0, 1);
        lcd.print("  Internet Yok  ");
      }
    }
    Serial.println();
  }
  else
  {
    Serial.println(F("Card read previously."));
    lcd.setCursor(0, 0);
    lcd.print("    BASARISIZ   ");
    lcd.setCursor(0, 1);
    lcd.print(" Tekrarli Giris");
  }
  // Halt PICC
  rfid.PICC_HaltA();

  // Stop encryption on PCD
  rfid.PCD_StopCrypto1();
  delay(3000);
}


/**
   Helper routine to dump a byte array as hex values to Serial.
*/
void printHex(byte *buffer, byte bufferSize) {
  for (byte i = 0; i < bufferSize; i++) {
    Serial.print(buffer[i] < 0x10 ? " 0" : " ");
    Serial.print(buffer[i], HEX);
  }
}

/**
   Helper routine to dump a byte array as dec values to Serial.
*/
void printDec(byte *buffer, byte bufferSize) {
  for (byte i = 0; i < bufferSize; i++) {
    Serial.print(buffer[i] < 0x10 ? " 0" : " ");
    Serial.print(buffer[i], DEC);
  }
}
