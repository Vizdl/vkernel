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
  	_text = .;			/* Text and read-only data */
	.text : { *(.text) }
  	_etext = .;			/* End of text section */
	.data : { *(.data) }
	.bss : { *(.bss) }
 	_edata = .;			/* End of data section */
	_end = . ;
}