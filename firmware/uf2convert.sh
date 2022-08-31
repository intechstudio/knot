# . $HOME/$whoami/Documents/esp-idf/export.sh

# esptool.py --chip ESP32-S3 merge_bin -o ./build/merged_image.bin \
# 0x1000 ./build/bootloader/bootloader.bin \
# 0x8000 ./build/partition_table/partition-table.bin \
# 0x10000 ./build/midi_host_fw.bin

sudo mkdir ./build/output

# python3 ./tools/uf2conv.py -f ESP32S3 ./build/merged_image.bin -b 0x0 -c -o ./build/output/merged.uf2

python3 ./tools/uf2conv.py -f ESP32S3 ./build/midi_host_fw.bin -b 0x0 -c -o ./build/output/midi_host_fw.uf2


