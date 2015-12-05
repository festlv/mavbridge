#!/usr/bin/env python2

import serial
import time

s = serial.Serial("/dev/ttyUSB0", timeout=0, baudrate=921600)

s.read(1024)

data = "B" * 1024

s.write(data)
time.sleep(0.5)
print s.read(1024)
