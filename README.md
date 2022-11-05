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
### 2.1 Define constants and implement data structures
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

### 2.2 Change priorities of schedulers
기존의 priority가 RT(Real Time) > CFS(Completely Fair Scheduler)였던 것을 RT > WRR > CFS로 변경해주기 위하여 kernel/sched/rt.c에서 rt_sched_class의 next를 wrr_sched_class로, kernel/sched/wrr.c에서 wrr_sched_class의 next를 fair_sched_class로 설정해주었습니다.

### 2.3 System Call functions
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

### 2.4 Load-Balancing
kernel/sched/core.c에서 sched_fork와 __setscheduler 함수에서 현재 프로세스의 policy에 따라 sched_class 구조체를 설정할 때 WRR이 제대로 작동하도록 수정하였습니다.  
__sched_setscheduler 함수에서는 해당 pid의 cpu affinity mask를 받아 include/linux/sched.h에서 설정한 CPU_WITHOUT_WRR 값에 따라 0번 core에는 WRR과 관련된 프로세스가 실행되지 않도록 설정했습니다.  
scheduler_tick 함수에서는 wrr.c에 정의된 trigger_load_balance_wrr 함수를 호출하여 매 tick마다, last time으로부터 curren time까지 2000ms(2 * HZ)가 지났는지 확인해서, 지났다면 load_balance를 수행하도록 설정했습니다. load_balance를 수행하는 과정은, 먼저 lock을 잡고 total_weight이 가장 큰 run queue와 가장 작은 run queue를 찾은 뒤 그 rq_max에서 다시 조건에 맞는 weight이 가장 큰 task를 찾아 rq_min으로 옮기는 것입니다.

### 2.5 Functions for member function pointers of wrr_sched_class
kernel/sched/wrr.c 파일에는  trigger_load_balance_wrr뿐만 아니라 wrr_sched_class 초기화 시에 멤버 함수 포인터들이 호출하는 함수 중 필요한 것들을 정의했습니다. 그 중에서도 구현한 함수는 다음과 같습니다.

```
static void enqueue_task_wrr(struct rq *rq, struct task_struct *p, int flags)
static void dequeue_task_wrr(struct rq *rq, struct task_struct *p, int flags)
static struct task_struct *pick_next_task_wrr(struct rq *rq, struct task_struct *prev, struct rq_flags *rf)
static int select_task_rq_wrr(struct task_struct *p, int cpu, int sd_flag, int flags)
static void task_tick_wrr(struct rq *rq, struct task_struct *p, int queued)
```
#### enqueue_task_wrr
새로운 task가 생성되었을 때 실행되는 함수로, 본 task를 초기화한 뒤에 run queue의 tail에 추가하면서, total weight을 증가시킵니다. scheduler entry에 대해서는, time slice를 주어진 weight 에 맞게 설정하고, on_rq 또한 켭니다. 그리고 다시 스케줄링합니다.

#### dequeue_task_wrr
기존의 task가 종료될 때 실행되는 함수로, 본 task를 run_queue에서 제거한 뒤에 초기화하면서, total weight을 감소시킵니다. 그리고 다시 스케줄링합니다.

#### pick_next_task_wrr
다음 task 수행을 위해, run queue가 비었는지 확인하고, 비어있지 않다면 그 queue의 front의 task를 반환하는 함수입니다.

#### select_task_rq_wrr
WRR policy를 따르는 CPU 중 total weight이 가장 작은 CPU 번호를 반환하는 함수입니다.

#### task_tick_wrr
scheduler_tick 함수에서 매 tick마다 호출되는 함수로, 매 tick마다 time_slice를 감소시켜 0으로 만들면(즉, time_slice에 해당하는 시간이 흐르면) 본 task를 run queue의 tail로 이동시킵니다. 그리고 다시 스케줄링합니다.(task의 finish에 관해서는 dequeue_task_wrr 함수가 관여하지, 이 함수는 관여하지 않습니다.)

### 2.6 Debug
sched_debug와 schedstat의 내용을 확인하기 위해 arch/arm64/configs/tizen_bcmrpi3_defconfig에서 CONFIG_SCHED_DEBUG와 CONFIG_SCHEDSTATS의 값을 y로 변경했습니다.

