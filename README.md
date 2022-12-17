### OSFALL2022 TEAM14 PROJ4

## 0. Overview
이번 프로젝트에서는 위치 정보를 inode에 함께 저장하는 파일 시스템을 구현했습니다. 
위치 정보는 위도, 경도로 나타내며 부동 소수점을 사용할 수 없기 때문에 정수부와 소수부를 나누어 정수 값으로 저장한 뒤 연산에 사용했습니다. 
코드를 테스트 할 때는 ext2 파일 시스템을 구현한 코드에 맞게 inode 정보를 추가한 뒤 mount 하여 위치 정보와 함께 write를 수행하도록 했습니다.

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
또한, fstab에서 read-only 옵션을 제거하여 권한 설정을 위해 별도의 remount 옵션을 입력하지 않아도 됩니다.
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

## 2. Implement of Geo-tagged File System
### 2.1 Static System Call
이전 프로젝트와 같이 arch/arm64/include/asm/unistd.h에서 새로 구현한 System Call에 맞게 __NR_compat_syscalls의 값을 늘려주었습니다.
```
#define __NR_compat_syscalls    400
```
arch/arm/tools/syscall.tbl에서 system call table을 직접 수정하였고,
```
398 common  set_gps_location  sys_set_gps_location
399 common  get_gps_location  sys_get_gps_location
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
set_gps_location의 경우 위도는 -90~90 범위의 값, 경도는 -180~180 범위의 값을 가지고 소수부는 0과 999999 범위의 값을 가지도록 하고, 범위를 벗어난 경우 -EINVAL을 리턴하였습니다.  
get_gps_location의 경우 inode에 정의한 operation을 통해 위치 정보를 얻고, 위치 정보가 없으면 -ENODEV를, 
저장된 위치 정보와 최근 위치 정보 사이의 거리를 계산하여 accuracy의 합보다 멀다면 -EACCES를 리턴하도록 했습니다. 

### 2.2 Define data structures for tracking device location
gps.h 파일에 gps_location 구조체와 fblock 구조체를 정의했습니다. gps_location 구조체는 location의 위도, 경도의 정수/소수(6자리까지)의 정보와 허용 오차 범위를 담는 데에 사용하고, fblock 구조체는 kernel 내에서 floating 연산이 되지 않아 실수 연산을 정수 연산으로 바꾸어 계산해야 할 때 사용합니다.
```
struct gps_location {
	int lat_integer;
	int lat_fractional;
	int lng_integer;
	int lng_fractional;
	int accuracy;
};
typedef struct _fblock {
	long long int integer;
	long long int fraction;
} fblock;
```
inode에서 저장할 위치 정보는 fs/ext2/ext2.h의 ext2_inode와 ext2_inode_info에 추가하였으며, 
테스트에 사용할 ext2 파일 시스템에도 적용하기 위해 e2fsprogs/lib/ext2fs/ext2_fs.h의 ext2_inode와 ext2_inode_large에 위치 정보를 추가했습니다.
```
__u32	i_lat_integer;
__u32	i_lat_fractional;
__u32	i_lng_integer;
__u32	i_lng_fractional;
__u32	i_accuracy;
```

### 2.3 Inode GPS-related operation & Update location information
추가한 inode operation은, inode에 위치 정보를 설정하는 ext2_set_gps_location과 
inode에 저장된 위치 정보를 gps_location 구조체에 저장하는 ext2_get_gps_location입니다.
fs/ext2/ext2.h에는 다른 파일에서 참조할 수 있도록 extern 키워드를 통해 정의해두었고,
```
extern int ext2_set_gps_location(struct inode *inode);
extern int ext2_get_gps_location(struct inode *inode, struct gps_location *location);
```
fs/ext2/file.c에서 ext2_file_inode_operations에 두 operation을 추가하여 inode->i_op를 통해 접근할 수 있도록 설정했습니다.
```
.set_gps_location = ext2_set_gps_location,
.get_gps_location = ext2_get_gps_location,
```
set_gps_location의 경우 가장 최근의 위치 정보를 통해 inode의 위치 정보를 설정하므로  
include/linux/gps.h에 최근 위치 정보를 저장할 구조체를 설정했고, sys_set_gps_location이 호출되면 latest_loc에 위치 정보를 저장한 후 
ext2_set_gps_location에서 해당 inode의 ext2_inode_info 구조체를 EXT2_I로 불러와서 latest_loc에 저장된 위치 정보를 통해 값을 설정합니다.  
ext2_get_gps_location에서는 ext2_inode_info에서 정보를 빼내고, 인자로 받은 gps_location 구조체에 위치 정보를 저장하면 
sys_get_gps_location에서 gps_location에 저장된 위치 정보를 처리하게 됩니다.
```
extern struct gps_location latest_loc;
```
또한, latest_loc의 경우 공유되는 구조체이기 때문에 중간에 값이 변하지 않도록 sys_set_gps_location이나 ext2_set_gps_location에서 
spin_lock을 잡은 후 latest_loc에서 값을 읽거나 쓰도록 하였습니다. 
따라서 서로 같은 lock을 공유하기 위해 spinlock_t를 include/linux/gps.h에 설정하였습니다.
```
extern spinlock_t gps_lock;
```

### 2.4 fblock function
fblock 구조체 간의 연산을 수행하기 위한 함수들을 정의했습니다. 코사인 함수와 코사인 역함수는 테일러 급수를 이용하여 정의했습니다.
```
fblock myadd(fblock num1, fblock num2);
fblock mysub(fblock num1, fblock num2);
fblock mymul(fblock num1, fblock num2);
fblock mydiv(fblock num, long long int div);
fblock mypow(fblock num, int exp);
long long int myfactorial(long long int num);
fblock mycos(fblock deg);
fblock myarccos(fblock deg);
```

### 2.5 calculate distance and check if able to access
get_dist 함수를 이용하여, 저장된 위치 정보와 최근 위치 정보가 나타내는 위치 간의 거리를 구하고, LocationCompare 함수를 이용하여 두 위치 정보가 갖는 accuracy의 합보다 거리가 더 가까운지를 판단하였습니다. 거리가 더 가깝다면 get_gps_location 시스템 콜 시에 받아들이고, 거리가 더 멀면 받아들이지 않습니다. get_dist 함수에서 거리를 구할 때에는, 지구가 구(sphere)라는 점에 착안하여 다음과 같은 haversine 공식을 이용해, 위도와 경도만으로 거리를 구했습니다.

<img src="https://user-images.githubusercontent.com/104059642/208203945-41e90d66-926c-4d4a-ba08-7889422308e7.png"  width="1000" height="300">

## 3. gpsupdate and file_loc test
### 3.1 gpsupdate.c
gpsupdate는 유저로부터 latitude와 longitude, accuracy를 입력받고, latitude와 longitude가 올바른 값(-90 <= latitude <= 90, -180 <= longitude <= 180)인지 확인한 뒤, set_gps_location 시스템콜로 extern 변수인 latest_loc에 정보를 저장합니다.

### 3.2 file_write.c
file_write는 유저로부터 파일명을 입력받아서, /root/proj4 폴더 안에 그 파일명에 해당하는 파일이 있다면 열고, 없다면 생성한 뒤, Garbage 값을 작성합니다. 즉, 파일이 created되거나 modified되는 상황을 제공합니다.

### 3.3 file_loc.c
file_loc도 마찬가지로 유저로부터 파일명을 입력받아서, /root/proj4 폴더 안의 그 파일명에 해당하는 파일의 path를 get_gps_location의 인자로 전달하여 시스템콜합니다. 그렇게 얻은 위치 정보를 출력합니다.

### 3.4 result

## 4. Lesson learned
