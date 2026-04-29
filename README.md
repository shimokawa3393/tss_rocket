# TSS Rocket Project

深宇宙探査を目指す長期プロジェクトの、制御技術実証フェーズ。

---

## 概要

モデルロケットに自作フライトコンピュータを搭載し、姿勢制御・頂点検出・パラシュート展開の再現性を実証する。

- 対象：モデルロケット（JAR第4級ライセンス準拠）
- メイン機材：ESP32、MPU-6050、BMP280、AHT20、LoRa Ra-02
- 開発方針：ソフトウェアで完全理解してからハードに落とす

---

## ロードマップ

| フェーズ | 内容 | 状態 |
|--------|------|------|
| Phase 0 | 開発環境構築・法規制把握 | ✅ |
| Phase 1 | 水ロケット（空力・安定性の体感） | ✅ |
| Phase 2 | 物理シミュレーション実装 | ✅ |
| Phase 3 | PID制御アルゴリズム | ✅ |
| Phase 4 | センサー検証（Raspberry Pi） | ✅ |
| Phase 4.5 | ESP32移植検証 | ✅ |
| Phase 5 | ESP32リアルタイム制御ループ | ✅ |
| Phase 6 | フライトコンピュータ・安全設計 | ✅ |
| Phase 7 | LoRa テレメトリ + SDログ + リアルタイムダッシュボード | ✅ |
| Phase 8 | モデルロケット初飛行（JAR） | 🎯 |
| Phase 9 | 自作FC搭載ロケット | 🎯 |

---

## ハードウェア構成

| 部品 | 用途 |
|------|------|
| ESP32-WROOM-32 | フライトコンピュータ本体 |
| MPU-6050 | 加速度・ジャイロ（6軸） |
| BMP280 | 気圧・高度推定 |
| AHT20 | 温湿度 |
| LoRa Ra-02 (433MHz) | テレメトリ送受信 |
| SDカードモジュール | フライトログ保存 |
| MOSFETモジュール | ニクロム線駆動 |
| ニクロム線 32AWG | パラシュート点火 |

---

## リポジトリ構成

```
tss_rocket/
├── phase2/
│   └── rocket_sim.py        # 物理シミュレーション
├── phase3/
│   └── control.py           # PID制御
├── phase4/
│   ├── sensor_reader.py     # Raspberry Pi センサー取得
│   └── plot_sensor.py       # センサーデータ可視化（Mac）
├── phase7/
│   ├── receiver.py          # LoRa受信・CSV保存・JSON書き出し
│   ├── dashboard.py         # Flaskリアルタイムダッシュボード
│   └── plot_telemetry.py    # テレメトリ可視化（Mac）
└── arduino/
    ├── phase5/sensor_reader/ # ESP32 センサー読み取り
    ├── phase6/state_machine/ # フライトコンピュータ
    └── phase7/lora_send/     # LoRa送信・SDログ
```

---

## セットアップ

### 共通環境

```bash
mkdir <yourproject>
cd <yourproject>
python3 -m venv venv
source venv/bin/activate
pip install numpy matplotlib pandas smbus2 bmp280
```

> ⚠️ Python 3.13では `adafruit-circuitpython-bmp280` が非対応。`bmp280` ライブラリを使うこと。

### Arduino IDE

- Arduino IDE 2.x をインストール
- ボードマネージャーで `ESP32 by Espressif Systems` を追加
- 以下のライブラリをインストール
  - `Adafruit BMP280 Library`
  - `Arduino LoRa` by Sandeep Mistry
  - `SD`

### Raspberry Pi（Phase 7）

```bash
# グローバル環境にインストール（pyLoRaはラズパイ5のGPIO対応のため）
pip install pyLoRa --break-system-packages

# venv環境にインストール
source .venv/bin/activate
pip install flask
```

---

## 各フェーズの手順

### Phase 2：物理シミュレーション

```bash
cd phase2
python rocket_sim.py
```

- 推力・バーンタイム・Cdを変えてA〜Cエンジンの違いを確認
- OpenRocketの結果と比較して差異を考察

---

### Phase 3：PID制御

```bash
cd phase3
python pid_control.py
```

- Kp→Kd→Kiの順にチューニング
- ノイズありで収束することを確認してから次へ

---

### Phase 4：センサー検証（Raspberry Pi）

**配線**
```
Raspberry Pi    センサー
3.3V  ───────── VCC
GND   ───────── GND
GPIO2 (SDA) ─── SDA
GPIO3 (SCL) ─── SCL
```

