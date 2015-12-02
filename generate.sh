#!/usr/bin/env bash

MAVLINK_DIR=mavlink/
OUTPUT_DIR=include/mavlink/


mavgen.py --lang=C --wire-protocol=1.0 --output=$OUTPUT_DIR $MAVLINK_DIR/message_definitions/v1.0/ardupilotmega.xml
