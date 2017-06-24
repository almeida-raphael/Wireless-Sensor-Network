import matplotlib.pyplot as plt
import numpy as np
from random import gauss, uniform

################################### DATA GEN FUNCTIONS ###################################
def genData(n, start=0, end=10):
    x = np.linspace(start, end, n)
    y = np.sin(10*x) - x*x
    return y

def genDataOsc(n):
    return np.array([1024 + (-2)**(-i/25) + gauss(0,2e-2) for i in range(n)])

def genDataRand(n):
    return np.random.randn(n) + 0.3*np.linspace(0, 10, n)
##########################################################################################

################################### COEF CAL FUNCTIONS ###################################
def calcCov(X, Y, xAvg, yAvg, convSum, dataSize):
    return (convSum + ((X - xAvg)*(Y - yAvg))) / (dataSize - 1 + 1e-10)

def angularCoef(X, Y, xAvg, yAvg, xyConvSum, xxConvSum, dataSize):
    return calcCov(X, Y, xAvg, yAvg, xyConvSum, dataSize) / (calcCov(X, X, xAvg, xAvg, xxConvSum, dataSize) + 1e-10)

def linearCoef(ang, xAvg, yAvg, dataSize):
    return yAvg - ang*xAvg
##########################################################################################

#################################### KALMAN FUNCTIONS ####################################
def kalmanCoef(estimate, measurement):
    return estimate / (estimate + measurement + 1e-10)

def kalmanFilter(estimate, measurement):
    return estimate + kalmanCoef(estimate, measurement) * (measurement - estimate)
##########################################################################################

##################################### DATA VARIABLES #####################################
count = 100
end = 100
time = np.linspace(0, end, count)
data = genDataOsc(count)
delta = end / count
##########################################################################################

##################################### PREDICTION VAR #####################################
predictions = []
finalPredictions = []
noData = []
##########################################################################################

##################################### SIMULATION VARS ####################################
errorProb = 0.5
##########################################################################################

######################################### VARS ###########################################
tSum = 0
dSum = 0
tdCovSum = 0
tVarSum = 0

dSumKG = 0
tdCovSumKG = 0
tVarSumKG = 0

a = 1
b = 0
aKG = 1
bKG = 0

dataSize = 0
decay = 0.5
decayCorrected = 0.4
##########################################################################################

count = 0

###################################### SIMULATIION #######################################
for t, d in zip(time, data):
    if(uniform(0,1) < errorProb): #didnt receive data
        count += 1
        predictions.append(a*t + b)
        finalPredictions.append(aKG*t + bKG)
        noData.append((t, aKG*t + bKG))
    else: #successfully received data
        dataSize += 1

        tSum += t
        dSum += d

        tAvg = tSum/dataSize
        dAvg = dSum/dataSize

        a = angularCoef(t, d, tAvg, dAvg, tdCovSum, tVarSum, dataSize)
        b = linearCoef(a, tAvg, dAvg, dataSize)

        predict = a*t + b
        corrected = kalmanFilter(predict, d)

        dSumKG += corrected

        dAvgKG = dSumKG/dataSize

        aKG = angularCoef(t, corrected, tAvg, dAvgKG, tdCovSumKG, tVarSum, dataSize)
        bKG = linearCoef(aKG, tAvg, dAvgKG, dataSize)

        tdCovSum = (tdCovSum*decay) + ((t - tAvg)*(d - dAvg))
        tdCovSumKG = (tdCovSumKG*decayCorrected) + ((t - tAvg)*(corrected - dAvgKG))
        tVarSum = (tVarSum*decay) + (t - tAvg)**2

        finalPrediction = aKG*t + bKG

        predictions.append(predict)
        finalPredictions.append(finalPrediction)
##########################################################################################

print(len(noData)/dataSize)

######################################## PLOT ############################################
plt.plot(time, data, label="Medidas")
plt.plot(time, predictions, label="Previsões")
plt.plot(time, finalPredictions, label="Previsões Corrigidas")
plt.scatter(list(list(zip(*noData))[0]), list(list(zip(*noData))[1]), label="Erros de Transfência", color="black")
plt.title("Previsões com {}% de taxa de erro".format(errorProb * 100))
plt.xlabel("Tempo")
plt.ylabel("Temperatura")
plt.legend()
plt.show()
##########################################################################################
