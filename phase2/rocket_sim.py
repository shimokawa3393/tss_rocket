import numpy as np
import matplotlib.pyplot as plt

# ---- パラメータ ----
dt = 0.01                 # 時間刻み [s]
t_max = 30.0              # シミュレーション時間 [s]

# ロケット諸元
mass_wet = 0.12           # 全備質量 [kg]
mass_dry = 0.10           # 乾燥質量（燃料燃焼後）[kg]
drag_coeff = 0.75         # 抗力係数
cross_area = 0.0007       # 断面積 [m²]（直径40mmなら約0.0014）
air_density = 1.225       # 空気密度 [kg/m³]
gravity = 9.81            # 重力加速度 [m/s²]

# 推力カーブ（簡易：バーンタイム中は一定推力）
thrust_force = 8.5       # 推力 [N]
burn_time = 0.5           # バーンタイム [s]（A8-3のバーンタイム）

# 推力関数: バーンタイム中は一定推力
def get_thrust(t):
    return thrust_force if t < burn_time else 0.0

# 質量関数: バーンタイム中は質量が減少
def get_mass(t):
    if t < burn_time:
        return mass_wet - (mass_wet - mass_dry) * (t / burn_time)
    return mass_dry

# ---- シミュレーションループ ----
t_list, alt_list, vel_list, acc_list = [], [], [], []
alt, vel = 0.0, 0.0

for t in np.arange(0, t_max, dt):
    mass   = get_mass(t)
    thrust = get_thrust(t)
    drag   = 0.5 * drag_coeff * air_density * cross_area * vel**2 * np.sign(vel)
    acc    = (thrust - mass * gravity - drag) / mass

    vel += acc * dt
    alt += vel * dt

    if alt < 0 and t > 1.0:
        break

    t_list.append(t)
    alt_list.append(alt)
    vel_list.append(vel)
    acc_list.append(acc)

print(f"最大高度: {max(alt_list):.1f} m")
print(f"最大速度: {max(vel_list):.1f} m/s")

fig, axes = plt.subplots(3, 1, figsize=(8, 9))
axes[0].plot(t_list, alt_list); axes[0].set_ylabel("Altitude [m]")
axes[1].plot(t_list, vel_list); axes[1].set_ylabel("Velocity [m/s]")
axes[2].plot(t_list, acc_list); axes[2].set_ylabel("Acceleration [m/s²]")
for ax in axes:
    ax.grid(True); ax.set_xlabel("Time [s]")
plt.tight_layout()
plt.savefig("../data/flight_profile.png")
plt.show()
