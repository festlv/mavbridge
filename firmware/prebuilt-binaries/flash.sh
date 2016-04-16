#!/usr/bin/env bash
set -e

PORT=$1
BAUD=115200
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

if [[ -z "$PORT" ]]
then
    echo "Usage: ./flash.sh <port>"
    echo "Example: ./flash.sh /dev/ttyUSB0"
    exit 1
fi

esptool.py -p "$PORT" -b $BAUD write_flash -ff 40m -fm qio -fs 32m 0x00000 rboot.bin 0x02000 rom0.bin 0x100000 spiff_rom.bin
