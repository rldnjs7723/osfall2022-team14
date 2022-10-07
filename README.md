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
tizen-image 폴더에 생성한 mnt_dir에 rootfs.img를 mount 하고, 
```
sudo mount ../tizen-image/rootfs.img ../tizen-image/mnt_dir
```
mnt_dir/root 폴더에 ptree_mod.ko와 test 파일을 복사합니다.
```
sudo cp ./kernel/ptree_mod.ko ./test/test ../tizen-image/mnt_dir/root
```
복사가 완료되었다면 mnt_dir을 unmount 합니다.
```
sudo umount ../tizen-image/mnt_dir
```

이후 kernel 폴더 안에 있는 qemu.sh를 실행하면 tizen kernel이 실행됩니다.
```
./qemu.sh
```

로그인 후 tizen 안에서 테스트 파일을 실행하려면
insmod 명령어로 ptree_mod.ko 모듈 파일을 insert하고,
```
insmod /root/ptree_mod.ko
```
nr 값을 input으로 주면서 test 파일을 실행하면 됩니다.
```
/root/test [nr]
```
insert 한 모듈을 제거하려면 rmmod 명령어를 입력하면 됩니다.
```
rmmod ptree_mod.ko
```


## 2. High Level Design and Implementation
Proj1의 전체적인 구성은 Module을 사용하여 System Call Table을 수정하는 부분,  
User Mode에서 nr 데이터를 읽고 prinfo를 탐색한 후 다시 User Mode의 buf와 nr로 결과를 복사하는 부분,  
task_struct의 doubly linked list 구조를 활용하여 child, sibling 프로세스를 탐색하는 부분,
탐색한 task에서 정보를 추출하여 kernel 영역의 prinfo buffer에 저장하는 부분,
새로 정의한 system call을 호출하여 얻은 데이터를 출력하는 테스트 부분으로 나눌 수 있습니다.

### 2.1 Module을 통한 System Call 정의
이번 과제에서는 기본으로 주어진 별도의 소스 코드를 수정하지 않고 system call을 정의해야 했습니다.  
ptree_mod.c에서 모듈을 구현한 방식은 hello_mod.c에 주어진 스켈레톤 코드를 참고하였습니다.  
module_init으로 insert시 실행할 함수를, module_exit으로 remove시 실행할 함수를 설정하였고,  
ptree_mod_init 함수에서 system call table의 398번 entry에 있는 system call을 잠시 legacy_syscall에 복사한 후 과제에서 구현한 ptree 함수를 398번 entry의 system call로 설정했습니다.  
ptree_mod_exit 함수에서는 398번 entry의 system call을 다시 legacy_syscall에 저장해두었던 system call로 복구하고 모듈을 제거합니다.  
이렇게 구현한 코드를 컴파일하여 생긴 ptree_mod.ko 파일을 통해 모듈을 insert 하면 test에서 398번 system call을 호출했을 때 ptree 함수를 실행하게 됩니다.

Module에서 System Call 구현에는 다음 사이트를 참고하였습니다.  
https://www.linuxtopia.org/online_books/Linux_Kernel_Module_Programming_Guide/x958.html

### 2.2 User 모드의 데이터 주고 받기 및 에러 처리 
ptree 함수에서는 User 모드의 데이터를 copy_from_user 함수를 통해 복사하고, copy_to_user를 통해 결과를 전달하는 부분을 구현했습니다.  
task_struct에서 프로세스를 탐색한 결과를 임시로 저장하기 위해 kmalloc을 통해 kernel에서 사용할 prinfo buffer를 선언했으며,  
copy_from_user, copy_to_user 그리고 kmalloc이 실패한 경우 -EFAULT를 리턴하도록 설정했습니다.  
system call을 호출했을 때 받은 argument는 NULL이거나 잘못된 값인 경우 -EINVAL을 리턴하도록 설정하여 에러 처리를 했습니다.  
또한 task_struct를 확인하면서 중간에 프로세스가 종료되거나, 새로 생성되는 경우 문제가 발생할 수 있기 때문에 read_lock과 read_unlock을 통해 프로세스 탐색을 atomic 하게 수행할 수 있도록 설정했습니다.

### 2.3 task_struct를 pre-order traversal 방식으로 프로세스 탐색
task_traverse 함수에서는 init_task를 통해 가장 먼저 선언된 swapper/0의 task_struct에 접근하고,  
child 프로세스가 있다면 child process부터 탐색하고, 그 이후 sibling 프로세스를 탐색하여 pre-order traversal을 구현했습니다.  
task_struct의 children과 sibling은 list_head 구조체로, next와 prev를 통해 다음과 이전 list_head에 접근할 수 있습니다.  
children에서 첫 번째 child의 task_struct를 구할 때는 children이 가리키는 next list_head를 list_entry 함수에 인자로 주어 얻었습니다.  
sibling의 경우에는 더 이상 children이 없는 task_struct에서 시작하여 sibling이 존재하는 task_struct를 찾아 parent task_struct를 탐색하였습니다.  
이후 children을 찾을 때와 마찬가지로 sibling이 가리키는 next list_head를 list_entry 함수에 인자로 주어 다음 sibling task_struct를 얻었습니다.

