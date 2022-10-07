#define _GNU_SOURCE
#include <unistd.h>
#include <stdlib.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <stdio.h>

#define __ptree_syscalls		398

struct prinfo {
  int64_t state;            /* current state of process */
  pid_t   pid;              /* process id */
  pid_t   parent_pid;       /* process id of parent */
  pid_t   first_child_pid;  /* pid of oldest child */
  pid_t   next_sibling_pid; /* pid of younger sibling */
  int64_t uid;              /* user id of process owner */
  char    comm[64];         /* name of program executed */
};

int main(int argc, char *argv[])
{
	int *nr=(int*)malloc(sizeof(int));
	struct prinfo *plist = (struct prinfo*)malloc(sizeof(struct prinfo)*(*nr));
	
	int total = syscall(__ptree_syscalls, plist, nr);
    printf("count: %d\n", total);
	free(nr);
	free(plist);
}

