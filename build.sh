#!/bin/sh

rm -f vtoydump*

machine=$(uname -m)

gcc -Wall -std=gnu99 -DHAVE_CONFIG_H  -O2 -D_FILE_OFFSET_BITS=64 ./src/vtoydump_linux.c ./src/libexfat/*.c -I ./src -I ./src/libexfat -o vtoydump

if [ -e vtoydump ]; then
    strip vtoydump
    echo -e "\n===== build vtoydump for $machine success =======\n"
else
    echo -e "\n===== build vtoydump for $machine failed =======\n"
fi
