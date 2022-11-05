#include <linux/module.h>

/* For Dynamic System Call */ 
#define SCHED_SETWEIGHT    398
#define SCHED_GETWEIGHT    399

MODULE_LICENSE("GPL v2");

extern long sched_setweight(pid_t pid, int weight);
extern long sched_getweight(pid_t pid);
extern void* compat_sys_call_table[];
void* legacy_syscall_prevset = NULL;
void* legacy_syscall_prevget = NULL;

static int wrr_mod_init(void) {
    printk("module loaded\n");
    legacy_syscall_prevset = compat_sys_call_table[SCHED_SETWEIGHT];
    legacy_syscall_prevget = compat_sys_call_table[SCHED_GETWEIGHT];
    compat_sys_call_table[SCHED_SETWEIGHT] = sched_setweight;
    compat_sys_call_table[SCHED_GETWEIGHT] = sched_getweight;

    return 0;
}
static void wrr_mod_exit(void) {
    printk("module exit\n");
    compat_sys_call_table[SCHED_SETWEIGHT] = legacy_syscall_prevset;
    compat_sys_call_table[SCHED_GETWEIGHT] = legacy_syscall_prevget;
}

module_init(wrr_mod_init);
module_exit(wrr_mod_exit);