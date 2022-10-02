ENTRY(multiboot_start)
OUTPUT_FORMAT(elf32-i386)

SECTIONS
{
  	. = 0x100000;
	.text.grub :
	{
		*(.text.grub)
	  	. = ALIGN(4096);
	}
	.text.init : 
	{ 
	 	*(.text.init) 
	  	. = ALIGN(4096);
	}
	.data.init :
	{
		*(.data.init)
		. = ALIGN(8192);
	}
	/* 
	以下段加载位置是 0x100000 后面, 但符号在 0xC0000000 后面, 
	是为了分页后,跳转到 0xC0000000 无障碍。
	*/
	. += 0xC0000000;
  	_text = .;			/* Text and read-only data */
	.text : AT(ADDR(.text) - 0xC0000000)
	{
		*(.text)
		. = ALIGN(4096);
	}
  	_etext = .;			/* End of text section */
	.data : AT(ADDR(.data) - 0xC0000000)
	{
		*(.data)
		*(.rodata)
		. = ALIGN(8192);
	}
	.bss : AT(ADDR(.bss) - 0xC0000000)
	{
		*(.bss)
		. = ALIGN(4096);
	}
	.data.init_task : AT(ADDR(.data.init_task) - 0xC0000000)
    {
        *(.data.init_task)
        . = ALIGN(8192);
    }
 	_edata = .;			/* End of data section */
	_end = . ;
}