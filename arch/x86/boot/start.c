#include <vkernel/kernel.h>
#include <vkernel/init.h>
#include <vkernel/types.h>
#include <asm/desc.h>
#include <asm/page.h>
#include <asm/system.h>
#include <asm/pgtable.h>
#include <asm/multiboot_parse.h>

extern void start_kernel(void);

pte_t pg0[PTRS_PER_PTE];

void __init sectioning_init(void)
{
	uint64_t gdtr;
	// 获取到 gdt_table 的物理地址
	struct gdt_desc * gdt_table_addr = (struct gdt_desc *)__pa(gdt_table);
    gdt_table_addr[0] = make_gdt_desc((uint32_t*)0, 0, 0, 0);
	// 内核代码段
    gdt_table_addr[1] = make_gdt_desc((uint32_t*)0, 0xfffff, GDT_CODE_ATTR_LOW_DPL0, GDT_ATTR_HIGH);
	// 内核数据段
    gdt_table_addr[2] = make_gdt_desc((uint32_t*)0, 0xfffff, GDT_DATA_ATTR_LOW_DPL0, GDT_ATTR_HIGH);
	// 内核显存段,这里虽然设置为 0xb8000,但 printk 仍然是利用数据段来设置显存。
    gdt_table_addr[3] = make_gdt_desc((uint32_t*)0xb8000, 0xfffff, GDT_DATA_ATTR_LOW_DPL0, GDT_ATTR_HIGH);

    gdtr = ((sizeof(gdt_table) - 1) | ((uint64_t)__pa(gdt_table) << 16));
    load_gdtr(gdtr);
    flush_cs(SELECTOR_K_CODE);
    flush_ds(SELECTOR_K_DATA);
    flush_ss(SELECTOR_K_STACK);
    flush_gs(SELECTOR_K_GS);
}

void __init boot_paging_init(void)
{
	pgd_t * pgd;
	pmd_t * pmd;
	pte_t * pte;
	int i;
	// 获得 swapper_pg_dir 的物理地址
	pgd = (pgd_t*)__pa(swapper_pg_dir);
	// 初始化 swapper_pg_dir
	for (i = 0; i < PTRS_PER_PGD; i++) {
		set_pgd(&pgd[i], __pgd(0));
	}
	// 获得 pg0 的物理地址
	pte = (pte_t *)__pa(pg0);
	// 初始化页表,物理地址设为 0x00000000 - 0x00400000
	for (i = 0; i < PTRS_PER_PTE; i++) {
		set_pte(&pte[i], mk_pte_phys((unsigned long)i * PAGE_SIZE, PAGE_KERNEL));
	}
	// 0x00000000 - 0x00400000 虚拟地址
	pmd = (pmd_t *)&pgd[0];
	set_pmd(pmd, __pmd(_PAGE_TABLE + __pa(pg0)));
	// 0xc0000000 - 0xc0400000 虚拟地址
	pmd = (pmd_t *)&pgd[768];
	set_pmd(pmd, __pmd(_KERNPG_TABLE + __pa(pg0)));
	// 加载 cr3, 开启分页模式, 刷新 cs 寄存器
	load_cr3(__pa(swapper_pg_dir));
	paging();
    flush_cs(SELECTOR_K_CODE);
}

void __init cmain(unsigned long magic, unsigned long addr)
{
	sectioning_init();
	boot_paging_init();
	/* Am I booted by a Multiboot-compliant boot loader? */
	if (magic != MULTIBOOT2_BOOTLOADER_MAGIC)
	{
		printk("Invalid magic number: 0x%x\n", (unsigned)magic);
		return;
	}

	if (addr & 7)
	{
		return;
	}
    multiboot_save(addr);
	start_kernel();
}