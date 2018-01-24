import serial
from serial import SerialException
import struct
import sys
from time import sleep

fileaddress = ' '.join(sys.argv[1:2])
flashport = ' '.join(sys.argv[2:3])
flashbaud = int(' '.join(sys.argv[3:]))

try:
    ser = serial.Serial(
        port=flashport,
        baudrate=flashbaud,
        parity=serial.PARITY_NONE,
        stopbits=serial.STOPBITS_ONE,
        bytesize=serial.EIGHTBITS
    )
except SerialException:
    print("ERROR: Serial Port " + flashport + " busy or inexistent!")
    exit()
    
try:
    F = open(fileaddress,"r")
except:
    print("ERROR: Cannot open file " + fileaddress + "\nCheck permissions!")
    ser.close()
    exit()
data = F.read()
F.close()
#data = struct.pack(hex, 0x7E, 0xFF, 0x03, 0x00, 0x01, 0x00, 0x02, 0x0A, 0x01, 0xC8,      0x04, 0xD0, 0x01, 0x02, 0x80, 0x00, 0x00, 0x00, 0x00, 0x8E, 0xE7, 0x7E)
print("Flashing with baudrate of "+ str(flashbaud) + "...")
sys.stdout.flush()
try:
    ser.write(data)
    print("Completed!\nRunning app...")
    sys.stdout.flush()
    sleep(1)
    ser.write("g")
except SerialException:
    print("ERROR: Serial port disconnected.\nCheck connections!")
    ser.close()
    exit()
ser.close()
