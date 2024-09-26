#Description:
#This program plots data written into a csv file by Battery_Tester_Logger.py
#
#What the user needs to do:
#This program needs to be run in the same folder as Battery_Tester_Logger.py in order to find the csv file
#It is recommended to increase the actualization time interval of FuncAnimation if the dataset is large
#Do not rename the dataset created by Battery_Tester_Logger.py otherwise this code will not find it
#
#Comments:
#
#The program is based on following videos:
#https://www.youtube.com/watch?v=Ercd-Ip5PfQ&t=1094s
#https://fabianlee.org/2021/11/11/python-find-the-most-recently-modified-file-matching-a-pattern/
#
# History of changes:
#
#Date: 26.09.2024
#Author: Julian Zoller

#Import libraries
import pandas as pd
import matplotlib.pyplot as plt
from matplotlib.animation import FuncAnimation
import matplotlib.style as mplstyle
mplstyle.use('fast') #hopefully this accelerates rendering of diagrams
import os
import glob

#Search for latest data file
pattern=os.path.dirname(os.path.realpath(__file__))+"\Battery_Tester_Logger_*" #create string with path and start of filename of searched data file
files = list(filter(os.path.isfile, glob.glob(pattern))) #get list of files with filenames starting with \Battery_Tester_Logger_
files.sort(key=lambda x: os.path.getmtime(x)) # sort by modified time
lastfile = files[-1] #get last item in list (most recent file)
lastfileparts = lastfile.split("Battery_Tester_Logger_") #Split line into a string using comma's as separator
latest_logger_data="Battery_Tester_Logger_"+lastfileparts[1] #create string of latest data file
print("Latest logger data file found: ", latest_logger_data)

#function that reads data from csv file and plots it
def animate(i):
    data = pd.read_csv(latest_logger_data)
    #save read data in according variables
    timecounter = data['t/s']
    cycle_counter = data['Cycles/1']
    battery_voltage_av_last = data['U/mV']
    shunt_current_av_last = data['I/mA']
    coulomb_counter = data['Q/As']
    temp = data['T/C']

    #Plot data
    plt.cla()
    plt.subplot(2,2,1) #Zeilen, Spalten, Platzierung
    plt.plot(timecounter, battery_voltage_av_last,'b',lw=2, label='battery voltage') #plot the data
    plt.xlabel('t / s')
    plt.ylabel('battery voltage U / mV')
    plt.subplot(2,2,2) #Zeilen, Spalten, Platzierung
    plt.plot(timecounter, coulomb_counter,'g',lw=2, label='battery voltage') #plot the data
    plt.xlabel('t / s')
    plt.ylabel('transfered charge Q / As')
    plt.subplot(2,2,3) #Zeilen, Spalten, Platzierung
    plt.plot(timecounter, shunt_current_av_last,'r',lw=2, label='current') #plot the data
    plt.xlabel('t / s')
    plt.ylabel('current / mA')
    plt.subplot(2,2,4) #Zeilen, Spalten, Platzierung
    plt.plot(timecounter, temp,'m',lw=2, label='current') #plot the data
    plt.xlabel('t / s')
    plt.ylabel('temperature T / °C')    

ani = FuncAnimation(plt.gcf(), animate, interval=1000) #call animate function to read and plot data every 1000 ms (this value may be increased for big data-sets)

plt.show()