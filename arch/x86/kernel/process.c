
#include <asm/system.h>
#include <asm/processor.h>
#include <vkernel/smp.h>
#include <vkernel/irq.h>
#include <vkernel/sched.h>
#include <vkernel/string.h>
#include <vkernel/kernel.h>
#include <vkernel/linkage.h>

/*
 * Create a kernel thread
 */
int kernel_thread(int (*fn)(void *), void * arg, unsigned long flags)
{
	long retval, d0;

	__asm__ __volatile__(
		"movl %%esp,%%esi\n\t"
		"int $0x80\n\t"		/* Linux/i386 system call */
		"cmpl %%esp,%%esi\n\t"	/* child or parent? */
		"je 1f\n\t"		/* parent - jump */
		/* 子节点调用 fn, 执行结束后调用 exit 退出线程 */
		"movl %4,%%eax\n\t"
		"pushl %%eax\n\t"		
		"call *%5\n\t"		/* call fn */
		"movl %3,%0\n\t"	/* exit */
		"int $0x80\n"
		"1:\t"
		:"=&a" (retval), "=&S" (d0)
		:"0" (__NR_clone), "i" (__NR_exit),
		 "r" (arg), "r" (fn),
		 "b" (flags | CLONE_VM)
		: "memory");
	return retval;
}

asmlinkage int sys_clone(struct pt_regs regs)
{
    printk("sys_clone...\n");
	return 0;
}

void __switch_to(struct task_struct *prev_p, struct task_struct *next_p)
{
	struct thread_struct *prev = &prev_p->thread,
				 *next = &next_p->thread;
	struct tss_struct *tss = init_tss + smp_processor_id();
    // 修改 tss 内容,这样如若从内核态回去才能回到正确的进程。
	tss->esp0 = next->esp0;
}