from math import sin
from random import gauss
import numpy as np
import matplotlib.pyplot as plt
from pykalman import KalmanFilter

cov_noise = 4e-2
decay = 0.9


# Ground-truth state
def x(t):
    return sin(t)


# Noisy data
def z(x):
    return x + gauss(0, cov_noise)


def kg_coef(est, measurement):
    return est / (est + measurement)


def kg_iter(prev, measurement):
    return prev + kg_coef(prev, measurement) * (measurement - prev)

T = np.linspace(0, 10, 101)
X = np.array([1024 + (-2) ** (-i / 25) for i in range(101)])
Z = np.fromiter(map(z, X), "float")
P = []
P_kg = [Z[0]]

T_bar = T[0]
Z_bar = Z[0]
cov_TZ = 0
measurements = []
prediction = []
P_new_kg = []

kf = KalmanFilter(initial_state_mean=0, n_dim_obs=2)

for t, z in zip(T, Z):
    kg_prediction_thales = z
    cov_TZ_temp = cov_TZ
    for i in range(0, 2):
        cov_TZ_temp = decay ** 2 * cov_TZ - decay * (1 - decay) * (T_bar * kg_prediction_thales + t * Z_bar) - (1 - decay) ** 2 * t * kg_prediction_thales
        T_bar_temp = decay * T_bar + (1 - decay) * t
        Z_bar_temp = decay * Z_bar + (1 - decay) * kg_prediction_thales

        if len(P) == 0:
            P.append(kg_prediction_thales)
            continue

        beta_hat = T_bar_temp * Z_bar_temp / cov_TZ_temp
        alpha_hat = Z_bar_temp - beta_hat * T_bar_temp

        p = alpha_hat + beta_hat * t
        kg_prediction_thales = kg_iter(p, z)

    new_z = kg_prediction_thales

    cov_TZ = decay ** 2 * cov_TZ - decay * (1 - decay) * (T_bar * z + t * Z_bar) - (1 - decay) ** 2 * t * new_z
    T_bar = decay * T_bar + (1 - decay) * t
    Z_bar = decay * Z_bar + (1 - decay) * new_z

    if len(P) == 0:
        P.append(new_z)
        continue

    beta_hat = T_bar * Z_bar / cov_TZ
    alpha_hat = Z_bar - beta_hat * T_bar

    p = alpha_hat + beta_hat * t
    kg_prediction_thales = kg_iter(p, new_z)
    new_pkg = kg_prediction_thales
    for i in range(0, 6):
        new_pkg = kg_iter(new_pkg, new_z)

    P_new_kg.append(new_pkg)
    P.append(p)
    P_kg.append(kg_prediction_thales)

plt.plot(X, label="Ground truth")
plt.plot(Z, label="Measurement")
plt.plot(P, label="Prediction")
#plt.plot(P_kg, label="Metodo Thales With Kalman Filter Bundao")
plt.plot(P_new_kg, label="Metodo Thales With Kalman Filter repetido")

plt.legend()
plt.show()
