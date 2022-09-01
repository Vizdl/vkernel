#include <vkernel/init.h>
#include <vkernel/linkage.h>
#include <vkernel/kernel.h>
#include <vkernel/sched.h>

extern void setup_arch(void);
extern void init_IRQ(void);

asmlinkage void __init start_kernel(void)
{
    printk("start kernel...\n");
    setup_arch();
    trap_init();
    init_IRQ();
    return;
}