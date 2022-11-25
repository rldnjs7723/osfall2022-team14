#include <linux/list.h>
#include <linux/types.h>
#include <linux/sched.h>

typedef struct __rotlock_t__ {
    pid_t pid;
    int degree;
    int range;
    int cond;
    struct list_head node;
} rotlock_t;

void wait(rotlock_t * rotlock);
int check_error(int degree, int range);
int check_rotation(int rotation, int degree, int range);
void count_rotation(int degree, int range, int change);
int get_lock(int prev_read);
rotlock_t* init_rotlock(int degree, int range);
void exit_rotlock(struct task_struct *p);
long set_rotation(int degree);
long rotlock_read(int degree, int range);
long rotlock_write(int degree, int range);
long rotunlock_read(int degree, int range);
long rotunlock_write(int degree, int range);
