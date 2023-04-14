#!/usr/bin/python3
# Connect to TTS MQTT Server and send downlink messages using the Paho MQTT Python client library

#
import sys
import paho.mqtt.client as mqtt
from base64 import b64encode
#!/usr/bin/python3

# Push the one byte downlink specified in "hexadecimal_payload" to the node specified in the following lines#
hexadecimal_payload = 'C1'

USER = "ardbeescale-af01@ttn"
PASSWORD = "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"
PUBLIC_ADDRESS = "eu1.cloud.thethings.network"
PUBLIC_ADDRESS_PORT = 1883
DEVICE_ID = "hso-energysaver"

# QoS = 0 - at most once
# The client publishes the message, and there is no acknowledgement by the broker.
# QoS = 1 - at least once
# The broker sends an acknowledgement back to the client.
# The client will re-send until it gets the broker's acknowledgement.
# QoS = 2 - exactly once
# Both sender and receiver are sure that the message was sent exactly once, using a kind of handshake
QOS = 2

def stop(client):
    client.disconnect()
    print("\nExit")
    sys.exit(0)

# The callback for when the client receives a CONNACK response from the server.
def on_connect(client, userdata, flags, rc):
    if rc == 0:
        print("\nConnected successfully to MQTT broker")
    else:
        print("\nFailed to connect, return code = " + str(rc))

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

print("Create new mqtt client instance")
mqttClient = mqtt.Client()

print("Assign callback functions")
mqttClient.on_connect = on_connect
mqttClient.on_subscribe = on_subscribe
mqttClient.on_disconnect = on_disconnect
# mqttClient.on_log = on_log  # Logging for debugging OK, waste

# Setup authentication from settings above
mqttClient.username_pw_set(USER, PASSWORD)

print("Connecting to broker: " + PUBLIC_ADDRESS + ":" + str(PUBLIC_ADDRESS_PORT))
mqttClient.connect(PUBLIC_ADDRESS, PUBLIC_ADDRESS_PORT, 60) # keep alive 60s

if len(DEVICE_ID) != 0:
    topic = "v3/" + USER + "/devices/" + DEVICE_ID + "/down/replace"
    print("Publishing message to topic " + topic + " with QoS = " + str(QOS))
    fport = 100
    # Convert hexadecimal payload to base64
    b64 = b64encode(bytes.fromhex(hexadecimal_payload)).decode()
    print('Convert hexadecimal_payload: ' + hexadecimal_payload + ' to base64: ' + b64 + '  Hexa payload:' + hexadecimal_payload)
    msg = '{"downlinks":[{"f_port":' + str(fport) + ',"frm_payload":"' + b64 + '","priority": "NORMAL"}]}'
    result = mqttClient.publish(topic, msg, QOS)

    # result: [0, 2]
    status = result[0]
    if status == 0:
        print("Send " + msg + " to topic " + topic)
    else:
        print("Failed to send message to topic " + topic)

else:
    print("Can not subscribe or publish to topic")
    stop(mqttClient)
    
