from flask import Flask, render_template_string
from flask_sock import Sock
from SX127x.LoRa import *
from SX127x.board_config import BOARD
import threading
import json

app = Flask(__name__)

HTML = '''
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <title>TSS-550 Telemetry</title>
    <style>
        body { background: #0a0a0a; color: #00ff88; font-family: monospace; padding: 20px; }
        h1 { color: #00aaff; }
        .data { font-size: 2em; margin: 10px 0; }
        .label { color: #888; font-size: 0.6em; }
    </style>
</head>
<body>
    <h1>🚀 TSS-550 Telemetry</h1>
    <div class="data"><span class="label">高度</span><br><span id="alt">--</span> m</div>
    <div class="data"><span class="label">ax</span><br><span id="ax">--</span> g</div>
    <div class="data"><span class="label">ay</span><br><span id="ay">--</span> g</div>
    <div class="data"><span class="label">az</span><br><span id="az">--</span> g</div>
    <div class="data"><span class="label">気温</span><br><span id="temp">--</span> ℃</div>
    <div class="data"><span class="label">時刻</span><br><span id="time">--</span> ms</div>
<script>
    const es = new EventSource('/stream');
    es.onmessage = (e) => {
        const d = JSON.parse(e.data);
        document.getElementById('alt').textContent = d.alt;
        document.getElementById('ax').textContent = d.ax;
        document.getElementById('ay').textContent = d.ay;
        document.getElementById('az').textContent = d.az;
        document.getElementById('temp').textContent = d.temp;
        document.getElementById('time').textContent = d.time;
    };
</script>
</body>
</html>
'''

latest = {}

@app.route('/')
def index():
    return render_template_string(HTML)

@app.route('/stream')
def stream():
    def generate():
        import time
        last = {}
        while True:
            if latest != last:
                last = latest.copy()
                yield f"data: {json.dumps(last)}\n\n"
            time.sleep(0.5)
    return app.response_class(generate(), mimetype='text/event-stream')

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
            latest.update({'time': t, 'alt': alt, 'ax': ax, 'ay': ay, 'az': az, 'temp': temp})
            print(f"受信: {raw}")
        except:
            print("パースエラー:", raw)
        self.set_mode(MODE.RXCONT)

def lora_thread():
    BOARD.setup()
    lora = Receiver()
    lora.set_mode(MODE.RXCONT)
    while True:
        pass

threading.Thread(target=lora_thread, daemon=True).start()

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=5000)