import string
import paho.mqtt.client as paho
import re

broker = "localhost"
port = 1883
timeout = 60

incoming_msg = ""
incoming_msg_array = []

def decode():
    incoming_msg_array = re.split(',', incoming_msg)

def encode(array) -> string:
    temp = ""
    for x in array:
        temp += x + ","
    
    return temp

def on_message(client, userdata, message):
    incoming_msg = message

    str = ""
    for x in incoming_msg_array:
        str += x + ", "
    print("Received message = ", str)

    decode()

    encode()
    feedback_client.publish("feedback/processedSignal", )


def on_connect(client, userdata, flags, rc):
        if rc == 0:
            print("Connected success!")
        else:
            print("Failed to connect, return code {rc}")

feedback_client = paho.Client("feedback_002")
feedback_client.on_connect = on_connect
feedback_client.on_message = on_message
feedback_client.connect(broker, port, timeout)
feedback_client.subscribe("feedback/rawSignal")
feedback_client.loop_forever()
