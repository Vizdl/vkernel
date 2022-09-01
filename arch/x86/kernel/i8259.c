#include <vkernel/init.h>
#include <vkernel/kernel.h>

void __init init_IRQ(void)
{
    printk("init_IRQ...\n");
    return ;
}