#include <Wire.h>

#define LOOP_INTERVAL_MS 10
#define MPU_ADDR 0x68

float Kp = 2.0, Ki = 0.1, Kd = 0.5;
float integral = 0, prev_error = 0;
unsigned long lastTime = 0;
int loopCount = 0;

void setup() {
    Serial.begin(115200);
    Wire.begin(32, 33);  // SDA=32, SCL=33
    Wire.beginTransmission(MPU_ADDR);
    Wire.write(0x6B); Wire.write(0x00);
    Wire.endTransmission();
    delay(200);
    Serial.println("初期化完了");
}

int16_t readRaw(uint8_t reg) {
    Wire.beginTransmission(MPU_ADDR);
    Wire.write(reg);
    Wire.endTransmission(false);
    Wire.requestFrom((uint8_t)MPU_ADDR, (uint8_t)2);
    if (Wire.available() < 2) return 0;
    return (Wire.read() << 8) | Wire.read();
}

void loop() {
    unsigned long now = millis();
    if (now - lastTime < LOOP_INTERVAL_MS) return;
    float dt = (now - lastTime) / 1000.0;
    lastTime = now;

    float ax = readRaw(0x3B) / 16384.0;
    float ay = readRaw(0x3D) / 16384.0;
    float az = readRaw(0x3F) / 16384.0;

    float roll = atan2(ay, az) * 180.0 / PI;

    float error = 0.0 - roll;
    integral += error * dt;
    float derivative = (error - prev_error) / dt;
    float output = Kp * error + Ki * integral + Kd * derivative;
    output = constrain(output, -45, 45);
    prev_error = error;

    loopCount++;
    if (loopCount % 10 == 0) {
        Serial.printf("roll=%.1f  output=%.1f  t=%lu\n", roll, output, now);
    }
}