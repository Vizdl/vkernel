#include <asm/irq.h>
#include <asm/signal.h>
#include <vkernel/init.h>
#include <vkernel/types.h>
#include <vkernel/kernel.h>
#include <vkernel/interrupt.h>

/*
 * timer_interrupt() needs to keep up the real-time clock,
 * as well as call the "do_timer()" routine every clocktick
 */
static inline void do_timer_interrupt(int irq, void *dev_id, struct pt_regs *regs)
{
    printk("do_timer_interrupt -> irq : %d, dev_id : %p, regs : %p\n", irq, dev_id, regs);
}

static void timer_interrupt(int irq, void *dev_id, struct pt_regs *regs)
{
    do_timer_interrupt(irq, NULL, regs);
}

static struct irqaction irq0  = { timer_interrupt, SA_INTERRUPT, 0, "timer", NULL, NULL};

void __init time_init(void)
{
    setup_irq(0, &irq0);
}
