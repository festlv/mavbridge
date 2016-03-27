#!/usr/bin/env zsh

PORT=/dev/ttyUSB0
for i in `seq 1 100`;
do
    # flash 
    /opt/esp-open-sdk/esptool/esptool.py -p $PORT -b 115200 write_flash --flash_baud 1500000 -ff 40m -fm qio -fs 32m 0x00000 out/firmware/rboot.bin 0x02000 out/firmware/rom0.bin 0x100000 out/firmware/spiff_rom.bin
    # wait for it to boot
    sleep 3
    # test
    pushd ../throughput-test/
    python2 -m unittest discover
    popd
    echo "Enter to continue"
    read
done

