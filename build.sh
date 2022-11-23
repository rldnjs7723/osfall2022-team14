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
#arm-linux-gnueabi-gcc -I/include test/test.c -o test/test
echo "-----------Move Test File & Module to rootfs-----------"
sudo mount ../tizen-image/rootfs.img ../tizen-image/mnt_dir
sudo cp ./kernel/rot_mod.ko ../tizen-image/mnt_dir/root
#sudo cp ./test/test ../tizen-image/mnt_dir/root
echo "-----------Wait Unmount-----------"
sleep 5
sudo umount ../tizen-image/mnt_dir
echo "-----------Done-----------"