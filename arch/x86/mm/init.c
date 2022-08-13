#include <vkernel/init.h>
#include <vkernel/kernel.h>

static void __init pagetable_init (void)
{
    printk("pagetable_init...\n");
    return;
}

void __init paging_init(void)
{
    printk("paging_init...\n");
    pagetable_init();
    return;
}