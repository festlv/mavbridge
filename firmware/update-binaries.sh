#!/usr/bin/env bash
set -e
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

cp $DIR/out/firmware/* $DIR/prebuilt-binaries/ && echo "Binaries in prebuilt-binaries updated"
