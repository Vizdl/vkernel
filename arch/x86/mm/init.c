#include <vkernel/init.h>
#include <vkernel/kernel.h>
#include <asm/page.h>
#include <asm/pgtable.h>

pgd_t swapper_pg_dir[PTRS_PER_PGD] __page_aligned;

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