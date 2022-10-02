#include <vkernel/init.h>
#include <vkernel/slab.h>
#include <vkernel/sched.h>
#include <vkernel/kernel.h>
#include <vkernel/linkage.h>
#include <vkernel/sched.h>

extern void setup_arch(void);
extern void init_IRQ(void);
extern void mem_init(void);

void __init show_init_task(void)
{
    printk("current task's state : %d...\n", current->state);
    return;
}

asmlinkage void __init start_kernel(void)
{
    printk("start kernel...\n");
    setup_arch();
    trap_init();
    init_IRQ();
    // for test irq
    __asm__ __volatile__ ("int $32");
    show_init_task();
    // kmem_cache_init();
    // mem_init();
    // kmem_cache_sizes_init();
    printk("end kernel...\n");
    return;
}