import serial
import sys
import time

try:
	magicstring = ' '.join(sys.argv[1:2])
	serialport = ' '.join(sys.argv[2:3])
	serialbaud = int(' '.join(sys.argv[3:]))
except Exception:
	print("Usage: python reboottool.py MAGICSTRING SERIALPORT BAUDRATE")
	exit()

try:
	ser = serial.Serial(
		port=serialport,
		baudrate=serialbaud,
		parity=serial.PARITY_NONE,
		stopbits=serial.STOPBITS_ONE,
		bytesize=serial.EIGHTBITS
	)
except Exception:
	print("ERROR: Serial Port " + serialport + " busy or inexistent!")
	exit()
    
print("Sending \"" + magicstring + "\" with baudrate of "+ str(serialbaud) + "...")
sys.stdout.flush()

try:
	ser.write(magicstring)
except Exception:
	print("ERROR: Serial port disconnected. Check connections!")
	ser.close()
	exit()

print("Waiting for reboot...");
sys.stdout.flush()
time.sleep(4)

print("Completed!")

time.sleep(0.2)
ser.flush()
time.sleep(0.2)
ser.close()
