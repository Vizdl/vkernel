#include <vkernel/timer.h>
#include <vkernel/kernel.h>
#include <vkernel/spinlock.h>
#include <vkernel/interrupt.h>

unsigned long volatile jiffies;

/*
 * Event timer code
 */
#define TVN_BITS 6
#define TVR_BITS 8
#define TVN_SIZE (1 << TVN_BITS)
#define TVR_SIZE (1 << TVR_BITS)
#define TVN_MASK (TVN_SIZE - 1)
#define TVR_MASK (TVR_SIZE - 1)

struct timer_vec {
	int index;
	struct list_head vec[TVN_SIZE];
};

struct timer_vec_root {
	int index;
	struct list_head vec[TVR_SIZE];
};

static struct timer_vec tv5;
static struct timer_vec tv4;
static struct timer_vec tv3;
static struct timer_vec tv2;
static struct timer_vec_root tv1;

// 全局定时器列表
static struct timer_vec * const tvecs[] = {
	(struct timer_vec *)&tv1, &tv2, &tv3, &tv4, &tv5
};

// 保护 tvecs 的锁
spinlock_t timerlist_lock = SPIN_LOCK_UNLOCKED;

static unsigned long timer_jiffies;

/**
 * @brief 添加定时器
 * 
 * @param timer 待添加的定时器
 */
static inline void internal_add_timer(struct timer_list *timer)
{
	/*
	 * must be cli-ed when calling this
	 */
	unsigned long expires = timer->expires;
	unsigned long idx = expires - timer_jiffies;
	struct list_head * vec;

	if (idx < TVR_SIZE) {
		int i = expires & TVR_MASK;
		vec = tv1.vec + i;
	} else if (idx < 1 << (TVR_BITS + TVN_BITS)) {
		int i = (expires >> TVR_BITS) & TVN_MASK;
		vec = tv2.vec + i;
	} else if (idx < 1 << (TVR_BITS + 2 * TVN_BITS)) {
		int i = (expires >> (TVR_BITS + TVN_BITS)) & TVN_MASK;
		vec =  tv3.vec + i;
	} else if (idx < 1 << (TVR_BITS + 3 * TVN_BITS)) {
		int i = (expires >> (TVR_BITS + 2 * TVN_BITS)) & TVN_MASK;
		vec = tv4.vec + i;
	} else if ((signed long) idx < 0) {
		/* can happen if you add a timer with expires == jiffies,
		 * or you set a timer to go off in the past
		 */
		vec = tv1.vec + tv1.index;
	} else if (idx <= 0xffffffffUL) {
		int i = (expires >> (TVR_BITS + 3 * TVN_BITS)) & TVN_MASK;
		vec = tv5.vec + i;
	} else {
		/* Can only get here on architectures with 64-bit jiffies */
		INIT_LIST_HEAD(&timer->list);
		return;
	}
	/*
	 * Timers are FIFO!
	 */
	list_add(&timer->list, vec->prev);
}

/**
 * @brief 拆除定时器
 * 
 * @param timer 待拆除的定时器
 * @return int 返回非0表示成功,0表示失败
 */
static inline int detach_timer (struct timer_list *timer)
{
	if (!timer_pending(timer))
		return 0;
	list_del(&timer->list);
	return 1;
}

void init_timervecs (void)
{
	int i;

	for (i = 0; i < TVN_SIZE; i++) {
		INIT_LIST_HEAD(tv5.vec + i);
		INIT_LIST_HEAD(tv4.vec + i);
		INIT_LIST_HEAD(tv3.vec + i);
		INIT_LIST_HEAD(tv2.vec + i);
	}
	for (i = 0; i < TVR_SIZE; i++)
		INIT_LIST_HEAD(tv1.vec + i);
}

void add_timer(struct timer_list *timer)
{
	unsigned long flags;

	spin_lock_irqsave(&timerlist_lock, flags);
	if (timer_pending(timer))
		goto bug;
	internal_add_timer(timer);
	spin_unlock_irqrestore(&timerlist_lock, flags);
	return;
bug:
	spin_unlock_irqrestore(&timerlist_lock, flags);
	printk("bug: kernel timer added twice at %p.\n",
			__builtin_return_address(0));
}

int del_timer(struct timer_list * timer)
{
	int ret;
	unsigned long flags;

	spin_lock_irqsave(&timerlist_lock, flags);
	ret = detach_timer(timer);
	timer->list.next = timer->list.prev = NULL;
	spin_unlock_irqrestore(&timerlist_lock, flags);
	return ret;
}

int mod_timer(struct timer_list *timer, unsigned long expires)
{
	int ret;
	unsigned long flags;

	spin_lock_irqsave(&timerlist_lock, flags);
	timer->expires = expires;
	ret = detach_timer(timer);
	internal_add_timer(timer);
	spin_unlock_irqrestore(&timerlist_lock, flags);
	return ret;
}

void do_timer(struct pt_regs *regs)
{
    printk("do_timer...\n");
	(*(unsigned long *)&jiffies)++;
}