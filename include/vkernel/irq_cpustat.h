#ifndef __irq_cpustat_h
#define __irq_cpustat_h

#include <vkernel/config.h>

// 对于每个 cpu 里中断信息的记载。
extern irq_cpustat_t irq_stat[];			/* defined in asm/hardirq.h */

#define __IRQ_STAT(cpu, member)	(irq_stat[cpu].member)

  /* arch independent irq_stat fields */
#define softirq_active(cpu)	__IRQ_STAT((cpu), __softirq_active)
#define softirq_mask(cpu)	__IRQ_STAT((cpu), __softirq_mask)
#define local_irq_count(cpu)	__IRQ_STAT((cpu), __local_irq_count)
#define local_bh_count(cpu)	__IRQ_STAT((cpu), __local_bh_count)
#define syscall_count(cpu)	__IRQ_STAT((cpu), __syscall_count)
  /* arch dependent irq_stat fields */
#define nmi_count(cpu)		__IRQ_STAT((cpu), __nmi_count)		/* i386, ia64 */

#endif	/* __irq_cpustat_h */
