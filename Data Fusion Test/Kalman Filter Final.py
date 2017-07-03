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



def iterativeCalcCovariance(X, Y, X_avg, Y_avg, conv_sum, data_size):
    return (conv_sum + ((X - X_avg)*(Y - Y_avg))) / (data_size - 1)

def iterativeAngularCoef(X, Y, X_sum, Y_sum, XY_conv_sum, XX_conv_sum, data_size):
    X_avg = X_sum/data_size
    Y_avg = Y_sum/data_size
    return iterativeCalcCovariance(X, Y, X_avg, Y_avg, XY_conv_sum, data_size) / iterativeCalcCovariance(X, X, X_avg, X_avg, XX_conv_sum, data_size)

def iterativeLinearCoef(a, X_sum, Y_sum, data_size):
    X_avg = X_sum/data_size
    Y_avg = Y_sum/data_size
    return Y_avg - a*X_avg




def kalmanCoeficient(est, measurement):
    return est / (est + measurement)

def iterativeKalmanFilter(prev, measurement):
    return prev + kalmanCoeficient(prev, measurement) * (measurement - prev)




count = 100
end = 100
time = np.linspace(0, end, count)
data = gen_data(count)

delta = end / count

iterativePredictions = []
iterativeKalmanPrediction = []
kalmanMinQuad = []

X_sum = time[0]
Y_sum = data[0]
X_sum_kg = time[0]
Y_sum_kg = data[0]

XY_conv_sum = 0
XX_conv_sum = 0
XY_conv_sum_kg = 0
XX_conv_sum_kg = 0

data_size = 2
decay = 0.99

for i in range(1, count):
    #Update data sum
    X_sum += time[i-1]
    Y_sum += data[i-1]

    #Calculate AVG
    X_avg = X_sum/data_size
    Y_avg = Y_sum/data_size

    #Calculate angular and linear coeficient using iterative function
    iterativeAngularCoeficient = iterativeAngularCoef(time[i - 1], data[i - 1], X_sum, Y_sum, XY_conv_sum, XX_conv_sum, data_size)
    print(time[i - 1], data[i - 1], X_sum, Y_sum, XY_conv_sum, XX_conv_sum, data_size)
    iterativeLinearCoeficient = iterativeLinearCoef(iterativeAngularCoeficient, X_sum, Y_sum, data_size)

    #Update XY and XX conv sum
    XY_conv_sum = (XY_conv_sum*decay) + ((time[i-1] - X_avg)*(data[i-1] - Y_avg))
    XX_conv_sum = (XX_conv_sum*decay) + ((time[i-1] - X_avg)*(time[i-1] - X_avg))

    #Predict
    iterativePrediction = (time[i - 1] + delta) * iterativeAngularCoeficient + iterativeLinearCoeficient
    print("angular coef", iterativeAngularCoeficient)
    print("iterative pred", iterativePrediction)
    print("data", data[i-1])

    #Calculating prediction using Kalman Filter
    kalmanFilterIterativePrediction = iterativeKalmanFilter(iterativePrediction, data[i - 1])

    #Update data sum
    X_sum_kg += time[i-1]
    Y_sum_kg += kalmanFilterIterativePrediction

    #Calculate AVG
    X_avg_kg = X_sum_kg/data_size
    Y_avg_kg = Y_sum_kg/data_size

    #Calculate angular and linear coeficient using iterative function
    iterativeAngularCoeficient_kg = iterativeAngularCoef(time[i - 1], kalmanFilterIterativePrediction, X_sum_kg, Y_sum_kg, XY_conv_sum_kg, XX_conv_sum_kg, data_size)
    iterativeLinearCoeficient_kg = iterativeLinearCoef(iterativeAngularCoeficient_kg, X_sum_kg, Y_sum_kg, data_size)

    #Update XY and XX conv sum
    print("kalman", kalmanFilterIterativePrediction)
    print("XY cov", XY_conv_sum_kg)
    XY_conv_sum_kg = (XY_conv_sum_kg*decay) + ((time[i-1] - X_avg_kg)*(kalmanFilterIterativePrediction - Y_avg_kg))
    print("after", XY_conv_sum_kg)
    XX_conv_sum_kg = (XX_conv_sum_kg*decay) + ((time[i-1] - X_avg_kg)*(time[i-1] - X_avg_kg))

    #Predict
    iterativePrediction_kg = (time[i - 1] + delta) * iterativeAngularCoeficient_kg + iterativeLinearCoeficient_kg

    #Creating arrays of data to create Graph
    iterativePredictions.append(iterativePrediction)
    iterativeKalmanPrediction.append(kalmanFilterIterativePrediction)
    kalmanMinQuad.append(iterativePrediction_kg)

    #Update data size
    data_size += 1

print(XY_conv_sum_kg)
print(XX_conv_sum_kg)
print(kalmanMinQuad)

#calculate Min. Qua. line
minQuadLineWithDecay = time * iterativeAngularCoeficient + iterativeLinearCoeficient

#calculate kalman Min. Qua. line
minQuadLineWithDecay_kg = time * iterativeAngularCoeficient_kg + iterativeLinearCoeficient_kg

plt.scatter(time, data, label="Medições", color="#FF5850")
plt.scatter(time[1:], iterativePredictions, label="Est. Min. Quad. Iterativo com Decaimento", color="#1CB262")
plt.scatter(time[1:], iterativeKalmanPrediction, label="Est. Kalman Iterativo com Decaimento", color="#FF00C0")
plt.scatter(time[1:], kalmanMinQuad, label="Kalman Prediction", color="#C0FF00")
plt.plot(time, minQuadLineWithDecay, label="Min. Quad. Final Iterativo com Decaimento", color="#FFA136")
plt.plot(time, minQuadLineWithDecay_kg, label="Min. Quad. Kalman", color="#FF36A1")
plt.xlabel("Tempo")
plt.ylabel("Temperatura")
plt.title("Aproximação Por Kalman Filter Iterativo com Decaimento de %0.2f" %decay)
# Place a legend to the right of this smaller subplot.
plt.legend()

plt.show()