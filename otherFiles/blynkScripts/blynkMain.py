#!/usr/bin/python
# -*- coding: utf-8 -*-

# Code for handling the data to be displayed on the Blynk app or to be read from this app
import time
import BlynkLib
import blynkFunctions
import ardConf
import pickle
from os.path import exists
import os
import datetime

BLYNK_AUTH = 'XXXXXXXXXXXX'
blynk = BlynkLib.Blynk(BLYNK_AUTH)

HOLDMEASURESTIME = 1 # global variable specifying the nb of hours during which measures will be held (for hive manipulations) when requested

# In order to get value from smartphone, the code is the following one
# Register virtual pin handlers

# Virtual pin V1 to V6 are trigerring pump 1 to pump 6
@blynk.on("V1") # V1 goes from 0 to 1 to trigger pump 1
def v1_write_handler(value):
    if int(value[0]) == 1:
        currentTime = datetime.datetime.now().strftime("%H:%M:%S")
        print(currentTime,": now feeding scale01...")
        blynkFunctions.sendDownlink("feedScale01")
@blynk.on("V2") # V2 goes from 0 to 1 to trigger pump 2
def v2_write_handler(value):
    if int(value[0]) == 1:
        currentTime = datetime.datetime.now().strftime("%H:%M:%S")
        print(currentTime,": now feeding scale02...")
        blynkFunctions.sendDownlink("feedScale02")
@blynk.on("V3") # V3 goes from 0 to 1 to trigger pump 3
def v3_write_handler(value):
    if int(value[0]) == 1:
        currentTime = datetime.datetime.now().strftime("%H:%M:%S")
        print(currentTime,": now feeding scale03...")
        blynkFunctions.sendDownlink("feedScale03")
@blynk.on("V4") # V4 goes from 0 to 1 to trigger pump 4
def v4_write_handler(value):
    if int(value[0]) == 1:
        currentTime = datetime.datetime.now().strftime("%H:%M:%S")
        print(currentTime,": now feeding scale04...")
        blynkFunctions.sendDownlink("feedScale04")
@blynk.on("V5") # V5 goes from 0 to 1 to trigger pump 5
def v5_write_handler(value):
    if int(value[0]) == 1:
        currentTime = datetime.datetime.now().strftime("%H:%M:%S")
        print(currentTime,": now feeding scale05...")
        blynkFunctions.sendDownlink("feedScale05")
@blynk.on("V6") # V6 goes from 0 to 1 to trigger pump 6
def v6_write_handler(value):
    if int(value[0]) == 1:
        currentTime = datetime.datetime.now().strftime("%H:%M:%S")
        print(currentTime,": now feeding scale06...")
        blynkFunctions.sendDownlink("feedScale06")
        
# Virtual pin V7 to V12 are resetting "last down link" of respectively scale 01 to 06        
@blynk.on("V7") # V7 goes from 0 to 1 to reset last DL scale01
def v7_write_handler(value):
    if int(value[0]) == 1:
        currentTime = datetime.datetime.now().strftime("%H:%M:%S")
        print(currentTime,": now resetting last DL scale01...")
        blynkFunctions.sendDownlink("resetLastDL01")
@blynk.on("V8") # V8 goes from 0 to 1 to reset last DL scale02
def v8_write_handler(value):
    if int(value[0]) == 1:
        currentTime = datetime.datetime.now().strftime("%H:%M:%S")
        print(currentTime,": now resetting last DL scale02...")
        blynkFunctions.sendDownlink("resetLastDL02")
@blynk.on("V9") # V9 goes from 0 to 1 to reset last DL scale03
def v9_write_handler(value):
    if int(value[0]) == 1:
        currentTime = datetime.datetime.now().strftime("%H:%M:%S")
        print(currentTime,": now resetting last DL scale03...")
        blynkFunctions.sendDownlink("resetLastDL03")
@blynk.on("V10") # V10 goes from 0 to 1 to reset last DL scale04
def v10_write_handler(value):
    if int(value[0]) == 1:
        currentTime = datetime.datetime.now().strftime("%H:%M:%S")
        print(currentTime,": now resetting last DL scale04...")
        blynkFunctions.sendDownlink("resetLastDL04")
@blynk.on("V11") # V11 goes from 0 to 1 to reset last DL scale05
def v11_write_handler(value):
    if int(value[0]) == 1:
        currentTime = datetime.datetime.now().strftime("%H:%M:%S")
        print(currentTime,": now resetting last DL scale05...")
        blynkFunctions.sendDownlink("resetLastDL05")
@blynk.on("V12") # V12 goes from 0 to 1 to reset last DL scale06
def v12_write_handler(value):
    if int(value[0]) == 1:
        currentTime = datetime.datetime.now().strftime("%H:%M:%S")
        print(currentTime,": now resetting last DL scale06...")
        blynkFunctions.sendDownlink("resetLastDL06")

# V28 is the nb of hours during which there won't be any measure taken (hive manipulations)
@blynk.on("V28") 
def v28_write_handler(value):
    global HOLDMEASURESTIME
    HOLDMEASURESTIME = int(value[0])
    currentTime = datetime.datetime.now().strftime("%H:%M:%S")
    print(currentTime,": measures will be hold during ",HOLDMEASURESTIME,"Hours when HOLD MEASURE button will be pressed")
    
