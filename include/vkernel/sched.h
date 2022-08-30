#ifndef _LINUX_SCHED_H
#define _LINUX_SCHED_H


#ifdef __KERNEL__

struct task_struct {
    volatile long state;	/* -1 unrunnable, 0 runnable, >0 stopped */
    unsigned long flags;	/* per process flags, defined below */
};

# define INIT_TASK_SIZE	2048*sizeof(long)

union task_union {
    struct task_struct task;
    unsigned long stack[INIT_TASK_SIZE/sizeof(long)];
};

extern void trap_init(void);

#endif /* __KERNEL__ */

#endif /* _LINUX_SCHED_H */