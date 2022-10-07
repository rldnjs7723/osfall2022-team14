#include <uapi/asm-generic/errno-base.h> // Error Code Ref
#include <asm/uaccess.h> // User Space Memory Access
#include <linux/slab.h> // kmalloc, kfree
#include <linux/sched.h> // task_struct
#include <linux/list.h> // Doubly Linked List in Linux Kernel
#include <linux/module.h>
#include <linux/syscalls.h>

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

// copy prinfo data
void copy_prinfo(struct prinfo *target, struct task_struct *task) {
    struct list_head child_list = task->children;
    struct list_head sibling_list = task->sibling;

    target->state = task->state;
    target->pid = task->pid;
    target->parent_pid = task->parent->pid;

    if(list_empty(&child_list) == 0) {
        target->first_child_pid = list_entry(child_list.next, struct task_struct, sibling)->pid;
    } else target->first_child_pid = 0;

    if(list_empty(&sibling_list) == 0) {
        target->next_sibling_pid = list_entry(sibling_list.next, struct task_struct, sibling)->pid;
    } else target->next_sibling_pid = 0;

    target->uid = (int64_t) (task->real_cred->uid.val);
    strncpy(target->comm, task->comm, TASK_COMM_LEN);
}

int task_traverse(struct prinfo *buf , int *nr) {
    int count = 0;
    struct task_struct *curr_task = &init_task;
    struct list_head *head;

    while(1) {
        if(count < *nr) {
            copy_prinfo(&buf[count++], curr_task);
        } else count++;

        if(list_empty(&curr_task->children)) {
            head = &curr_task->real_parent->children;
            // Find remain sibling
            while(list_is_last(&curr_task->sibling, head)) {
                curr_task = curr_task->real_parent;
                head = &curr_task->real_parent->children;
            }
            head = &curr_task->sibling;
            curr_task = list_entry(head->next, struct task_struct, sibling);
        } else {
            head = &curr_task->children;
            curr_task = list_entry(head->next, struct task_struct, sibling);
        }

        if(curr_task->pid == init_task.pid) break;
    }

    return count;
}

int ptree(struct prinfo *buf, int *nr) {

    int nr_max, nr_kernel = 0;
    struct prinfo *kernel_prinfos;

    if (buf == NULL || nr == NULL) {
        printk("ERROR: Invalid Argument");
        return -EINVAL;
    }

    // get nr from user
    if (copy_from_user(&nr_max, nr, sizeof(int))) {
        printk("ERROR: User space nr copy fault");
        return -EFAULT;
    }

    if (nr_max <= 1) {
        printk("ERROR: Invalid nr argument");
        return -EINVAL;
    }

    kernel_prinfos = (struct prinfo *) kmalloc(sizeof(struct prinfo) * nr_max, GFP_KERNEL);

    if (kernel_prinfos == NULL) {
        printk("ERROR: Kernel kmalloc err");
        return -EFAULT;
    }

    // Lock Until Traversal Done
    read_lock(&tasklist_lock);

    nr_kernel = task_traverse(kernel_prinfos, &nr_max);

    read_unlock(&tasklist_lock);

    if (nr_max > nr_kernel) nr_max = nr_kernel;

    if (copy_to_user(nr, &nr_max, sizeof(int))) {
        printk("ERROR: User space nr paste fault");
        return -EFAULT;
    }

    if (copy_to_user(buf, kernel_prinfos, sizeof(struct prinfo) * nr_max)) {
        printk("ERROR: User space nr paste fault");
        return -EFAULT;
    }

    kfree(kernel_prinfos);

    return nr_kernel;
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
