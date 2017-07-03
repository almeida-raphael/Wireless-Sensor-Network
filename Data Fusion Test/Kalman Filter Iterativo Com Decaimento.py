import matplotlib.pyplot as plt
import numpy as np

def gen_data(n, start=0, end=10):
    x = np.linspace(start, end, n)
    y = np.sin(10*x) - x*x
    return y

def gen_data_osc(n):
    return np.array([1024 + (-2)**(-i/25) for i in range(n)])

def gen_data_rand(n):
    return np.random.randn(n) + 0.3*np.linspace(0, 10, n)

def calc_cov(X, Y):
    return np.sum((X - np.average(X))*(Y - np.average(Y))) / (X.shape[0] - 1)

def angular_coef(X,Y):
    return calc_cov(X,Y)/calc_cov(X,X)

def linear_coef(a, X, Y):
    return np.average(Y) - a*np.average(X)

def kg_coef(est, measurement):
    return est / (est + measurement)

def kg_iter(prev, measurement):
    return prev + kg_coef(prev, measurement) * (measurement - prev)

def calc_cov_step(X, Y, X_avg, Y_avg, conv_sum, data_size):
    return (conv_sum + ((X - X_avg)*(Y - Y_avg))) / (data_size - 1)

def angular_coef_step(X, Y, X_sum, Y_sum, XY_conv_sum, XX_conv_sum, data_size):
    X_avg = X_sum/data_size
    Y_avg = Y_sum/data_size
    return calc_cov_step(X, Y, X_avg, Y_avg, XY_conv_sum, data_size)/calc_cov_step(X, X, X_avg, X_avg, XX_conv_sum, data_size)

def linear_coef_step(a, X_sum, Y_sum, data_size):
    X_avg = X_sum/data_size
    Y_avg = Y_sum/data_size
    return Y_avg - a*X_avg



count = 100
end = 100
time = np.linspace(0, end, count)
data = gen_data(count)

delta = end / count

preds = []
kg_preds = []
preds_step = []
kg_preds_step = []

X_sum = time[0]
Y_sum = data[0]

data_size = 2
XY_conv_sum = 0
XX_conv_sum = 0

decay = 0.99

for i in range(1, count):
    #Update data sum
    X_sum += time[i-1]
    Y_sum += data[i-1]

    #Calculate AVG
    X_avg = X_sum/data_size
    Y_avg = Y_sum/data_size

    #Calculate angular and linear coeficient using iterative function
    a_step = angular_coef_step(time[i-1], data[i-1], X_sum, Y_sum, XY_conv_sum, XX_conv_sum, data_size)
    b_step = linear_coef_step(a_step, X_sum, Y_sum, data_size)

    #Update XY and XX conv sum
    XY_conv_sum = (XY_conv_sum*(1-decay)) + ((time[i-1] - X_avg)*(data[i-1] - Y_avg))
    XX_conv_sum = (XX_conv_sum*(1-decay)) + ((time[i-1] - X_avg)*(time[i-1] - X_avg))

    #Calculate angular and linear coeficient using old function
    a = angular_coef(time[:i], data[:i])
    b = linear_coef(a, time[:i], data[:i])

    #Calculating prediction using Min. Quad.
    prediction = (time[i]+delta)*a + b
    prediction_step = (time[i-1]+delta)*a_step + b_step
    #Calculating prediction using Kalman Filter
    kg_prediction = kg_iter(prediction, data[i-1])
    kg_prediction_step = kg_iter(prediction_step, data[i-1])

    #Creating arrays of data to create Graph
    preds.append(prediction)
    kg_preds.append(kg_prediction)
    preds_step.append(prediction_step)
    kg_preds_step.append(kg_prediction_step)

    #Update data size
    data_size += 1

#calculate Min. Qua. line
minQuadLine = time*a + b

#calculate Min. Qua. line
minQuadLineWithDecay = time*a_step + b_step

plt.scatter(time, data, label="Medições", color="#FF5850")
plt.scatter(time[1:], preds, label="Est. Min. Quad.", color="#62B21C")
plt.scatter(time[1:], kg_preds, label="Est. Kalman", color="#C000FF")
plt.scatter(time[1:], preds_step, label="Est. Min. Quad. Iterativo com Decaimento", color="#1CB262")
plt.scatter(time[1:], kg_preds_step, label="Est. Kalman Iterativo com Decaimento", color="#FF00C0")
plt.plot(time, minQuadLine, label="Min. Quad. Final", color="#36A1FF")
plt.plot(time, minQuadLineWithDecay, label="Min. Quad. Final Iterativo com Decaimento", color="#FFA136")
plt.xlabel("Tempo")
plt.ylabel("Temperatura")
plt.title("Aproximação Por Kalman Filter Iterativo com Decaimento de %0.2f" %decay)
# Place a legend to the right of this smaller subplot.
plt.legend()

plt.show()