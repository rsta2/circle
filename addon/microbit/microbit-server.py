#
# microbit-server.py
#
# by R. Stange, 2020
#

#
# Communication parameters: 115200 Bps, 8N1
#
# Protocol:
#
# Client sends:
#	'!' ObjectID ':' FunctionID [ ':' Parameter ... ] '\n'
#	(ObjectID and FunctionID are two-letter codes)
#
# Server returns:
#	'OK\n' | 'ERROR\n' | ['-']Number '\n'
#

from microbit import *

def init ():
    uart.init (115200)

def send_ok ():
    uart.write ('OK\n')

def send_error ():
    uart.write ('ERROR\n')

def send_result (s):
    uart.write (s)
    uart.write ('\n')

def send_result_num (num):
    send_result (str (num))

def send_result_bool (b):
    if b == True:
        send_result ('1')
    else:
        send_result ('0')

def mainloop ():
    state = 0
    while True:
        if uart.any ():
            buf = str (uart.read (20), 'UTF-8')
            for i in range (len (buf)):
                ch = buf[i]
                if state == 0:
                    if ch == '!':
                        cmd = []
                        state = 1
                elif state == 1:
                    if ch != '\n':
                        cmd.append (ch)
                    else:
                        execute (''.join (cmd))
                        state = 0

def execute (cmd):
    try:
        par = cmd.split (':')
        obj = par[0]
        fn = par[1]
        del par[:2]
        if   obj == 'MI':   execute_microbit (fn, par)
        elif obj == 'DI':   execute_display (fn, par)
        elif obj == 'BU':   execute_button (fn, par)
        elif obj == 'PI':   execute_pin (fn, par)
        elif obj == 'AC':   execute_accelerometer (fn, par)
        elif obj == 'CO':   execute_compass (fn, par)
        else:               raise
    except:
        send_error ()

def execute_microbit (fn, par):
    if fn == 'TE':
        send_result_num (temperature ())
    else:
        raise

def execute_display (fn, par):
    if fn == 'SP':
        display.set_pixel (int (par[0]), int (par[1]), int (par[2]))
        send_ok ()
    elif fn == 'CL':
        display.clear ()
        send_ok ()
    elif fn == 'SH':
        display.show (':'.join (par), wait=False, clear=True)
        send_ok ()
    elif fn == 'SC':
        display.scroll (':'.join (par), wait=False)
        send_ok ()
    elif fn == 'ON':
        display.on ()
        send_ok ()
    elif fn == 'OF':
        display.off ()
        send_ok ()
    elif fn == 'LL':
        send_result_num (display.read_light_level ())
    else:
        raise

def execute_button (fn, par):
    if par[0] == 'A':
        bn = button_a
    elif par[0] == 'B':
        bn = button_b
    else:
        raise
    if fn == 'IS':
        send_result_bool (bn.is_pressed ())
    elif fn == 'WA':
        send_result_bool (bn.was_pressed ())
    elif fn == 'GE':
        send_result_num (bn.get_presses ())
    else:
        raise

def execute_pin (fn, par):
    pn = int (par[0])
    if   pn == 0:   pin = pin0
    elif pn == 1:   pin = pin1
    elif pn == 2:   pin = pin2
    elif pn == 3:   pin = pin3
    elif pn == 4:   pin = pin4
    elif pn == 5:   pin = pin5
    elif pn == 6:   pin = pin6
    elif pn == 7:   pin = pin7
    elif pn == 8:   pin = pin8
    elif pn == 9:   pin = pin9
    elif pn == 10:  pin = pin10
    elif pn == 11:  pin = pin11
    elif pn == 12:  pin = pin12
    elif pn == 13:  pin = pin13
    elif pn == 14:  pin = pin14
    elif pn == 15:  pin = pin15
    elif pn == 16:  pin = pin16
    elif pn == 19:  pin = pin19
    elif pn == 20:  pin = pin20
    else:           raise
    if fn == "RD":
        send_result_num (pin.read_digital ())
    elif fn == "WD":
        pin.write_digital (int (par[1]))
        send_ok ()
    elif fn == "RA":
        send_result_num (pin.read_analog ())
    elif fn == "WA":
        pin.write_analog (int (par[1]))
        send_ok ()
    elif fn == "PM":
        pin.set_analog_period (int (par[1]))
        send_ok ()
    elif fn == "PU":
        pin.set_analog_period_microseconds (int (par[1]))
        send_ok ()
    elif fn == "IT":
        send_result_bool (pin.is_touched ())
    else:
        raise

def execute_accelerometer (fn, par):
    if fn == "GX":
        send_result_num (accelerometer.get_x ())
    elif fn == "GY":
        send_result_num (accelerometer.get_y ())
    elif fn == "GZ":
        send_result_num (accelerometer.get_z ())
    elif fn == "CG":
        send_result (accelerometer.current_gesture ())
    elif fn == "IG":
        send_result_bool (accelerometer.is_gesture (par[0]))
    elif fn == "WG":
        send_result_bool (accelerometer.was_gesture (par[0]))
    else:
        raise

def execute_compass (fn, par):
    if fn == "CA":
        compass.calibrate ()
        send_ok ()
    elif fn == "IC":
        send_result_bool (compass.is_calibrated ())
    elif fn == "CC":
        compass.clear_calibration ()
        send_ok ()
    elif fn == "GX":
        send_result_num (compass.get_x ())
    elif fn == "GY":
        send_result_num (compass.get_y ())
    elif fn == "GZ":
        send_result_num (compass.get_z ())
    elif fn == "HE":
        send_result_num (compass.heading ())
    elif fn == "GF":
        send_result_num (compass.get_field_strength ())
    else:
        raise

def main ():
    init ()
    mainloop ()

main ()
