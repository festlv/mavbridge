Flashing pre-built binaries
----------------------------------

Prebuilt binaries can be flashed to ESP8266 boards (including official
hardware) via serial port.

Required hardware:
* ESP8266-based board,
* USB-TTL adapter (TTL-level, not RS232!)

Required software:
* esptool (https://github.com/themadinventor/esptool)
* OR, on Windows: http://www.electrodragon.com/w/ESP8266_firmware_flasher
  (untested but should work)

For Linux and MacOS, there is a script, `flash.sh` which can be used to
automate the flashing procedure.

Flashing process
------------------------

1. Connect ESP to your serial port (RX to TX, TX to RX, GND to GND, VIN to
   +5V). Do not plug the serial port in USB port of your computer yet.
2. Start the ESP in bootloader mode. For official hardware, the button on board
   needs to be pressed while plugging the serial adapter in USB port. For other
   boards, there might be a similar procedure (see
   https://github.com/themadinventor/esptool#entering-the-bootloader for
   howto).
3. You can now release the button and launch the flashing script with serial
   port device or name as argument:

On success, you should see something like this:

    $ ./flash.sh /dev/ttyUSB0
    Connecting...
    Erasing flash...
    Wrote 2048 bytes at 0x00000000 in 0.2 seconds (82.7 kbit/s)...
    Erasing flash...
    Wrote 309248 bytes at 0x00002000 in 29.9 seconds (82.8 kbit/s)...
    Erasing flash...
    Wrote 524288 bytes at 0x00100000 in 49.1 seconds (85.4 kbit/s)...

    Leaving...

If using GUI to do the flashing you can look up settings in the flashing script
(which .bin file goes to what memory location).
