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

void print_process(struct prinfo *plist, int nr)
{
	struct prinfo p;
	int *parent_index_list = (int *) malloc(sizeof(int) * nr);
	int parent_index;
	for (int i = 0; i < nr; i++) {
		p = plist[i];
		
		// for checking if process right before this process is a parent of it
		parent_index = i - 1;
		
		// find index of this' parent using the fact that the previous process is
		// a parent of this process or has the common parent of this process
		while (parent_index >= 0 && p.parent_pid != plist[parent_index].pid)
			parent_index = parent_index_list[parent_index];
		
		// store the index of this' parent (parent_index is -1 for root process)
		parent_index_list[i] = parent_index;
		
		// print "\t" by distance from the root process
		while (parent_index >= 0) {
			printf("\t");
			parent_index = parent_index_list[parent_index];
		}
		printf("%s,%d,%lld,%d,%d,%d,%lld\n", p.comm, p.pid, p.state, p.parent_pid, p.first_child_pid, p.next_sibling_pid, p.uid);
	}
}

int main(int argc, char *argv[]) {
	if(argc != 2)
	{
		printf("Type \"/root/test [nr]\"");
		return -1;
	}
	int *nr = (int *) malloc(sizeof(int));
	*nr = atoi(argv[1]);
	int tmp = *nr;
	struct prinfo *plist = (struct prinfo *) malloc(sizeof(struct prinfo) * (*nr));
	
	int total = syscall(__ptree_syscalls, plist, nr);
	print_process(plist, *nr);
	if (tmp == *nr) printf("The value of nr is %d\n", *nr);
	else printf("The value of nr was changed from %d to %d\n", tmp, *nr);
	printf("Total number of process is %d\n", total);
	free(nr);
	free(plist);
}
