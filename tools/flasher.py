import serial
import sys
import time
import math

try:
    fileaddress = ' '.join(sys.argv[1:2])
    flashport = ' '.join(sys.argv[2:3])
    flashbaud = int(' '.join(sys.argv[3:]))
except Exception:
    print("Usage: python flasher.py FILENAME SERIALPORT BAUDRATE")
    exit()

try:
    ser = serial.Serial(
        port=flashport,
        baudrate=flashbaud,
        parity=serial.PARITY_NONE,
        stopbits=serial.STOPBITS_ONE,
        bytesize=serial.EIGHTBITS
    )
except Exception:
    print("ERROR: Serial Port " + flashport + " busy or inexistent!")
    exit()
    
try:
    F = open(fileaddress,"rb")
    F.seek(0, 2)
    size = F.tell()
    F.seek(0, 0)
except Exception:
    print("ERROR: Cannot access file " + fileaddress + ". Check permissions!")
    ser.close()
    exit()

print("Flashing with baudrate of "+ str(flashbaud) + "...")
sys.stdout.flush()

try:
    ser.write(b"R")
    blocksize = math.trunc(flashbaud / 5)
    offset = 0
    while offset < size:
        if sys.stdout.isatty():
            percent = math.trunc(offset * 100 / size)
            print("\r" + str(percent) + "%", end='')
            sys.stdout.flush()
        readlen = blocksize
        if size < readlen:
            readlen = size
        data = F.read(readlen)
        ser.write(data)
        offset += readlen
    if sys.stdout.isatty():
        print("\r", end='')
    print("Completed!\nRunning app...")
    sys.stdout.flush()
    time.sleep(1)
    ser.write(b"g")
except Exception:
    print("ERROR: Serial port disconnected. Check connections!")
    F.close()
    time.sleep(0.2)
    ser.flush()
    time.sleep(0.2)
    ser.close()
    exit()

F.close()
time.sleep(0.2)
ser.flush()
time.sleep(0.2)
ser.close()
