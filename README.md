MAVBridge
------------------

MAVBridge is an ESP8266 based serial MAVLink <-> wireless bridge. It is compatible
with MAVLink v1.0 specification.

Compatibility has been tested with:

* PX4 Pixhawk hardware
* APMPlanner
* MissionPlanner
* DroidTower
* qGroundcontrol

Serial port side supports up to 921600 baud rate and supports any commonly used
baud rates (9600, 57600, 115200).

On wifi side, access-point and client modes are supported (even
simultaneously).

At the moment, only UDP transport layer is supported. TCP support is
work-in-progress (functionality is there, but ESP runs out of RAM and crashes
when transmitting a lot of data).

As for performance- UDP has been successfully tested at 921600 baud rate and
almost 70 KiB/s data rate without packet-loss (depends on number of WiFi
networks around you, of course).

Configuration interface
--------------------------------

Index/status page:
![Screenshot of index page](img/index.png)

Settings:
![Screenshot of settings page](img/settings.png)


Building firmware
-------------------

Prerequisites for building:
* https://github.com/pfalcon/esp-open-sdk with Espressif IOT SDK 1.5.2
* Sming framework: https://github.com/SmingHub/Sming/releases/tag/2.1.0

Assuming the requirements above are satisfied, building is as simple as:

    cd firmware
    make


Hardware
-----------

Hardware is designed using Kicad and the design is available in hardware
folder.


Testing
--------------

Python scripts that automatically test performance and stability of the
MAVBridge are available in `throughput-test/`. Tests require pymavlink.
