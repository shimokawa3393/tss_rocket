#include <SD.h>
#include <SPI.h>
#include <Wire.h>

#define SD_CS   5
#define SD_MOSI 23
#define SD_MISO 19
#define SD_SCK  18
#define SDA_PIN 32
#define SCL_PIN 33
#define MPU_ADDR 0x68

enum FlightState {
    IDLE, LAUNCHED, COAST, APOGEE, DESCENT, LANDED
};

FlightState state = IDLE;
unsigned long launchTime = 0;
float maxAccel = 0;

const char* stateName(FlightState s) {
    switch(s) {
        case IDLE:     return "IDLE";
        case LAUNCHED: return "LAUNCHED";
        case COAST:    return "COAST";
        case APOGEE:   return "APOGEE";
        case DESCENT:  return "DESCENT";
        case LANDED:   return "LANDED";
        default:       return "UNKNOWN";
    }
}

int16_t readRaw(uint8_t reg) {
    Wire.beginTransmission(MPU_ADDR);
    Wire.write(reg);
    Wire.endTransmission(false);
    Wire.requestFrom(MPU_ADDR, 2);
    return (Wire.read() << 8) | Wire.read();
}

void updateState(float ax, float ay, float az) {
    float accel = sqrt(ax*ax + ay*ay + az*az);
    switch(state) {
        case IDLE:
            if (accel > 3.0) {
                state = LAUNCHED;
                launchTime = millis();
            }
            break;
        case LAUNCHED:
            if (millis() - launchTime > 500) {
                state = COAST;
                maxAccel = accel;
            }
            break;
        case COAST:
            if (accel > maxAccel) maxAccel = accel;
            if (accel < maxAccel - 0.5) state = APOGEE;
            break;
        case APOGEE:
            Serial.println("★ パラシュート展開！");
            state = DESCENT;
            break;
        case DESCENT:
            if (accel > 0.9 && accel < 1.1) state = LANDED;
            break;
        case LANDED:
            break;
    }
}

void setup() {
    Serial.begin(115200);
    delay(1000);

    Wire.begin(SDA_PIN, SCL_PIN);
    Wire.beginTransmission(MPU_ADDR);
    Wire.write(0x6B);
    Wire.write(0x00);
    Wire.endTransmission();
    Serial.println("MPU-6050 OK");

    SPI.begin(SD_SCK, SD_MISO, SD_MOSI, SD_CS);
    delay(500);
    if (!SD.begin(SD_CS, SPI, 4000000)) {
        Serial.println("SD: NG");
        while (true);
    }
    Serial.println("SD: OK");

    File f = SD.open("/flight.csv", FILE_WRITE);
    if (f) {
        f.println("time,ax,ay,az,state");
        f.close();
    }
    Serial.println("準備完了・IDLE待機中");
}

void loop() {
    float ax = readRaw(0x3B) / 16384.0;
    float ay = readRaw(0x3D) / 16384.0;
    float az = readRaw(0x3F) / 16384.0;

    updateState(ax, ay, az);

    Serial.printf("accel=%.2f  state=%s\n",
        sqrt(ax*ax + ay*ay + az*az), stateName(state));

    File f = SD.open("/flight.csv", FILE_APPEND);
    if (f) {
        f.printf("%.3f,%.4f,%.4f,%.4f,%s\n",
            millis()/1000.0, ax, ay, az, stateName(state));
        f.close();
    }

    delay(100);
}