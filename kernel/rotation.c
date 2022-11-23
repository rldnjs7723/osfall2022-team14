#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/list.h>
#include <linux/mutex.h>
#include <linux/wait.h>

void exit_rotlock(struct task_struct *tsk) {
    //TODO
    //Release Holding Locks
    //Remove Holding Locks
}

long set_rotation(int degree){
    return 0;
}

long rotlock_read(int degree, int range){
    return 0;
}

long rotlock_write(int degree, int range){
    return 0;
}

long rotunlock_read(int degree, int range){
    return 0;
}

long rotunlock_write(int degree, int range){
    return 0;
}

EXPORT_SYMBOL(exit_rotlock);
EXPORT_SYMBOL(set_rotation);
EXPORT_SYMBOL(rotlock_read);
EXPORT_SYMBOL(rotlock_write);
EXPORT_SYMBOL(rotunlock_read);
EXPORT_SYMBOL(rotunlock_write);