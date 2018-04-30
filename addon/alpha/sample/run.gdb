# Adapt the two <PARAMETERS> below to you setup

source <PATH TO ALPHA FOR RPI>/sdk/alpha.gdb

# Connect to the RPi
set serial baud 115200
target remote <UART NODE SUCH AS /dev/ttyUSB>

# Load the executable in the target
# By default, the loaded file is the one GDB debugs, given as argument
# to gdb or using the command file.
load

# Allow the usage of File I/O system(), used in the example.
set remote system-call-allowed 1

# Start executing the sample
continue