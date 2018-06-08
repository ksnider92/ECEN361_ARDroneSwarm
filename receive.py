
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
        if string != "":
                ready = True
        if ready:
                print("String = ")
                print(string)
                global goals
                global xLoc
                global yLoc
                global xGoal
                global yGoal
                global ready
                global wait
                global angle
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
radio.printDetails()
radio.startListening()

print("Waiting for initial data")
while not ready:
	time.sleep(1/100)
#print goals
newGoal = True
xLoc = 5
yLoc = 0
#angle = 0
xGoalReached = False
yGoalReached = False
reverse = False
ctr = 0
while True:
	print("Current Goal {},{}".format(xGoal,yGoal))
	if newGoal and not wait:
		prevAngle = angle
		newGoal = False
		print("Angle: {}".format(prevAngle))
		if prevAngle == -90:
			reverse = True
	while not yGoalReached and not wait:
#		print("Angle: {}".format(prevAngle))
		if yLoc != yGoal:
			if prevAngle == 90:
				print("Forward") 
			elif reverse:
				print("Reverse")
				print("YLoc: {}".format(yLoc))
		if yLoc == yGoal:
			print("stop")
	#		if (prevAngle > 0 and prevAngle < 90) or (prevAngle < 0 and prevAngle > -90):
	#			dt.right(90)
	#			print("Turned Right")
	#		elif angle > 90 and angle < 180:
	#			dt.left(90)
	#			print("Turned Left")
			yGoalReached = True
			print("Y Goal Reached")

	yGoalReached = False

	while not xGoalReached and not wait:
#		print("Angle: {}".format(prevAngle))
		if xLoc != xGoal and prevAngle == 90:
			print("Forward")
		elif xLoc == xGoal:
			print("stopped")
			xGoalReached = True
			print("X Goal Reached")

	#		if prevAngle == 90:
	#			dt.right(90)
	#			print("turned right")
	#		elif prevAngle == 90:
	#			dt.left(90)
	#			print("turned left")
	xGoalReached = False
	newGoal = True
	reverse = False
	while wait:
		time.sleep(1/100)
	if ctr <= ((len(goals)+1)/2):
		xGoalCount += 2
		yGoalCount += 2
		ctr += 1;
	while ctr > ((len(goals)-1)/2):
		time.sleep(1)
