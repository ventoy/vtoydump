#!/bin/sh

rm -f vtoydump*

#/opt/diet64/bin/diet -Os gcc -std=gnu99 -DHAVE_CONFIG_H -D_FILE_OFFSET_BITS=64      ./src/vtoydump_linux.c ./src/libexfat/*.c -I ./src -I ./src/libexfat  -o  vtoydump64
#/opt/diet32/bin/diet -Os gcc -Wall -std=gnu99 -DHAVE_CONFIG_H -D_FILE_OFFSET_BITS=64 -m32 ./src/vtoydump_linux.c ./src/libexfat/*.c -I ./src -I ./src/libexfat  -o  vtoydump32

gcc -Wall -std=gnu99 -DHAVE_CONFIG_H  -O2 -D_FILE_OFFSET_BITS=64 ./src/vtoydump_linux.c ./src/libexfat/*.c -I ./src -I ./src/libexfat -o vtoydump64
gcc -Wall -std=gnu99 -DHAVE_CONFIG_H  -O2 -D_FILE_OFFSET_BITS=64 -m32 ./src/vtoydump_linux.c ./src/libexfat/*.c -I ./src -I ./src/libexfat -o vtoydump32

aarch64-linux-gnu-gcc -Wall -std=gnu99 -DHAVE_CONFIG_H  -O2 -D_FILE_OFFSET_BITS=64 ./src/vtoydump_linux.c ./src/libexfat/*.c -I ./src -I ./src/libexfat -o vtoydumpaa64

if [ -e vtoydump64 ] && [ -e vtoydump32 ] && [ -e vtoydumpaa64 ]; then
    rm -f ../Vtoyboot/vtoyboot/tools/vtoydump*
    
    strip vtoydump64
    strip vtoydump32
    aarch64-linux-gnu-strip vtoydumpaa64
    
    cp -a vtoydumpaa64 ../Vtoyboot/vtoyboot/tools/
    cp -a vtoydump64 ../Vtoyboot/vtoyboot/tools/
    cp -a vtoydump32 ../Vtoyboot/vtoyboot/tools/

    echo -e '\n===== success =======\n'
else
    echo -e '\n===== failed =======\n'
fi
