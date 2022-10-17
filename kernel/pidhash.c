#include <vkernel/sched.h>

struct task_struct *pidhash[PIDHASH_SZ];

#define pid_hashfn(x)	((((x) >> 8) ^ (x)) & (PIDHASH_SZ - 1))

/**
 * @brief 将 task p 添加到进程哈希表中
 * 
 * @param p 待添加的进程
 */
void hash_pid(struct task_struct *p)
{
	struct task_struct **htable = &pidhash[pid_hashfn(p->pid)];

	if((p->pidhash_next = *htable) != NULL)
		(*htable)->pidhash_pprev = &p->pidhash_next;
	*htable = p;
	p->pidhash_pprev = htable;
}

/**
 * @brief 将 task p 从进程哈希表中删除
 * 
 * @param p 待删除的进程
 */
void unhash_pid(struct task_struct *p)
{
	if(p->pidhash_next)
		p->pidhash_next->pidhash_pprev = p->pidhash_pprev;
	*p->pidhash_pprev = p->pidhash_next;
}

/**
 * @brief 根据进程号找到对应的 task_struct
 * 
 * @param pid 进程号
 * @return struct task_struct* 找到的 task_struct
 */
struct task_struct *find_task_by_pid(int pid)
{
	struct task_struct *p, **htable = &pidhash[pid_hashfn(pid)];

	for(p = *htable; p && p->pid != pid; p = p->pidhash_next)
		;

	return p;
}
