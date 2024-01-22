. $HOME/$whoami/Documents/esp-idf/export.sh

idf.py build

$HOME/$whoami/.espressif/tools/xtensa-esp32s3-elf/esp-2022r1-RC1-11.2.0/xtensa-esp32s3-elf/bin/xtensa-esp32s3-elf-gdb \
-x gdbinit $HOME/$whoami/Documents/midi-host/firmware/build/midi_host_fw.elf \
-iex "target remote localhost:3333" \
-iex "set remote hardware-watchpoint-limit 2" \
-iex "monitor reset halt" \
-iex "flushregs" \
-iex "mon program_esp $HOME/$whoami/Documents/midi-host/firmware/build/bootloader/bootloader.bin 0x0 verify" \
-iex "mon program_esp $HOME/$whoami/Documents/midi-host/firmware/build/partition_table/partition-table.bin 0x8000 verify" \
-iex "mon program_esp $HOME/$whoami/Documents/midi-host/firmware/build/midi_host_fw.bin 0x10000 verify" \
-iex "monitor reset halt" \
-iex "flushregs" \
-iex "c"
