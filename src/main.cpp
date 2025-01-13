#include <Arduino.h>
#include <SPI.h>
#include <MFRC522.h>
#include <TFT_eSPI.h>

#define RFID_CS 7      // SDA do RFID
#define RFID_RST 20    // RST do RFID
#define RFID_SCK 4     // SCK do RFID
#define RFID_MOSI 6    // MOSI do RFID
#define RFID_MISO 5    // MISO do RFID

SPIClass SPI1(VSPI);  // SPI para RFID
MFRC522 rfid(RFID_CS, RFID_RST); //-------------------------defines tela
TFT_eSPI tft = TFT_eSPI();

void writeToBlock();
void readAllBlocks();
bool waitForCardPresent();
void writeOnScreen(const char* texto, int x, int y, int tamanho, uint16_t cor) {
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(cor);
  tft.setTextSize(tamanho);
  tft.setCursor(x, y);
  tft.println(texto);
  Serial.println(texto);
}

void setup() { //------------------------------------------Setup
  Serial.begin(9600); //----------------------------Serial
  delay(150);
  tft.init();  //------------------------------------------------Tela  
  delay(150);
  tft.setRotation(0);
//---------------------------------------------------RFID
  SPI1.begin(RFID_SCK, RFID_MISO, RFID_MOSI, RFID_CS);
  rfid.PCD_Init();
  delay(250);
}

void loop() {  //------------------------------------------Loop
tft.fillScreen(TFT_BLACK);
writeOnScreen("aproxime o cartao", 10, 10, 2, TFT_WHITE);

  if (waitForCardPresent()) {
    writeOnScreen("Tag detectada!\n O que você deseja fazer? Digite 'R' para ler todos os blocos ou 'W' para escrever em um bloco específico.", 10, 10, 2, TFT_WHITE);
    while (!Serial.available());   // Aguarda até que algo seja digitado
    //char option = Serial.read();
    String option = Serial.readStringUntil('\n'); // Limpa o buffer do serial (importante!)
    option.toUpperCase();
    if( option.indexOf("R") > -1){

    //if (option == 'R' || option == 'r') {
      Serial.println("Aproxime o cartão novamente para leitura:");
      if (waitForCardPresent()) {
        readAllBlocks();
      }
    } else if( option.indexOf("W") > -1){ //if (option == 'W' || option == 'w') {
      writeToBlock();
    } else {
      Serial.println("Opção inválida. Digite 'R' ou 'W'.");
    }

    rfid.PICC_HaltA();
    rfid.PCD_StopCrypto1();
    writeOnScreen("Aguardando tag...", 10, 10, 2, TFT_WHITE);
  }
}

// Funcoes --------------------------------------------------Funcoes

bool waitForCardPresent() {
  while(true){
    if (rfid.PICC_IsNewCardPresent()) {
        if (rfid.PICC_ReadCardSerial()) {
          return true;
        }
      }
    delay(50);
  }
  return false;
}

void readAllBlocks() {
  MFRC522::StatusCode status;
  byte buffer[18];
  byte size = sizeof(buffer);
  MFRC522::MIFARE_Key key;

  Serial.print("Tag UID: ");
  for (byte i = 0; i < rfid.uid.size; i++) {
    Serial.print(rfid.uid.uidByte[i], HEX);
    Serial.print(" ");
  }
  Serial.println();

  for (byte i = 0; i < 6; i++) {
    key.keyByte[i] = 0xFF;
  }

  for (byte sector = 0; sector < 16; sector++) {
    for (byte block = 0; block < 4; block++) {
      byte blockNum = sector * 4 + block;

      status = rfid.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, blockNum, &key, &(rfid.uid));
      if (status != MFRC522::STATUS_OK) {
        Serial.print("Erro na autenticação do bloco ");
        Serial.print(blockNum);
        Serial.print(": ");
        Serial.println(rfid.GetStatusCodeName(status));
        continue;
      }

      status = rfid.MIFARE_Read(blockNum, buffer, &size);
      if (status != MFRC522::STATUS_OK) {
        Serial.print("Erro na leitura do bloco ");
        Serial.print(blockNum);
        Serial.print(": ");
        Serial.println(rfid.GetStatusCodeName(status));
        continue;
      }

      Serial.print("Bloco ");
      Serial.print(blockNum);
      Serial.print(": ");
      for (byte i = 0; i < 16; i++) {
        if (buffer[i] < 0x10) Serial.print("0");
        if ( blockNum > 0 && blockNum < 3) { // Mostra como caractere apenas os 4 primeiros bytes dos 4 primeiros blocos
            Serial.print((char)buffer[i]);
        } else {
            Serial.print(buffer[i], HEX);
        }
        Serial.print(" ");
      }
      Serial.println();
    }
  }
}

void writeToBlock() {
  Serial.println("Digite o número do bloco (0 a 63) para escrever:");

  while (!Serial.available());
  int blockNum = Serial.parseInt();
  Serial.readStringUntil('\n'); // Limpa o buffer do serial (crucial para evitar erros)

  if (blockNum < 0 || blockNum > 63) {
    Serial.println("Número de bloco inválido. Deve ser entre 0 e 63.");
    return;
  }

  Serial.println("Digite a mensagem (até 16 caracteres):");

  while (!Serial.available());
  String input = Serial.readStringUntil('\n');
  input.trim();

  if (input.length() > 16) {
    Serial.println("Mensagem muito longa. Use até 16 caracteres.");
    return;
  }

  byte data[16] = {0};
  for (byte i = 0; i < input.length(); i++) {
    data[i] = input[i];
  }

  MFRC522::MIFARE_Key key;
  for (byte i = 0; i < 6; i++) {
    key.keyByte[i] = 0xFF;
  }

  Serial.println("Aproxime o cartão novamente para escrita:");
  if(waitForCardPresent()){
    MFRC522::StatusCode status = rfid.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, blockNum, &key, &(rfid.uid));
    if (status != MFRC522::STATUS_OK) {
      Serial.print("Erro na autenticação do bloco ");
      Serial.print(blockNum);
      Serial.print(": ");
      Serial.println(rfid.GetStatusCodeName(status));
      return;
    }

    status = rfid.MIFARE_Write(blockNum, data, 16);
    if (status != MFRC522::STATUS_OK) {
      Serial.print("Erro na escrita do bloco ");
      Serial.print(blockNum);
      Serial.print(": ");
      Serial.println(rfid.GetStatusCodeName(status));
      return;
    }

    Serial.println("Dados escritos com sucesso!");
  }
}