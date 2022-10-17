#include <asm/errno.h>
#include <vkernel/init.h>
#include <vkernel/sched.h>
#include <vkernel/kernel.h>
#include <vkernel/threads.h>
#include <vkernel/spinlock.h>

struct task_struct *pidhash[PIDHASH_SZ];

int max_threads;
int last_pid;		// 上一次分配的 pid
int nr_running;		// 就绪态的进程个数

/* Protects next_safe and last_pid. */
spinlock_t lastpid_lock = SPIN_LOCK_UNLOCKED;

/**
 * @brief 获取 pid
 * 
 * @param flags clone_flags
 * @return int pid
 */
static int get_pid(unsigned long flags)
{
	static int next_safe = PID_MAX;
	struct task_struct *p;

	if (flags & CLONE_PID)
		return current->pid;

	spin_lock(&lastpid_lock);
	if((++last_pid) & 0xffff8000) {
		last_pid = 300;		/* Skip daemons etc. */
		goto inside;
	}
	if(last_pid >= next_safe) {
inside:
		next_safe = PID_MAX;
		read_lock(&tasklist_lock);
	repeat:
		for_each_task(p) {
			if(p->pid == last_pid	||
			   p->pgrp == last_pid	||
			   p->session == last_pid) {
				if(++last_pid >= next_safe) {
					if(last_pid & 0xffff8000)
						last_pid = 300;
					next_safe = PID_MAX;
				}
				goto repeat;
			}
			if(p->pid > last_pid && next_safe > p->pid)
				next_safe = p->pid;
			if(p->pgrp > last_pid && next_safe > p->pgrp)
				next_safe = p->pgrp;
			if(p->session > last_pid && next_safe > p->session)
				next_safe = p->session;
		}
		read_unlock(&tasklist_lock);
	}
	spin_unlock(&lastpid_lock);

	return last_pid;
}

int do_fork(unsigned long clone_flags, unsigned long stack_start,
	    struct pt_regs *regs, unsigned long stack_size)
{
    int retval = -ENOMEM;
	struct task_struct *p;
    printk("do_fork...\n");
	if (clone_flags & CLONE_PID) {
		// 只有 0 号进程可以克隆 pid
		if (current->pid)
			return -EPERM;
	}

	// 1. 申请 task_struct 内存
	p = alloc_task_struct();
	if (!p)
		goto fork_out;

	p->state = TASK_UNINTERRUPTIBLE;

    p->pid = get_pid(clone_flags);
    printk("do_fork, pid = %d...\n", p->pid);
	
	p->run_list.next = NULL;
	p->run_list.prev = NULL;


	wake_up_process(p);
fork_out:
	return retval;
}

void __init fork_init(unsigned long mempages)
{
	printk("fork_init...\n");
	max_threads = mempages / (THREAD_SIZE/PAGE_SIZE) / 2;
}