import paho.mqtt.client as paho
import numpy as np
import matplotlib.pyplot as plt
from scipy.fft import rfft, rfftfreq, fft, fftfreq 

broker = "kenyuen.mynetgear.com"
port = 1883
timeout = 60

def fir_filter():
    pass

def wiener_filter():
    pass

def ai():
    pass

def on_connect(client, userdata, flags, rc):
        if rc == 0:
            print("Connected success!")
        else:
            print("Failed to connect, return code {rc}")

def on_message(client, userdata, msg):
    print(f"{msg.topic} {msg.payload}")

    #decode
    incoming_msg_arr = str(msg.payload).replace('b','').replace('\'','').strip()
    incoming_msg_arr = np.fromstring(incoming_msg_arr, dtype=float, sep=',')
    
    #fir_filter()
    #wiener_filter()

    #fft
    yf = rfft(incoming_msg_arr)
    n = yf.size
    sample_rate = 1100
    xf = rfft(n, 1 / sample_rate)

    #plt.plot(xf, yf)
    #plt.show()

    #ai()

armband_client = paho.Client("master_in")
armband_client.on_connect = on_connect
armband_client.on_message = on_message
armband_client.connect(broker, port, timeout)
armband_client.subscribe("armband/signal")
armband_client.loop_forever()
