#include "sched.h"
#include <uapi/asm-generic/errno-base.h> 
#include <asm/uaccess.h>
#include <linux/slab.h>     
#include <linux/kernel.h>   
#include <linux/list.h>     
#include <linux/jiffies.h>  
#include <linux/slab.h>
#include <linux/sched.h>
#include <linux/module.h>

void init_wrr_rq(struct wrr_rq *wrr_rq)
{
    INIT_LIST_HEAD(&wrr_rq->queue_head);
    wrr_rq->total_weight = 0;
    wrr_rq->last_time = get_jiffies_64();
}

void trigger_load_balance_wrr(struct rq *rq)
{
    int cpu;
    int max_cpu = 0;
    int min_cpu = 0;
    unsigned int min_total_weight = -1;
    unsigned int max_total_weight = 0;
    struct wrr_rq *wrr_rq = &rq->wrr;
    unsigned long last_time = wrr_rq->last_time;
    unsigned long curr_time = get_jiffies_64();
    struct wrr_rq *curr_rq;
    struct wrr_rq *max_wrr_rq = NULL;
    struct wrr_rq *min_wrr_rq = NULL;
    struct sched_wrr_entity *wrr_se = NULL;
    struct sched_wrr_entity *mig = NULL;
    struct task_struct *task;
    unsigned long flags;
    unsigned int weight;
    unsigned int max_weight = 0;

    if (time_before(curr_time, last_time + 2 * HZ))
        return;
    wrr_rq->last_time = curr_time;
    rcu_read_lock();

    for_each_online_cpu(cpu) {
        if (cpu) {
            curr_rq = &cpu_rq(cpu)->wrr;
            if (curr_rq->total_weight > max_total_weight) {
            	max_cpu = cpu;
            	max_total_weight = curr_rq->total_weight;
            }
            if (curr_rq->total_weight < min_total_weight) {
            	min_cpu = cpu;
            	min_total_weight = curr_rq->total_weight;
            }
        }
    }
    max_wrr_rq = &cpu_rq(max_cpu)->wrr;
    min_wrr_rq = &cpu_rq(min_cpu)->wrr;
    rcu_read_unlock();

    if (max_cpu == min_cpu || !max_cpu || !min_cpu)
        return;

    local_irq_save(flags);
    double_rq_lock(cpu_rq(max_cpu), cpu_rq(min_cpu));

    list_for_each_entry(wrr_se, &max_wrr_rq->queue_head, run_list) {
        task = container_of(wrr_se, struct task_struct, wrr);
        weight = wrr_se->weight;
        
        if (cpu_rq(max_cpu)->curr != task && cpumask_test_cpu(max_cpu, &task->cpus_allowed)
        && max_total_weight - weight >= min_total_weight + weight && weight > max_weight) {
        	mig = wrr_se;
        	max_weight = weight;
        }
    }
    if (mig) {
        task = container_of(mig, struct task_struct, wrr);
        deactivate_task(cpu_rq(max_cpu), task, 0);
        set_task_cpu(task, min_cpu);
        activate_task(cpu_rq(min_cpu), task, 0);
        resched_curr(cpu_rq(min_cpu));
    }
    double_rq_unlock(cpu_rq(max_cpu), cpu_rq(min_cpu));
    local_irq_restore(flags);
}

static void enqueue_task_wrr(struct rq *rq, struct task_struct *p, int flags)
{  
    struct wrr_rq *wrr_rq = &rq->wrr;
    struct sched_wrr_entity *wrr_se = &p->wrr;

    INIT_LIST_HEAD(&wrr_se->run_list);
    list_add_tail(&wrr_se->run_list, &wrr_rq->queue_head);

    wrr_se->time_slice = wrr_se->weight * HZ / 100;
    wrr_se->on_rq = 1;
    wrr_rq->total_weight += wrr_se->weight;
    add_nr_running(rq, 1);
    resched_curr(rq);
}

static void dequeue_task_wrr(struct rq *rq, struct task_struct *p, int flags)
{
    struct wrr_rq *wrr_rq = &rq->wrr;
    struct sched_wrr_entity *wrr_se = &p->wrr;

    list_del_init(&wrr_se->run_list);
    wrr_se->on_rq = 0;
    wrr_rq->total_weight -= wrr_se->weight;
    sub_nr_running(rq, 1);
    resched_curr(rq);    
}

