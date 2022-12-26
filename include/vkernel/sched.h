#ifndef _LINUX_SCHED_H
#define _LINUX_SCHED_H

#include <asm/page.h>
#include <asm/param.h>
#include <asm/ptrace.h>
#include <asm/current.h>
#include <asm/pgtable.h>
#include <asm/processor.h>
#include <vkernel/list.h>
#include <vkernel/kernel.h>
#include <vkernel/linkage.h>
#include <vkernel/spinlock.h>
#include <vkernel/task_list.h>

struct mm_struct {
	pgd_t * pgd;
};

#define DEF_COUNTER	(10*HZ/100)	/* 100 ms time slice */
#define DEF_NICE	(0)

/*
 * cloning flags:
 */
#define CSIGNAL			0x000000ff	/* signal mask to be sent at exit */
#define CLONE_VM		0x00000100	/* set if VM shared between processes */
#define CLONE_FS		0x00000200	/* set if fs info shared between processes */
#define CLONE_FILES		0x00000400	/* set if open files shared between processes */
#define CLONE_SIGHAND	0x00000800	/* set if signal handlers and blocked signals shared */
#define CLONE_PID		0x00001000	/* set if pid shared */
#define CLONE_PTRACE	0x00002000	/* set if we want to let tracing continue on the child too */
#define CLONE_VFORK		0x00004000	/* set if the parent wants the child to wake it up on mm_release */
#define CLONE_PARENT	0x00008000	/* set if we want to have the same parent as the cloner */
#define CLONE_THREAD	0x00010000	/* Same thread group? */

#define CLONE_SIGNAL	(CLONE_SIGHAND | CLONE_THREAD)

// Ptrace flags
#define PT_PTRACED		0x00000001
#define PT_TRACESYS		0x00000002
#define PT_DTRACE		0x00000004	/* delayed trace (used on m68k, i386) */
#define PT_TRACESYSGOOD	0x00000008

// 进程状态
#define TASK_RUNNING			0	/* 就绪态/可运行态 */
#define TASK_INTERRUPTIBLE		1	/* 可中断睡眠 */
#define TASK_UNINTERRUPTIBLE	2	/* 不可中断睡眠 */
#define TASK_ZOMBIE				4
#define TASK_STOPPED			8	/* 停止 */

// 调度策略
#define SCHED_OTHER		0
#define SCHED_FIFO		1
#define SCHED_RR		2
#define SCHED_YIELD		0x10

// 遍历任务列表
#define for_each_task(p) \
	for (p = &init_task ; (p = p->next_task) != &init_task ; )

#define __set_task_state(tsk, state_value)		\
	do { (tsk)->state = (state_value); } while (0)

// 设置任务状态
#define set_task_state(tsk, state_value)		\
	__set_task_state((tsk), (state_value))

#define __set_current_state(state_value)			\
	do { current->state = (state_value); } while (0)

// 设置当前cpu执行的任务的状态
#define set_current_state(state_value)		\
	__set_current_state(state_value)

struct task_struct {
	volatile long state;			// 进程状态 : -1 unrunnable, 0 runnable, >0 stopped
	int processor;
	struct thread_struct thread;	// 线程上下文
	struct mm_struct *mm;
	struct mm_struct *active_mm;
	struct list_head run_list;		// 就绪态时该链表设到
    unsigned long ptrace;
	/* 进程号进程组相关 */
	pid_t pid;						// 进程ID
	pid_t session;					// 进程会话
	pid_t pgrp;						// 与进程组有关
	/* 调度有关属性 */
    long nice;						// 调度优先值
    unsigned long policy;			// 该进程的调度策略, eg : SCHED_OTHER
    long counter;					// 如若 counter <= 0 则重调度
    unsigned long rt_priority;		// 实时调度策略优先级别
    volatile long need_resched;		// 在返回用户态的时候如若设立该标志则会重调度
	/* task_struct 之间的联系(所有进程都会在这三个结构内) */
	// 1. 哈希表
	struct task_struct *pidhash_next;
	struct task_struct **pidhash_pprev;
	// 2. 进程组
	struct task_struct *p_opptr, *p_pptr, *p_cptr, *p_ysptr, *p_osptr;
	// 3. 进程组成的链表
	struct task_struct *next_task, *prev_task;
};

#ifndef INIT_TASK_SIZE
# define INIT_TASK_SIZE	2048*sizeof(long)
#endif

#define INIT_MM(name) \
{			 							\
	pgd:		swapper_pg_dir, 		\
}

#define INIT_TASK(tsk)	\
{									\
    state:		0,					\
	thread:		INIT_THREAD,		\
	mm:			NULL,				\
    active_mm:	&init_mm,			\
    counter:	DEF_COUNTER,		\
    nice:		DEF_NICE,			\
    policy:		SCHED_OTHER,		\
    next_task:	&tsk,				\
    prev_task:	&tsk,				\
}

union task_union {
	struct task_struct task;
	unsigned long stack[INIT_TASK_SIZE/sizeof(long)];
};


/* PID hashing. (shouldnt this be dynamic?) */
#define PIDHASH_SZ (4096 >> 2)
extern struct task_struct *pidhash[PIDHASH_SZ];
extern union task_union init_task_union;
extern struct mm_struct init_mm;
extern rwlock_t tasklist_lock;
extern int nr_running;
extern int last_pid;
extern struct list_head runqueue_head;

extern void trap_init(void);
extern int do_fork(unsigned long, unsigned long, struct pt_regs *, unsigned long);
extern void FASTCALL(wake_up_process(struct task_struct * tsk));

// timer
void do_timer(struct pt_regs *regs);


// copy task
extern int  copy_thread(int, unsigned long, unsigned long, unsigned long, struct task_struct *, struct pt_regs *);

// shcedule
extern void sched_init(void);
asmlinkage void schedule(void);

// pid hashtable 相关
int get_pid(unsigned long flags);
void hash_pid(struct task_struct*);
void unhash_pid(struct task_struct*);
struct task_struct *find_task_by_pid(int);
// runqueue 相关
void add_to_runqueue(struct task_struct *);
void move_last_runqueue(struct task_struct * );
void move_first_runqueue(struct task_struct *);
int task_on_runqueue(struct task_struct *p);
// goodness 相关
int goodness(struct task_struct * p, int this_cpu, struct mm_struct *this_mm);
int preemption_goodness(struct task_struct * prev, struct task_struct * p, int cpu);
#endif /* _LINUX_SCHED_H */