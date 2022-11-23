#include <linux/module.h>

/* For Dynamic System Call */ 
#define SET_ROTATION      398
#define ROTLOCK_READ      399
#define ROTLOCK_WRITE     400
#define ROTUNLOCK_READ    401
#define ROTUNLOCK_WRITE   402

MODULE_LICENSE("GPL v2");

extern long set_rotation(int degree);
extern long rotlock_read(int degree, int range);
extern long rotlock_write(int degree, int range);
extern long rotunlock_read(int degree, int range);
extern long rotunlock_write(int degree, int range);
extern void* compat_sys_call_table[];
void* legacy_syscall_398 = NULL;
void* legacy_syscall_399 = NULL;
void* legacy_syscall_400 = NULL;
void* legacy_syscall_401 = NULL;
void* legacy_syscall_402 = NULL;

static int rotation_mod_init(void) {
    printk("module loaded\n");
    legacy_syscall_398 = compat_sys_call_table[398];
    legacy_syscall_399 = compat_sys_call_table[399];
    legacy_syscall_400 = compat_sys_call_table[400];
    legacy_syscall_401 = compat_sys_call_table[401];
    legacy_syscall_402 = compat_sys_call_table[402];
    compat_sys_call_table[SET_ROTATION] = set_rotation;
    compat_sys_call_table[ROTLOCK_READ] = rotlock_read;
    compat_sys_call_table[ROTLOCK_WRITE] = rotlock_write;
    compat_sys_call_table[ROTUNLOCK_READ] = rotunlock_read;
    compat_sys_call_table[ROTUNLOCK_WRITE] = rotunlock_write;

    return 0;
}
static void rotation_mod_exit(void) {
    printk("module exit\n");
    compat_sys_call_table[398] = legacy_syscall_398;
    compat_sys_call_table[399] = legacy_syscall_399;
    compat_sys_call_table[400] = legacy_syscall_400;
    compat_sys_call_table[401] = legacy_syscall_401;
    compat_sys_call_table[402] = legacy_syscall_402;
}

module_init(rotation_mod_init);
module_exit(rotation_mod_exit);