static void yield_task_wrr(struct rq *rq)
{
}

static void check_preempt_curr_wrr(struct rq *rq, struct task_struct *p, int flags)
{
}

static struct task_struct *pick_next_task_wrr(struct rq *rq, struct task_struct *prev, struct rq_flags *rf)
{
    struct wrr_rq *wrr_rq = &rq->wrr;
    struct sched_wrr_entity *picked;
    if (list_empty(&wrr_rq->queue_head))
        return NULL;

    picked = list_first_entry(&wrr_rq->queue_head, struct sched_wrr_entity, run_list);
    struct task_struct *p = container_of(picked, struct task_struct, wrr);

    return p;
}

static void put_prev_task_wrr(struct rq *rq, struct task_struct *prev)
{
}

static int select_task_rq_wrr(struct task_struct *p, int cpu, int sd_flag, int flags)
{
    int online_cpu;
    int min_cpu = 0;
    unsigned int min_total_weight = -1;
    struct wrr_rq *curr_rq;

    if (!wrr_policy(p->policy))
        return -EINVAL;
    rcu_read_lock();
    for_each_online_cpu(online_cpu) {
        curr_rq = &cpu_rq(online_cpu)->wrr;
        if (curr_rq->total_weight < min_total_weight) {
            min_cpu = online_cpu;
            min_total_weight = curr_rq->total_weight;
        }
    }
    if (!min_cpu) {
        rcu_read_unlock();
        return -EINVAL;
    }
    rcu_read_unlock();
    return min_cpu;
}

static void rq_online_wrr(struct rq *rq)
{
}

static void rq_offline_wrr(struct rq *rq)
{
}

static void task_woken_wrr(struct rq *rq, struct task_struct *p)
{
}

static void switched_from_wrr(struct rq *rq, struct task_struct *p)
{
}

static void set_curr_task_wrr(struct rq *rq)
{
}

static void task_tick_wrr(struct rq *rq, struct task_struct *p, int queued)
{
    struct wrr_rq *wrr_rq = &rq->wrr;
    struct sched_wrr_entity *wrr_se = &p->wrr;

    if (!wrr_policy(p->policy) || --wrr_se->time_slice)
        return;
	list_del(&wrr_se->run_list);
    wrr_se->time_slice = wrr_se->weight * HZ / 100;
    list_add_tail(&wrr_se->run_list, &wrr_rq->queue_head);
    resched_curr(rq);
}

static unsigned int get_rr_interval_wrr(struct rq *rq, struct task_struct *task)
{
	return msecs_to_jiffies((&task->wrr)->weight *10);
}

static void prio_changed_wrr(struct rq *rq, struct task_struct *p, int oldprio)
{
}

static void switched_to_wrr(struct rq *rq, struct task_struct *p)
{
}

static void update_curr_wrr(struct rq *rq)
{
}

static void migrate_task_rq_wrr(struct task_struct *p)
{
}

static void task_fork_wrr(struct task_struct *p)
{
    if (!p)
        return;
    p->wrr.weight = p->real_parent->wrr.weight;
    p->wrr.time_slice = p->wrr.weight * HZ / 100;
}

static void task_dead_wrr(struct task_struct *p) 
{
}

const struct sched_class wrr_sched_class = {
    .next = &fair_sched_class,
    .enqueue_task = enqueue_task_wrr,
    .dequeue_task = dequeue_task_wrr,
    .yield_task = yield_task_wrr,
    
    .check_preempt_curr = check_preempt_curr_wrr,
    
    .pick_next_task = pick_next_task_wrr,
    .put_prev_task = put_prev_task_wrr,
    
    .select_task_rq = select_task_rq_wrr,
    
    .set_cpus_allowed = set_cpus_allowed_common,
    .rq_online = rq_online_wrr,
    .rq_offline = rq_offline_wrr,
    .task_woken = task_woken_wrr,
    .switched_from = switched_from_wrr,
    
    .migrate_task_rq = migrate_task_rq_wrr,
    .task_dead = task_dead_wrr,

    .set_curr_task = set_curr_task_wrr,
    .task_tick = task_tick_wrr,
    
    .get_rr_interval = get_rr_interval_wrr,

    .prio_changed = prio_changed_wrr,
    .switched_to = switched_to_wrr,
    
    .update_curr = update_curr_wrr,
    
    .task_fork = task_fork_wrr,
};
