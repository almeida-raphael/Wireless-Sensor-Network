from mpl_toolkits.mplot3d import Axes3D
import matplotlib.pyplot as plt
from matplotlib import cm
from matplotlib.ticker import LinearLocator, FormatStrFormatter
import numpy as np

def divide(a):
    return float(a[0]) / float(a[1])

def getDataTypeFromLogPart(logpart, terminalChar = "\n"):
    data = {
        "lights1": list(map(lambda x: x.split("Light 1: ")[1].split(" -")[0], logpart)),
        "lights2": list(map(lambda x: x.split("Light 2: ")[1].split(" -")[0], logpart)),
        "energy": list(map(lambda x: x.split("Energy Consumption: ")[1].split(" -")[0], logpart)),
        "time": list(map(lambda x: int(x.split("Time: ")[1].split(terminalChar)[0]), logpart)),
    }
    data = {
        "lights1": list(map(lambda x: divide(x.split("/")), data["lights1"])),
        "lights2": list(map(lambda x: divide(x.split("/")), data["lights2"])),
        "energy": list(map(lambda x: divide(x.split("/")), data["energy"])),
        "time": list(map(lambda x: x-(x%10), data["time"]))
    }
    return data

def getDataTypeFromLogPartUpdate(logpart, terminalChar = " -"):
    data = {
        "lights1": list(map(lambda x: x.split("Light 1: ")[1].split(" -")[0], logpart)),
        "lights2": list(map(lambda x: x.split("Light 2: ")[1].split(" -")[0], logpart)),
        "energy": list(map(lambda x: x.split("Energy Consumption: ")[1].split(" -")[0], logpart)),
        "time": list(map(lambda x: x.split("Time: ")[1].split(terminalChar)[0], logpart)),
    }
    data = {
        "lights1": list(map(lambda x: divide(x.split("/")), data["lights1"])),
        "lights2": list(map(lambda x: divide(x.split("/")), data["lights2"])),
        "energy": list(map(lambda x: divide(x.split("/")), data["energy"])),
        "time": list(map(lambda x: int(divide(x.split("/"))), data["time"]))
    }
    data["time"] = list(map(lambda x: x, data["time"]))
    return data

def avgDataType(data):
    maxLen = 0
    for run in data:
        if len(run["time"]) > maxLen:
            maxLen = len(run["time"])

    for run in data:
        for key in run:
            run[key] = np.array([0]*(maxLen - len(run[key])) + run[key])

    struct = {
        "lights1": np.zeros(maxLen),
        "lights2": np.zeros(maxLen),
        "energy": np.zeros(maxLen),
        "time": np.zeros(maxLen)
    }

    for run in data:
        for key in run:
            struct[key] += run[key]

    for key in struct:
        struct[key] /= len(data)

    return struct

def buildChart(Y, X, Y_update, X_update):
    #plt.plot(X, Y[-1][:], label="Prev. Consumo de Energia")
    plt.plot(X_update, Y_update[-1][:], label="Medição Consumo")
    plt.plot(X_update, np.ones(X_update.shape[0])*np.average(Y_update[-1][:]), label="Consumo Médio: 32.767mW")
    print(np.average(Y_update[-1][:]));

    plt.legend(bbox_to_anchor=(1.00, 1.00))

    plt.show()

avg_data_pred = []
avg_data_update = []
for txRate in range(10):
    parsed_data_pred = []
    parsed_data_update = []
    for testNum in range(30):
        file = "./data/"+str((txRate+1)*10)+"%/log_1_"+str(testNum+1)+".txt"
        pred_log = []
        update_log = []
        with open(file, "r") as f:
            for line in f.readlines():
                if("Prediction" in line):
                    pred_log.append(line)
                if("Model Updated - " in line):
                    update_log.append(line)
        parsed_data_pred.append(getDataTypeFromLogPart(pred_log))
        parsed_data_update.append(getDataTypeFromLogPartUpdate(update_log))
    avg_data_pred.append(avgDataType(parsed_data_pred))
    avg_data_update.append(avgDataType(parsed_data_update))

maxLen = 0
for txRate in avg_data_update:
    if txRate["time"].shape[0] > maxLen:
        maxLen = txRate["time"].shape[0]

for txRate in avg_data_update:
    for key in txRate:
        txRate[key] = np.array([0]*(maxLen - txRate[key].shape[0]) + list(txRate[key]))

avg_data_update = np.array(avg_data_update)

maxLen = 0
for txRate in avg_data_pred:
    if txRate["time"].shape[0] > maxLen:
        maxLen = txRate["time"].shape[0]

for txRate in avg_data_pred:
    for key in txRate:
        txRate[key] = np.array([0]*(maxLen - txRate[key].shape[0]) + list(txRate[key]))

avg_data_pred = np.array(avg_data_pred)

YE_update = []
for txRate in avg_data_update:
    YE_update.append(list(txRate["energy"]))

YE_update = np.array(YE_update)

X_update = avg_data_update[-1]["time"]

YE = []
for txRate in avg_data_pred:
    YE.append(list(txRate["energy"]))

YE = np.array(YE)

X = avg_data_pred[-1]["time"]

buildChart(YE, X, YE_update, X_update)