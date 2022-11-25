#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/list.h>
#include <linux/mutex.h>
#include <linux/wait.h>
#include <linux/syscalls.h>
#include <linux/rotation.h>

#define PLUS        1
#define MINUS      -1
#define PREV_WRITE  0
#define PREV_READ   1

static int rotation = 0;
static int rotation_cnt_write_waiting[360] = {0, };

static LIST_HEAD(read_waiting);
static LIST_HEAD(write_waiting);
static LIST_HEAD(read_acquired);
static LIST_HEAD(write_acquired);

static DEFINE_MUTEX(mutex);
static DECLARE_WAIT_QUEUE_HEAD(wait_queue);

void wait(rotlock_t * rotlock)
{
    DEFINE_WAIT(wait_entry);
	rotlock->cond = 0;
	add_wait_queue(&wait_queue, &wait_entry);
	while(!rotlock->cond) {
		prepare_to_wait(&wait_queue, &wait_entry, TASK_INTERRUPTIBLE);
		schedule();
	}
	finish_wait(&wait_queue, &wait_entry);
}

/* 0 <= degree < 360 , 0 < range < 180 */
int check_error(int degree, int range) {
    if (degree < 0 || degree >= 360) {
        printk("Degree should be 0 <= degree < 360\n");
        return 0;
    }
    if (range <= 0 || range >= 180) {
        printk("Range should be 0 < range < 180\n");
        return 0;
    }
    return 1;
}

/* degree - range <= LOCK RANGE <= degree + range */
int check_rotation(int rotation, int degree, int range) {
    return (rotation >= degree - range - 360 && rotation <= degree + range - 360)
        || (rotation >= degree - range && rotation <= degree + range)
        || (rotation >= degree - range + 360 && rotation <= degree + range + 360);
}

void count_rotation(int degree, int range, int change) {
    int rot;
    for(rot = 0; rot < 360; rot++) {
        if(check_rotation(rot, degree, range)) {
            rotation_cnt_write_waiting[rot] += change;
        }
    }
}

// Compare rotation with read/write_lock range and take lock
int get_lock(int prev_lock) {
    rotlock_t *curr;
    int cnt = 0;

    if (prev_lock == PREV_READ && rotation_cnt_write_waiting[rotation] && list_empty(&write_acquired) && list_empty(&read_acquired)) {
        // take write_lock after release read_lock
        list_for_each_entry(curr, &write_waiting, node) {
            if (check_rotation(rotation, curr->degree, curr->range)) {
                cnt++;
                curr->cond = 1;
                wake_up_process(find_task_by_vpid(curr->pid));
                return cnt;
            }
        }
    } else {
        // take read_lock after release write_lock
        if (list_empty(&write_acquired)) {
            list_for_each_entry(curr, &read_waiting, node) {
                if (check_rotation(rotation, curr->degree, curr->range)) {
                    cnt++;
                    curr->cond = 1;
                    wake_up_process(find_task_by_vpid(curr->pid));
                }
            }
        }

        // if no read_lock taken, take write_lock
        if (cnt == 0 && rotation_cnt_write_waiting[rotation] && list_empty(&write_acquired) && list_empty(&read_acquired)) {
            list_for_each_entry(curr, &write_waiting, node) {
                if (check_rotation(rotation, curr->degree, curr->range)) {
                    cnt++;
                    curr->cond = 1;
                    wake_up_process(find_task_by_vpid(curr->pid));
                    return cnt;
                }
            }
        }
    }
    return cnt;
}

rotlock_t* init_rotlock(int degree, int range) {
    rotlock_t* rotlock;
    rotlock = (rotlock_t *)kmalloc(sizeof(rotlock_t), GFP_KERNEL);
    rotlock->pid = current->pid;
    rotlock->degree = degree;
    rotlock->range = range;
    rotlock->cond = 0;
    INIT_LIST_HEAD(&rotlock->node);

    return rotlock;
}

// Release holding locks & Remove waiting locks
void exit_rotlock(struct task_struct *p) {
    pid_t pid = p->pid;
    rotlock_t *curr;
    rotlock_t *next;
    int prev_lock = -1;

    mutex_lock(&mutex);

    list_for_each_entry_safe(curr, next, &read_waiting, node) {
        if(curr->pid == pid) {
            list_del(&curr->node);
        }
    }
    list_for_each_entry_safe(curr, next, &write_waiting, node) {
        if(curr->pid == pid) {
            list_del(&curr->node);
            count_rotation(curr->degree, curr->range, MINUS);
        }
    }
    list_for_each_entry_safe(curr, next, &read_acquired, node) {
        if(curr->pid == pid) {
            list_del(&curr->node);
            prev_lock = PREV_READ;
        }
    }
    if (prev_lock == PREV_READ) {
        get_lock(PREV_READ);
    } else {
        list_for_each_entry_safe(curr, next, &write_acquired, node) {
            if(curr->pid == pid) {
                list_del(&curr->node);
                prev_lock = PREV_WRITE;
            }
        }
        get_lock(PREV_WRITE);
    }
    
    mutex_unlock(&mutex);
}

