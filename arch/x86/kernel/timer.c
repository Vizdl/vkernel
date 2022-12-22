#include <vkernel/kernel.h>

unsigned long volatile jiffies;

void do_timer(struct pt_regs *regs)
{
    printk("do_timer...\n");
	(*(unsigned long *)&jiffies)++;
}