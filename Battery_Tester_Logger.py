#Description:
#This program saves data from Battery_Tester_Procedure.ino running on an arduino into a csv file
#
#What the user needs to do:
#Baud-rate and com port need to be specified for serial communication
#
#Comments:
#The csv file is named Battery_Tester_Logger_YYMMDD_HHMMSS.csv with the date and time at the start of the program
#The file is saved in the same folder as this code
#The program is based on following video:
#https://www.youtube.com/watch?v=Ercd-Ip5PfQ&t=1094s
#
#History of changes:
#
#Date: 26.09.2024
#Author: Julian Zoller

#Import libraries
import csv #for handling csv files
import serial #for serial communication
from datetime import datetime #for nomenclature or filename including date and time
now = datetime.now()

#Configure serial communication
Serial_Port = 'COM4' #Port to which Arduino is connected
BAUD_RATE = 9600 #Baud rate of communication
ser = serial.Serial(Serial_Port, BAUD_RATE) #open serial port

lasttime = 0 #used to ensure consecutive data points
fieldnames = ["t/s", "Cycles/1", "U/mV", "I/mA", "Q/As", "T/C"] #name of table header
Filename=now.strftime("Battery_Tester_Logger_%Y%m%d_%H%M%S.csv") #creating filename using strftime to include date and time
print(Filename)

#create csv file and writing header into it
with open(Filename, 'w', newline='') as csv_file:
    csv_writer = csv.DictWriter(csv_file, fieldnames=fieldnames)
    csv_writer.writeheader()

#there may be old datasets with high timestamps present in the serial communication line
#this while loop flushes the serial communication line
print("Cleared data")
while ser.in_waiting:
    line = ser.readline().decode('utf-8').strip() #decode signal from utf-8 and remove any leading or trailing whitespace
    print(line)

#this is the main loop which writes data into the csv file
print("Recorded data")
while True:

    line = ser.readline().decode('utf-8').strip() #decode signal from utf-8 and remove any leading or trailing whitespace
    sensorValues = line.split(', ') #Split line into a string using comma's as separator
    print(line)

    #exclude incomplete datasets for simpler plotting
    if(len(sensorValues)==6):
        #exclude too small timestamps for simpler plotting
        if(float(sensorValues[0])>lasttime):
            #write data to file
            with open(Filename, 'a', newline='') as csv_file:
                csv_writer = csv.DictWriter(csv_file, fieldnames=fieldnames)

                info = {
                        "t/s": sensorValues[0], #time t in seconds
                        "Cycles/1": sensorValues[1], #number of charge-discharge cycles
                        "U/mV": sensorValues[2], #battery voltage in mV
                        "I/mA": sensorValues[3], #shunt current in mA
                        "Q/As": sensorValues[4], #transfered charge in As
                        "T/C": sensorValues[5] #temperature in degree C
                }
                lasttime=float(sensorValues[0])
                csv_writer.writerow(info)
            
            


 