# TSS Rocket Project

---

## 概要

ESP32ベースのフライトコンピュータを自作し、センサー統合・状態管理・テレメトリ通信基盤を構築した。

**メイン機材**

- ESP32
- MPU-6050
- BMP280
- AHT20
- LoRa Ra-02

**開発方針**

- ソフトウェアで完全理解してからハードウェアへ落とし込む

---

## ロードマップ


| フェーズ      | 内容                        | 状態  |
| --------- | ------------------------- | --- |
| Phase 0   | 開発環境構築・法規制把握              | ✅   |
| Phase 1   | 水ロケット（空力・安定性の体感）          | ✅   |
| Phase 2   | 物理シミュレーション実装              | ✅   |
| Phase 3   | PID制御アルゴリズム               | ✅   |
| Phase 4   | センサー検証（Raspberry Pi）      | ✅   |
| Phase 4.5 | ESP32移植検証                 | ✅   |
| Phase 5   | ESP32リアルタイム制御ループ          | ✅   |
| Phase 6   | フライトコンピュータ・安全設計           | ✅   |
| Phase 7   | LoRaテレメトリ + リアルタイムダッシュボード | ✅   |
| Phase 8   | JAR飛行                     | 🎯  |


---

## ハードウェア構成


| 部品                 | 用途           |
| ------------------ | ------------ |
| ESP32-WROOM-32     | フライトコンピュータ本体 |
| MPU-6050           | 加速度・ジャイロ（6軸） |
| BMP280             | 気圧・高度推定      |
| AHT20              | 温湿度          |
| LoRa Ra-02（433MHz） | テレメトリ送受信     |
| MOSFETモジュール        | ニクロム線駆動      |
| ニクロム線 32AWG        | パラシュート点火     |


---

## リポジトリ構成

```text
tss_rocket/
├── phase2/
│   └── rocket_sim.py        # 物理シミュレーション
│
├── phase3/
│   └── control.py           # PID制御
│
├── phase4/
│   ├── sensor_reader.py     # Raspberry Pi センサー取得
│   └── plot_sensor.py       # センサーデータ可視化（Mac）
│
├── phase7/
│   ├── receiver.py          # LoRa受信・CSV保存・JSON書き出し
│   ├── dashboard.py         # Flaskリアルタイムダッシュボード
│   └── plot_telemetry.py    # テレメトリ可視化（Mac）
│
└── arduino/
    ├── phase5/sensor_reader/   # ESP32 センサー読み取り
    ├── phase6/state_machine/   # フライトコンピュータ
    └── phase7/lora_send/       # LoRa送信
```

---

## セットアップ

### 共通環境

```bash
python3 -m venv venv
source venv/bin/activate

pip install numpy matplotlib pandas smbus2 bmp280
```

> ⚠️ Python 3.13では `adafruit-circuitpython-bmp280` は非対応。`bmp280` ライブラリを使用する。

### Arduino IDE

1. Arduino IDE 2.x をインストール
2. ボードマネージャーで `ESP32 by Espressif Systems` を追加
3. 以下のライブラリをインストール

```text
Adafruit BMP280 Library
Arduino LoRa (Sandeep Mistry)
```

### Raspberry Pi（Phase 7）

```bash
# グローバル環境へインストール
pip install pyLoRa --break-system-packages

# venv環境
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

実施内容：

- 推力・バーンタイム・Cd変更
- A〜Cエンジン比較
- OpenRocketとの差異分析

---

### Phase 3：PID制御

```bash
cd phase3
python control.py
```

実施内容：

- 実機を使わずアルゴリズム挙動を理解
- Kp → Ki → Kd の順で試行
- 各パラメータの役割を体感

---

### Phase 4：センサー検証（Raspberry Pi）

#### 配線

```text
Raspberry Pi      センサー
3.3V      ─────── VCC
GND       ─────── GND
GPIO2 SDA ─────── SDA
GPIO3 SCL ─────── SCL
```

```bash
sudo raspi-config
# Interface Options → I2C → Enable

i2cdetect -y 1

cd phase4
python sensor_reader.py

