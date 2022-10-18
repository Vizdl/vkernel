#include <vkernel/sched.h>

/**
 * @brief 计算调度权值
 * 
 * @param p 
 * @param this_cpu 
 * @param this_mm 
 * @return int 
 */
int goodness(struct task_struct * p, int this_cpu, struct mm_struct *this_mm)
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
int preemption_goodness(struct task_struct * prev, struct task_struct * p, int cpu)
{
	return goodness(p, cpu, prev->active_mm) - goodness(prev, cpu, prev->active_mm);
}
