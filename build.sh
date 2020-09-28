#!/bin/sh

rm -f vtoydump*

#/opt/diet64/bin/diet -Os gcc -std=gnu99 -DHAVE_CONFIG_H -D_FILE_OFFSET_BITS=64      ./src/vtoydump_linux.c ./src/libexfat/*.c -I ./src -I ./src/libexfat  -o  vtoydump64
#/opt/diet32/bin/diet -Os gcc -Wall -std=gnu99 -DHAVE_CONFIG_H -D_FILE_OFFSET_BITS=64 -m32 ./src/vtoydump_linux.c ./src/libexfat/*.c -I ./src -I ./src/libexfat  -o  vtoydump32

gcc -Wall -std=gnu99 -DHAVE_CONFIG_H  -O2 -D_FILE_OFFSET_BITS=64 ./src/vtoydump_linux.c ./src/libexfat/*.c -I ./src -I ./src/libexfat -o vtoydump64
gcc -Wall -std=gnu99 -DHAVE_CONFIG_H  -O2 -D_FILE_OFFSET_BITS=64 -m32 ./src/vtoydump_linux.c ./src/libexfat/*.c -I ./src -I ./src/libexfat -o vtoydump32

if [ -e vtoydump64 ] && [ -e vtoydump32 ]; then
    rm -f ../Vtoyboot/vtoyboot/tools/vtoydump*
    cp -a vtoydump64 ../Vtoyboot/vtoyboot/tools/
    cp -a vtoydump32 ../Vtoyboot/vtoyboot/tools/

    echo -e '\n===== success =======\n'
else
    echo -e '\n===== failed =======\n'
fi
