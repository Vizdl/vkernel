#ifndef _ASM_HW_IRQ_H
#define _ASM_HW_IRQ_H

#include <asm/segment.h>
#include <vkernel/cache.h>
#include <vkernel/linkage.h>

typedef struct {
	unsigned int __softirq_active;
	unsigned int __softirq_mask;
	unsigned int __local_irq_count;
	unsigned int __local_bh_count;
	unsigned int __syscall_count;
	unsigned int __nmi_count;	/* arch dependent */
} ____cacheline_aligned irq_cpustat_t;

#include <vkernel/irq_cpustat.h>

#define in_interrupt() ({ int __cpu = smp_processor_id(); \
	(local_irq_count(__cpu) + local_bh_count(__cpu) != 0); })

#define irq_enter(cpu, irq) do { local_irq_count(cpu)++; } while(0)
#define irq_exit(cpu, irq) do { local_irq_count(cpu)++; } while(0)

#define FIRST_EXTERNAL_VECTOR	0x20
#define SYSCALL_VECTOR		    0x80

// IRQ_NAME2(0x1) ==> 0x1_interrupt(void)
#define IRQ_NAME2(nr) nr##_interrupt(void)
// IRQ_NAME(0x1) ==> IRQ_NAME2(IRQ0x1) ==> IRQ0x1_interrupt(void)
#define IRQ_NAME(nr) IRQ_NAME2(IRQ##nr)

#define __STR(x) #x
#define STR(x) __STR(x)

// 保存寄存器信息,切换 __KERNEL_DS 作为选择符
#define SAVE_ALL \
	"cld\n\t" \
	"pushl %es\n\t" \
	"pushl %ds\n\t" \
	"pushl %eax\n\t" \
	"pushl %ebp\n\t" \
	"pushl %edi\n\t" \
	"pushl %esi\n\t" \
	"pushl %edx\n\t" \
	"pushl %ecx\n\t" \
	"pushl %ebx\n\t" \
	"movl $" STR(__KERNEL_DS) ",%edx\n\t" \
	"movl %edx,%ds\n\t" \
	"movl %edx,%es\n\t"

// 中断汇聚到 common_interrupt 的辅助宏
#define BUILD_IRQ(nr) \
	asmlinkage void IRQ_NAME(nr); \
	__asm__( \
	"\n"__ALIGN_STR"\n" \
	SYMBOL_NAME_STR(IRQ) #nr "_interrupt:\n\t" \
	"pushl $"#nr"-256\n\t" \
	"jmp common_interrupt");

// 保存寄存器,保存函数,执行跳转到 do_IRQ 函数
#define BUILD_COMMON_IRQ() \
asmlinkage void call_do_IRQ(void); \
__asm__( \
	"\n" __ALIGN_STR"\n" \
	"common_interrupt:\n\t" \
	SAVE_ALL \
	"pushl $ret_from_intr\n\t" \
	SYMBOL_NAME_STR(call_do_IRQ)":\n\t" \
	"jmp "SYMBOL_NAME_STR(do_IRQ));

static inline void hw_resend_irq(struct hw_interrupt_type *h, unsigned int i) {}

#endif /* _ASM_HW_IRQ_H */
