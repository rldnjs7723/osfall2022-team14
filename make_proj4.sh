#!/bin/bash

echo "-----------Extract e2fsprogs-----------"
rm -r -f ./e2fsprogs
tar xvzf e2fsprogs-1.46.5.tar.gz -C ./
mv e2fsprogs-1.46.5 e2fsprogs
sudo cp ./ext2_fs.h ./e2fsprogs/lib/ext2fs
sudo cp ./ext2fsP.h ./e2fsprogs/lib/ext2fs
echo "-----------Move proj4.fs-----------"
cd ./e2fsprogs
./configure
make
cd ../
FREE=$(losetup -f)
dd if=/dev/zero of=proj4.fs bs=1M count=1
sudo losetup "$FREE" proj4.fs
sudo ./e2fsprogs/misc/mke2fs -I 256 -L os.proj4 "$FREE"
sudo losetup -d "$FREE"