#include <asm/errno.h>
#include <vkernel/init.h>
#include <vkernel/sched.h>
#include <vkernel/kernel.h>
#include <vkernel/threads.h>
#include <vkernel/spinlock.h>

int max_threads;
int nr_running;		// 就绪态的进程个数

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
	p->run_list.next = NULL;
	p->run_list.prev = NULL;

    p->pid = get_pid(clone_flags);
    printk("do_fork, pid = %d...\n", p->pid);
	// 2. 将该 task_struct 加入到三大数据结构组织里
	// 2.1 根据克隆 flags 的值不同,将其加入到进程组里
	if ((clone_flags & CLONE_VFORK) || !(clone_flags & CLONE_PARENT)) {
		p->p_opptr = current;
		if (!(p->ptrace & PT_PTRACED))
			p->p_pptr = current;
	}
	p->p_cptr = NULL;
	// 2.2 添加到 task list 里
	SET_LINKS(p);
	// 2.3 根据 pid 加入到 pid 哈希表里
	hash_pid(p);

	wake_up_process(p);
fork_out:
	return retval;
}

void __init fork_init(unsigned long mempages)
{
	printk("fork_init...\n");
	max_threads = mempages / (THREAD_SIZE/PAGE_SIZE) / 2;
}