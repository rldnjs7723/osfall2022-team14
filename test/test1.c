#define _GNU_SOURCE
#include <unistd.h>
#include <stdlib.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <stdio.h>
#include <time.h>
#include <pthread.h>

#define SCHED_WRR 7
#define SCHED_SETWEIGHT 398
#define SCHED_GETWEIGHT 399

void prime_factor(int num) {
    printf("%d = ", num);
    int i = 2;
    while (i < num) {
        if (num % i)
            i++;
        else {
            num /= i;
            printf("%d X ", i);
        }
    }
    printf("%d\n", num);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
	printf("Type \"/root/test1 [process_num]\"\n");
	return -1;
    }
    int num = 357000023 * 2 * 3;
    int process_num = atoi(argv[1]);
    int weight;
    struct sched_param param;
    struct timespec start, end;
    double time_interval;
    param.sched_priority = 0;

    if (sched_setscheduler(getpid(), SCHED_WRR, &param) < 0)
        return -1;
    
    printf("Running %d processes whose weights are 1 through %d respectively\n", process_num, process_num);
    
    for (int i = 1; i <= process_num; i++) {
        weight = i;
        if (fork() == 0) {
            if (syscall(SCHED_SETWEIGHT, getpid(), weight) <= 0)
                exit(-1);
            clock_gettime(CLOCK_MONOTONIC, &start);
            prime_factor(num);
            clock_gettime(CLOCK_MONOTONIC, &end);
            time_interval = (double)(end.tv_sec - start.tv_sec) +
                (double)(end.tv_nsec - start.tv_nsec) / 1000000000;
            printf("Process with weight %ld spent %fsec\n", syscall(SCHED_GETWEIGHT, getpid()), time_interval);
            exit(0);         
        }
    }

    return 0;
}
