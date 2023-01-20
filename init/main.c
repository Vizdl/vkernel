#include <asm/delay.h>
#include <vkernel/mm.h>
#include <vkernel/init.h>
#include <vkernel/slab.h>
#include <vkernel/sched.h>
#include <vkernel/kernel.h>
#include <vkernel/linkage.h>
#include <vkernel/interrupt.h>

extern void setup_arch(void);
extern void init_IRQ(void);
extern void mem_init(void);
extern void time_init(void);
extern void fork_init(unsigned long);

void cpu_idle (void);

void __init show_init_task(void)
{
    printk("current task's state : %d...\n", current->state);
    return;
}

void __init int_test(void)
{
    __asm__ __volatile__ ("int $32");
}

static int init(void * unused)
{
    int count = 0;
    printk("init : %p\n", unused);
    while (1) {
        if (count > 20000) {
            printk("init running...\n");
            count = 0;
            continue;
        }
        count++;
        udelay(100);
    };
    return 0;
}

asmlinkage void __init start_kernel(void)
{
	unsigned long mempages;
    setup_arch();
    trap_init();
    init_IRQ();
    // int_test();
    // show_init_task();
    sched_init();
    time_init();
	softirq_init();
    // kmem_cache_init();
    mem_init();
    mempages = num_physpages;
    fork_init(mempages);
    // kmem_cache_sizes_init();
	kernel_thread(init, NULL, CLONE_FS | CLONE_FILES | CLONE_SIGNAL);
    printk("end kernel...\n");
    cpu_idle();
    return;
}