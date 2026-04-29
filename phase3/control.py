import numpy as np
import matplotlib.pyplot as plt

dt = 0.01
t_max = 10.0

Kp, Ki, Kd = 2.0, 0.0, 0.0

target_angle = 0.0
angle = 10.0        # 初期傾き（外乱想定）
angle_rate = 0.0
integral = 0.0
prev_error = 0.0
angles, outputs = [], []

for t in np.arange(0, t_max, dt):
    noise = np.random.normal(0, 0.1)   # センサーノイズの模擬
    measured = angle + noise

    error = target_angle - measured
    integral += error * dt
    derivative = (error - prev_error) / dt

    output = Kp * error + Ki * integral + Kd * derivative
    output = np.clip(output, -10, 10)  # アクチュエータ限界

    angle_rate += output * dt * 0.5
    angle_rate *= 0.98
    angle += angle_rate * dt
    prev_error = error

    angles.append(angle)
    outputs.append(output)

plt.figure(figsize=(10, 5))
plt.subplot(2,1,1)
plt.plot(np.arange(0,t_max,dt), angles)
plt.axhline(target_angle, color='r', linestyle='--', label='target')
plt.ylabel("angle [deg]"); plt.legend(); plt.grid()
plt.subplot(2,1,2)
plt.plot(np.arange(0,t_max,dt), outputs)
plt.ylabel("control output"); plt.xlabel("time [s]"); plt.grid()
plt.tight_layout(); plt.savefig("../data/pid_response.png"); plt.show()