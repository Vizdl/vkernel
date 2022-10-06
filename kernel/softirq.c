#include <asm/cache.h>
#include <vkernel/irq.h>
#include <vkernel/smp.h>
#include <vkernel/init.h>
#include <vkernel/types.h>
#include <vkernel/kernel.h>
#include <vkernel/threads.h>
#include <vkernel/interrupt.h>

irq_cpustat_t irq_stat[NR_CPUS];

static struct softirq_action softirq_vec[32] __cacheline_aligned;

static spinlock_t softirq_mask_lock = SPIN_LOCK_UNLOCKED;

/**
 * @brief 注册软中断
 * 
 * @param nr 软中断向量号
 * @param action 软中断处理结构
 * @param data 上下文信息
 */
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

/**
 * @brief 处理软中断
 * 
 */
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

/**
 * @brief 初始化软中断
 * 
 */
void __init softirq_init(void)
{

}


struct tasklet_head tasklet_vec[NR_CPUS] __cacheline_aligned;