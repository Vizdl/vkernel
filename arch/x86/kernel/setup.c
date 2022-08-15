#include <vkernel/linkage.h>
#include <vkernel/init.h>
#include <vkernel/kernel.h>
#include <vkernel/bootmem.h>
#include <vkernel/mmzone.h>
#include <asm/page.h>

extern char _text, _etext, _edata, _end;

extern void paging_init(void);

#define PFN_UP(x)	(((x) + PAGE_SIZE-1) >> PAGE_SHIFT)
#define PFN_DOWN(x)	((x) >> PAGE_SHIFT)
#define PFN_PHYS(x)	((x) << PAGE_SHIFT)

void __init setup_arch(void)
{
    unsigned long start_pfn;
    start_pfn = PFN_UP((unsigned long)&_end);
    printk("start_pfn is %u\n", start_pfn);
    printk("setup arch...\n");
    paging_init();
    return;
}