```bash
# I2C有効化
sudo raspi-config  # Interface Options → I2C → Enable

# デバイス確認（0x38=AHT20 / 0x68=MPU-6050 / 0x77=BMP280）
i2cdetect -y 1

cd phase4
python sensor_reader.py

# MacにCSVをコピーして可視化
scp <user>@<hostname>.local:~/<yourproject>/*.csv .
python plot_sensor.py
```

---

### Phase 4.5：ESP32移植検証

**配線**
```
ESP32           MPU-6050
3V3    ───────── VCC
GND    ───────── GND
GPIO32 (SDA) ─── SDA
GPIO33 (SCL) ─── SCL
```

- `arduino/phase5/sensor_verify/sensor_verify.ino` を書き込み
- シリアルモニタで加速度値を確認
- ラズパイでの取得値と比較してノイズ・ドリフトを検証

---

### Phase 5：リアルタイム制御ループ

- `arduino/phase5/sensor_reader/sensor_reader.ino` を書き込み
- `Wire.begin(32, 33)` が明示的に記述されていることを確認
- シリアルモニタでroll角と制御出力を確認

> ⚠️ `delay()` ではなく `millis()` 差分で制御ループを管理すること。

---

### Phase 6：フライトコンピュータ・安全設計

- `arduino/phase6/state_machine/state_machine.ino` を書き込み
- 静止→手で振る→APOGEE遷移を確認
- APOGEE検出でLED（ニクロム線代替）が点灯することを確認

**ステートマシン**
```
IDLE → LAUNCHED → COAST → APOGEE → DESCENT → LANDED
```

**パラシュート展開の安全条件**
- 発射確認済み（IDLEからの遷移あり）
- 発射から2秒以上経過
- 一度でも上昇を記録
- 頂点確認（高度が2m以上低下）

---

### Phase 7：LoRa テレメトリ + SDログ + リアルタイムダッシュボード

**配線（ESP32 → Ra-02）**

| Ra-02 | ESP32 |
|-------|-------|
| VCC | 3V3 |
| GND | GND |
| SCK | GPIO18 |
| MOSI | GPIO23 |
| MISO | GPIO19 |
| NSS(CS) | GPIO15 |
| RST | GPIO14 |
| DIO0 | GPIO26 |

**配線（ラズパイ → Ra-02）**

| Ra-02 | ラズパイ |
|-------|---------|
| VCC | Pin1（3.3V） |
| GND | Pin6 |
| SCK | Pin23 |
| MOSI | Pin19 |
| MISO | Pin21 |
| NSS(CS) | Pin24 |
| RST | Pin15（GPIO22） |
| DIO0 | Pin7（GPIO4） |

**起動手順**

```bash
# ターミナル1：SSHトンネル（Mac）
ssh -L 5000:localhost:5000 <user>@<hostname>.local

# ターミナル2：受信（ラズパイ・グローバルpython）
deactivate
python3 receiver.py

# ターミナル3：ダッシュボード（ラズパイ・venv）
source .venv/bin/activate
python3 dashboard.py
```

ブラウザで `http://localhost:5000` を開く。

**SDカードデータの可視化（Mac）**

```bash
scp <user>@<hostname>.local:~/<yourproject>/telemetry_*.csv ~/Desktop/
python3 phase7/plot_telemetry.py
```

> ⚠️ pyLoRaはラズパイ5のGPIOに非対応のため、receiver.pyはグローバルpythonで起動すること。

---

## 開発ログ

**Phase 4**
- `adafruit-circuitpython-bmp280` がPython 3.13と非互換 → `bmp280` ライブラリに切り替えで解決

**Phase 5**
- 制御ループに `delay()` を使わない。`millis()` 差分管理が正解
- Kdはノイズフィルタリングなしでは実機で暴れる

**Phase 6**
- 地上振動でパラシュートが誤爆する。「発射確認済み」フラグが必須
- ニクロム線はMOSFET経由で駆動。GPIOから直接は流せない

**Phase 7**
- pyLoRaがラズパイ5のGPIO（RPi.GPIO）に非対応。グローバル環境にrpi-lgpioを入れることで解決
- LoRaのRSTピンはGPIO22（物理Pin15）。Pin22と混同しないこと
- SDカードとLoRaはSPIを共有。CSピンだけ個別に割り当てる（LoRa:GPIO15、SD:GPIO5）
- 無線ノイズによる文字化けパケットは `pd.to_numeric(errors='coerce')` でドロップ

---

## ライセンス

MIT