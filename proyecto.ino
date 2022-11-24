#include <SPI.h>
#include <MFRC522.h>
#include <ModbusRTU.h>

#define SLAVE_ID 1
#define RST_PIN 22
#define SS_1_PIN  5       
#define SS_2_PIN  4   
#define NUM 2
#define UID 1
#define DATA 6
#define BOBINA 11

byte ssPins[NUM] = {SS_1_PIN, SS_2_PIN};
byte RELE[NUM] = {2,15};

MFRC522 mfrc522[NUM]; 
int rfidAntState[NUM]= {NULL};
int rfidState[NUM]= {NULL};
MFRC522::MIFARE_Key key;
ModbusRTU mb;

void setup() {
  Serial.begin(115200);
  Serial2.begin(115200);
  
  mb.begin(&Serial2,26);
  mb.slave(SLAVE_ID);

  
                                      // Creacion memorias UID
  for(int i=0; i < NUM; i++){
    mb.addHreg(UID+i);
    //mb.Hreg(UID[i], 100);
    Serial.print("Memoria UID = ");
    Serial.println(UID+i);
    delay(500);
 }
                                      // Creacion memorias DATA
  for(int i=0; i < NUM; i++){
    mb.addHreg(DATA+i);
    Serial.print("Memoria DATA = ");
    Serial.println(DATA+i);
    delay(500);
 }
                                       // Creacion memorias COIL
  for(int i=0; i < NUM; i++){
    pinMode(RELE[i], OUTPUT);
    mb.addCoil(BOBINA+i,false);
    Serial.print("Memoria COIL = ");
    Serial.println(BOBINA+i);
    delay(500);
 }
                                       // Encendido de los TAG
  SPI.begin();
  for (uint8_t i = 0; i < NUM; i++) {
    mfrc522[i].PCD_Init(ssPins[i], RST_PIN); 
  }   
}

unsigned long count = 0;
unsigned long countRFID = 0;
unsigned long countLeer= 0;

void Bobinas(){
  for (uint8_t i = 0; i < NUM; i++) {
      digitalWrite(RELE[i], mb.Coil(BOBINA+i));
    } 
}

void loop() {
  
  if(millis()-count >= 1000){   
    for(int i=0; i < NUM; i++){
      Serial.print("UID ");
      Serial.print(i);
      Serial.print(": ");
      Serial.print(mb.Hreg(UID+i));
      Serial.print(" ");

      Serial.print("DATA ");
      Serial.print(i);
      Serial.print(": ");
      Serial.print(mb.Hreg(DATA+i));
      Serial.print(" ");

      Serial.print("BOB ");
      Serial.print(i);
      Serial.print(": ");
      Serial.print(mb.Coil(BOBINA+i));
      Serial.print("     ");   
    }
    Serial.println("");
    count = millis();  
  }
  
  if(millis()-countRFID >= 500){ 
    RFID();
    countRFID = millis();  
  }
    if(millis()-countLeer >= 400){ 
    Leer();
    countLeer = millis();  
  }
  Bobinas();
  mb.task();
  yield();
}
                                         //lectura de targetas RFID
void RFID() {
  
unsigned long countRst= 0;
  long entero1;
  if(millis()-countRFID >= 100){ 
    digitalWrite(RST_PIN, 1);
    countRst = millis();  
  }
  
  for (uint8_t i = 0; i < NUM; i++) {
    if (mfrc522[i].PICC_IsCardPresent() ) {
      if (mfrc522[i].PICC_ReadCardSerial()){

         String con = "";
         long entero = 0;
         for (byte j = 0; j <mfrc522[i].uid.size; j++) {
            con.concat(String(mfrc522[i].uid.uidByte[j]));
         }
        entero = atol(con.c_str());
        mb.Hreg(UID+i, entero);
 
        rfidAntState[i] = rfidState[i];
        rfidState[i] = 1;
      }
    } 
    else{
      if (rfidAntState[i] == rfidState[i]){
        mb.Hreg(UID+i, 0);
        mb.Hreg(DATA+i, 0);
    }
    rfidAntState[i] = rfidState[i];
    rfidState[i] = 0;
    }
  } 
}
                                          //lectura DATA de la targetas PICC
void Leer(){
   
   for (uint8_t i = 0; i < NUM; i++) {
    if (mfrc522[i].PICC_IsCardPresent() ) {
      if (mfrc522[i].PICC_ReadCardSerial()){

        for (byte i = 0; i < 6; i++) key.keyByte[i] = 0xFF;
      
        byte block;
        byte len;
        MFRC522::StatusCode status;
      
        len = 18;
        byte buffer2[18];
        block = 1;
      
        status = mfrc522[i].PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, 1, &key, &(mfrc522[i].uid)); 
        if (status != MFRC522::STATUS_OK) {
          return;
        }
      
        status = mfrc522[i].MIFARE_Read(block, buffer2, &len);
        if (status != MFRC522::STATUS_OK) {
          return;
        }
        
        String dataString = "";
        long dataLong = 0;
        for (uint8_t i = 0; i < 16; i++) {
         dataString+=(char)buffer2[i];
        }
        
        dataLong= atol(dataString.c_str());
        mb.Hreg(DATA+i, dataLong);
    
        mfrc522[i].PICC_HaltA();
        mfrc522[i].PCD_StopCrypto1();
      }
    }
  }
}