kernel/sched/debug.c에서 print_wrr_rq를 통해 주어진 cpu core의 run queue에 할당된 프로세스의 weight 총 합을 출력하도록 설정하였고, 
print_wrr_stats를 통해 각 cpu마다 weight 값을 출력하도록 설정했습니다.  
디버깅을 위해 print_cpu 함수에서 cfs, rt, dl stats는 출력하지 않도록 하고, print_wrr_stats를 호출하도록 하여 wrr 관련 정보만 출력하도록 했습니다.

## 3. Investigation
실험은 test1.c를 컴파일하여 rootfs에 넣은 /root/test1 파일을 통해 진행했습니다.  
test1에서는 WRR로 scheduler를 설정한 후, fork를 통해 자식 프로세스를 만들어 값 2,142,000,138 = 357000023 * 2 * 3에 대해 소인수분해를 수행한 시간을 출력하도록 구현했습니다.  
실행 결과는 다음과 같습니다.  

|Weight|Time[sec]|
|:----:|:-------:|
|1|125.9846|
|2|118.3837|
|3|113.9740|
|4|109.4155|
|5|107.4999|
|6|102.9470|
|7|100.0511|
|8|97.7505|
|9|95.4235|
|10|92.3051|
|11|89.4204|
|12|81.7225|
|13|78.8406|
|14|78.6654|
|15|76.5051|
|16|71.0799|
|17|68.2312|
|18|65.0233|
|19|58.3411|
|20|55.3458|

실행 결과에 대한 그래프는 plot.pdf로 최상위 경로에 첨부해두었으며, 기본적으로 Weight가 클수록 계산 시간이 더 작은 경향을 보이는 것을 확인할 수 있습니다.  
이번 실행 결과에는 계산 시간이 내림차순으로 잘 나왔으나, 여러 번 실행해봤을 때 weight가 작더라도 수행 시간이 더 적게 걸리는 경우를 확인할 수 있었습니다.
이는 Load Balancing을 수행하더라도 각 run queue의 total_weight 값이 동일하지 않을 수 있는데, 
계산할 task가 여러 번 time_slice를 전부 소모해야 완료할 수 있을 정도로 복잡하다면 total_weight가 작은 run_queue에서는 schedule 순서가 더 빨리 다가오므로 
weight가 더 작더라도 연산을 빨리 끝낼 수 있어 나타나는 현상으로 볼 수 있습니다.

## 4. Lesson Learned
### 4.1
test 파일을 통해 실험을 수행하면서 kernel panic이 자주 발생하였기 때문에 실험을 진행하고, 디버깅하는 과정이 매우 어려웠습니다. qemu.sh에서 메모리의 용량을 늘리거나 
cpu 코어 수를 줄일 경우 kernel panic이 발생하는 빈도수가 약간 줄어들긴 했으나, 근본적인 해결책은 되지 못했습니다. 디버깅에 사용한 sched_debug의 경우 tizen kernel 자체에 
문제가 발생할 경우 종료할 수밖에 없어 확인히 불가능하였기 때문에, Kernel을 개발하는 과정에서 에러가 얼마나 치명적이고 다루기 어려운지 실감할 수 있었습니다.

### 4.2
CPU_WITHOUT_WRR 값을 include/linux/sched.h에서 정의하였는데, 값 하나만 바꿨는데도 build를 다시 실행했을 때 수많은 연관된 파일들을 다시 컴파일 해야 돼서 
오랜 시간 동안 build가 완료되기를 기다리게 되었습니다. 이를 통해 kernel에는 정말 수많은 코드가 존재하고 서로 수많은 코드를 참조하면서 연결되어 있음을 알게 되었습니다.

### 4.3
앞서 CPU_WITHOUT_WRR을 0으로 설정하여 Core 0에는 WRR 관련 Task를 Schedule 하지 않도록 설정하였는데, 이 과정에서 cpu affinity mask를 통해 
해당 프로세스가 할당될 수 있는 cpu를 설정하는 방식을 통해 각 프로세스가 할당될 수 있는 cpu를 집합의 형태로 저장한다는 것을 알 수 있었습니다.
