#ifndef _ASM_IRQ_H
#define _ASM_IRQ_H

#include <asm/ptrace.h>
#include <vkernel/linkage.h>
#include <vkernel/spinlock.h>
#include <vkernel/cache.h>

#define NR_IRQS 224

#define IRQ_INPROGRESS  1
#define IRQ_DISABLED    2
#define IRQ_PENDING     4

asmlinkage unsigned int do_IRQ(struct pt_regs regs);

struct hw_interrupt_type {
    const char * typename;
    unsigned int (*startup)(unsigned int irq);
    void (*shutdown)(unsigned int irq);
    void (*enable)(unsigned int irq);
    void (*disable)(unsigned int irq);
    void (*ack)(unsigned int irq);
    void (*end)(unsigned int irq);
    void (*set_affinity)(unsigned int irq, unsigned long mask);
};
extern volatile unsigned long irq_err_count;
typedef struct hw_interrupt_type  hw_irq_controller;

typedef struct {
	unsigned int status;		/* IRQ status */
	hw_irq_controller *handler;
	struct irqaction *action;	/* IRQ action list */
	unsigned int depth;		/* nested irq disables */
	spinlock_t lock;
} ____cacheline_aligned irq_desc_t;

extern irq_desc_t irq_desc [NR_IRQS];

extern struct hw_interrupt_type no_irq_type;

extern void disable_irq(unsigned int);
extern void disable_irq_nosync(unsigned int);
extern void enable_irq(unsigned int);

extern volatile unsigned long irq_err_count;

#endif /* _ASM_IRQ_H */