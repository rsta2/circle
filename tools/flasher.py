import serial
import sys
import time

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
    F = open(fileaddress,"r")
    data = F.read()
    F.close()
except Exception:
    print("ERROR: Cannot access file " + fileaddress + ". Check permissions!")
    ser.close()
    exit()

print("Flashing with baudrate of "+ str(flashbaud) + "...")
sys.stdout.flush()

try:
    ser.write(data)
    print("Completed!\nRunning app...")
    sys.stdout.flush()
    time.sleep(1)
    ser.write("g")
except Exception:
    print("ERROR: Serial port disconnected. Check connections!")
    ser.close()
    exit()

ser.close()
