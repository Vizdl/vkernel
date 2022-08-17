#include <vkernel/kernel.h>
#include <vkernel/init.h>
#include <vkernel/types.h>
#include <asm/gdt.h>
#include <asm/page.h>
#include <asm/pgtable.h>
#include <asm/multiboot_parse.h>

int is_boot_time __initdata = 1;
extern void start_kernel(void);

pte_t pg0[PTRS_PER_PTE];

__init void sectioning_init(void)
{
	uint64_t gdtr;
    gdt_table[0] = make_gdt_desc((uint32_t*)0, 0, 0, 0);
	// 内核代码段
    gdt_table[1] = make_gdt_desc((uint32_t*)0, 0xfffff, GDT_CODE_ATTR_LOW_DPL0, GDT_ATTR_HIGH);
	// 内核数据段
    gdt_table[2] = make_gdt_desc((uint32_t*)0, 0xfffff, GDT_DATA_ATTR_LOW_DPL0, GDT_ATTR_HIGH);
	// 内核显存段,这里虽然设置为 0xb8000,但 printk 仍然是利用数据段来设置显存。
    gdt_table[3] = make_gdt_desc((uint32_t*)0xb8000, 0xfffff, GDT_DATA_ATTR_LOW_DPL0, GDT_ATTR_HIGH);

    gdtr = ((sizeof(gdt_table) - 1) | ((uint64_t)(uint32_t)gdt_table << 16));
    load_gdtr(gdtr);
    flush_cs(SELECTOR_K_CODE);
    flush_ds(SELECTOR_K_DATA);
    flush_ss(SELECTOR_K_STACK);
    flush_gs(SELECTOR_K_GS);
}

__init void boot_paging_init(void)
{
	pmd_t * pmd;
    unsigned long cr0;
	int i;
	printk("boot_paging_init ...\n");
	// 初始化 swapper_pg_dir
	for (i = 0; i < PTRS_PER_PGD; i++) {
		set_pgd(&swapper_pg_dir[i], __pgd(0));
	}
	// 初始化页表,物理地址设为 0x00000000 - 0x00400000
	for (i = 0; i < PTRS_PER_PTE; i++) {
		pg0[i] = mk_pte_phys((unsigned long)i * PAGE_SIZE, PAGE_KERNEL);
	}
	// 0x00000000 - 0x00400000 虚拟地址
	pmd = (pmd_t *)&swapper_pg_dir[0];
	set_pmd(pmd, __pmd(_PAGE_TABLE + (unsigned long)pg0));
	// 0xc0000000 - 0xc0400000 虚拟地址
	pmd = (pmd_t *)&swapper_pg_dir[768];
	set_pmd(pmd, __pmd(_KERNPG_TABLE + (unsigned long)pg0));
	load_cr3(swapper_pg_dir);
	asm volatile ("mov %%cr0, %0" : "=r" (cr0));
    cr0 |= 0x80000000;
    asm volatile ("mov %0, %%cr0" : : "r" (cr0));
    flush_cs(SELECTOR_K_CODE);
}

void __init cmain(unsigned long magic, unsigned long addr)
{

	/* Am I booted by a Multiboot-compliant boot loader? */
	if (magic != MULTIBOOT2_BOOTLOADER_MAGIC)
	{
		printk("Invalid magic number: 0x%x\n", (unsigned)magic);
		return;
	}

	if (addr & 7)
	{
		printk("Unaligned mbi: 0x%x\n", addr);
		return;
	}
    multiboot_save(addr);
	sectioning_init();
	boot_paging_init();
	start_kernel();
}