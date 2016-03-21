Performance / stability testing scripts
---------------------------------------------

This set of scripts allows to test the performance and reliability of the
MAVBridge. `mavspoof.py` sends out MAVLink packets over serial port,
`mavrecv.py` receives them on network.

Usage:

    mavspoof.py --baud 115200 /dev/ttyUSB0

Receiving UDP data:

    mavrecv.py 192.168.4.2:14550

Where `192.168.4.2` is IP address of PC and 14550 is default port for MAVLink
over UDP. It might be neccessary to initiate a connection to `192.168.4.255:14550` first
because pymavlink doesn't listen for broadcasts.

Receiving TCP stream:

    mavrecv.py tcp:192.168.4.1:5760

Where `192.168.4.1` is IP address of MAVBridge (server) and 5760 is default port
for MAVLink over TCP.
