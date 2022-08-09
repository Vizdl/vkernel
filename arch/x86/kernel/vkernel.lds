ENTRY(multiboot_start)
OUTPUT_FORMAT(elf32-i386)

SECTIONS
{
  . = 0x100000;
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
	.text : { *(.text) }
	.data : { *(.data) }
	.bss : { *(.bss) }
}