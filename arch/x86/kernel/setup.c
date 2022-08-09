#include <vkernel/linkage.h>
#include <vkernel/init.h>
#include <vkernel/kernel.h>

void __init setup_arch(void)
{
    printk("setup arch...\n");
    return;
}