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
    f, axarr = plt.subplots(3, sharex=True)
    pos = 0
    color = 0xFF0000
    for i in range(Y.shape[0]):
        if i == 4:
            pos +=1
        if i == 7:
            pos +=1
        axarr[pos].plot(X, Y[i][:], color="#%x" % (color), label="%i%% TX Sucedida"%((i+1)*10))
        axarr[pos].scatter(X_update, Y_update[i][:], color="#%x" % (color), label="Medições @%i%%"%((i+1)*10))
        color += 0x000019
        color -= 0x190000

    axarr[0].legend(bbox_to_anchor=(1.00, 1.00))
    axarr[1].legend(bbox_to_anchor=(1.00, 1.00))
    axarr[2].legend(bbox_to_anchor=(1.00, 1.00))

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

YL1_update = []
YL2_update = []
YE_update = []
for txRate in avg_data_update:
    YL1_update.append(list(txRate["lights1"]))
    YL2_update.append(list(txRate["lights2"]))
    YE_update.append(list(txRate["energy"]))

YL1_update = np.array(YL1_update)
YL2_update = np.array(YL2_update)
YE_update = np.array(YE_update)

X_update = avg_data_update[-1]["time"]

YL1 = []
YL2 = []
YE = []
for txRate in avg_data_pred:
    YL1.append(list(txRate["lights1"]))
    YL2.append(list(txRate["lights2"]))
    YE.append(list(txRate["energy"]))

YL1 = np.array(YL1)
YL2 = np.array(YL2)
YE = np.array(YE)

X = avg_data_pred[-1]["time"]

buildChart(YL1, X, YL1_update, X_update)
buildChart(YL2, X, YL2_update, X_update)
buildChart(YE, X, YE_update, X_update)