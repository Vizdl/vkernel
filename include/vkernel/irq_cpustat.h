#ifndef __irq_cpustat_h
#define __irq_cpustat_h

#include <vkernel/config.h>

// 对于每个 cpu 里中断信息的记载。
extern irq_cpustat_t irq_stat[];

#define __IRQ_STAT(cpu, member)	(irq_stat[cpu].member)

#define softirq_active(cpu)	__IRQ_STAT((cpu), __softirq_active)     // 软中断标志位,表示特定软中断是否发生
#define softirq_mask(cpu)	__IRQ_STAT((cpu), __softirq_mask)         // 软中断掩码,表示特定软中断是否设置
#define local_irq_count(cpu)	__IRQ_STAT((cpu), __local_irq_count)  // 硬件中断正在发生的个数
#define local_bh_count(cpu)	__IRQ_STAT((cpu), __local_bh_count)     // 软中断正在发生的个数
#define syscall_count(cpu)	__IRQ_STAT((cpu), __syscall_count)      // 系统调用正在发生的个数
#define nmi_count(cpu)		__IRQ_STAT((cpu), __nmi_count)            // nmi中断(不可屏蔽)正在发生的个数

#endif	/* __irq_cpustat_h */