scp <user>@<hostname>.local:~/tss_rocket_project/*.csv .

python plot_sensor.py
```

---

### Phase 4.5：ESP32移植検証

#### 配線

```text
ESP32             MPU-6050
3V3       ─────── VCC
GND       ─────── GND
GPIO32    ─────── SDA
GPIO33    ─────── SCL
```

実施内容：

- `arduino/phase5/sensor_verify/sensor_verify.ino` を書き込み
- `Wire.begin(32,33)` を確認
- シリアルモニタで加速度確認
- Raspberry Piとの差異・ノイズ検証

---

### Phase 5：リアルタイム制御ループ

実施内容：

- `arduino/phase5/sensor_reader/sensor_reader.ino` 書き込み
- `Wire.begin(32,33)` を確認
- roll角と制御出力確認

> ⚠️ `delay()` ではなく `millis()` 差分管理を使用する。

---

### Phase 6：フライトコンピュータ・安全設計

実施内容：

- `arduino/phase6/state_machine/state_machine.ino` を書き込み
- 静止 → 手で振る → APOGEE遷移確認
- APOGEE検出時にLED点灯確認

#### ステートマシン

```text
IDLE
 ↓
LAUNCHED
 ↓
COAST
 ↓
APOGEE
 ↓
DESCENT
 ↓
LANDED
```

#### パラシュート展開条件

- 発射確認済み
- 発射から2秒経過
- 一度以上上昇記録あり
- 頂点確認（高度2m以上低下）

---

### Phase 7：LoRaテレメトリ + リアルタイムダッシュボード

#### 配線（ESP32 → Ra-02）


| Ra-02   | ESP32  |
| ------- | ------ |
| VCC     | 3V3    |
| GND     | GND    |
| SCK     | GPIO18 |
| MOSI    | GPIO23 |
| MISO    | GPIO19 |
| NSS(CS) | GPIO15 |
| RST     | GPIO14 |
| DIO0    | GPIO26 |


#### 配線（Raspberry Pi → Ra-02）


| Ra-02   | Raspberry Pi  |
| ------- | ------------- |
| VCC     | Pin1（3.3V）    |
| GND     | Pin6          |
| SCK     | Pin23         |
| MOSI    | Pin19         |
| MISO    | Pin21         |
| NSS(CS) | Pin24         |
| RST     | Pin15（GPIO22） |
| DIO0    | Pin7（GPIO4）   |


#### 起動手順

```bash
# ターミナル1（Mac）
ssh -L 5000:localhost:5000 <user>@<hostname>.local

# ターミナル2（ラズパイ）
deactivate
python3 receiver.py

# ターミナル3（ラズパイ・venv）
source .venv/bin/activate
python3 dashboard.py
```

ブラウザ：

```text
http://localhost:5000
```

> ⚠️ pyLoRaはラズパイ5 GPIO非対応のため、receiver.pyはグローバルPythonで起動する。

---

### Phase 8：JAR飛行

実施内容：

- JARライセンス講習受講（Bタイプ）
- Estes製キット機体使用
- A8-3エンジン飛行
- パラシュート回収確認

---

## 開発ログ

### Phase 2

- 実機との差で最大高度13.6m
- 質量が3〜5倍重かった
- 修正後56.1mへ改善

### Phase 3

- Kd追加で制御が暴走
- ノイズを約100倍増幅
- Ki追加で微小ズレ発生
- シミュレーション上ではKpのみで収束

### Phase 4

- `adafruit-circuitpython-bmp280` がPython 3.13非互換
- `bmp280` に変更して解決

### Phase 5

- `delay()` 使用禁止
- `millis()` 差分管理が有効
- Kdはフィルタなしでは不安定

### Phase 6

- 地上振動で誤爆
- 発射確認フラグ必須
- ニクロム線はMOSFET経由で駆動

### Phase 7

- pyLoRaがRPi.GPIO非対応
- `rpi-lgpio` で解決
- RSTピン番号を混同しやすい
- SDカードとLoRaのSPI競合発生
- CSV保存を受信側へ移行
- 文字化けパケットは `pd.to_numeric(errors="coerce")` で除外

### Phase 8

- モデルロケットは受動安定中心
- 能動姿勢制御は別プロジェクトへ

---

## おわりに

本プロジェクトでは、フライトデータ収集・可視化システムを構築した。

一方で、能動姿勢制御については、モデルロケットの構造上その介入余地が少ないことも確認できた。

構築したデータ収集・可視化基盤は、今後のロケット開発や別プロジェクトにも応用していく。

---

## ライセンス

MIT