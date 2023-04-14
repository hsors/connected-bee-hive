#!/usr/bin/python
# -*- coding: utf-8 -*-

# script needed to set the time of a node
# listen directory /home/pi/Documents/hso-scale0x (hso-scale0x is a parameter) 
# If a file is created in it (file name will alsways be time.dat, as per getTime.py script), will open this file and send a downlink
# to the node in order to set its time with the content of file time.dat
# syntax example: python setTime.py hso-energysaver

import pyinotify
import pickle
import os
import sys
import time
import paho.mqtt.client as mqtt
from base64 import b64encode

# the programme needs to be launched with one argument: setTime.py deviceId
deviceId = sys.argv[1]

USER = "ardbeescale-af01@ttn"
PASSWORD = "XXXXXXXXXXXXXXXXXXXXX"
PUBLIC_ADDRESS = "eu1.cloud.thethings.network"
PUBLIC_ADDRESS_PORT = 1883


wm = pyinotify.WatchManager()  # Watch Manager
mask = pyinotify.IN_DELETE | pyinotify.IN_CREATE | pyinotify.IN_CLOSE_WRITE # watched events

class EventHandler(pyinotify.ProcessEvent):
    def process_IN_CLOSE_WRITE(self, event):
        filePath = event.pathname
        myFile = open(filePath, 'rb')
        timeList = pickle.load(myFile)
        print(timeList)
        myFile.close()
        os.remove("/home/pi/Documents/"+deviceId+"/time.dat")
        rtcHour0x = timeList[0]
        rtcMn0x = timeList[1]
        rtcSec0x = timeList[2]
        print("Time (hexa) retrieved is ", timeList)
        print("Create new mqtt client instance")
        mqttClient = mqtt.Client()
        # Setup authentication from settings above
        mqttClient.username_pw_set(USER, PASSWORD)
        print("Connecting to broker: " + PUBLIC_ADDRESS + ":" + str(PUBLIC_ADDRESS_PORT))
        mqttClient.connect(PUBLIC_ADDRESS, PUBLIC_ADDRESS_PORT, 60) # keep alive 60s
        print("waiting 30s before sending the downlink....")
        time.sleep(30)
        QOS = 0
        topic = "v3/" + USER + "/devices/" + deviceId + "/down/push"
        print("Publishing message to topic " + topic + " with QoS = " + str(QOS))
        hexadecimal_payload = rtcHour0x+rtcMn0x+rtcSec0x
        fport = 100
        # Convert hexadecimal payload to base64
        b64 = b64encode(bytes.fromhex(hexadecimal_payload)).decode()
        print('Convert hexadecimal_payload: ' + hexadecimal_payload + ' to base64: ' + b64 + '  Hexa payload:' + hexadecimal_payload)
        msg = '{"downlinks":[{"f_port":' + str(fport) + ',"frm_payload":"' + b64 + '","priority": "NORMAL"}]}'
        result = mqttClient.publish(topic, msg, QOS)

        status = result[0]
        if status == 0:
            print("Sent " + msg + " to topic " + topic)
        else:
            print("Failed to send message to topic " + topic)

        print("\nExit")
        sys.exit(0)
        

dirToWatch = "/home/pi/Documents/"+deviceId
handler = EventHandler()
notifier = pyinotify.Notifier(wm, handler)
wdd = wm.add_watch(dirToWatch, mask, rec=True)

notifier.loop()





