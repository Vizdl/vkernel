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