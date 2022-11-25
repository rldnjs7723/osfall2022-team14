### OSFALL2022 TEAM14 PROJ2

## 0. Overview
이번 프로젝트에서는 Reader-Writer Problem을 해결하기 위한 Rotation Lock을 구현했습니다. One-Dimensional Orientation을 가정하였을 때, 
degree - range <= rotation <= degree + range의 범위에 들어오는 경우에만 lock을 잡을 수 있도록 하였습니다. 
Linkedlist와 wait_queue를 사용하여 read/write lock을 잡고 있거나 기다리는 프로세스의 목록을 관리했으며, 
Writer의 Starvation을 막기 위해 변수를 추가로 설정하여 writer에 우선권을 주도록 설정했습니다.

## 1. How to Build Kernel
git clone을 통해 kernel 파일을 모두 다운 받으면 해당 위치에 osfall2022-team14 라는 폴더가 생성됩니다.
```
git clone -b proj3 https://github.com/rldnjs7723/osfall2022-team14.git
```
이후 다음 명령어를 통해 kernel 폴더로 이동합니다.
```
cd osfall2022-team14
```
kernel 폴더 내부에 위치한 build.sh를 실행하면 커널 파일 빌드를 수행하고, rootfs로 테스트 파일을 이동합니다.
```
sudo ./build.sh
```
이후 kernel 폴더 안에 있는 qemu.sh를 실행하면 tizen kernel이 실행됩니다.
```
sudo ./qemu.sh
```

로그인 후 tizen 안에서 테스트 파일을 실행하려면
다음 명령어로 쓰기 권한을 설정하고 test 파일을 실행하면 됩니다.
```
mount -o rw,remount /dev/vdd /
```
주어진 rotd의 실행파일과 selector, trial 테스트 파일은 root 폴더에 위치해 있습니다.
```
/root/rotd
/root/selector (number) & 
/root/trial (identifier) &
```

## 2. Implement of Rotation-based reader/writer locks
### 2.1 Static System Call

### 2.2 Define constants and implement data structures


## 3. Selector and trial test file


## 4. Lesson learned