/*
 * sets the current device rotation in the kernel.
 * syscall number 398
 * long set_rotation(int degree)
 */
SYSCALL_DEFINE1(set_rotation, int, degree) {
    long cnt = 0;

    /* 0 <= degree < 360 */
    if (!check_error(degree, 1)) return -1;

    mutex_lock(&mutex);
    rotation = degree;
    // printk("current rotation: %d\n", rotation);

    // Allow a single blocked writer to take the lock first
    cnt = get_lock(PREV_READ);
    mutex_unlock(&mutex);
    return cnt;
}

/*
 * Take a read lock using the given rotation range
 * returning 0 on success, -1 on failure.
 * system call numbers 399
 * long rotlock_read(int degree, int range)
 */
SYSCALL_DEFINE2(rotlock_read, int, degree, int, range) {
    rotlock_t *rotlock;
    /* 0 <= degree < 360 , 0 < range < 180 */
    if (!check_error(degree, range)) return -1;

    rotlock = init_rotlock(degree, range);
    mutex_lock(&mutex);

    list_add_tail(&rotlock->node, &read_waiting);
    while (!check_rotation(rotation, degree, range) || rotation_cnt_write_waiting[rotation] || !list_empty(&write_acquired)) {
        mutex_unlock(&mutex);
        while (!rotlock->cond) wait(rotlock);
        mutex_lock(&mutex);
    }
    list_del(&rotlock->node);
    list_add_tail(&rotlock->node, &read_acquired);

    mutex_unlock(&mutex);
    return 0;
}

/*
 * Take a write lock using the given rotation range
 * returning 0 on success, -1 on failure.
 * system call numbers 400
 * long rotlock_write(int degree, int range)
 */
SYSCALL_DEFINE2(rotlock_write, int, degree, int, range) {
    rotlock_t *rotlock;
    /* 0 <= degree < 360 , 0 < range < 180 */
    if (!check_error(degree, range)) return -1;

    rotlock = init_rotlock(degree, range);
    mutex_lock(&mutex);

    // Wait Lock
    list_add_tail(&rotlock->node, &write_waiting);
    count_rotation(degree, range, PLUS);
    while (!check_rotation(rotation, degree, range) || !list_empty(&write_acquired) || !list_empty(&read_acquired)) {
        rotlock->cond = 0;

        mutex_unlock(&mutex);
        while (!rotlock->cond) wait(rotlock);
        mutex_lock(&mutex);
    }
    // Take Lock
    list_del(&rotlock->node);
    list_add_tail(&rotlock->node, &write_acquired);
    count_rotation(degree, range, MINUS);

    mutex_unlock(&mutex);
    return 0;
}

/*
 * Release a read lock using the given rotation range
 * returning 0 on success, -1 on failure.
 * system call numbers 401
 * long rotunlock_read(int degree, int range)
 */
SYSCALL_DEFINE2(rotunlock_read, int, degree, int, range) {
    rotlock_t* curr;
    rotlock_t* next;
    /* 0 <= degree < 360 , 0 < range < 180 */
    if (!check_error(degree, range)) return -1;

    mutex_lock(&mutex);

    list_for_each_entry_safe(curr, next, &read_acquired, node) {
        // A process cannot release locks acquired by other processes
        if(curr->pid == current->pid && curr->degree == degree && curr->range == range) {
            list_del(&curr->node);
            kfree(curr);
        }
    }

    if (list_empty(&read_acquired)) {
        get_lock(PREV_READ);
    }

    mutex_unlock(&mutex);
    return 0;
}

/*
 * Release a write lock using the given rotation range
 * returning 0 on success, -1 on failure.
 * system call numbers 402
 * long rotunlock_write(int degree, int range)
 */
SYSCALL_DEFINE2(rotunlock_write, int, degree, int, range) {
    rotlock_t* curr;
    rotlock_t* next;
    /* 0 <= degree < 360 , 0 < range < 180 */
    if (!check_error(degree, range)) return -1;

    mutex_lock(&mutex);

    list_for_each_entry_safe(curr, next, &write_acquired, node) {
        // A process cannot release locks acquired by other processes
        if(curr->pid == current->pid && curr->degree == degree && curr->range == range) {
            list_del(&curr->node);
            kfree(curr);
        }
    }

    get_lock(PREV_WRITE);

    mutex_unlock(&mutex);
    return 0;
}