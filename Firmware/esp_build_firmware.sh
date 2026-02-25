idf.py build

mkdir -p ../binary

python3 ./tools/uf2conv.py -f ESP32S3 ./build/midi_host_fw.bin -b 0x0 -c -o ../binary/midi_host_fw.uf2
