#include <Wire.h>
#include <Adafruit_BMP280.h>

// ピン定義
#define SDA_PIN    32
#define SCL_PIN    33
#define MPU_ADDR   0x68
#define MOSFET_PIN 4  // ニクロム線駆動用MOSFETピン

Adafruit_BMP280 bmp;
float groundAltitude = 0; // 地上高度の基準値

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
unsigned long launchTime = 0; // 発射時刻
float maxAltitude = 0;        // 到達最大高度

// ステート名を文字列で返す関数
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

// MPU-6050の生データを読み取る関数
int16_t readRaw(uint8_t reg) {
    Wire.beginTransmission(MPU_ADDR);
    Wire.write(reg);
    Wire.endTransmission(false);
    Wire.requestFrom(MPU_ADDR, 2);
    return (Wire.read() << 8) | Wire.read();
}

// フライト状態を更新する関数
void updateState(float ax, float ay, float az, float altitude) {
    float accel  = sqrt(ax*ax + ay*ay + az*az); // 合成加速度
    float relAlt = altitude - groundAltitude;    // 地上からの相対高度

    switch(state) {
        case IDLE:
            // 加速度3G超で発射判定
            if (accel > 3.0) {
                state = LAUNCHED;
                launchTime = millis();
            }
            break;
        case LAUNCHED:
            // 発射から500ms後にCOASTへ
            if (millis() - launchTime > 500) {
                state = COAST;
                maxAltitude = relAlt;
            }
            break;
        case COAST:
            // 最大高度を更新し続ける
            if (relAlt > maxAltitude) maxAltitude = relAlt;
            // 高度が下がるか加速度が急減したら頂点判定
            if (relAlt < maxAltitude - 2.0 ||
                (accel < 0.5 && maxAltitude > 0)) state = APOGEE;
            break;
        case APOGEE:
            // パラシュート展開（MOSFET経由でニクロム線点火）
            Serial.println("★ パラシュート展開！");
            digitalWrite(MOSFET_PIN, HIGH);
            delay(1000);
            digitalWrite(MOSFET_PIN, LOW);
            state = DESCENT;
            break;
        case DESCENT:
            // 高度5m以下かつ加速度が1G付近で着地判定
            if (relAlt < 5.0 && accel > 0.8 && accel < 1.2) state = LANDED;
            break;
        case LANDED:
            break;
    }
}

void setup() {
    Serial.begin(115200);
    delay(1000);

    // MOSFETピンをLOWに初期化（誤爆防止）
    pinMode(MOSFET_PIN, OUTPUT);
    digitalWrite(MOSFET_PIN, LOW);

    // MPU-6050のスリープ解除
    Wire.begin(SDA_PIN, SCL_PIN);
    Wire.beginTransmission(MPU_ADDR);
    Wire.write(0x6B);
    Wire.write(0x00);
    Wire.endTransmission();
    Serial.println("MPU-6050 OK");

    // BMP280初期化
    if (!bmp.begin(0x77)) {
        Serial.println("BMP280: NG");
        while (true);
    }
    Serial.println("BMP280 OK");

    // 地上高度を基準値として記録
    delay(500);
    groundAltitude = bmp.readAltitude(1013.25);
    Serial.printf("地上高度基準: %.1fm\n", groundAltitude);

    Serial.println("準備完了・IDLE待機中");
}

void loop() {
    // センサーデータ取得
    float ax       = readRaw(0x3B) / 16384.0;      // X軸加速度 [g]
    float ay       = readRaw(0x3D) / 16384.0;      // Y軸加速度 [g]
    float az       = readRaw(0x3F) / 16384.0;      // Z軸加速度 [g]
    float altitude = bmp.readAltitude(1013.25);     // 絶対高度 [m]
    float relAlt   = altitude - groundAltitude;     // 相対高度 [m]

    // 状態更新
    updateState(ax, ay, az, altitude);

    // シリアルモニタに出力
    Serial.printf("accel=%.2f  高度=%.1fm  state=%s\n",
        sqrt(ax*ax + ay*ay + az*az), relAlt, stateName(state));

    delay(100);
}