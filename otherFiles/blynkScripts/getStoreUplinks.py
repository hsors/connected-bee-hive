#!/usr/bin/python3
# -*- coding: utf-8 -*-

# Connect to TTS MQTT Server and receive uplink messages using the Paho MQTT Python client library
# Save corresponding information in a file ("/home/pi/Documents/hiveData/"+application_id+device_id) that will be read
# by blynkMain.py which will send the uplink content to the smartphone on blynkApp

# See file MQTT-SubscribeUpLinkExample.py for details about MQTT subscription
import os
import sys
import paho.mqtt.client as mqtt
import json
import pickle
import logging
import ardConf
from os.path import exists
DEBUG = False

USER = "ardbeescale-af01@ttn"
PASSWORD = "XXXXXXXXXX"
PUBLIC_ADDRESS = "eu1.cloud.thethings.network"
PUBLIC_ADDRESS_PORT = 1883
QOS = 0 # Meaning Quality of Service (QoS)

def get_value_from_json_object(obj, key):
    try:
        return obj[key]
    except KeyError:
        return '-'

def stop(client):
    client.disconnect()
    print("\nExit")
    sys.exit(0)

# Write uplink to tab file
def save_to_file(some_json):
    end_device_ids = some_json["end_device_ids"]
    device_id = end_device_ids["device_id"]
    application_id = end_device_ids["application_ids"]["application_id"]
    received_at = get_value_from_json_object(some_json, "received_at") # pas sûr de comprendre pourquoi on ne fait pas plutôt received_at = some_json["received_at"]
    logText = "device_id:"+device_id+" application_id:"+application_id+" received_at:"+received_at
    print(logText)
    
    if 'received_at' in some_json:
        uplinkTime= received_at[11:16]

    if 'uplink_message' in some_json:
        uplink_message = some_json["uplink_message"] # uplink_message is a dictionnary
        if 'decoded_payload' in uplink_message:
            if device_id in ardConf.listOfScales:
                decoded_payload = uplink_message["decoded_payload"]
                lastDownlink = decoded_payload["lastDownlink"]
                weight = decoded_payload["weight"]
                temperature = decoded_payload["temperature"]
                internalTemp = decoded_payload["internalTemp"]
                humidity = decoded_payload["humidity"]
                pressure = decoded_payload["pressure"]
                voltage = decoded_payload["voltage"]
                logText+= " lastDownlink:"+str(lastDownlink)+" weight:"+str(weight)
                logText+= " temperature:"+str(round(temperature,2))+" internalTemp:"+str(internalTemp)+" humidity:"+str(humidity)+" pressure:"+str(pressure)+" voltage:"+str(voltage)
                print(logText) 
                uplinkList = [uplinkTime,lastDownlink, weight, temperature,humidity,pressure,voltage,internalTemp]
                
            if device_id in ardConf.otherDevices:
                decoded_payload = uplink_message["decoded_payload"]
                voltage = decoded_payload["voltage"]
                lastDownlink = decoded_payload["lastDownlink"]
                logText = "uplink: "                
                logText+= " lastDownlink:"+str(lastDownlink)+" voltage:"+str(voltage)
                print(logText) 
                uplinkList = [uplinkTime,lastDownlink, voltage]
                
            directory = ardConf.rootDirectory+application_id+"/"+device_id+"/"
            filePath = directory+"/"+ardConf.fileName
            
            # store data
            # first check if the directory exists and if not creates it
            if not exists(directory):
                os.makedirs(directory)
            print("now creating the file "+ardConf.fileName+" in "+directory)
            myFile = open(filePath, 'wb')
            pickle.dump(uplinkList, myFile)
            myFile.close()
            print("Done ! File closed")
        
# The callback for when the client receives a CONNACK response from the server.
def on_connect(client, userdata, flags, rc):
    if rc == 0:
        print("\nConnected successfully to MQTT broker")
        QOS = 0
        #topic= "v3/ardbeescale-af01@ttn/devices/hso-scale01/up" #This is to connect to a single topic
        topic= "#" # This is to connect to all topics
        print("Subscribe to "+ topic )
        mqttc.subscribe(topic, QOS) 

    else:
        print("\nFailed to connect, return code = " + str(rc))

# The callback for when a PUBLISH message is received from the server.
def on_message(client, userdata, message):
    print("\nMessage received on topic '" + message.topic)
    parsed_json = json.loads(message.payload)
    # print(parsed_json["received_at"])
    if DEBUG:
        print("Payload (Collapsed): " + str(message.payload))
        print("Payload (Expanded): \n" + json.dumps(parsed_json, indent=4))
    save_to_file(parsed_json)

# mid = message ID
# It is an integer that is a unique message identifier assigned by the client.
# If you use QoS levels 1 or 2 then the client loop will use the mid to identify messages that have not been sent.
def on_subscribe(client, userdata, mid, granted_qos):
    print("\nSubscribed with message id (mid) = " + str(mid) + " and QoS = " + str(granted_qos))

def on_disconnect(client, userdata, rc):
    print("\nDisconnected with result code = " + str(rc))

def on_log(client, userdata, level, buf):
    print("\nLog: " + buf)
    logging_level = client.LOGGING_LEVEL[level]
    logging.log(logging_level, buf)

# Generate client ID with pub prefix randomly
#client_id = f'python-mqtt-{random.randint(0, 1000)}'

print("Create new mqtt client instance")
mqttc = mqtt.Client()

print("Assign callback functions")
mqttc.on_connect = on_connect
mqttc.on_subscribe = on_subscribe
mqttc.on_message = on_message
mqttc.on_disconnect = on_disconnect

# Setup authentication from settings above
mqttc.username_pw_set(USER, PASSWORD)

print("Connecting to broker: " + PUBLIC_ADDRESS + ":" + str(PUBLIC_ADDRESS_PORT))
mqttc.connect(PUBLIC_ADDRESS, PUBLIC_ADDRESS_PORT, 60)

print("And run forever")
try:
    mqttc.loop_forever()
except KeyboardInterrupt:
    stop(mqttc)
