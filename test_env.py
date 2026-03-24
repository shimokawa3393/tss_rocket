import numpy as np
import matplotlib.pyplot as plt

t = np.linspace(0, 10, 100)
plt.plot(t, np.sin(t))
plt.title("環境確認OK")
plt.show()