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
또한, fstab에서 read-only 옵션을 제거하여 별도의 mount 옵션을 입력하지 않아도 됩니다.
```
sudo ./build.sh
```
이후 kernel 폴더 안에 있는 qemu.sh를 실행하면 tizen kernel이 실행됩니다.
```
sudo ./qemu.sh
```
로그인 후 tizen 안에서 root 폴더에 위치한 rotd, selector, trial 파일을 통해 테스트를 수행할 수 있습니다.
```
/root/rotd
/root/selector (number) & 
/root/trial (identifier) &
```

## 2. Implement of Rotation-based reader/writer locks
### 2.1 Static System Call
이전 프로젝트와 같이 arch/arm64/include/asm/unistd.h에서 새로 구현한 System Call에 맞게 __NR_compat_syscalls의 값을 늘려주었습니다.
```
#define __NR_compat_syscalls		403
```
다만 이번에는 insmod를 추가로 입력하지 않도록 구현해야하는 조건에 따라 Static 하게 System Call을 구현했습니다.  
arch/arm/tools/syscall.tbl에서 system call table을 직접 수정하였고,
```
398 common  set_rotation        sys_set_rotation
399 common  rotlock_read        sys_rotlock_read
400 common  rotlock_write       sys_rotlock_write
401 common  rotunlock_read      sys_rotunlock_read
402 common  rotunlock_write     sys_rotunlock_write
```
arch/arm64/include/asm/unistd32.h에서 system call number를 설정하고 include/linux/syscalls.h에서 연결한 함수를 지정했습니다.
```
#define __NR_set_rotation 398
__SYSCALL(__NR_set_rotation, sys_set_rotation)
#define __NR_rotlock_read 399
__SYSCALL(__NR_rotlock_read, sys_rotlock_read)
#define __NR_rotlock_write 400
__SYSCALL(__NR_rotlock_write, sys_rotlock_write)
#define __NR_rotunlock_read 401
__SYSCALL(__NR_rotunlock_read, sys_rotunlock_read)
#define __NR_rotunlock_write 402
__SYSCALL(__NR_rotunlock_write, sys_rotunlock_write)
```
```
asmlinkage long sys_set_rotation(int degree);
asmlinkage long sys_rotlock_read(int degree, int range);
asmlinkage long sys_rotlock_write(int degree, int range);
asmlinkage long sys_rotunlock_read(int degree, int range);
asmlinkage long sys_rotunlock_write(int degree, int range);
```
System Call 함수는 SYSCALL_DEFINE을 통해 구현하였습니다.
```
SYSCALL_DEFINE1(set_rotation, int, degree)
SYSCALL_DEFINE2(rotlock_read, int, degree, int, range)
SYSCALL_DEFINE2(rotlock_write, int, degree, int, range)
SYSCALL_DEFINE2(rotunlock_read, int, degree, int, range)
SYSCALL_DEFINE2(rotunlock_write, int, degree, int, range)
```

### 2.2 Define constants and implement data structures
Rotation lock과 관련된 system call을 rotation.c에 구현한 내용은 linux/rotation.h에도 정의해 두었으며,  
헤더 파일에는 rotation lock의 process id, degree, range, lock을 잡았는지의 여부, list_head를 저장하기 위한 구조체 rotlock_t를 정의했습니다.  
구조체 rotlock_t는 rotation.c에서 init_rotlock 함수로 초기화 할 수 있습니다.
```
typedef struct __rotlock_t__ {
    pid_t pid;
    int degree;
    int range;
    int cond;
    struct list_head node;
} rotlock_t;
```
rotation.c에서는 read/write lock을 잡았거나 기다리는 프로세스 목록을 관리하기 위해 각 Linked list의 list_head를 초기화 하였으며,  
lock을 위한 mutex, 프로세스의 block, unblock을 관리하기 위한 wait_queue 또한 초기화 했습니다.
```
static LIST_HEAD(read_waiting);
static LIST_HEAD(write_waiting);
static LIST_HEAD(read_acquired);
static LIST_HEAD(write_acquired);

static DEFINE_MUTEX(mutex);
static DECLARE_WAIT_QUEUE_HEAD(wait_queue);
```

