### OSFALL2022 TEAM14 PROJ1

## 1. How to Build Kernel
git clone을 통해 kernel 파일을 모두 다운 받으면 해당 위치에 osfall2022-team14 라는 폴더가 생성됩니다.
```
git clone https://github.com/rldnjs7723/osfall2022-team14.git
```
이후 다음 명령어를 통해 kernel 폴더로 이동합니다.
```
cd osfall2022-team14
```
kernel 폴더가 위치한 디렉토리에 tizen-image라는 폴더를 생성하고, rootfs를 mount할 폴더도 함께 생성합니다.
컴파일은 build-rpi3-arm64.sh를 통해 수행하며, 이번 과제에 맞는 config 파일은 github에 올려놓았으므로 별도의 작업이 필요 없이 아래의 명령어만으로 커널 코드를 컴파일 할 수 있습니다.
scripts 디렉토리의 mkbootimg_rpi3.sh를 통해 이미지 파일 boot.img, modules.img를 생성하고 이를 tizen-image 폴더로 이동합니다.
이후 kernel 폴더 안에 있는 이미지 압축 파일의 내용물을 tizen-image 폴더에 압축 해제합니다.
마지막으로 arm-linux-gnueabi-gcc 명령을 통해 test 폴더 안의 test_ptree.c를 컴파일하여 test 파일을 생성해주면 kernel 실행을 위한 빌드가 끝납니다.
```
mkdir -p ../tizen-image/mnt_dir
./build-rpi3-arm64.sh
sudo ./scripts/mkbootimg_rpi3.sh
mv boot.img modules.img ../tizen-image
tar xvzf tizen-unified_20181024.1_iot-headless-2parts-armv7l-rpi3.tar.gz -C ../tizen-image
arm-linux-gnueabi-gcc -I/include test/test_ptree.c -o test/test
```
앞서 tree_mod.c 파일을 컴파일하여 생긴 ptree_mod.ko 파일과, test_ptree.c 파일을 컴파일하여 생긴 test 파일을 tizen에서 실행할 수 있도록 rootfs.img에 넣어주어야 합니다.
tizen-image 폴더에 생성한 mnt_dir에 rootfs.img를 mount 하고, mnt_dir/root 폴더에 ptree_mod.ko와 test 파일을 복사합니다.
```
sudo mount ../tizen-image/rootfs.img ../tizen-image/mnt_dir
sudo cp ./kernel/ptree_mod.ko ./test/test ../tizen-image/mnt_dir/root
```
복사가 완료되었다면 mnt_dir을 unmount 합니다.
```
sudo umount ../tizen-image/mnt_dir
```

이후 kernel 폴더 안에 있는 qemu.sh를 실행하면 tizen kernel이 실행됩니다.
```
sudo ./qemu.sh
```

로그인 후 tizen 안에서 테스트 파일을 실행하려면
insmod 명령어로 ptree_mod.ko 모듈 파일을 insert하고,
```
insmod /root/ptree_mod.ko
```
test 파일을 실행하면 됩니다.
```
/root/test
```
insert 한 모듈을 제거하려면 rmmod 명령어를 입력하면 됩니다.
```
rmmod ptree_mod.ko
```


## 2. High Level Design and Implementation
### 2.1 Module을 통한 System Call 정의
이번 과제에서는 기본으로 주어진 별도의 소스 코드를 수정하지 않고 system call을 정의해야 했습니다.
ptree_mod.c에서 모듈을 구현한 방식은 hello_mod.c에 주어진 스켈레톤 코드를 참고하였습니다.
module_init으로 insert시 실행할 함수를, module_exit으로 remove시 실행할 함수를 설정하였고, 
ptree_mod_init 함수에서 system call table의 398번 entry에 있는 system call을 잠시 legacy_syscall에 복사한 후 과제에서 구현한 ptree 함수를 398번 entry의 system call로 설정했습니다.
ptree_mod_exit 함수에서는 398번 entry의 system call을 다시 legacy_syscall에 저장해두었던 system call로 복구하고 모듈을 제거합니다.
이렇게 구현한 코드를 컴파일하여 생긴 ptree_mod.ko 파일을 통해 모듈을 insert 하면 test에서 398번 system call을 호출했을 때 ptree 함수를 실행하게 됩니다.

Module에서 System Call 구현에는 다음 사이트를 참고하였습니다.
https://www.linuxtopia.org/online_books/Linux_Kernel_Module_Programming_Guide/x958.html