### 2.4 task_struct에서 정보를 추출하여 prinfo buffer에 저장
copy_prinfo 함수에서는 입력으로 받은 task_struct에서 state, pid, parent_pid의 정보를 그대로 kernel 영역에 할당한 buffer에 저장하였고,  
children list가 비어있다면 first_child_pid를 0으로, 비어 있지 않다면 list_entry 함수를 통해 첫 번째 children.next의 task_struct 찾아서 pid를 저장했습니다.  
sibling list의 경우에도 마찬가지로 비어있다면 first_sibling_list를 0으로 설정했으며, 비어 있지 않은 경우에는 sibling.next의 task_struct를 list_entry 함수를 통해 얻고 pid를 저장했습니다.  
uid는 현재 task_struct의 신원 정보를 나타내는 real_cred에 저장된 uid에서 값을 가져와 저장했습니다.  
프로그램의 이름인 comm은 prinfo 구조체에서는 길이가 64로 선언되어 있으나, task_struct에서는 16으로 선언되어 있어 strncpy를 통해 TASK_COMM_LEN 만큼 복사했습니다.

### 2.5 새로 정의한 syscall 테스트하여 결과 출력
앞서 구현한 시스템 콜을 테스트하기 위해 test/test_ptree.c 파일을 생성했습니다.
일단 main 함수 내부에서 plist에 syscall을 수행하면서, nr개의 프로세스 정보를 복사하면서, 동시에 return value인 total 값(실행 중인 총 프로세스 개수)를 저장했습니다. (이때, nr value는 실행할 떄 input으로 입력받습니다.) 복사한 plist와 nr을 인자로 하여 print_process 함수를 호출함으로써 plist 내에 있는 모든 프로세스를 주어진 형식에 맞게 출력했습니다.
print_process 함수 내에서는, nr개의 integer list인 parent_index_list를 할당했는데, 이는 plist에 속한 process의 각 인덱스에 대하여, 그 프로세스의 부모 프로세스의 인덱스를 저장합니다. root process의 부모 프로세스의 인덱스는 -1로 정의했습니다.

ex) plist[3]의 부모 프로세스는 plist[1]이므로 parent_index_list[3] == 1  
ex) plist[1]의 부모 프로세스는 plist[0]이므로 parent_index_list[1] == 0  
ex) plist[0]이 root process이므로 parent_index_list[1] == 0  

이어서, 현재 출력하려는 프로세스가 root process로부터 떨어진 거리(process tree에서 그 node의 depth)만큼 "\t"을 반복 출력한 뒤, 프로세스 정보를 양식대로 출력했습니다.

ex) plist[3]의 부모 프로세스는 plist[1], plist[1]의 부모 프로세스는 plist[0]이므로 root process까지의 거리는 2 -> "\t"을 2번 출력한 뒤 프로세스 정보 출력

마지막으로, print_process 함수 실행이 종료된 이후에는, main 함수로 돌아와 처음 nr 값과 시스템콜 후의 nr 값을 비교하여, 바뀐 경우에는 초기 값과 최종 값을 출력하고, 바뀌지 않은 경우에는 단순히 nr 값을 출력했습니다. 이어서 total process 수도 출력을 했습니다.

## 3. Investigation of the Process Tree

모든 프로세스는 tree 구조로 이루어져 있습니다. 그래서 이것을 pre-order로 순회하여 리스트를 만들 경우, 모든 0이 아닌 인덱스 i에 대하여, i번째 프로세스는 (i-1)번째 프로세스가 부모 프로세스이거나, (i-1)번째 부모 프로세스와 공통인 부모 프로세스를 갖습니다. 그래서 2.5의 테스트 출력을 구현할 때에도 이를 이용하여 부모 프로세스의 인덱스를 찾을 수 있었습니다.  
리눅스가 부팅되면 systemd와 kthreadd가 차례로 실행된다는 것을 pid를 통해 확인할 수 있습니다.(각각 1과 2) systemd는 user space와 시스템 부팅에 관련된 프로세스를 관리하는 최상위 프로세스이고, kthreadd는 커널 프로세스를 생성하는 최상위 프로세스라고 하는데, 각각의 자식 프로세스들의 pid는 kthreadd가 훨씬 작은 것을 볼 때, 리눅스가 부팅되면 kernel space가 먼저 일괄적으로 만들어진 뒤에야 user space가 생성되며 부팅 작업이 이루어진다는 것을 짐작할 수 있겠습니다.

## 4. Lesson Learned

### 4.1 
c언어가 아직 익숙하지 않아 포인터를 처리하는 과정에서 에러가 많이 발생했습니다.  
여러 헤더 파일을 한 눈에 볼 수 없어 각 구조체에 정의된 변수가 포인터로 선언되었는지 아닌지의 여부와 어떤 자료형, 구조체로 선언 되었는지 확인하는 작업이 필수적이었기에 관련 문서를 찾아보는 등 이해하는 데 많은 시간을 들였습니다.  
이 경험을 통해 처음 보는 헤더 파일에 대해서도 구조를 이해할 수 있는 능력을 조금이나마 길렀습니다.
