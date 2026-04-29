#include <Wire.h>
#define MPU_ADDR 0x68

void setup() {
    Serial.begin(115200);
    Wire.begin(32, 33); // SDA=32, SCL=33
    Wire.beginTransmission(MPU_ADDR);
    Wire.write(0x6B);
    Wire.write(0x00);
    Wire.endTransmission();
    delay(100);
    Serial.println("MPU-6050 初期化完了");
}

int16_t readRaw(uint8_t reg) {
    Wire.beginTransmission(MPU_ADDR);
    Wire.write(reg);
    Wire.endTransmission(false);
    Wire.requestFrom(MPU_ADDR, 2);
    return (Wire.read() << 8) | Wire.read();
}

void loop() {
    float ax = readRaw(0x3B) / 16384.0;
    float ay = readRaw(0x3D) / 16384.0;
    float az = readRaw(0x3F) / 16384.0;
    Serial.printf("ax=%.3f  ay=%.3f  az=%.3f g\n", ax, ay, az);
    delay(100);
}