/* boot.S - bootstrap the kernel */
/* Copyright (C) 1999, 2001, 2010 Free Software Foundation, Inc.
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program. If not, see <http://www.gnu.org/licenses/>.
*/

#define ASM_FILE 1
#include <asm/page.h>
#include <asm/multiboot2.h>
#include <vkernel/linkage.h>

/* C symbol format. HAVE ASM USCORE is defined by configure. */
#ifdef HAVE_ASM_USCORE
# define EXT_C(sym) _ ## sym
#else
# define EXT_C(sym) sym
#endif

/* The flags for the Multiboot header. */
#ifdef __ELF__
# define AOUT_KLUDGE 0
#else
# define AOUT_KLUDGE MULTIBOOT_AOUT_KLUDGE
#endif

#define GRUB_MULTIBOOT_ARCHITECTURE_I386 MULTIBOOT_ARCHITECTURE_I386

	.section .text.grub
	.globl multiboot_start
multiboot_start:
	jmp multiboot_entry

	/* Align 64 bits boundary. */
	.align 8

	/* Multiboot header. */
multiboot_header:
	/* magic */
	.long MULTIBOOT2_HEADER_MAGIC
	/* ISA: i386 */
	.long GRUB_MULTIBOOT_ARCHITECTURE_I386
	/* Header length. */
	.long multiboot_header_end - multiboot_header
	/* checksum */
	.long -(MULTIBOOT2_HEADER_MAGIC + GRUB_MULTIBOOT_ARCHITECTURE_I386 + (multiboot_header_end - multiboot_header))
#ifndef __ELF__

	.align 8
address_tag_start:
	.short MULTIBOOT_HEADER_TAG_ADDRESS
	.short MULTIBOOT_HEADER_TAG_OPTIONAL
	.long address_tag_end - address_tag_start
	/* header_addr */
	.long multiboot_header
	/* load_addr */
	.long multiboot_start
	/* load_end_addr */
	.long multiboot_edata
	/* bss end addr */
	.long multiboot_end
address_tag_end:

	.align 8
entry_address_tag_start:
	.short MULTIBOOT_HEADER_TAG_ENTRY_ADDRESS
	.short MULTIBOOT_HEADER_TAG_OPTIONAL
	.long entry_address_tag_end - entry_address_tag_start
	/* entry addr */
	.long multiboot_entry
entry_address_tag_end:
#endif /* ELF */

/*
	.align 8
framebuffer_tag_start:
	.short MULTIBOOT_HEADER_TAG_FRAMEBUFFER
	.short MULTIBOOT_HEADER_TAG_OPTIONAL
	.long framebuffer_tag_end - framebuffer_tag_start
	.long 1024
	.long 768
	.long 32
framebuffer_tag_end:
*/

	.align 8
	.short MULTIBOOT_HEADER_TAG_END
	.short 0
	.long 8
multiboot_header_end:
multiboot_entry:
	/* Initialize the stack pointer. */
	movl $__PAGE_OFFSET, %ecx
    movl (stack_start), %esp
	subl %ecx, %esp

	/* Reset EFLAGS. */
	pushl $0
	popf

	/* Push the pointer to the Multiboot information structure. */
	pushl %ebx
	/* Push the magic value. */
	pushl %eax

	/* Now enter the C main function... */
	call EXT_C(cmain)

	/* Halt. */
	pushl $halt_message
	call EXT_C(printk)

loop:
	hlt
	jmp loop

halt_message:
	.asciz "Halted."

.section .data.init
ENTRY(stack_start)
	# init_task 栈指针, 8192 是 INIT_TASK_SIZE
	# 这里放弃了一部分内核栈空间,否则应该是 
	# SYMBOL_NAME(init_task_union)+8192 + sizeof(struct task_struct)
	.long SYMBOL_NAME(init_task_union)+8192
.previous
