#ifndef _ASM_IRQ_H
#define _ASM_IRQ_H

extern volatile unsigned long irq_err_count;

extern void disable_irq(unsigned int);
extern void disable_irq_nosync(unsigned int);
extern void enable_irq(unsigned int);

#include <vkernel/interrupt.h>

extern int setup_irq(unsigned int irq, struct irqaction * new);

#endif /* _ASM_IRQ_H */