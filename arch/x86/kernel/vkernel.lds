ENTRY(multiboot_start)
OUTPUT_FORMAT(elf32-i386)

SECTIONS
{
  . = 0x100000;
  .grub.header : 
  { 
    *(.grub.header) 
	  . = ALIGN(4096);
  }
  .text : { *(.text) }
  .data : { *(.data) }
  .bss : { *(.bss) }
}