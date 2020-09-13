#!/bin/sh

make -j8
dd if=/dev/zero of=./dummy.bin bs=128k count=1
cat dummy.bin >> u-boot.bin
./tcc_crc -o u-boot.rom -v __BL u-boot.bin
