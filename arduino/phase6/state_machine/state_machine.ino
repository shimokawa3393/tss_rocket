#include <SD.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_BMP280.h>

#define SD_CS      5
#define SD_MOSI    23
#define SD_MISO    19
#define SD_SCK     18
#define SDA_PIN    32
#define SCL_PIN    33
#define MPU_ADDR   0x68
#define MOSFET_PIN 4

Adafruit_BMP280 bmp;
float groundAltitude = 0;


/*
 * フライトステートマシン
 *
 * 【IDLE】待機中
 *   条件：加速度 > 3.0G
 *     ↓
 * 【LAUNCHED】発射直後
 *   条件：発射から500ms経過
 *     ↓
 * 【COAST】慣性上昇
 *   条件：高度が2m以上低下 または 加速度 < 0.5G
 *     ↓
 * 【APOGEE】頂点
 *   → LEDが1秒点灯（将来はニクロム線点火）
 *   → 即座に次へ
 *     ↓
 * 【DESCENT】降下中
 *   条件：相対高度 < 5m かつ 加速度 0.8〜1.2G
 *     ↓
 * 【LANDED】着地
 *   → 終了
 */
 
enum FlightState {
    IDLE, LAUNCHED, COAST, APOGEE, DESCENT, LANDED
};

FlightState state = IDLE;
unsigned long launchTime = 0;
float maxAltitude = 0;

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

void updateState(float ax, float ay, float az, float altitude) {
    float accel = sqrt(ax*ax + ay*ay + az*az);
    float relAlt = altitude - groundAltitude;

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
                maxAltitude = relAlt;
            }
            break;
        case COAST:
            if (relAlt > maxAltitude) maxAltitude = relAlt;
            // 高度が下がるか、加速度が急減したら頂点判定
            if (relAlt < maxAltitude - 2.0 || 
                (accel < 0.5 && maxAltitude > 0)) state = APOGEE;
            break;
        case APOGEE:
            Serial.println("★ パラシュート展開！");
            digitalWrite(MOSFET_PIN, HIGH);
            delay(1000);
            digitalWrite(MOSFET_PIN, LOW);
            state = DESCENT;
            break;
        case DESCENT:
            if (relAlt < 5.0 && accel > 0.8 && accel < 1.2) state = LANDED;
            break;
        case LANDED:
            break;
    }
}

void setup() {
    Serial.begin(115200);
    delay(1000);

    pinMode(MOSFET_PIN, OUTPUT);
    digitalWrite(MOSFET_PIN, LOW);

    Wire.begin(SDA_PIN, SCL_PIN);
    Wire.beginTransmission(MPU_ADDR);
    Wire.write(0x6B);
    Wire.write(0x00);
    Wire.endTransmission();
    Serial.println("MPU-6050 OK");

    if (!bmp.begin(0x77)) {
        Serial.println("BMP280: NG");
        while (true);
    }
    Serial.println("BMP280 OK");

    // 地上高度を基準値として記録
    delay(500);
    groundAltitude = bmp.readAltitude(1013.25);
    Serial.printf("地上高度基準: %.1fm\n", groundAltitude);

    SPI.begin(SD_SCK, SD_MISO, SD_MOSI, SD_CS);
    delay(500);
    if (!SD.begin(SD_CS, SPI, 4000000)) {
        Serial.println("SD: NG");
        while (true);
    }
    Serial.println("SD OK");

    File f = SD.open("/flight.csv", FILE_WRITE);
    if (f) {
        f.println("time,ax,ay,az,altitude,state");
        f.close();
    }
    Serial.println("準備完了・IDLE待機中");
}

void loop() {
    float ax = readRaw(0x3B) / 16384.0;
    float ay = readRaw(0x3D) / 16384.0;
    float az = readRaw(0x3F) / 16384.0;
    float altitude = bmp.readAltitude(1013.25);
    float relAlt = altitude - groundAltitude;

    updateState(ax, ay, az, altitude);

    Serial.printf("accel=%.2f  高度=%.1fm  state=%s\n",
        sqrt(ax*ax + ay*ay + az*az), relAlt, stateName(state));

    File f = SD.open("/flight.csv", FILE_APPEND);
    if (f) {
        f.printf("%.3f,%.4f,%.4f,%.4f,%.2f,%s\n",
            millis()/1000.0, ax, ay, az, relAlt, stateName(state));
        f.close();
    }

    delay(100);
}