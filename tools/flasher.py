import serial
import struct
import sys


fileaddress = ' '.join(sys.argv[1:])
ser = serial.Serial(
    port='/dev/ttyUSB0',
    baudrate=1008065,
    parity=serial.PARITY_NONE,
    stopbits=serial.STOPBITS_ONE,
    bytesize=serial.EIGHTBITS
)

print(ser.isOpen())
F = open(fileaddress,"r")
data = F.read()
#data = struct.pack(hex, 0x7E, 0xFF, 0x03, 0x00, 0x01, 0x00, 0x02, 0x0A, 0x01, 0xC8,      0x04, 0xD0, 0x01, 0x02, 0x80, 0x00, 0x00, 0x00, 0x00, 0x8E, 0xE7, 0x7E)

ser.write(data)
ser.write("g")
ser.close()
