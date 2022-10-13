#ifndef _LINUX_SCHED_H
#define _LINUX_SCHED_H

#include <asm/current.h>
#include <asm/page.h>
#include <asm/pgtable.h>
#include <asm/processor.h>

struct mm_struct {
	pgd_t * pgd;
};

/*
 * cloning flags:
 */
#define CSIGNAL		0x000000ff	/* signal mask to be sent at exit */
#define CLONE_VM	0x00000100	/* set if VM shared between processes */
#define CLONE_FS	0x00000200	/* set if fs info shared between processes */
#define CLONE_FILES	0x00000400	/* set if open files shared between processes */
#define CLONE_SIGHAND	0x00000800	/* set if signal handlers and blocked signals shared */
#define CLONE_PID	0x00001000	/* set if pid shared */
#define CLONE_PTRACE	0x00002000	/* set if we want to let tracing continue on the child too */
#define CLONE_VFORK	0x00004000	/* set if the parent wants the child to wake it up on mm_release */
#define CLONE_PARENT	0x00008000	/* set if we want to have the same parent as the cloner */
#define CLONE_THREAD	0x00010000	/* Same thread group? */

#define CLONE_SIGNAL	(CLONE_SIGHAND | CLONE_THREAD)

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
	int processor;
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


/* PID hashing. (shouldnt this be dynamic?) */
#define PIDHASH_SZ (4096 >> 2)
extern struct task_struct *pidhash[PIDHASH_SZ];
extern union task_union init_task_union;
extern struct mm_struct init_mm;

extern void trap_init(void);
extern void sched_init(void);

#endif /* _LINUX_SCHED_H */