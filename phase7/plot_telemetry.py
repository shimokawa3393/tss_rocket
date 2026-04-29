import pandas as pd
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt

df = pd.read_csv('../data/telemetry_20260429_161234.csv')
df = df.apply(pd.to_numeric, errors='coerce')
df = df.dropna()
df['time_s'] = df['time_ms'] / 1000.0

fig, axes = plt.subplots(3, 1, figsize=(10, 10))
fig.suptitle('TSS-550 Phase 7 Telemetry', fontsize=14)

axes[0].plot(df['time_s'], df['alt'], color='royalblue')
axes[0].set_ylabel('高度 [m]')
axes[0].grid(True)

axes[1].plot(df['time_s'], df['ax'], label='ax', color='red')
axes[1].plot(df['time_s'], df['ay'], label='ay', color='green')
axes[1].plot(df['time_s'], df['az'], label='az', color='blue')
axes[1].set_ylabel('加速度 [g]')
axes[1].axhline(0, color='black', linewidth=0.5)
axes[1].legend()
axes[1].grid(True)

axes[2].plot(df['time_s'], df['temp'], color='orange')
axes[2].set_ylabel('気温 [℃]')
axes[2].set_xlabel('時間 [s]')
axes[2].grid(True)

plt.tight_layout()
plt.savefig('..data/telemetry_plot.png', dpi=150)
print("保存完了: telemetry_plot.png")