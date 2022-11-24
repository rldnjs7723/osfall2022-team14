#!/bin/bash

echo "-----------Make Directory-----------"
mkdir -p ../tizen-image/mnt_dir
echo "-----------Build-----------"
sudo ./build-rpi3-arm64.sh
echo "-----------Make Image File-----------"
sudo ./scripts/mkbootimg_rpi3.sh
echo "-----------Remove Prev Image File-----------"
rm -f ../tizen-image/boot.img ../tizen-image/modules.img ../tizen-image/ramdisk.img ../tizen-image/ramdisk-recovery.img ../tizen-image/rootfs.img ../tizen-image/system-data.img
echo "-----------Move Image File-----------"
mv boot.img modules.img ../tizen-image
tar xvzf tizen-unified_20181024.1_iot-headless-2parts-armv7l-rpi3.tar.gz -C ../tizen-image
echo "-----------Compile Test File-----------"
arm-linux-gnueabi-gcc -I/include test/rotd.c -o test/rotd
arm-linux-gnueabi-gcc -I/include test/selector.c -o test/selector
arm-linux-gnueabi-gcc -I/include test/trial.c -o test/trial
echo "-----------Move Test File-----------"
sudo mount ../tizen-image/rootfs.img ../tizen-image/mnt_dir
sudo cp ./test/rotd ./test/selector ./test/trial ../tizen-image/mnt_dir/root
echo "-----------Wait Unmount-----------"
sleep 5
sudo umount ../tizen-image/mnt_dir
echo "-----------Done-----------"