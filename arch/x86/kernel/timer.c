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

// 进入定时器与退出定时器
#define timer_enter(t)		do { } while (0)
#define timer_exit()		do { } while (0)

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
static struct timer_vec_root tv1;   // 需要立即执行的定时器列表

// 全局定时器二维列表
static struct timer_vec * const tvecs[] = {
	(struct timer_vec *)&tv1, &tv2, &tv3, &tv4, &tv5
};

// 保护 tvecs 的锁
spinlock_t timerlist_lock = SPIN_LOCK_UNLOCKED;
// 定时器列表的个数
#define NOOF_TVECS (sizeof(tvecs) / sizeof(tvecs[0]))
// 定时器时间戳
static unsigned long timer_jiffies;

/**
 * @brief 添加定时器
 * 
 * @param timer 待添加的定时器
 */
static inline void internal_add_timer(struct timer_list *timer)
{
	unsigned long expires = timer->expires;
	unsigned long idx = expires - timer_jiffies;    // 还有多久定时器发生
	struct list_head * vec;
    // 根据定时器发生时间的级数插入到不同的定时器列表
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
        // 如若定时时间小于当前时间,应该立即发生。
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

/**
 * @brief 更新定时器到它应该到的定时器列表(通过 del + add 更新)
 * 
 * @param tv 待更新的定时器列表
 */
static inline void cascade_timers(struct timer_vec *tv)
{
	/* cascade all the timers from tv up one level */
	struct list_head *head, *curr, *next;

	head = tv->vec + tv->index;
	curr = head->next;
	/*
	 * We are removing _all_ timers from the list, so we don't  have to
	 * detach them individually, just clear the list afterwards.
	 */
	while (curr != head) {
		struct timer_list *tmp;

		tmp = list_entry(curr, struct timer_list, list);
		next = curr->next;
		list_del(curr); // not needed
		internal_add_timer(tmp);
		curr = next;
	}
	INIT_LIST_HEAD(head);
	tv->index = (tv->index + 1) & TVN_MASK;
}

/**
 * @brief 执行 timer 列表(二维)
 * 
 */
static inline void run_timer_list(void)
{
	spin_lock_irq(&timerlist_lock);
	while ((long)(jiffies - timer_jiffies) >= 0) {
		struct list_head *head, *curr;
        // 如若当前应该发生的定时器数量为 0
		if (!tv1.index) {
			int n = 1;
			do {
				cascade_timers(tvecs[n]);
			} while (tvecs[n]->index == 1 && ++n < NOOF_TVECS); // 如若当前定时器已清空,则更新下一个更高级别的定时器。
		}
repeat:
        // 执行 tv1 定时器列表上的定时器
		head = tv1.vec + tv1.index;
		curr = head->next;
		if (curr != head) {
			struct timer_list *timer;
			void (*fn)(unsigned long);
			unsigned long data;

			timer = list_entry(curr, struct timer_list, list);
 			fn = timer->function;
 			data= timer->data;
            // 拆除定时器
			detach_timer(timer);
			timer->list.next = timer->list.prev = NULL;
            // 执行定时器回调
			timer_enter(timer);
			spin_unlock_irq(&timerlist_lock);
			fn(data);
			spin_lock_irq(&timerlist_lock);
			timer_exit();
			goto repeat;
		}
		++timer_jiffies; 
		tv1.index = (tv1.index + 1) & TVR_MASK;
	}
	spin_unlock_irq(&timerlist_lock);
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

/**
 * @brief bh 回调函数
 * 
 */
void timer_bh(void)
{
    printk("timer_bh...\n");
	run_timer_list();
}

/**
 * @brief 硬件中断回调函数
 * 
 * @param regs 寄存器组
 */
void do_timer(struct pt_regs *regs)
{
	(*(unsigned long *)&jiffies)++;
	// 标记定时器 bh 发生,等待软中断处理。
	mark_bh(TIMER_BH);
}