### 2.3 set_rotation
set_rotation에서는 degree와 range를 입력하면 0 <= degree < 360 , 0 < range < 180의 범위에 맞는 값인지 check_error 함수를 통해 확인하며, 올바르지 않는 값이 입력되면 -1을 리턴하도록 하였습니다.  
현재의 rotation 값을 수정할 때는 mutex_lock을 통해 atomic 하게 수행할 수 있도록 하였고, get_lock 함수를 통해 lock을 기다리고 있는 프로세스 중에서 조건에 맞는 프로세스를 깨우도록 구현했습니다.  
이 때, get_lock 함수에는 prev_lock이라는 정수값을 인자로 받도록 하여 이전에 lock을 잡고 있던 프로세스가 reader라면 writer에게 우선권을 주고, writer라면 reader에게 우선권을 주도록 구현했습니다.  
set_rotation에서는 prev_lock 값을 PREV_READ로 설정하여, writer가 우선권을 받도록 설정했습니다.

### 2.4 rotlock_read/write
rotlock에서도 마찬가지로 check_error 함수를 통해 잘못된 입력값을 확인하여 -1을 리턴하도록 하였고, init_rotlock을 통해 새로운 rotlock_t를 초기화합니다.  
rotlock_t는 list_add_tail을 통해 read/write_waiting linkedlist에 연결하고, list_del을 통해 linkedlist에서 제외합니다.  
rotlock_write에서는 count_rotation이라는 함수를 통해 0 ~ 359까지의 값 중에서 현재 rotlock의 범위에 포함되어 있는 값들을 체크합니다.  
rotation_cnt_write_waiting 배열은 각 값에서 기다리고 있는 write_lock의 수를 저장합니다.  
rotlock_read에서는 이러한 rotation_cnt_write_waiting 배열을 참고하여 현재 rotation 값에서 기다리고 있는 write_lock이 존재한다면 lock을 잡지 못하고 wait를 하도록 하며,  
write_lock을 잡고 있는 프로세스가 있거나 rotation이 범위에 포함되어 있지 않다면 wait를 수행합니다.
```
while (!check_rotation(rotation, degree, range) || rotation_cnt_write_waiting[rotation] || !list_empty(&write_acquired))
```
rotlock_write에서도 마찬가지로 write_lock을 잡고 있는 프로세스가 있거나 rotation이 범위에 포함되어 있지 않다면 wait를 수행하며, read_lock을 잡고 있는 프로세스가 있을 때도 wait를 수행합니다.
```
while (!check_rotation(rotation, degree, range) || !list_empty(&write_acquired) || !list_empty(&read_acquired))
```
wait 함수에서는 DEFINE_WAIT로 wait_entry를 초기화 하고, add_wait_queue로 wait_queue에 넣으며 prepare_to_wait와 schedule을 통해 프로세스를 block 합니다.  
이후 다른 프로세스가 lock을 release 하거나 set_rotation에서 rotation의 값을 바꾼 경우 get_lock 함수를 호출하여 wake_up_process를 통해서 unblock 합니다.  
깨어난 이후에는 finish_wait를 통해 wait_queue에서 제거하고, rotlock을 waiting list에서 제거한 뒤 acquired list에 추가하여 lock을 잡았다고 기록합니다.  
rotlock_write에서는 추가로 count_rotation 함수를 실행하여 rotation_cnt_write_waiting 배열에서 wait_lock을 기다리는 count를 1 감소시킵니다.  
이러한 코드는 전부 mutex_lock을 통해 atomic 하게 수행하도록 하였습니다.

