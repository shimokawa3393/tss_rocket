from SX127x.LoRa import *
from SX127x.board_config import BOARD
import csv
import json
from datetime import datetime

BOARD.setup()

filename = f"telemetry_{datetime.now().strftime('%Y%m%d_%H%M%S')}.csv"
f = open(filename, 'w', newline='')
writer = csv.writer(f)
writer.writerow(['time_ms', 'alt', 'ax', 'ay', 'az', 'temp'])
print(f"保存先: {filename}")

class Receiver(LoRa):
    def __init__(self, verbose=False):
        super().__init__(verbose)
        self.set_mode(MODE.SLEEP)
        self.set_freq(433.0)
        self.set_spreading_factor(9)

    def on_rx_done(self):
        self.clear_irq_flags(RxDone=1)
        payload = self.read_payload(nocheck=True)
        raw = bytes(payload).decode('utf-8', errors='ignore')
        try:
            t, alt, ax, ay, az, temp = raw.split(',')
            writer.writerow([t, alt, ax, ay, az, temp])
            f.flush()
            print(f"t={t}ms  高度={alt}m  ax={ax}g ay={ay}g az={az}g  気温={temp}℃")
            with open('/tmp/telemetry.json', 'w') as jf:
                json.dump({'time': t, 'alt': alt, 'ax': ax, 'ay': ay, 'az': az, 'temp': temp}, jf)
        except:
            print("パースエラー:", raw)
        self.set_mode(MODE.RXCONT)

lora = Receiver()
lora.set_mode(MODE.RXCONT)

try:
    while True:
        pass
except KeyboardInterrupt:
    lora.set_mode(MODE.SLEEP)
    BOARD.teardown()
    f.close()
    print(f"\n保存完了: {filename}")