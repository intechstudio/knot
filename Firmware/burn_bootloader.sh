esptool.py --chip esp32s3 -p $(ls /dev/ttyACM*) --before=default_reset \
--after=no_reset write_flash --flash_mode dio --flash_size detect --flash_freq 80m \
0x0 bootloader/bootloader.bin \
0x8000 bootloader/partition-table.bin \
0xe000 bootloader/ota_data_initial.bin \
0x410000 bootloader/tinyuf2.bin