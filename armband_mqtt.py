import paho.mqtt.client as paho
import numpy as np
import re

broker = "localhost"
port = 1883
timeout = 60

incoming_msg = ""
incoming_msg_array = []
fft_result = []

def decode():
    incoming_msg_array = re.split(',', incoming_msg)

def on_connect(client, userdata, flags, rc):
        if rc == 0:
            print("Connected success!")
        else:
            print("Failed to connect, return code {rc}")

def on_message(client, userdata, message):
    incoming_msg = message

    str = ""
    for x in incoming_msg_array:
        str += x + ", "
    print("Received message = ", str)

    decode()

    temp = np.array(float(incoming_msg_array))
    fft_result = np.fft.fft(temp, 128)

armband_client = paho.Client("armband_001")
armband_client.on_connect = on_connect
armband_client.on_message = on_message
armband_client.connect(broker, port, timeout)
armband_client.subscribe("armband/signal")
armband_client.loop_forever()
