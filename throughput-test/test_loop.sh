#!/usr/bin/env zsh

for i in `seq 1 100`;
do
    python2 -m unittest discover
    echo "Enter to continue"
    read
done
