#include <vkernel/init.h>
#include <vkernel/slab.h>
#include <vkernel/sched.h>
#include <vkernel/kernel.h>
#include <vkernel/linkage.h>
#include <vkernel/sched.h>

extern void setup_arch(void);
extern void init_IRQ(void);
extern void mem_init(void);
extern void time_init(void);

void __init show_init_task(void)
{
    printk("current task's state : %d...\n", current->state);
    return;
}

static int init(void * unused)
{
    printk("init...\n");
    return 0;
}

asmlinkage void __init start_kernel(void)
{
    printk("start kernel...\n");
    setup_arch();
    trap_init();
    init_IRQ();
    // for test irq
    // __asm__ __volatile__ ("int $32");
    show_init_task();
    // time_init();
    sched_init();
    // kmem_cache_init();
    // mem_init();
    // kmem_cache_sizes_init();
    printk("end kernel...\n");
	kernel_thread(init, NULL, CLONE_FS | CLONE_FILES | CLONE_SIGNAL);
    while (1);
    return;
}