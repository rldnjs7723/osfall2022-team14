#!/bin/bash

sudo ./build-rpi3-arm64.sh
cd ../tizen-image
sudo mount rootfs.img ../mnt_dir
cd ../osfall2022-team14
cd test
make
sudo cp gpsupdate ../../mnt_dir/root;
sudo cp file_loc ../../mnt_dir/root;
sudo cp file_write ../../mnt_dir/root;
cd ../e2fsprogs
./configure
make
cd ../
dd if=/dev/zero of=proj4.fs bs=1M count=1
sudo losetup /dev/loop14 proj4.fs
sudo ./e2fsprogs/misc/mke2fs -I 256 -L os.proj4 /dev/loop14
sudo losetup -d /dev/loop14
sudo mv proj4.fs ../mnt_dir/root/
sudo umount ../mnt_dir
sudo ./qemu.sh
