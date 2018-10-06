#! /bin/bash

../openocd-esp32/src/openocd -s ../openocd-esp32/tcl -f ../openocd-esp32/tcl/interface/ftdi/esp32_devkitj_v1.cfg -f ../openocd-esp32/tcl/board/esp-wroom-32.cfg &
../xtensa-esp32-elf/bin/xtensa-esp32-elf-gdb -ex 'target remote localhost:3333' ./build/bruconbadge2018.elf
killall openocd
