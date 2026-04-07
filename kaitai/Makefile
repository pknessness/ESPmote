build-flash-monitor: build flash monitor

build:
	idf.py build

flash:
	python -m esptool -p /dev/ttyUSB0 --chip esp32 -b 460800 --before default-reset --after hard-reset write-flash --flash-mode dio --flash-size 2MB --flash-freq 40m 0x1000 build/bootloader/bootloader.bin 0x8000 build/partition_table/partition-table.bin 0x10000 build/espmote-btstack.bin

monitor:
	idf.py monitor --no-reset -p /dev/ttyUSB0

clean:
	rm -r build

install:
	$(IDF_PATH)/install.fish

setup-idf: install setup

setup:
	fish $(IDF_PATH)/export.fish