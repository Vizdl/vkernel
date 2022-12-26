#ifndef _LINUX_INTERRUPT_H
#define _LINUX_INTERRUPT_H

#include <asm/ptrace.h>
#include <asm/bitops.h>
#include <asm/softirq.h>
#include <asm/hardirq.h>
#include <vkernel/threads.h>
#include <vkernel/linkage.h>
#include <vkernel/spinlock.h>

enum {
	TIMER_BH = 0
};

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

// tasklet
extern struct tasklet_head tasklet_vec[NR_CPUS];

// bh
extern struct tasklet_head tasklet_hi_vec[NR_CPUS];
extern struct tasklet_struct bh_task_vec[];
extern spinlock_t global_bh_lock;

// tasklet 操作宏
#define tasklet_trylock(t) 1
#define tasklet_unlock_wait(t) do { } while (0)
#define tasklet_unlock(t) do { } while (0)

/**
 * @brief 初始化软中断
 * 
 */
extern void softirq_init(void);

/**
 * @brief 处理软中断
 * 
 */
asmlinkage void do_softirq(void);

/**
 * @brief 注册软中断
 * 
 * @param nr 软中断向量号
 * @param action 软中断处理结构
 * @param data 上下文信息
 */
extern void open_softirq(int nr, void (*action)(struct softirq_action*), void *data);

/**
 * @brief 外部唤醒软中断
 * 
 * @param cpu 
 * @param nr 
 */
static inline void __cpu_raise_softirq(int cpu, int nr)
{
	softirq_active(cpu) |= (1<<nr);
}

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

/**
 * @brief 停止 tasklet(其实本质是加速执行)
 * 
 * @param t 待停止的 tasklet
 */
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

/**
 * @brief 添加指定 tasklet 到待执行链表中
 * 
 * @param t 待执行 tasklet
 */
static inline void tasklet_hi_schedule(struct tasklet_struct *t)
{
	if (!test_and_set_bit(TASKLET_STATE_SCHED, &t->state)) {
		int cpu = smp_processor_id();
		unsigned long flags;

		local_irq_save(flags);
		t->next = tasklet_hi_vec[cpu].list;
		tasklet_hi_vec[cpu].list = t;
		__cpu_raise_softirq(cpu, HI_SOFTIRQ);
		local_irq_restore(flags);
	}
}

/**
 * @brief 添加 bh
 * 
 * @param nr bh 号
 * @param routine 回调函数
 */
extern void init_bh(int nr, void (*routine)(void));

/**
 * @brief 移除 bh
 * 
 * @param nr bh 号
 */
extern void remove_bh(int nr);

/**
 * @brief 标记指定 bh 待执行
 * 
 * @param nr bh 号
 */
static inline void mark_bh(int nr)
{
	tasklet_hi_schedule(bh_task_vec+nr);
}

#endif