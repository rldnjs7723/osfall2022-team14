#include <linux/rotation.h>
#include <linux/sched.h>
#include <linux/syscalls.h>
#include <linux/slab.h>
#include <linux/list.h>
#include <linux/mutex.h>
#include <linux/wait.h>

static int rotation = 0;
static int write_waiting_cnt[360] = {0, };

static LIST_HEAD(read_waiting);
static LIST_HEAD(write_waiting);
static LIST_HEAD(read_acquired);
static LIST_HEAD(write_acquired);

static DEFINE_MUTEX(mutex);
static DECLARE_WAIT_QUEUE_HEAD(wait_queue);

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

int get_lock(void) {
    rotlock_t *curr;
    int cnt = 0;

    if (write_waiting_cnt[rotation] != 0 && list_empty(&write_acquired) && list_empty(&read_acquired)) {
        list_for_each_entry(curr, &write_waiting, node) {
            if (check_rotation(rotation, curr->degree, curr->range)) {
                cnt++;
                curr->cond = 1;
	            wake_up_process(find_task_by_vpid(curr->pid));
                return 1;
            }
        }
    }
    else if (write_waiting_cnt[rotation] == 0 && list_empty(&write_acquired)) {
        list_for_each_entry(curr, &read_waiting, node) {
            if (check_rotation(rotation, curr->degree, curr->range)) {
                cnt++;
                curr->cond = 1;
	            wake_up_process(find_task_by_vpid(curr->pid));
            }
        }
    }
    return cnt;
}

int check_rotation(int rotation, int degree, int range) {
    return (rotation >= degree - range - 360 && rotation <= degree + range - 360)
        || (rotation >= degree - range && rotation <= degree + range)
        || (rotation >= degree - range + 360 && rotation <= degree + range + 360);
}

void exit_rotlock(struct task_struct *p) {
    pid_t pid = p->pid;
    rotlock_t *curr;
    rotlock_t *next;
    int i;

    mutex_lock(&mutex);

    list_for_each_entry_safe(curr, next, &read_waiting, node) {
        if (curr->pid == pid) {
            list_del(&curr->node);
        }
    }
    list_for_each_entry_safe(curr, next, &write_waiting, node) {
        if (curr->pid == pid) {
            list_del(&curr->node);
            for (i = (curr->degree - curr->range + 360) % 360; i != curr->degree + curr->range + 1; i = (i + 1) % 360)
                write_waiting_cnt[i] = 0;
        }
    }
    list_for_each_entry_safe(curr, next, &read_acquired, node) {
        if (curr->pid == pid) {
            list_del(&curr->node);
        }
    }
    if (list_empty(&read_acquired)) {
        get_lock();
    }
    curr = list_first_entry(&write_acquired, rotlock_t, node);
    if (curr->pid == pid) {
        list_del(&curr->node);
        get_lock();
    }
    mutex_unlock(&mutex);
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

int find_node_and_del(int degree, int range, struct list_head* head) {
    rotlock_t* curr;
    rotlock_t* next;
    int cnt = 0;

    list_for_each_entry_safe(curr, next, head, node) {
        if (curr->pid == current->pid && curr->degree == degree && curr->range == range) {
            list_del(&curr->node);
            cnt++;
            kfree(curr);
        }
    }
    return cnt;
}

SYSCALL_DEFINE1(set_rotation, int, degree) {
    int cnt = 0;
    if (!check_error(degree, 1)) return -1;
    mutex_lock(&mutex);
    rotation = degree;
    cnt = get_lock();
    mutex_unlock(&mutex);
    return cnt;
}

SYSCALL_DEFINE2(rotlock_read, int, degree, int, range) {
    rotlock_t *rotlock;

    if (!check_error(degree, range)) return -1;

    rotlock = init_rotlock(degree, range);
    mutex_lock(&mutex);
    list_add_tail(&rotlock->node, &read_waiting);
    while (!check_rotation(rotation, degree, range) || write_waiting_cnt[rotation] != 0 || !list_empty(&write_acquired)) {
        mutex_unlock(&mutex);
        rotlock->cond = 0;
        while (!rotlock->cond) schedule();
        mutex_lock(&mutex);
    }
    list_del(&rotlock->node);
    list_add_tail(&rotlock->node, &read_acquired);
    mutex_unlock(&mutex);
    return 0;
}

SYSCALL_DEFINE2(rotlock_write, int, degree, int, range) {
    rotlock_t *rotlock;
    int i;

    if (!check_error(degree, range)) return -1;

    rotlock = init_rotlock(degree, range);
    mutex_lock(&mutex);    
    if (check_rotation(rotation, degree, range) && list_empty(&read_acquired) && list_empty(&write_acquired)) {
        list_add_tail(&rotlock->node, &write_acquired);
        mutex_unlock(&mutex);
        return 0;
    }
    list_add_tail(&rotlock->node, &write_waiting);
    for (i = (degree - range + 360) % 360; i != degree + range + 1; i = (i + 1) % 360)
        write_waiting_cnt[i]++;
    while (!check_rotation(rotation, degree, range) || !list_empty(&read_acquired) || !list_empty(&write_acquired)) {
        mutex_unlock(&mutex);
        rotlock->cond = 0;
        while (!rotlock->cond) schedule();
        mutex_lock(&mutex);
    }
    for (i = (degree - range + 360) % 360; i != degree + range + 1; i = (i + 1) % 360)
        write_waiting_cnt[i]--;
    list_del(&rotlock->node);
    list_add_tail(&rotlock->node, &write_acquired);
    mutex_unlock(&mutex);
    return 0;
}

SYSCALL_DEFINE2(rotunlock_read, int, degree, int, range) {
    int cnt;

    if (!check_error(degree, range)) return -1;

    mutex_lock(&mutex);

    cnt = find_node_and_del(degree, range, &read_acquired);

    if (list_empty(&read_acquired)) {
        get_lock();
    }
    mutex_unlock(&mutex);
    return 0;
}

SYSCALL_DEFINE2(rotunlock_write, int, degree, int, range) {
    int cnt;

    if (!check_error(degree, range)) return -1;

    mutex_lock(&mutex);

    cnt = find_node_and_del(degree, range, &write_acquired);

    get_lock();

    mutex_unlock(&mutex);
    return 0;
}
