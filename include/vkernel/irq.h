#ifndef __irq_h
#define __irq_h

#include <asm/irq.h>
#include <asm/ptrace.h>
#include <vkernel/cache.h>
#include <vkernel/spinlock.h>

#define NR_IRQS 224

#define IRQ_INPROGRESS  1
#define IRQ_DISABLED    2
#define IRQ_PENDING     4

// 针对不同的可编程芯片做不同的处理
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

#include <asm/hw_irq.h>

#endif /* __irq_h */