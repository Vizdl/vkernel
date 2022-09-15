#include <asm/ptrace.h>
#include <vkernel/kernel.h>
#include <vkernel/linkage.h>


asmlinkage unsigned int do_IRQ(struct pt_regs regs)
{
    printk("do IRQ...\n");
    return 0;
}