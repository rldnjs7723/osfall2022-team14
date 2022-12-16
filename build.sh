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
echo "-----------Extract e2fsprogs-----------"
rm -r -f ./e2fsprogs
tar xvzf e2fsprogs-1.46.5.tar.gz -C ./
sudo cp ./ext2_fs.h ./e2fsprogs/lib/ext2fs
mv e2fsprogs-1.46.5 e2fsprogs
echo "-----------Compile & Move Test File-----------"
sudo mount ../tizen-image/rootfs.img ../tizen-image/mnt_dir
cd ./test
make
sudo cp gpsupdate file_loc file_write ../../tizen-image/mnt_dir/root
echo "-----------Move proj4.fs-----------"
cd ../e2fsprogs
./configure
make
cd ../
FREE=$(losetup -f)
dd if=/dev/zero of=proj4.fs bs=1M count=1
sudo losetup "$FREE" proj4.fs
sudo ./e2fsprogs/misc/mke2fs -I 256 -L os.proj4 "$FREE"
sudo losetup -d "$FREE"
sudo mv proj4.fs ../tizen-image/mnt_dir/root/
echo "-----------Change fstab Permission-----------"
sudo cp ./fstab ../tizen-image/mnt_dir/etc/
echo "-----------Wait Unmount-----------"
sleep 5
sudo umount ../tizen-image/mnt_dir
echo "-----------Done-----------"
sudo ./qemu.sh
