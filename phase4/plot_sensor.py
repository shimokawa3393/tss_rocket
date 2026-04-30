import pandas as pd
import matplotlib.pyplot as plt

df = pd.read_csv('../data/flight_log_20260410_203250.csv')

fig, axes = plt.subplots(4, 1, figsize=(10, 12))

axes[0].plot(df['time'], df['az'], label='az')
axes[0].set_ylabel('acceleration [g]') #加速度
axes[0].legend(); axes[0].grid()

axes[1].plot(df['time'], df['gx'], label='gx')
axes[1].plot(df['time'], df['gy'], label='gy')
axes[1].plot(df['time'], df['gz'], label='gz')
axes[1].set_ylabel('gyro [deg/s]') #ジャイロ
axes[1].legend(); axes[1].grid()

axes[2].plot(df['time'], df['temp'])
axes[2].set_ylabel('temperature [℃]') #温度
axes[2].grid()

axes[3].plot(df['time'], df['altitude'])
axes[3].set_ylabel('altitude [m]') #高度
axes[3].set_xlabel('time [s]') #時間
axes[3].grid()

plt.tight_layout()
plt.savefig('../data/sensor_plot.png')
plt.show()