# V29 is the button triggerring the "hold measure" downlink    
@blynk.on("V29")
def v29_write_handler(value):
    global HOLDMEASURESTIME
    if int(value[0]) == 1:
        switcher = {
            1: "1H",
            2: "2H",
            3: "3H"
            }
        DL01="holdMeasures01-"+switcher[HOLDMEASURESTIME]
        DL02="holdMeasures02-"+switcher[HOLDMEASURESTIME]
        DL03="holdMeasures03-"+switcher[HOLDMEASURESTIME]
        DL04="holdMeasures04-"+switcher[HOLDMEASURESTIME]
        DL05="holdMeasures05-"+switcher[HOLDMEASURESTIME]
        DL06="holdMeasures06-"+switcher[HOLDMEASURESTIME]
        currentTime = datetime.datetime.now().strftime("%H:%M:%S")
        print(currentTime,": now holding the measure during ", HOLDMEASURESTIME," hours")
        blynkFunctions.sendDownlink(DL01)
        blynkFunctions.sendDownlink(DL02)
        blynkFunctions.sendDownlink(DL03)
        blynkFunctions.sendDownlink(DL04)
        blynkFunctions.sendDownlink(DL05)
        blynkFunctions.sendDownlink(DL06)

validScalesForWeatherData =list(ardConf.validScalesForWeatherData) # the is the list of scales transmitting valid weather information (ardConf.validScalesForWeatherData
# is a dictionnary defined in ardConf.py. This list is needed for the average temp, hum, pressure calculations.
avgTemp = 0
avgHum = 0
avgPressure = 0

while True:
    blynk.run()

    # following code is sending data to the smartphone via virtual pins
    application_id = ardConf.application_id
    fileName = ardConf.fileName
    for i in ardConf.listOfScales:
        device_id = i
        directory = ardConf.rootDirectory+application_id+"/"+device_id+"/"
        filePath = directory+fileName
        if exists(filePath):
            try:
                myFile = open(filePath,'rb')
            except:
                currentTime = datetime.datetime.now().strftime("%H:%M:%S")
                print(currentTime,": the file ",filePath," doesn't exist")
            try:
                uplinkList = pickle.load(myFile)
                currentTime = datetime.datetime.now().strftime("%H:%M:%S")
                print(currentTime,": uplinkList: ",uplinkList," (File:",filePath,")")
                if i in validScalesForWeatherData:
                    ardConf.validScalesForWeatherData[i]["prevT"] = ardConf.validScalesForWeatherData[i]["lastT"]
                    ardConf.validScalesForWeatherData[i]["lastT"] = uplinkList[3]
                    ardConf.validScalesForWeatherData[i]["prevH"] = ardConf.validScalesForWeatherData[i]["lastH"]
                    ardConf.validScalesForWeatherData[i]["lastH"] = uplinkList[4]
                    ardConf.validScalesForWeatherData[i]["prevP"] = ardConf.validScalesForWeatherData[i]["lastP"]
                    ardConf.validScalesForWeatherData[i]["lastP"] = uplinkList[5]
                    # to get the new avg temp, we substract from the previous one the previous temp of this device (divided by the number of devices transmitting temp)
                    # and we add the new temp measured by this device (still divided by the number of devices transmitting temp, in order to get a correct avg)
                    # same for humidity and pressure
                    avgTemp = avgTemp + (ardConf.validScalesForWeatherData[i]["lastT"] - ardConf.validScalesForWeatherData[i]["prevT"])/len(validScalesForWeatherData)
                    avgHum = avgHum + (ardConf.validScalesForWeatherData[i]["lastH"] - ardConf.validScalesForWeatherData[i]["prevH"])/len(validScalesForWeatherData)
                    avgPressure = avgPressure + (ardConf.validScalesForWeatherData[i]["lastP"] - ardConf.validScalesForWeatherData[i]["prevP"])/len(validScalesForWeatherData)
                #print("File:",filePath," uplinkList:",uplinkList)
                myFile.close()
                os.remove(filePath)
                blynk.virtual_write(13+ardConf.listOfScales.index(device_id),uplinkList[1]) # last downlink
                blynk.virtual_write(19+ardConf.listOfScales.index(device_id),uplinkList[0]) # received_at
                blynk.virtual_write(25,avgTemp) # average temperature
                blynk.virtual_write(26,avgHum) # average humidity
                blynk.virtual_write(27,avgPressure) # average pressure
            except:
                currentTime = datetime.datetime.now().strftime("%H:%M:%S")
                print(currentTime,": there was an error in 'pickle.load'() instruction above, file ",filePath)
        
    # get12V battery voltage
    device_id = ardConf.otherDevices[0]
    directory = ardConf.rootDirectory+application_id+"/"+device_id+"/"
    filePath = directory+fileName
    if exists(filePath):
        try:
            myFile = open(filePath,'rb')
        except:
            currentTime = datetime.datetime.now().strftime("%H:%M:%S")
            print(currentTime,": the file ",filePath," doesn't exist")
        try:
            uplinkList = pickle.load(myFile)
            currentTime = datetime.datetime.now().strftime("%H:%M:%S")
            print(currentTime,": uplinkList: ",uplinkList," (File:",filePath,")")
            blynk.virtual_write(0,uplinkList[2]) # 12V battery Voltage
            myFile.close()
            os.remove(filePath)
        except:
            currentTime = datetime.datetime.now().strftime("%H:%M:%S")
            print(currentTime,": there was an error in 'pickle.load'() instruction above, file ",filePath) # I have no clue why this error (very rarely) happens
