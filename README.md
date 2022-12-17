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
이 경우, osfall2022-team14 폴더 내에 위치한 proj4.fs 파일을 root에 위치하도록 복사합니다.  
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
set_gps_location의 경우 위도는 -90에서 90 범위의 값, 경도는 -180에서 180 범위의 값을 가지고 소수부는 0과 999999 범위의 값을 가지도록 하고, 범위를 벗어난 경우 -EINVAL을 리턴하였습니다.  
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
또한 메모리와 디스크에서 각각 사용하는 두 ext2 inode 구조체를 변환하기 위해 fs/ext2/inode.c의  
__ext2_write_inode에서는 다음 코드를 추가하고,
```
raw_inode->i_lat_integer = cpu_to_le32(ei->i_lat_integer);
raw_inode->i_lat_fractional = cpu_to_le32(ei->i_lat_fractional);
raw_inode->i_lng_integer = cpu_to_le32(ei->i_lng_integer);
raw_inode->i_lng_fractional = cpu_to_le32(ei->i_lng_fractional);
raw_inode->i_accuracy = cpu_to_le32(ei->i_accuracy);
```
ext2_iget에서는 다음 코드를 추가했습니다.
```
ei->i_lat_integer = le32_to_cpu(raw_inode->i_lat_integer);
ei->i_lat_fractional = le32_to_cpu(raw_inode->i_lat_fractional);
ei->i_lng_integer = le32_to_cpu(raw_inode->i_lng_integer);
ei->i_lng_fractional = le32_to_cpu(raw_inode->i_lng_fractional);
ei->i_accuracy = le32_to_cpu(raw_inode->i_accuracy);
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

위치 정보는 파일이 create 되거나 modify 된 경우에 업데이트 하도록 설정하였고, 
create는 fs/ext2/namei.c의 ext2_create에서 set_gps_location operation을 호출하도록 하였습니다.
modify는 fs/ext2에 위치한 소스 코드 중에서 i_mtime을 current_time()으로 갱신하는 경우에 전부 set_gps_location operation을 호출하도록 설정했습니다. (dir.c / ialloc.c / inode.c / super.c)
```
if(inode->i_op->set_gps_location != NULL) inode->i_op->set_gps_location(inode);
```

### 2.4 fblock function
fblock 구조체 간의 연산을 수행하기 위한 함수들을 정의했습니다. 코사인 함수~~와 코사인 역함수~~는 테일러 급수를 이용하여 정의했습니다.
```
fblock myadd(fblock num1, fblock num2);
fblock mysub(fblock num1, fblock num2);
fblock mymul(fblock num1, fblock num2);
fblock mydiv(fblock num, long long int div);
fblock mypow(fblock num, int exp);
long long int myfactorial(long long int num);
fblock mycos(fblock deg);
```
~~fblock myarccos(fblock deg);~~

### 2.5 calculate distance and check if able to access
get_dist 함수를 이용하여, 저장된 위치 정보와 최근 위치 정보가 나타내는 위치 간의 거리를 구하고, LocationCompare 함수를 이용하여 두 위치 정보가 갖는 accuracy의 합보다 거리가 더 가까운지를 판단하였습니다. 거리가 더 가깝다면 get_gps_location 시스템 콜 시에 받아들이고, 거리가 더 멀면 받아들이지 않습니다. get_dist 함수에서 거리를 구할 때에는, 지구가 구(sphere)라는 점에 착안하여 다음과 같은 haversine 공식을 이용해, 위도와 경도만으로 거리를 구했습니다.

<img src="https://user-images.githubusercontent.com/104059642/208236611-66c37503-acaf-4c91-938d-21af509be9b2.png"  width="1000" height="400">

그런데, 테일러 급수를 통한 arccos을 사용할 경우, 수렴 속도가 너무 느려 arccos(1) 계산 시 오차가 너무 컸습니다. 그래서 distance와 accuracy_sum을 R(지구 반지름)로 나누고 cos을 씌워 그 값을 비교했습니다. 이때, 코사인 함수는 감소함수라는 점에 착안하여, 결과적으로 cos(distance / R) >= cos(accuracy / R) 시에만 받아들이도록 하였습니다.

## 3. gpsupdate and file_loc test
### 3.1 gpsupdate.c
gpsupdate는 유저로부터 latitude와 longitude, accuracy를 입력받고, latitude와 longitude가 올바른 값(-90 <= latitude <= 90, -180 <= longitude <= 180)인지 확인한 뒤, set_gps_location 시스템콜로 extern 변수인 latest_loc에 정보를 저장합니다.

### 3.2 file_write.c
file_write는 유저로부터 파일명을 입력받아서, /root/proj4 폴더 안에 그 파일명에 해당하는 파일이 있다면 열고, 없다면 생성한 뒤, Garbage 값을 작성합니다. 즉, 파일이 created되거나 modified되는 상황을 제공합니다.

### 3.3 file_loc.c
file_loc도 마찬가지로 유저로부터 파일명을 입력받아서, /root/proj4 폴더 안의 그 파일명에 해당하는 파일의 path를 get_gps_location의 인자로 전달하여 시스템콜합니다. 그렇게 얻은 위치 정보를 출력하고, 구글 맵 링크 형태로도 출력합니다.

### 3.4 result
proj4 디렉토리를 생성한 뒤, 그곳에 proj4.fs를 마운트합니다.(proj4.fs에는 이미 Gyeongbokgung, Dokdo 파일과 SeoulNat 폴더가 존재합니다.) 먼저 gpsupdate를 하고 test1을 생성합니다. 그러면 file_loc을 통해 현재 위치에서는 test1에 접근 가능한 것을 확인할 수 있습니다. 위치를 다시 바꾸면 test1에 접근 불가능해집니다. 이 위치 정보를 test2에 저장하고 처음 test1의 위치와 비슷한 위치로 gpsupdate하면 다시 test1에 접근 가능해집니다. 여기에서 test1 파일을 수정하면, 저장되었던 위치 정보가 현재의 것으로 바뀝니다. 위도나 경도의 범위가 제한을 벗어나면 Invalid location을 출력합니다. 이미 존재했던 파일인 Gyeongbokgung, Dokdo는 해당 위치로 각각 이동하면 접근 가능하지만, 폴더의 경우에는 아예 유효한 파일 위치로 인식하지 못합니다.

```
root:~> mkdir proj4
root:~> mount -o loop -t ext2 /root/proj4.fs /root/proj4
root:~> ./gpsupdate 37 448743 126 950364 17
root:~> ./file_write test1
root:~> ./file_loc test1
Location information: (latitude, longitude) / accuracy = (37.448743, 126.950364) / 17
Google Maps link: https://www.google.com/maps/place/@37.448743,126.950364,17z/
root:~> ./gpsupdate -37 448743 -126 950364 17
root:~> ./file_loc test1
[   93.870528] Can't access that location
root:~> ./file_write test2
root:~> ./file_loc test2
Location information: (latitude, longitude) / accuracy = (-37.448743, -126.950364) / 17
Google Maps link: https://www.google.com/maps/place/@-37.448743,-126.950364,17z/
root:~> ./gpsupdate 37 458743 126 960364 17
root:~> ./file_loc test1
Location information: (latitude, longitude) / accuracy = (37.448743, 126.950364) / 17
Google Maps link: https://www.google.com/maps/place/@37.448743,126.950364,17z/
root:~> ./file_write test1
root:~> ./file_loc test1
Location information: (latitude, longitude) / accuracy = (37.458743, 126.960364) / 17
Google Maps link: https://www.google.com/maps/place/@37.458743,126.960364,17z/
root:~> ./gpsupdate 37 458743 186 960364 17
[   94.264703] Invalid location
ERROR
root:~> ls -l proj4
total 17
-rw-r--r-- 1 root root     5 Jan  1  1970 Dokdo
-rw-r--r-- 1 root root     5 Jan  1  1970 Gyeongbokgung
drwxr-xr-x 2 root root  1024 Jan  1  1970 SeoulNat
drwx------ 2 root root 12288 Dec 17  2022 lost+found
-rw-r--r-- 1 root root     5 Jan  1 00:01 test1
-rw-r--r-- 1 root root     5 Jan  1 00:01 test2
root:~> ./gpsupdate 37 579617 126 977041 100
root:~> ./file_loc Gyeongbokgung
Location information: (latitude, longitude) / accuracy = (37.579617, 126.977041) / 100
Google Maps link: https://www.google.com/maps/place/@37.579617,126.977041,100z/
root:~> ./gpsupdate 37 242936 131 866842 1000
root:~> ./file_loc Dokdo
Location information: (latitude, longitude) / accuracy = (37.242936, 131.866842) / 1000
Google Maps link: https://www.google.com/maps/place/@37.242936,131.866842,1000z/
root:~> ./gpsupdate 37 456509 126 950038 500
root:~> ./file_loc SeoulNat
[   96.681153] Invalid path
```

## 4. Lesson learned
### 4.1
ext2_create에 주석으로 적혀진 내용을 읽었을 때, ext2_create가 호출되는 시점이 새로운 파일의 Directory Cache Entry가 생성된 이후이므로 inode가 더 늦게 할당된다는 점을 알았고, 
inode의 정보를 갱신하는 작업을 Operation을 추가로 생성하여 진행하면서 파일 시스템의 구성 방식에 대해 좀 더 이해할 수 있었습니다.

### 4.2
proj4.fs를 umount한 후 다시 mount 할 때 위치 정보가 손실되는 현상을 보고, 디스크와 메모리에서 각자 사용하는 구조체의 종류가 다르다는 것을 알았고, 변환을 위한 추가적인 작업이 필요함을 알게 되었습니다.

### 4.3
헤더 파일에 extern 키워드로 구조체 포인터를 할당한 후, Segmentation Fault가 발생하여 문제 원인을 찾느라 여러 작성한 코드를 되돌리면서 많은 시간을 소모했습니다. 이러한 경험으로 
한 번에 여러 수정사항을 반영한 후 동작 여부를 확인하지 않고, 빌드에 많은 시간이 소모되더라도 하나씩 확실하게 구현해나가야 한다는 점을 배웠습니다.
