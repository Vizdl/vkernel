#include <vkernel/init.h>
#include <vkernel/kernel.h>

void __init trap_init(void){
    printk("trap init ...\n");
    return;
}