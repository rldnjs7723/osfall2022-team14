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

int check_error(int degree, int range);
int get_lock(void);
int check_rotation(int rotation, int degree, int range);
rotlock_t* init_rotlock(int degree, int range);
int find_node_and_del(int degree, int range, struct list_head* head);
void exit_rotlock(struct task_struct *p);
long set_rotation(int degree);
long rotlock_read(int degree, int range);
long rotlock_write(int degree, int range);
long rotunlock_read(int degree, int range);
long rotunlock_write(int degree, int range);
