#!/usr/bin/env bash

set -e

export HW_VER=$1

export SW_VER=$2

function usage() {
    echo "Usage: ./deploy.sh <HW_VER> <SW_VER>"
    exit 1
}

if [ -z $HW_VER ] || [ -z $SW_VER ] ; then
    usage
fi

#first, make it
make clean && make

#then, deploy

FW_FOLDER="/srv/http/imprimus/public/mavbridge/hw-$HW_VER/sw-$SW_VER"

LATEST_HW_FOLDER="/srv/http/imprimus/public/mavbridge/hw-$HW_VER/latest"

TMP_FOLDER="/tmp/mavbridge-fw-tmp/"

echo "Deploying HW ver. $HW_VER, SW ver. $SW_VER to $FW_FOLDER"

ssh wot "mkdir -p $TMP_FOLDER"

scp -r out/firmware/* wot:$TMP_FOLDER

ssh -t wot "sudo mkdir -p $FW_FOLDER && sudo cp -r $TMP_FOLDER/* $FW_FOLDER && sudo chown -R imprimus:http $FW_FOLDER && sudo rm -f $LATEST_HW_FOLDER && sudo ln -Ts $FW_FOLDER $LATEST_HW_FOLDER && echo \"Upload OK.\"" 
