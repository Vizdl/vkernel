
#include <asm/system.h>
#include <asm/processor.h>
#include <vkernel/smp.h>
#include <vkernel/irq.h>
#include <vkernel/sched.h>
#include <vkernel/string.h>
#include <vkernel/kernel.h>
#include <vkernel/linkage.h>

asmlinkage void ret_from_fork(void) __asm__("ret_from_fork");

#define savesegment(seg,value) \
	asm volatile("mov %%" #seg ",%0":"=m" (*(int *)&(value)))

/**
 * @brief 拷贝 thread_struct
 * 
 * @param nr 
 * @param clone_flags 
 * @param esp 
 * @param unused 
 * @param p 
 * @param regs 
 * @return int 
 */
int copy_thread(int nr, unsigned long clone_flags, unsigned long esp,
	unsigned long unused,
	struct task_struct * p, struct pt_regs * regs)
{
	struct pt_regs * childregs;
	// 获取待拷贝 task struct pt_regs 的指针位置
	childregs = ((struct pt_regs *) (THREAD_SIZE + (unsigned long) p)) - 1;
	struct_cpy(childregs, regs);
	childregs->eax = 0;
	childregs->esp = esp;

	p->thread.esp = (unsigned long) childregs;
	p->thread.esp0 = (unsigned long) (childregs+1);

	p->thread.eip = (unsigned long) ret_from_fork;

	savesegment(fs,p->thread.fs);
	savesegment(gs,p->thread.gs);

	return 0;
}

/**
 * @brief 创建内核线程 : 本质是创建一个结构体放置在 rq 上等待调度
 * 
 * @param fn 内核线程函数
 * @param arg 内核线程参数
 * @param flags 控制 flags
 * @return int 
 */
int kernel_thread(int (*fn)(void *), void * arg, unsigned long flags)
{
	long retval, d0;

	__asm__ __volatile__(
		"movl %%esp,%%esi\n\t"
		"int $0x80\n\t"			/* Linux/i386 system call */
		"cmpl %%esp,%%esi\n\t"	/* child or parent? */
		"je 1f\n\t"				/* parent - jump */
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

/**
 * @brief 克隆的系统调用函数接口
 * 
 * @param regs 寄存器组
 * @return int 
 */
asmlinkage int sys_clone(struct pt_regs regs)
{
	unsigned long clone_flags;
	unsigned long newsp;
    printk("sys_clone...\n");

	clone_flags = regs.ebx;
	newsp = regs.ecx;
	if (!newsp)
		newsp = regs.esp;
	return do_fork(clone_flags, newsp, &regs, 0);
}

void __fastcall __switch_to(struct task_struct *prev_p, struct task_struct *next_p)
{
	// struct thread_struct *next = &next_p->thread;
	// struct tss_struct *tss = init_tss + smp_processor_id();
    printk("__switch_to, prev : %p,%d, next : %p,%d\n", prev_p, prev_p->pid, next_p, next_p->pid);
    // 修改 tss 内容,这样如若从内核态回去才能回到正确的进程。
	// tss->esp0 = next->esp0;
}