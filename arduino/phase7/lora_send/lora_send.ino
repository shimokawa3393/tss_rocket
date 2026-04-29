#include <LoRa.h>
#include <Wire.h>
#include <Adafruit_BMP280.h>
#include <SD.h>
#include <SPI.h>

#define SCK  18
#define MISO 19
#define MOSI 23
#define LORA_SS  15
#define LORA_RST 14
#define LORA_DIO0 26
#define SD_CS 5

#define MPU_ADDR 0x68

Adafruit_BMP280 bmp(&Wire);
String filename;
File logFile;

int16_t readRaw(uint8_t reg) {
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(reg);
  Wire.endTransmission(false);
  Wire.requestFrom(MPU_ADDR, 2);
  return (Wire.read() << 8) | Wire.read();
}

void setup() {
  Serial.begin(115200);
  Wire.begin(32, 33);
  delay(100);

  // MPU-6050初期化
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x6B);
  Wire.write(0x00);
  Wire.endTransmission();

  // BMP280初期化
  if (!bmp.begin(0x77)) {
    Serial.println("BMP280失敗");
  }

  // SD初期化
  SPI.begin(SCK, MISO, MOSI);
  if (!SD.begin(SD_CS)) {
    Serial.println("SDマウント失敗");
  } else {
    Serial.println("SDマウントOK");
    int fileNum = 0;
    while (SD.exists("/log_" + String(fileNum) + ".csv")) {
      fileNum++;
    }
    filename = "/log_" + String(fileNum) + ".csv";
    logFile = SD.open(filename, FILE_WRITE);
    if (logFile) {
      logFile.println("time_ms,alt,ax,ay,az,temp");
      logFile.flush();
      Serial.println("SD OK: " + filename);
    } else {
      Serial.println("ファイルオープン失敗");
    }
  }

  // LoRa初期化
  LoRa.setPins(LORA_SS, LORA_RST, LORA_DIO0);
  if (!LoRa.begin(433E6)) {
    Serial.println("LoRa失敗");
    while(1);
  }
  LoRa.setSpreadingFactor(9);
  Serial.println("起動OK");
}

void loop() {
  float ax = readRaw(0x3B) / 16384.0;
  float ay = readRaw(0x3D) / 16384.0;
  float az = readRaw(0x3F) / 16384.0;
  float alt = bmp.readAltitude();
  float temp = bmp.readTemperature();

  String packet = String(millis()) + "," +
                  String(alt, 1) + "," +
                  String(ax, 3) + "," +
                  String(ay, 3) + "," +
                  String(az, 3) + "," +
                  String(temp, 1);

  // LoRa送信
  LoRa.beginPacket();
  LoRa.print(packet);
  LoRa.endPacket();

  // SD書き込み
  if (logFile) {
    logFile.println(packet);
    logFile.flush();
  }

  Serial.println("送信: " + packet);
  delay(1000);
}