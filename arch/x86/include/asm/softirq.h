#ifndef __ASM_SOFTIRQ_H
#define __ASM_SOFTIRQ_H

#include <asm/atomic.h>
#include <asm/hardirq.h>

#define cpu_bh_disable(cpu)	do { local_bh_count(cpu)++; barrier(); } while (0)
#define cpu_bh_enable(cpu)	do { barrier(); local_bh_count(cpu)--; } while (0)

// 进入软中断
#define local_bh_disable()	cpu_bh_disable(smp_processor_id())
// 退出软中断
#define local_bh_enable()	cpu_bh_enable(smp_processor_id())

// 是否在软中断中
#define in_softirq() (local_bh_count(smp_processor_id()) != 0)

#endif	/* __ASM_SOFTIRQ_H */
