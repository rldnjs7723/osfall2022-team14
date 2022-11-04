### OSFALL2022 TEAM14 PROJ2

## 0. Overview
WRR scheduler는 기존의 RR scheduler에서 각 task마다 weight(time slice)을 부여해 변형한 scheduler입니다. Proj2에서는 이러한 WWR scheduler를 구현하는 것을 목표로 했습니다. 추가적으로, 기존의 scheduler와의 priority나 상호작용 관련 문제 해결과, run queue 간의 total weight의 균형을 맞추는 load-balancing의 구현을 요했습니다. 이 모든 것들을 구현하기 위하여 여러 constant와 data structure, function을 정의하였고, 그 중 주된 것들을 2장에서 소개하겠습니다. 시스템 콜을 할 때에는 Proj1 때와 마찬가지로 동적으로 구현했습니다.

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

## 2. Define and implement WRR(Weighted Round-Robin) scheduler
### 2-1 Define constants and implement data structures
include/uapi/linux/sched.h 파일에는 SCHED_WRR이라는 constant를 7로서 새롭게 추가해주었고, include/linux/sched.h 파일에는 각 WRR entry를 의미하는 sched_wrr_entity 구조체를 선언한 뒤, wrr라는 인스턴스를 추가로 생성해주었습니다.

```
struct sched_wrr_entity {
    struct list_head                run_list;
    unsigned int                    weight;
    unsigned int                    time_slice;
    unsigned short	                on_rq;
};
```

include/linux/init_task.h 파일에는 다음과 같이 wrr의 member를 초기화해주었습니다. default weight은 10이고, 그때의 time_slice은 100ms이므로 HZ / 10으로 초기화하였습니다. HZ의 value는 1초 동안 jiffies가 세는 tick의 수(버전에 따라 100 또는 1000), 어쨌든 1초를 의미하므로 HZ / 10은 0.1초(100ms)를 의미합니다.

```
.wrr = { 				    			             \
     .weight = 10,   					             \
     .time_slice = HZ / 10, 					     \
     .on_rq = 0, 						             \
     .run_list = LIST_HEAD_INIT(tsk.wrr.run_list), 	 \
}, 
```

kernel/sched/sched.h 파일에는 SCHED_WRR에 대한 policy를 생성하고 사용하면서, 구현하는 WRR scheduler가 작동할 수 있는 환경을 만들었습니다. 또한, sched_wrr_entity가 구성하는 WRR run queue인 wrr_rq 구조체를 정의했습니다. 이 구조체의 인스턴스는 kernel/sched/core.c에서 초기화됩니다.

```
struct wrr_rq {
    	struct list_head queue_head;
    	unsigned int total_weight;
    	unsigned int last_time;
};
```

### 2-2 Change priorities of schedulers
기존의 priority가 RT(Real Time) > CFS(Completely Fair Scheduler)였던 것을 RT > WRR > CFS로 변경해주기 위하여 kernel/sched/rt.c에서 rt_sched_class의 next를 wrr_sched_class로, kernel/sched/wrr.c에서 wrr_sched_class의 next를 fair_sched_class로 설정해주었습니다.

### 2-3 System Call functions
sched_setweight, sched_getweight 두 가지 System Call을 추가할 때는 Project 1에서 했던 것처럼 Dynamic하게 설정했습니다.  
398번, 399번 entry에 syscall을 추가하였으므로 arch/arm64/include/asm/unistd.h에서 __NR_compat_syscalls의 값을 400으로 늘렸습니다.

kernel/sched/core.c에는 sched_setweight, sched_getweight 함수를 작성하여 EXPORT_SYMBOL로 외부에서 접근 가능하도록 설정하였습니다.  
setweight에서는 pid가 음수일 경우, weight가 1과 20 사이의 값이 아닌 경우, Schedule Policy가 WRR이 아닌 경우 Invalid Argument Error를 리턴하도록 설정했으며, 
해당 pid를 가지는 process가 없는 경우 No Such Process Error를 리턴하도록 설정했습니다. 
pid가 0인 경우 calling process의 weight를 수정하도록 했습니다.  
권한의 경우 Weight를 조정하려는 user가 프로세스의 owner이거나 root인 경우에 변경할 수 있으며, 그렇지 않은 경우 Permission Denied Error를 리턴하도록 설정했습니다.  
Weight를 증가시키는 경우에는 root인 경우에만 가능하도록 하였고, 해당 프로세스가 포함된 run_queue를 찾아 total_weight를 수정한 후 프로세스의 weight를 수정합니다.  
getweight에서 pid와 관련된 처리는 setweight와 동일하며, 문제가 없다면 해당 pid를 가지는 프로세스의 weight를 리턴하도록 설정했습니다.

Dynamic한 system call 할당을 위해 wrr_mod.c 파일을 추가했으며, extern 함수로 project1과 동일하게 system call table을 수정했습니다.  
이 때 system call table을 수정하기 위해 arch/arm64/kernel/sys32.c의 compat_sys_call_table을 project1에서와 같이 const하지 않도록 수정했습니다.  
wrr_mod.c를 컴파일 하기 위해 kernel/sched/Makefile에 다음과 같은 줄을 추가했습니다.
```
obj-m += wrr_mod.o
```
컴파일 하면 wrr_mod.ko 파일을 생성하며, 이를 rootfs에 넣어 /root/wrr_mod.ko에 위치하도록 설정했습니다.

### 2-4 Load-Balancing

### 2-5 Other functions
