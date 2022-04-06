import string
import paho.mqtt.client as paho
import numpy as np

broker = "kenyuen.mynetgear.com"
port = 1883
timeout = 60

def on_message(client, userdata, msg):
    print(f"{msg.topic} {msg.payload}")


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
