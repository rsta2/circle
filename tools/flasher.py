import serial
import struct
import sys


fileaddress = ' '.join(sys.argv[1:])
ser = serial.Serial(
    port='/dev/ttyUSB0',
    baudrate=115200,
    parity=serial.PARITY_NONE,
    stopbits=serial.STOPBITS_ONE,
    bytesize=serial.EIGHTBITS
)

print(ser.isOpen())
F = open(fileaddress,"r")
data = F.read()

ser.write(data)
ser.write("g")
ser.close()
