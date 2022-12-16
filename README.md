### OSFALL2022 TEAM14 PROJ4

## 0. Overview


## 1. How to Build Kernel
git clone을 통해 kernel 파일을 모두 다운 받으면 해당 위치에 osfall2022-team14 라는 폴더가 생성됩니다.
```
git clone -b proj4 https://github.com/rldnjs7723/osfall2022-team14.git
```
이후 다음 명령어를 통해 kernel 폴더로 이동합니다.
```
cd osfall2022-team14
```
kernel 폴더 내부에 위치한 build.sh를 실행하면 커널 파일 빌드를 수행하고, rootfs로 테스트 파일을 이동한 뒤, tizen kernel을 실행합니다.  
또한, fstab에서 read-only 옵션을 제거하여 별도의 mount 옵션을 입력하지 않아도 됩니다.
```
sudo ./build.sh
```
로그인 후 tizen 안에서 root 폴더에 proj4 폴더를 만들고 마운트한 뒤, gpsupdate, file_write, file_loc 파일을 통해 테스트를 수행할 수 있습니다.
```
mkdir proj4
mount -o loop -t ext2 /root/proj4.fs /root/proj4
./gpsupdate [lat_int] [lat_frac] [lng_int] [lng_frac] [accuracy]
./file_write [filename]
./file_loc /root/proj4/[filename]
```

## 2. Implement of Rotation-based reader/writer locks
### 2.1 Static System Call
이전 프로젝트와 같이 arch/arm64/include/asm/unistd.h에서 새로 구현한 System Call에 맞게 __NR_compat_syscalls의 값을 늘려주었습니다.
```
#define __NR_compat_syscalls		400
```
arch/arm/tools/syscall.tbl에서 system call table을 직접 수정하였고,
```
398 common  set_gps_location	sys_set_gps_location
399 common	get_gps_location	sys_get_gps_location
```
arch/arm64/include/asm/unistd32.h에서 system call number를 설정하고 include/linux/syscalls.h에서 연결한 함수를 지정했습니다.
```
#define __NR_set_gps_location 398
__SYSCALL(__NR_set_gps_location, sys_set_gps_location)
#define __NR_get_gps_location 399
__SYSCALL(__NR_get_gps_location, sys_get_gps_location)
```
```
asmlinkage long sys_set_gps_location(struct gps_location __user *loc);
asmlinkage long sys_get_gps_location(const char __user *pathname, struct gps_location __user *loc);
```
System Call 함수는 SYSCALL_DEFINE을 통해 구현하였습니다.
```
SYSCALL_DEFINE1(set_gps_location, struct gps_location __user *, loc)
SYSCALL_DEFINE2(get_gps_location, const char __user *, pathname, struct gps_location __user *, loc)
```

### 2.2 Define constants and implement data structures


## 3. gpsupdate and file_loc test


## 4. Lesson learned
