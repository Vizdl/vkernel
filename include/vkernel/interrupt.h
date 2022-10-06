#ifndef _LINUX_INTERRUPT_H
#define _LINUX_INTERRUPT_H

#include <asm/ptrace.h>
#include <asm/softirq.h>
#include <asm/hardirq.h>

struct irqaction {
	void (*handler)(int, void *, struct pt_regs *);
	unsigned long flags;
	unsigned long mask;
	const char *name;
	void *dev_id;
	struct irqaction *next;
};

struct softirq_action
{
	void	(*action)(struct softirq_action *);
	void	*data;
};

static inline void __cpu_raise_softirq(int cpu, int nr)
{
	softirq_active(cpu) |= (1<<nr);
}

struct tasklet_struct
{
	struct tasklet_struct *next;
	unsigned long state;
	atomic_t count;
	void (*func)(unsigned long);
	unsigned long data;
};

struct tasklet_head
{
	struct tasklet_struct *list;
} __attribute__ ((__aligned__(SMP_CACHE_BYTES)));

#endif