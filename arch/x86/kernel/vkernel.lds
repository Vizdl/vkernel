ENTRY(start)
OUTPUT_FORMAT(elf32-i386)

SECTIONS
{
  . = 0x100000;
  PROVIDE( _start = . );
  .grub.header : { *(.grub.header) }
  .grub.stack : { *(.grub.stack) }
  .text : { *(.text) }
  .data : { *(.data) }
  .bss : { *(.bss) }
  PROVIDE( _end = . );
}