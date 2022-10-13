#ifndef _LINUX_SCHED_H
#define _LINUX_SCHED_H


#ifdef __KERNEL__

#include <asm/current.h>
#include <asm/page.h>
#include <asm/pgtable.h>
#include <asm/processor.h>

struct mm_struct {
	pgd_t * pgd;
};

// 进程状态
#define TASK_RUNNING			0	/* 运行态 */
#define TASK_INTERRUPTIBLE		1	/* 可中断睡眠 */
#define TASK_UNINTERRUPTIBLE	2	/* 不可中断睡眠 */
#define TASK_ZOMBIE				4
#define TASK_STOPPED			8	/* 停止 */

#define __set_task_state(tsk, state_value)		\
	do { (tsk)->state = (state_value); } while (0)

#define set_task_state(tsk, state_value)		\
	__set_task_state((tsk), (state_value))

#define __set_current_state(state_value)			\
	do { current->state = (state_value); } while (0)

#define set_current_state(state_value)		\
	__set_current_state(state_value)

struct task_struct {
	volatile long state;			/* -1 unrunnable, 0 runnable, >0 stopped */
	struct thread_struct thread;	/* CPU-specific state of this task */
	struct mm_struct *mm;
	struct mm_struct *active_mm;
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
}

union task_union {
	struct task_struct task;
	unsigned long stack[INIT_TASK_SIZE/sizeof(long)];
};

extern void trap_init(void);

#endif /* __KERNEL__ */

#endif /* _LINUX_SCHED_H */