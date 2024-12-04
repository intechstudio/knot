esptool.py --chip esp32s3 --before=default_reset \
--after=hard_reset write_flash --flash_mode dio --flash_size detect --flash_freq 80m \
0x0 bootloader.bin \
0x8000 partition-table.bin \
0xe000 otadata_boot_from_ota0.bin \
0x10000 knot_esp32_release_2024-05-29-1330.uf2 \
0x410000 tinyuf2.bin
