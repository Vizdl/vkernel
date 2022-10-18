#include <vkernel/smp.h>
#include <vkernel/init.h>
#include <vkernel/sched.h>
#include <vkernel/kernel.h>
#include <vkernel/cache.h>
#include <vkernel/spinlock.h>

spinlock_t runqueue_lock __cacheline_aligned = SPIN_LOCK_UNLOCKED;  /* inner */
rwlock_t tasklist_lock __cacheline_aligned = RW_LOCK_UNLOCKED;	/* outer */

void __init sched_init(void)
{
	/*
	 * We have to do a little magic to get the first
	 * process right in SMP mode.
	 */
	int cpu = smp_processor_id();
	int nr;
    printk("sched_init ...\n");

	init_task.processor = cpu;

	for(nr = 0; nr < PIDHASH_SZ; nr++)
		pidhash[nr] = NULL;
}

/**
 * @brief 计算调度权值
 * 
 * @param p 
 * @param this_cpu 
 * @param this_mm 
 * @return int 
 */
static inline int goodness(struct task_struct * p, int this_cpu, struct mm_struct *this_mm)
{
	int weight;
	
	weight = -1;
	if (p->policy & SCHED_YIELD)
		goto out;

	if (p->policy == SCHED_OTHER) {
		weight = p->counter;
		if (!weight)
			goto out;
			
		if (p->mm == this_mm || !p->mm)
			weight += 1;
		weight += 20 - p->nice;
		goto out;
	}

	weight = 1000 + p->rt_priority;
out:
	return weight;
}

/**
 * @brief 是否抢占调度
 * 
 * @param prev 
 * @param p 
 * @param cpu 
 * @return int 
 */
static inline int preemption_goodness(struct task_struct * prev, struct task_struct * p, int cpu)
{
	return goodness(p, cpu, prev->active_mm) - goodness(prev, cpu, prev->active_mm);
}

static void reschedule_idle(struct task_struct * p)
{
	int this_cpu = smp_processor_id();
	struct task_struct *tsk;

    tsk = current;
	if (preemption_goodness(tsk, p, this_cpu) > 1)
		tsk->need_resched = 1;
}


/**
 * @brief 将进程设为就绪态等待调度
 * 
 * @param p 待设为就绪态的进程
 */
inline void wake_up_process(struct task_struct * p)
{
	unsigned long flags;

	/*
	 * We want the common case fall through straight, thus the goto.
	 */
	spin_lock_irqsave(&runqueue_lock, flags);
	p->state = TASK_RUNNING;
	if (task_on_runqueue(p))
		goto out;
	add_to_runqueue(p);
	reschedule_idle(p);
out:
	spin_unlock_irqrestore(&runqueue_lock, flags);
}