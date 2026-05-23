#include <LoRa.h>
#include <Wire.h>
#include <Adafruit_BMP280.h>
#include <SPI.h>

// ピン定義
#define SCK       18
#define MISO      19
#define MOSI      23
#define LORA_SS   15
#define LORA_RST  14
#define LORA_DIO0 26

// MPU-6050のI2Cアドレス
#define MPU_ADDR 0x68

// センサーのグローバル変数
Adafruit_BMP280 bmp(&Wire);
float alt_offset = 0; // 起動地点の高度基準値

// MPU-6050の生データを読み取る関数
int16_t readRaw(uint8_t reg) {
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(reg);
  Wire.endTransmission(false);
  Wire.requestFrom(MPU_ADDR, 2);
  return (Wire.read() << 8) | Wire.read();
}

void setup() {
  Serial.begin(115200);
  Wire.begin(32, 33); // SDA=32, SCL=33
  delay(100);

  // MPU-6050のスリープ解除
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x6B);
  Wire.write(0x00);
  Wire.endTransmission();

  // BMP280初期化
  if (!bmp.begin(0x77)) {
    Serial.println("BMP280失敗");
  }
  // 起動地点の高度を基準値として記録
  alt_offset = bmp.readAltitude();

  // LoRa初期化
  LoRa.setPins(LORA_SS, LORA_RST, LORA_DIO0);
  if (!LoRa.begin(433E6)) {
    Serial.println("LoRa失敗");
    while(1);
  }
  LoRa.setSpreadingFactor(9);
  Serial.println("LoRa起動OK");
}

void loop() {
  // センサーデータ取得
  float ax   = readRaw(0x3B) / 16384.0;         // X軸加速度 [g]
  float ay   = readRaw(0x3D) / 16384.0;         // Y軸加速度 [g]
  float az   = readRaw(0x3F) / 16384.0;   // Z軸加速度（逆さまのため符号反転）[g]
  float alt  = bmp.readAltitude() - alt_offset;  // 起動地点からの相対高度 [m]
  float temp = bmp.readTemperature();            // 気温 [℃]

  // 送信パケットをCSV形式で生成
  String packet = String(millis()) + "," +
                  String(alt,  1) + "," +
                  String(ax,   3) + "," +
                  String(ay,   3) + "," +
                  String(az,   3) + "," +
                  String(temp, 1);

  // LoRaで地上局へ送信
  LoRa.beginPacket();
  LoRa.print(packet);
  LoRa.endPacket();

  Serial.println("送信: " + packet);
  delay(1000);
} 