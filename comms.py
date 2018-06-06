import RPi.GPIO as GPIO
from lib_nrf24 import NRF24
import time
import spidev
import math

def callback(data):
	print("Interrupted")
	receivedMessage = []
        radio.read(receivedMessage, radio.getDynamicPayloadSize())
        string = ""
        for n in receivedMessage:
                if (n >= 32 and n <= 126):
                        string += chr(n)
	print(string)
	ready = True
	radio.stopListening()
	radio.startListening()

ready = False
wait = False
# Set up interrupt pin
GPIO.setmode(GPIO.BCM)
GPIO.setup(24,GPIO.IN)
GPIO.add_event_detect(24,GPIO.BOTH,callback=callback,bouncetime = 30)

# Set up radio
pipes = [[0xE8, 0xE8, 0xF0, 0xF0, 0xE1], [0xF0, 0xF0, 0xF0, 0xF0, 0xE1]]

radio = NRF24(GPIO, spidev.SpiDev())
radio.begin(0,4)

radio.setPayloadSize(32)
radio.setChannel(0x66)
radio.setDataRate(NRF24.BR_1MBPS)
radio.setPALevel(NRF24.PA_MIN)

radio.setAutoAck(True)
radio.enableDynamicPayloads()
radio.enableAckPayload()

radio.openReadingPipe(1, pipes[1])
radio.openWritingPipe(pipes[0])
radio.printDetails()
radio.startListening()

while True:
	"Input Something:"
	result = raw_input(">")
	radio.write(result)
	