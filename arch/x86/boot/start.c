#include <vkernel/kernel.h>
#include <vkernel/init.h>
#include <vkernel/types.h>
#include <asm/gdt.h>
#include <asm/page.h>
#include <asm/multiboot_parse.h>

int is_boot_time __initdata = 1;
extern void start_kernel(void);

__init void gdt_create(void)
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

void __init test_multiboot (void)
{
	// 测试 cr0 的值
    uint32_t cr0;
	asm volatile ("mov %%cr0, %0" : "=r" (cr0));
	printk("cr0 = %x\n", cr0);
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
	gdt_create();
	test_multiboot();
	start_kernel();
}