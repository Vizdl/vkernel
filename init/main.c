#include <vkernel/init.h>
#include <vkernel/linkage.h>
#include <vkernel/kernel.h>

extern void setup_arch(void);

asmlinkage void __init start_kernel(void)
{
    printk("start kernel...\n");
    setup_arch();
    return;
}