### OSFALL2022 TEAM14 PROJ2

## 1. How to Build Kernel
git clone을 통해 kernel 파일을 모두 다운 받으면 해당 위치에 osfall2022-team14 라는 폴더가 생성됩니다.
```
git clone -b proj2 https://github.com/rldnjs7723/osfall2022-team14.git
```
이후 다음 명령어를 통해 kernel 폴더로 이동합니다.
```
cd osfall2022-team14
```
kernel 폴더 내부에 위치한 build.sh를 실행하면 커널 파일 빌드를 수행하고, rootfs로 테스트 파일과 모듈 파일을 이동합니다.
```
sudo ./build.sh
```
이후 kernel 폴더 안에 있는 qemu.sh를 실행하면 tizen kernel이 실행됩니다.
```
sudo ./qemu.sh
```

로그인 후 tizen 안에서 테스트 파일을 실행하려면
insmod 명령어로 wrr_mod.ko 모듈 파일을 insert하고,
```
insmod /root/wrr_mod.ko
```
test 파일을 실행하면 됩니다.
```
/root/test
```
insert 한 모듈을 제거하려면 rmmod 명령어를 입력하면 됩니다.
```
rmmod /root/wrr_mod.ko
```