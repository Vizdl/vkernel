#include <vkernel/linkage.h>
#include <asm/ptrace.h>
#include <asm/irq.h>
#include <vkernel/types.h>
#include <vkernel/spinlock.h>
#include <vkernel/kernel.h>

#define IRQ_INPROGRESS	1	/* IRQ handler active - do not enter! */
#define IRQ_DISABLED	2	/* IRQ disabled - do not enter! */
#define IRQ_PENDING	4	/* IRQ pending - replay on enable */
#define IRQ_REPLAY	8	/* IRQ has been replayed but not acked yet */
#define IRQ_AUTODETECT	16	/* IRQ is being autodetected */
#define IRQ_WAITING	32	/* IRQ not yet seen - for autodetection */
#define IRQ_LEVEL	64	/* IRQ level triggered */
#define IRQ_MASKED	128	/* IRQ masked - shouldn't be seen again */
#define IRQ_PER_CPU	256	/* IRQ is per CPU */

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

volatile unsigned long irq_err_count;

irq_desc_t irq_desc[NR_IRQS] __cacheline_aligned =
        { [0 ... NR_IRQS-1] = { 0, &no_irq_type, NULL, 0, SPIN_LOCK_UNLOCKED}};

void inline disable_irq_nosync(unsigned int irq)
{
    irq_desc_t *desc = irq_desc + irq;
    unsigned long flags;

    spin_lock_irqsave(&desc->lock, flags);
    if (!desc->depth++) {
        desc->status |= IRQ_DISABLED;
        desc->handler->disable(irq);
    }
    spin_unlock_irqrestore(&desc->lock, flags);
}

void enable_irq(unsigned int irq)
{
    irq_desc_t *desc = irq_desc + irq;
    unsigned long flags;

    spin_lock_irqsave(&desc->lock, flags);
    switch (desc->depth) {
        case 1: {
            unsigned int status = desc->status & ~IRQ_DISABLED;
            desc->status = status;
            if ((status & (IRQ_PENDING | IRQ_REPLAY)) == IRQ_PENDING) {
                desc->status = status | IRQ_REPLAY;
            }
            /* fall-through */
        }
        default:
            desc->depth--;
            break;
        case 0:
            printk("enable_irq(%u) unbalanced\n", irq);
    }
    spin_unlock_irqrestore(&desc->lock, flags);
}

void disable_irq(unsigned int irq)
{
	disable_irq_nosync(irq);
}

asmlinkage unsigned int do_IRQ(struct pt_regs regs)
{
    return 0;
}