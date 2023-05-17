import serial
import datetime as dt
import matplotlib.pyplot as plt
import matplotlib.animation as animation
import numpy as np
import time

#random data

fig = plt.figure()
ax = fig.add_subplot(1, 1, 1)
t = []
x = []
y = []
z = []
save = []
yPos = 'center'
allow_change_y = True
xPos = 'center'
allow_change_x = True
zPos = 'center'
allow_change_z = True

def saveData(save):
    fileName = 'data.txt'
    f = open(fileName, 'w')
    for i in range(len(save)):
        f.write(str(save[i][0]) + ' ' + str(save[i][1]) + ' ' + str(save[i][2]) + ' ' + str(save[i][3]) + '\n')
    f.close()

def get_data(data):
    data = str(data)
    try:
        if data[2] == 'x':
            data = data.split(' ')
            x = int(data[1])
            y = int(data[3])
            z = int(data[5][:-5])
            return x, y, z
    except:
        pass
    return None, None, None

def determinePos(data, pastPos, allow_change=True, change_trigger=30000, reset_trigger=5000):
    newPos = pastPos
    if data >= change_trigger and allow_change:
        if pastPos == 'center':
            newPos = 'positive'
        elif pastPos == 'negative':
            newPos = 'center'
        allow_change = False
    elif data <= -change_trigger and allow_change:
        if pastPos == 'center':
            newPos = 'negative'
        elif pastPos == 'positive':
            newPos = 'center'
        allow_change = False
    elif abs(data) <= reset_trigger:
        allow_change = True
        
    return newPos, allow_change


def animate_x(i, startTime, t, x, y, z):
    global ser
    global yPos
    global allow_change_y
    global xPos
    global allow_change_x
    global zPos
    global allow_change_z
    for i in range(5):
        data = ser.readline()
        x_data, y_data, z_data = get_data(data)
        Time = time.time() - startTime
        if x_data != None:
            t.append(Time)
            x.append(x_data)
            y.append(y_data)
            z.append(z_data)
            save.append((x_data, y_data, z_data, Time))
            yPos, allow_change_y = determinePos(y_data, yPos, allow_change_y)
            xPos, allow_change_x = determinePos(x_data, xPos, allow_change_x)
            zPos, allow_change_z = determinePos(z_data, zPos, allow_change_z)

    t = t[-100:]
    x = x[-100:]
    y = y[-100:]
    z = z[-100:]

    ax.clear()
    ax.plot(t, x, color='red')
    ax.plot(t, y, color='blue')
    ax.plot(t, z, color='green')

    ax.text(0.1, 0.2, 'x: ' + str(x[-1]), transform=ax.transAxes)
    ax.text(0.1, 0.15, 'y: ' + str(y[-1]), transform=ax.transAxes)
    ax.text(0.1, 0.1, 'z: ' + str(z[-1]), transform=ax.transAxes)
    ax.text(0.1, 0.95, 'Y Loc: ' + yPos, transform=ax.transAxes)
    ax.text(0.1, 0.90, 'X Loc: ' + xPos, transform=ax.transAxes)
    ax.text(0.1, 0.85, 'Z Loc: ' + zPos, transform=ax.transAxes)

    ax.axhline(y=5000, color='black', linestyle='--')
    ax.axhline(y=-5000, color='black', linestyle='--')
    ax.axhline(y=30000, color='red', linestyle='--')
    ax.axhline(y=-30000, color='red', linestyle='--')
            

    ax.set_ylim(-40000, 40000)
    ax.legend(['x', 'y', 'z'], loc='upper right')
    ax.set_xlabel('Time (s)')
    ax.set_ylabel('Angular Velocity')
    ax.grid(True)

ser = serial.Serial(port = '/dev/tty.usbmodem143403', baudrate=9600, timeout=0.1)
startTime = time.time()
ani = animation.FuncAnimation(fig, animate_x, fargs=(startTime, t, x, y, z), interval=1)
plt.show()
ser.close()
saveData(save)