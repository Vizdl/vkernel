#ifndef _LINUX_INTERRUPT_H
#define _LINUX_INTERRUPT_H

#include <asm/ptrace.h>
#include <asm/bitops.h>
#include <asm/softirq.h>
#include <asm/hardirq.h>
#include <vkernel/threads.h>
#include <vkernel/spinlock.h>

enum
{
	TASKLET_STATE_SCHED,	/* Tasklet is scheduled for execution */
	TASKLET_STATE_RUN		/* Tasklet is running (SMP only) */
};

enum
{
	HI_SOFTIRQ=0,
	NET_TX_SOFTIRQ,
	NET_RX_SOFTIRQ,
	TASKLET_SOFTIRQ
};

struct irqaction {
	void (*handler)(int, void *, struct pt_regs *);
	unsigned long flags;
	unsigned long mask;
	const char *name;
	void *dev_id;
	struct irqaction *next;
};

struct softirq_action
{
	void	(*action)(struct softirq_action *);
	void	*data;
};

static inline void __cpu_raise_softirq(int cpu, int nr)
{
	softirq_active(cpu) |= (1<<nr);
}

struct tasklet_struct
{
	struct tasklet_struct *next;
	unsigned long state;			// 标志 tasklet 的状态
	atomic_t count;					// 0 表示可执行,非 0 表示不可执行
	void (*func)(unsigned long);	// tasklet 执行回调函数
	unsigned long data;				// tasklet 执行上下文
};

struct tasklet_head
{
	struct tasklet_struct *list;	// tasklet_struct 列表
} __attribute__ ((__aligned__(SMP_CACHE_BYTES)));


extern struct tasklet_head tasklet_vec[NR_CPUS];


// tasklet 操作宏
#define tasklet_trylock(t) 1
#define tasklet_unlock_wait(t) do { } while (0)
#define tasklet_unlock(t) do { } while (0)

/**
 * @brief 初始化指定 tasklet
 * 
 * @param t 待初始化的 tasklet
 * @param func tasklet 回调函数
 * @param data tasklet 回调函数上下文
 */
extern void tasklet_init(struct tasklet_struct *t,
			 void (*func)(unsigned long), unsigned long data);

/**
 * @brief 将 tasklet_struct 添加到 tasklet_vec 中,并唤醒软中断
 * 
 * @param t 待调度的 tasklet
 */
static inline void tasklet_schedule(struct tasklet_struct *t)
{
	if (!test_and_set_bit(TASKLET_STATE_SCHED, &t->state)) {
		int cpu = smp_processor_id();
		unsigned long flags;
		// 将 tasklet 加入 tasklet_vec 对应位置并调度软中断
		local_irq_save(flags);
		t->next = tasklet_vec[cpu].list;
		tasklet_vec[cpu].list = t;
		__cpu_raise_softirq(cpu, TASKLET_SOFTIRQ);
		local_irq_restore(flags);
	}
}

extern void tasklet_kill(struct tasklet_struct *t);

/**
 * @brief 禁用 tasklet : 通过增加 count 计数器
 * 
 * @param t 被禁用的 tasklet
 */
static inline void tasklet_disable_nosync(struct tasklet_struct *t)
{
	atomic_inc(&t->count);
}

/**
 * @brief 禁用 tasklet : 通过增加 count 计数器
 * 
 * @param t 被禁用的 tasklet
 */
static inline void tasklet_disable(struct tasklet_struct *t)
{
	tasklet_disable_nosync(t);
	tasklet_unlock_wait(t);
}

/**
 * @brief 启用 tasklet : 通过减少 count 计数器
 * 
 * @param t 被启用的 tasklet
 */
static inline void tasklet_enable(struct tasklet_struct *t)
{
	atomic_dec(&t->count);
}

#endif