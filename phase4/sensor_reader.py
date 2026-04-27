import smbus2
import time
import csv
from datetime import datetime
from bmp280 import BMP280

# BMP280設定
bus_bmp = smbus2.SMBus(1)
bmp280 = BMP280(i2c_dev=bus_bmp, i2c_addr=0x77)

# MPU-6050設定
MPU_ADDR = 0x68
bus = smbus2.SMBus(1)
bus.write_byte_data(MPU_ADDR, 0x6B, 0)

def read_raw(addr, reg):
    high = bus.read_byte_data(addr, reg)
    low  = bus.read_byte_data(addr, reg + 1)
    val = (high << 8) | low
    return val - 65536 if val > 32768 else val

def read_mpu6050():
    ax = read_raw(MPU_ADDR, 0x3B) / 16384.0
    ay = read_raw(MPU_ADDR, 0x3D) / 16384.0
    az = read_raw(MPU_ADDR, 0x3F) / 16384.0
    gx = read_raw(MPU_ADDR, 0x43) / 131.0
    gy = read_raw(MPU_ADDR, 0x45) / 131.0
    gz = read_raw(MPU_ADDR, 0x47) / 131.0
    return ax, ay, az, gx, gy, gz

filename = f"flight_log_{datetime.now().strftime('%Y%m%d_%H%M%S')}.csv"
with open(filename, 'w', newline='') as f:
    writer = csv.writer(f)
    writer.writerow(['time', 'ax', 'ay', 'az', 'gx', 'gy', 'gz', 'temp', 'pressure', 'altitude'])
    t0 = time.time()
    try:
        while True:
            t = time.time() - t0
            imu = read_mpu6050()
            temp = bmp280.get_temperature()
            pressure = bmp280.get_pressure()
            altitude = bmp280.get_altitude()
            writer.writerow([f"{t:.3f}"] + [f"{v:.4f}" for v in imu] + [f"{temp:.2f}", f"{pressure:.2f}", f"{altitude:.2f}"])
            print(f"t={t:.1f}s az={imu[2]:.2f}g temp={temp:.1f}℃ alt={altitude:.1f}m")
            time.sleep(0.01)
    except KeyboardInterrupt:
        print(f"\n保存完了: {filename}")