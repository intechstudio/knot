clear && idf.py build && esptool.py --chip esp32s3 -p $(ls /dev/ttyACM* | head -n 1) --before=default_reset --after=no_reset write_flash --flash_mode dio --flash_size detect --flash_freq 80m 0x10000 build/midi_host_fw.bin && otatool.py -p $(ls /dev/ttyACM* | head -n 1) switch_ota_partition --slot 0