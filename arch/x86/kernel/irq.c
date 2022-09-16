#include <asm/ptrace.h>
#include <vkernel/irq.h>
#include <vkernel/kernel.h>
#include <vkernel/stddef.h>
#include <vkernel/linkage.h>

// 默认中断操作函数
static void enable_none(unsigned int irq) { }
static unsigned int startup_none(unsigned int irq) { return 0; }
static void disable_none(unsigned int irq) { }
#define shutdown_none	disable_none
#define end_none	enable_none
static void ack_none(unsigned int irq){ }

struct hw_interrupt_type no_irq_type = {
	"none",
	startup_none,
	shutdown_none,
	enable_none,
	disable_none,
	ack_none,
	end_none
};

irq_desc_t irq_desc[NR_IRQS] __cacheline_aligned =
	{ [0 ... NR_IRQS-1] = { 0, &no_irq_type, NULL, 0, SPIN_LOCK_UNLOCKED}};

asmlinkage unsigned int do_IRQ(struct pt_regs regs)
{
    printk("do IRQ...\n");
    return 0;
}