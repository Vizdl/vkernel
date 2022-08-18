#include <vkernel/linkage.h>
#include <vkernel/init.h>
#include <vkernel/kernel.h>
#include <vkernel/bootmem.h>
#include <vkernel/mmzone.h>
#include <vkernel/debug.h>
#include <asm/page.h>
#include <asm/e820.h>
#include <asm/multiboot_parse.h>

// 这些都是虚拟地址
extern char _text, _etext, _edata, _end;

struct e820map e820;

extern void paging_init(void);

#define PFN_UP(x)	(((x) + PAGE_SIZE-1) >> PAGE_SHIFT)
#define PFN_DOWN(x)	((x) >> PAGE_SHIFT)
#define PFN_PHYS(x)	((x) << PAGE_SHIFT)

#define VMALLOC_RESERVE	(unsigned long)(128 << 20)
#define MAXMEM		(unsigned long)(-PAGE_OFFSET-VMALLOC_RESERVE)
#define MAXMEM_PFN	PFN_DOWN(MAXMEM)

static void __init print_memory_map(char *who)
{
	int i;

	for (i = 0; i < e820.nr_map; i++) {
		// printk(" %s: %016Lx @ %016Lx ", who,
		// 	e820.map[i].size, e820.map[i].addr);
		printk(" %s: base_addr = 0x%x%x, length =  0x%x%x ", who,
			(unsigned)(e820.map[i].addr >> 32), (unsigned)(e820.map[i].addr & 0xffffffff),
			(unsigned)(e820.map[i].size >> 32), (unsigned)(e820.map[i].size & 0xffffffff));
		switch (e820.map[i].type) {
		case E820_RAM:	printk("(usable)\n");
				break;
		case E820_RESERVED:
				printk("(reserved)\n");
				break;
		case E820_ACPI:
				printk("(ACPI data)\n");
				break;
		case E820_NVS:
				printk("(ACPI NVS)\n");
				break;
		default:	printk("type %lu\n", e820.map[i].type);
				break;
		}
	}
}

void __init add_memory_region(unsigned long long start,
                                  unsigned long long size, int type)
{
	int x = e820.nr_map;

	if (x == E820MAX) {
	    printk("Ooops! Too many entries in the memory map!\n");
	    return;
	}

	e820.map[x].addr = start;
	e820.map[x].size = size;
	e820.map[x].type = type;
	e820.nr_map++;
} /* add_memory_region */

static int __init copy_e820_map_from_multiboot(void)
{
    multiboot_memory_map_t *mmap;
    struct multiboot_tag *tag = multiboot_memory_map;
    for (mmap = ((struct multiboot_tag_mmap *)tag)->entries;
        (multiboot_uint8_t *)mmap < (multiboot_uint8_t *)tag + tag->size;
        mmap = (multiboot_memory_map_t *)((unsigned long)mmap + ((struct multiboot_tag_mmap *)tag)->entry_size)){
            printk(" base_addr = 0x%x%x,"
                " length = 0x%x%x, type = 0x%x\n",
                (unsigned)(mmap->addr >> 32),
                (unsigned)(mmap->addr & 0xffffffff),
                (unsigned)(mmap->len >> 32),
                (unsigned)(mmap->len & 0xffffffff),
                (unsigned)mmap->type);        
		    add_memory_region(mmap->addr, mmap->len, mmap->type);
        }
    return 0;
}

void __init setup_memory_region(void)
{
	char *who = "multiboot";
	if (copy_e820_map_from_multiboot())
        BUG();
	printk("BIOS-provided physical RAM map:\n");
	print_memory_map(who);
} /* setup_memory_region */


void __init setup_arch(void)
{
	int i;
	unsigned long bootmap_size;
	unsigned long start_pfn, max_pfn, max_low_pfn;
    printk("setup arch...\n");
	setup_memory_region();
    start_pfn = PFN_UP((unsigned long)__pa(&_end));
    /*
	 * Find the highest page frame number we have available
	 */
	max_pfn = 0;
	for (i = 0; i < e820.nr_map; i++) {
		unsigned long start, end;
		/* RAM? */
		if (e820.map[i].type != E820_RAM)
			continue;
		start = PFN_UP(e820.map[i].addr);
		end = PFN_DOWN(e820.map[i].addr + e820.map[i].size);
		if (start >= end)
			continue;
		if (end > max_pfn)
			max_pfn = end;
	}

	/*
	 * Determine low and high memory ranges:
	 */
	max_low_pfn = max_pfn;
	if (max_low_pfn > MAXMEM_PFN) {
		max_low_pfn = MAXMEM_PFN;
	}
    printk("start_pfn is %u, max_pfn is %u, max_low_pfn is %u\n", start_pfn, max_pfn, max_low_pfn);

	/*
	 * Initialize the boot-time allocator (with low memory only):
	 */
	bootmap_size = init_bootmem(start_pfn, max_low_pfn);

	/*
	 * Register fully available low RAM pages with the bootmem allocator.
	 */
	for (i = 0; i < e820.nr_map; i++) {
		unsigned long curr_pfn, last_pfn, size;
 		/*
		 * Reserve usable low memory
		 */
		if (e820.map[i].type != E820_RAM)
			continue;
		/*
		 * We are rounding up the start address of usable memory:
		 */
		curr_pfn = PFN_UP(e820.map[i].addr);
		if (curr_pfn >= max_low_pfn)
			continue;
		/*
		 * ... and at the end of the usable range downwards:
		 */
		last_pfn = PFN_DOWN(e820.map[i].addr + e820.map[i].size);

		if (last_pfn > max_low_pfn)
			last_pfn = max_low_pfn;

		/*
		 * .. finally, did all the rounding and playing
		 * around just make the area go away?
		 */
		if (last_pfn <= curr_pfn)
			continue;

		size = last_pfn - curr_pfn;
		free_bootmem(PFN_PHYS(curr_pfn), PFN_PHYS(size));
	}
	/*
	 * Reserve the bootmem bitmap itself as well. We do this in two
	 * steps (first step was init_bootmem()) because this catches
	 * the (very unlikely) case of us accidentally initializing the
	 * bootmem allocator with an invalid RAM area.
	 */
	reserve_bootmem(HIGH_MEMORY, (PFN_PHYS(start_pfn) +
			 bootmap_size + PAGE_SIZE-1) - (HIGH_MEMORY));

	/*
	 * reserve physical page 0 - it's a special BIOS page on many boxes,
	 * enabling clean reboots, SMP operation, laptop functions.
	 */
	reserve_bootmem(0, PAGE_SIZE);

    paging_init();
    return;
}