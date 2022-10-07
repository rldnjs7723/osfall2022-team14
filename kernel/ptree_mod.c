#include <uapi/asm-generic/errno-base.h> // Error Code Ref
#include <asm/uaccess.h> // User Space Memory Access
#include <linux/slab.h> // kmalloc, kfree
#include <linux/sched.h> // task_struct
#include <linux/sched/task.h> 
#include <linux/list.h> // Doubly Linked List in Linux Kernel
#include <linux/unistd.h>
#include <linux/syscalls.h>
#include <linux/module.h>
#include <linux/moduleparam.h>


#define __ptree_syscalls		398

MODULE_LICENSE("GPL v2");

// For utilizing kernel symbols
struct prinfo {
  int64_t state;            /* current state of process */
  pid_t   pid;              /* process id */
  pid_t   parent_pid;       /* process id of parent */
  pid_t   first_child_pid;  /* pid of oldest child */
  pid_t   next_sibling_pid; /* pid of younger sibling */
  int64_t uid;              /* user id of process owner */
  char    comm[64];         /* name of program executed */
};

extern void* compat_sys_call_table[];
extern rwlock_t tasklist_lock;

void* legacy_syscall = NULL;

int ptree(struct prinfo *buf, int *nr) {

    int nr_kernel;
    int count = 0;
    struct prinfo *buf_kernel;
    
    count = 10;

    return count;
}

static int ptree_mod_init(void) {
    printk("module loaded\n");
    legacy_syscall = compat_sys_call_table[__ptree_syscalls];
    compat_sys_call_table[__ptree_syscalls] = ptree;

    return 0;
}

static void ptree_mod_exit(void) {
    // Revert syscall to legacy
    printk("module exit\n");
    compat_sys_call_table[__ptree_syscalls] = legacy_syscall;
}

module_init(ptree_mod_init);
module_exit(ptree_mod_exit);
