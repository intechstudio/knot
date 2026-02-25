SCRIPT_DIR=$(dirname "$(readlink -f "$0")")

esptool.py --chip esp32s3 -p /dev/ttyUSB0 -b 2000000 --before=default_reset \
--after=no_reset write_flash --flash_mode dio --flash_size detect --flash_freq 80m \
0x0 $SCRIPT_DIR/Firmware/bootloader/bootloader.bin \
0x8000 $SCRIPT_DIR/Firmware/bootloader/partition-table.bin \
0xe000 $SCRIPT_DIR/Firmware/bootloader/ota_data_initial.bin \
0x410000 $SCRIPT_DIR/Firmware/bootloader/tinyuf2.bin
