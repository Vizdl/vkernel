#ifndef __ASM_HARDIRQ_H
#define __ASM_HARDIRQ_H

#include <vkernel/smp.h>
#include <vkernel/cache.h>

typedef struct {
	unsigned int __softirq_active;
	unsigned int __softirq_mask;
	unsigned int __local_irq_count;
	unsigned int __local_bh_count;
	unsigned int __syscall_count;
	unsigned int __nmi_count;	/* arch dependent */
} ____cacheline_aligned irq_cpustat_t;

#include <vkernel/irq_cpustat.h>

// 是否在硬中断中
#define in_interrupt() ({ int __cpu = smp_processor_id(); \
	(local_irq_count(__cpu) + local_bh_count(__cpu) != 0); })
// 进入硬中断处理流程
#define irq_enter(cpu, irq) do { local_irq_count(cpu)++; } while(0)
// 退出硬中断处理流程
#define irq_exit(cpu, irq) do { local_irq_count(cpu)++; } while(0)

#endif /* __ASM_HARDIRQ_H */