### 2.5 rotunlock_read/write
마찬가지로 check_error 함수로 잘못된 입력값을 체크하며, list_for_each_entry_safe를 통해 read/write_acquired list에서 현재 프로세스가 잡은 lock을 탐색하여 list_del로 삭제하고, rotlock은 kfree를 통해 메모리를 반환하도록 합니다.  
이 때, rotunlock_write에서 release 했거나 rrotunlock_read에서 마지막 reader가 release 한 경우에 get_lock을 호출하여 waiting 중인 프로세스를 깨우도록 하였고,  
reader에서 release 했다면 PREV_READ를 입력하여 writer가 우선권을 얻고, writer에서 release 했다면 PREV_WRITE를 입력하여 reader가 우선권을 얻도록 하였습니다.  
이러한 코드는 전부 mutex_lock을 통해 atomic 하게 수행하도록 하였습니다.

### 2.6 exit_rotlock
현재 프로세스가 잡고 있던 모든 lock과 기다리고 있는 lock을 제거하기 위해 list_for_each_entry_safe를 통해 각 linkedlist를 탐색하여 process id가 동일한 rotlock을 전부 제거합니다.  
write_waiting의 경우 count_rotation을 통해 wait_lock의 count를 감소시키며, read/write_acquired의 경우 release 한다면 get_lock을 호출하여 각각 우선권을 얻도록 합니다.  
이렇게 구현한 exit_rotlock 함수는 kernel/exit.c의 do_exit 함수 내부에서 호출하여 프로세스를 종료할 때 해당 프로세스와 관련된 모든 lock을 제거하도록 합니다.


## 3. Selector and trial test file
### 3.1 selector.c
사용자로부터 number(num)을 입력받고, 1초마다 다음의 과정을 수행합니다.   
 먼저 degree = 90, range = 90에 대하여 rotlock_write를 시스템콜하며 lock을 잡고 "integer" 파일을 열어(없다면 생성) 입력받은 num을 저장합니다. selector가 num을 저장했음을 출력하면서 num을 증가시키고 rotunlock_write 시스템콜로 lock을 해제하면서 1초 후 다시 loop을 돕니다.

### 3.2 trial.c
사용자로부터 identifier(process_num)을 입력받고, 1초마다 다음의 과정을 수행합니다.   
 먼저 degree = 90, range = 90에 대하여 rotlock_read를 시스템콜하며 lock을 잡고 이미 존재하는 "integer" 파일을 열어(없다면 생성될 때까지 대기) num을 읽고 prime_factor를 수행합니다. 각 process를 구분할 수 있도록 identifier와 함께 결과를 출력하고, rotunlock_read를 시스템콜로 lock을 해제하면서 1초 후 다시 loop을 돕니다.

### 3.3 result
rotd는 2초마다 rotation을 30씩 증가시키므로 selector와 trial 모두 24초를 주기로 14번 출력하고 10번 쉬는 것을 반복합니다.

## 4. Lesson learned
### 4.1
write_lock을 기다리는 프로세스의 수를 관리할 때 rotation_cnt_write_waiting 배열 대신 변수 하나만 사용하려고 했는데, 
예상치 못한 상황에 의해 starvation과 race condition이 발생하는 것을 보고 lock 구현에서는 고려할 부분이 굉장히 많다는 점을 알게 되었습니다.

### 4.2
프로세스를 Block하고자 할 때 wait_queue를 사용하지 않고 구현하는 과정에서 프로세스가 깨어나지 않는 등 문제가 발생하는 경우가 많았습니다. 
이번 프로젝트를 통해 프로세스를 sleep 시키고 깨우는 작업은 이미 구현되어 있는 wait_queue 관련 함수를 제대로 사용할 때 더 쉽고 안전하게 구현할 수 있음을 배웠습니다.

### 4.3
이전까지 Dynamic하게 System Call을 구현했기 때문에 insmod 명령을 추가로 입력하지 않고 커널 부팅 시 module을 insert 할 수 있는 방법을 찾아보았지만 제대로 작동하지 않아 
Static하게 System Call을 구현했습니다. 이번 프로젝트를 통해 module insert를 하지 않고 system call을 적용할 수 있는 방법에 대해 배웠습니다.