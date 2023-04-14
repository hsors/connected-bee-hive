#!/usr/bin/python3

# script needed to set the time of a node.
# Listen to MQQT messages emitted by a scale. Get the "received_at" time and store it on the disk
# so that another script (setTime.py) can set the time of this scale

# The programme needs to be launched with 3 arguments: python getTime.py deviceId measureIntervalMn

import os
import sys
import paho.mqtt.client as mqtt
import json
import pickle
from os.path import exists

DEBUG = False

deviceId = sys.argv[1]
measureIntervalMn = int(sys.argv[2])

directory = "/home/pi/Documents/"+deviceId
if not exists(directory):
    os.makedirs(directory)
filePath=directory+"/time.dat" # this is the file in which the time to send to the scale will be stored

#
USER = "ardbeescale-af01@ttn"
PASSWORD = "XXXXXXXXXXXXX"
PUBLIC_ADDRESS = "eu1.cloud.thethings.network"
PUBLIC_ADDRESS_PORT = 1883

QOS = 0

# Write uplink to tab file
def save_to_file(filePath,hour0x, mn0x, sec0x):
            listToStore = [hour0x, mn0x, sec0x]
            # store data
            myFile = open(filePath, 'wb')
            pickle.dump(listToStore, myFile)
            myFile.close()


def decimalToHex(number):
    if number <16:
        number0x = "0"+hex(number)[2:4].upper()
    else:
        number0x = hex(number)[2:4].upper()
    return number0x

def get_value_from_json_object(obj, key):
    try:
        return obj[key]
    except KeyError:
        return '-'

def stop(client):
    client.disconnect()
    print("\nExit")
    sys.exit(0)

# Get current time from the uplink message
def getUplinkTime(some_json):
    end_device_ids = some_json["end_device_ids"]
    device_id = end_device_ids["device_id"]
    application_id = end_device_ids["application_ids"]["application_id"]
    received_at = get_value_from_json_object(some_json, "received_at") # pas sûr de comprendre pourquoi on ne fait pas plutôt received_at = some_json["received_at"]
    print("end_device_ids=",end_device_ids)
    print("device_id=",device_id)
    print("application_id=",application_id)
    print("received_at=",received_at)
    hour = int(received_at[11:13])
    mn = int(received_at[14:16])
    sec = int(received_at[17:19])
    print("time stored in time.dat file (received from TTN in uplink 'received_at') is ",hour,"H",mn,"MN",sec,"SEC")
    return hour,mn,sec
            
# The callback for when the client receives a CONNACK response from the server.
def on_connect(client, userdata, flags, rc):
    if rc == 0:
        print("\nConnected successfully to MQTT broker")
        QOS = 0
        print("Subscribe to "+deviceId+" topics with QoS = " + str(QOS))
        #mqttc.subscribe("#", QOS)
        topic= "v3/ardbeescale-af01@ttn/devices/"+deviceId+"/up"
        mqttc.subscribe(topic, QOS) # mettre le signe "#" pour s'abonner à tous les noeuds (topics)

    else:
        print("\nFailed to connect, return code = " + str(rc))

# The callback for when a PUBLISH message is received from the server.
def on_message(client, userdata, message):
    print("\nMessage received on topic '" + message.topic + "' with QoS = " + str(message.qos))
    parsed_json = json.loads(message.payload)
    hour, mn, sec = getUplinkTime(parsed_json)
    rtcSec = sec
    hoursBetweenMeasures = int(measureIntervalMn/60)
    mnBetweenMeasures = measureIntervalMn - 60*hoursBetweenMeasures
    rtcMn = (mn + mnBetweenMeasures)%60
    if (mn + mnBetweenMeasures)>=60:
        incrementalHour = 1
    else:
        incrementalHour = 0
    rtcHour = (hour + hoursBetweenMeasures + incrementalHour)%24
    rtcSec0x = decimalToHex(rtcSec)
    rtcMn0x  = decimalToHex(rtcMn)
    rtcHour0x= decimalToHex(rtcHour)
    save_to_file(filePath,rtcHour0x, rtcMn0x, rtcSec0x)
    stop(client)
    
def on_subscribe(client, userdata, mid, granted_qos):
    print("\nSubscribed with message id (mid) = " + str(mid) + " and QoS = " + str(granted_qos))

def on_disconnect(client, userdata, rc):
    print("\nDisconnected with result code = " + str(rc))

def on_log(client, userdata, level, buf):
    print("\nLog: " + buf)

# Generate client ID with pub prefix randomly
#client_id = f'python-mqtt-{random.randint(0, 1000)}'

print("Create new mqtt client instance")
mqttc = mqtt.Client()

print("Assign callback functions")
mqttc.on_connect = on_connect
mqttc.on_subscribe = on_subscribe
mqttc.on_message = on_message
mqttc.on_disconnect = on_disconnect
# mqttc.on_log = on_log  # Logging for debugging OK, waste

# Setup authentication from settings above
mqttc.username_pw_set(USER, PASSWORD)

print("Connecting to broker: " + PUBLIC_ADDRESS + ":" + str(PUBLIC_ADDRESS_PORT))
mqttc.connect(PUBLIC_ADDRESS, PUBLIC_ADDRESS_PORT, 60) # keep alive for 60s

print("And run forever")
try:
    mqttc.loop_forever()
except KeyboardInterrupt:
    stop(mqttc)
