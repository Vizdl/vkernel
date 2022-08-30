#ifndef __ARCH_DESC_H
#define __ARCH_DESC_H

#include <asm/ldt.h>
#include <asm/gdt.h>
#include <asm/idt.h>
#include <asm/tss.h>

#define __FIRST_TSS_ENTRY 12
#define __FIRST_LDT_ENTRY (__FIRST_TSS_ENTRY+1)

#define __TSS(n) (((n)<<2) + __FIRST_TSS_ENTRY)
#define __LDT(n) (((n)<<2) + __FIRST_LDT_ENTRY)

#ifndef __ASSEMBLY__

extern struct gdt_desc gdt_table[GDT_TABLE_SIZE];
extern struct idt_desc idt_table[IDT_TABLE_SIZE];

#endif /* !__ASSEMBLY__ */

#endif
