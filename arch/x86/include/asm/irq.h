#ifndef _ASM_IRQ_H
#define _ASM_IRQ_H

extern volatile unsigned long irq_err_count;

extern void disable_irq(unsigned int);
extern void disable_irq_nosync(unsigned int);
extern void enable_irq(unsigned int);

#endif /* _ASM_IRQ_H */