#include <asm/cache.h>
#include <vkernel/irq.h>
#include <vkernel/smp.h>
#include <vkernel/init.h>
#include <vkernel/types.h>
#include <vkernel/sched.h>
#include <vkernel/kernel.h>
#include <vkernel/threads.h>
#include <vkernel/interrupt.h>

irq_cpustat_t irq_stat[NR_CPUS];

// 全局 softirq 列表, 一维, 下标表示软中断向量号
static struct softirq_action softirq_vec[32] __cacheline_aligned;

static spinlock_t softirq_mask_lock = SPIN_LOCK_UNLOCKED;

// 全局 tasklet 队列, 二维, 第一维是 cpu, 第二维是 tasklet
struct tasklet_head tasklet_vec[NR_CPUS] __cacheline_aligned;

// bh 处理函数列表
static void (*bh_base[32])(void);
spinlock_t global_bh_lock = SPIN_LOCK_UNLOCKED;
// 注册的 bh tasklet 链表
struct tasklet_struct bh_task_vec[32];
// bh tasklet 待执行链表
struct tasklet_head tasklet_hi_vec[NR_CPUS] __cacheline_aligned;

void open_softirq(int nr, void (*action)(struct softirq_action*), void *data)
{
	unsigned long flags;
	int i;

	spin_lock_irqsave(&softirq_mask_lock, flags);
    // 1. 设置软中断处理函数
	softirq_vec[nr].data = data;
	softirq_vec[nr].action = action;
    // 2. 设置每个 cpu 中对应的掩码
	for (i=0; i<NR_CPUS; i++)
		softirq_mask(i) |= (1<<nr);
	spin_unlock_irqrestore(&softirq_mask_lock, flags);
}

asmlinkage void do_softirq(void)
{
	// 1. 获取当前 CPU
	int cpu = smp_processor_id();
	__u32 active, mask;
	
	// 2. 如若在中断上下文中则返回
	if (in_interrupt())
		return;

	local_bh_disable();
    // 3. 进入软中断处理流程
	local_irq_disable();
	// 4. 查看是否有软中断事件发生
	mask = softirq_mask(cpu);
	active = softirq_active(cpu) & mask;
    // 5. 软中断处理
	if (active) {
		struct softirq_action *h;

restart:
		// 去除所有软中断标志位
		softirq_active(cpu) &= ~active;
        // 关闭硬中断
		local_irq_enable();

		h = softirq_vec;
		mask &= ~active;
        // 遍历所有软中断事件
		do {
			if (active & 1)
				h->action(h);
			h++;
			active >>= 1;
		} while (active);
        // 开启硬中断
		local_irq_disable();
        // 查看再处理软中断期间是否又发生了软中断。
		active = softirq_active(cpu);
		if ((active &= mask) != 0)
			goto retry;
	}
    // 6. 退出软中断处理流程
	local_bh_enable();
	return;

retry:
	goto restart;
}

void tasklet_init(struct tasklet_struct *t,
		  void (*func)(unsigned long), unsigned long data)
{
	t->func = func;
	t->data = data;
	t->state = 0;
	atomic_set(&t->count, 0);
}

void tasklet_kill(struct tasklet_struct *t)
{
	if (in_interrupt())
		printk("Attempt to kill tasklet from interrupt\n");
	// 设置任务状态为待调度
	while (test_and_set_bit(TASKLET_STATE_SCHED, &t->state)) {
		current->state = TASK_RUNNING;
		do {
			current->policy |= SCHED_YIELD;
			// 通过调度程序使其执行软中断
			schedule();
		} while (test_bit(TASKLET_STATE_SCHED, &t->state)); // 直到 tasklet 真正被执行
	}
	tasklet_unlock_wait(t);
	// 清除标志位
	clear_bit(TASKLET_STATE_SCHED, &t->state);
}


/**
 * @brief 执行 tasklet_vec 对应当前 cpu 的 tasklet 链表上的 tasklet
 * 
 * @param a 无意义的参数,实际上会被传入 NULL
 */
static void tasklet_action(struct softirq_action *a)
{
	int cpu = smp_processor_id();
	struct tasklet_struct *list;
	// 1. 根据 cpu 选择 取出对应的 tasklet list
	local_irq_disable();
	list = tasklet_vec[cpu].list;
	tasklet_vec[cpu].list = NULL;
	local_irq_enable();
	// 遍历 tasklet list
	while (list != NULL) {
		struct tasklet_struct *t = list;

		list = list->next;
		// 2. 锁住 tasklet 并执行
		if (tasklet_trylock(t)) {
			if (atomic_read(&t->count) == 0) {
				// 去除等待调度标志,表示已经执行
				clear_bit(TASKLET_STATE_SCHED, &t->state);
				// 真正地执行 tasklet
				t->func(t->data);
				tasklet_unlock(t);
				continue;
			}
			tasklet_unlock(t);
		}
		// 如若锁失败, 将 tasklet 还回 tasklet list 末尾,等待下一次调度
		local_irq_disable();
		t->next = tasklet_vec[cpu].list;
		tasklet_vec[cpu].list = t;
		__cpu_raise_softirq(cpu, TASKLET_SOFTIRQ);
		local_irq_enable();
	}
}


void init_bh(int nr, void (*routine)(void))
{
	printk("init_bh : %d\n", nr);
	bh_base[nr] = routine;
	// mb();
}

void remove_bh(int nr)
{
	tasklet_kill(bh_task_vec+nr);
	bh_base[nr] = NULL;
}

/**
 * @brief 该函数被设置为 tasklet 回调函数
 * 
 * @param nr 回调时的 bh 号
 */
static void bh_action(unsigned long nr)
{
	int cpu = smp_processor_id();
	if (!spin_trylock(&global_bh_lock))
		goto resched;

	if (!hardirq_trylock(cpu))
		goto resched_unlock;

	if (bh_base[nr])
		bh_base[nr]();

	hardirq_endlock(cpu);
	spin_unlock(&global_bh_lock);
	return;

resched_unlock:
	spin_unlock(&global_bh_lock);
resched:
	mark_bh(nr);
}

/**
 * @brief bh tasklet 中断回调函数
 * 
 * @param a 中断回调上下文
 */
static void tasklet_hi_action(struct softirq_action *a)
{
	int cpu = smp_processor_id();
	struct tasklet_struct *list;
	// 1. 取出待执行的 tasklet
	local_irq_disable();
	list = tasklet_hi_vec[cpu].list;
	tasklet_hi_vec[cpu].list = NULL;
	local_irq_enable();
	// 2. 遍历执行 tasklet
	while (list != NULL) {
		struct tasklet_struct *t = list;

		list = list->next;

		if (tasklet_trylock(t)) {
			if (atomic_read(&t->count) == 0) {
				clear_bit(TASKLET_STATE_SCHED, &t->state);

				t->func(t->data);
				tasklet_unlock(t);
				continue;
			}
			tasklet_unlock(t);
		}
		local_irq_disable();
		t->next = tasklet_hi_vec[cpu].list;
		tasklet_hi_vec[cpu].list = t;
		__cpu_raise_softirq(cpu, HI_SOFTIRQ);
		local_irq_enable();
	}
}

void __init softirq_init(void)
{
	int i;
	// 初始化 bh tasklet
	for (i=0; i<32; i++)
		tasklet_init(bh_task_vec+i, bh_action, i);

	open_softirq(TASKLET_SOFTIRQ, tasklet_action, NULL);
	open_softirq(HI_SOFTIRQ, tasklet_hi_action, NULL);
}