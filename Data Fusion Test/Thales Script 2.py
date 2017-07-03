from math import sin
from random import gauss
import numpy as np
import matplotlib.pyplot as plt

d = 0.99

# Ground-truth state
def x(t):
    return sin(t)


# Noisy data
def z(x):
    return x + gauss(0, 5e-2)


def cov_with_decay(cov_XY, X_bar, Y_bar, x, y, d):
    return d ** 2 * cov_XY - d * (1 - d) * (X_bar * y + x * Y_bar) - (1 - d) ** 2 * x * y


def avg_with_decay(X_bar, x, d):
    return d * X_bar + (1 - d) * x


def kg_coef(est, measurement):
    return est / (est + measurement)


def kg_iter(prev, measurement):
    return prev + kg_coef(prev, measurement) * (measurement - prev)

T = np.linspace(0, 10, 101)
X = np.fromiter(map(x, T), "float")
Z = np.fromiter(map(z, X), "float")
P = []
P_kg = []

T_bar = T[0]
Z_bar = Z[0]
cov_TZ = 0

for t, z in zip(T, Z):
    cov_TZ_ = cov_with_decay(cov_TZ, T_bar, Z_bar, t, z, d)
    T_bar_ = avg_with_decay(T_bar, t, d)
    Z_bar_ = avg_with_decay(Z_bar, z, d)

    if len(P) == 0:
        P.append(z)
        cov_TZ = cov_TZ_
        T_bar = T_bar_
        Z_bar = Z_bar_
        continue

    beta_hat = T_bar_ * Z_bar_ / cov_TZ_
    alpha_hat = Z_bar_ - beta_hat * T_bar_

    p_bar = alpha_hat + beta_hat * t
    x_bar = kg_iter(p_bar, z)
    P.append(x_bar)

    cov_TZ = cov_with_decay(cov_TZ, T_bar, Z_bar, t, x_bar, d)
    T_bar = T_bar_
    Z_bar = avg_with_decay(Z_bar, x_bar, d)
    P_kg.append(x_bar)


plt.plot(X, label="Ground truth")
plt.plot(Z, label="Measurement")
plt.plot(P, label="Prediction")
plt.plot(P_kg, label="Prediction W/ Kalman")
plt.legend()
plt.show()
