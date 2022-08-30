#include <asm/desc.h>
#include <asm/segment.h>
#include <vkernel/init.h>
#include <vkernel/kernel.h>

struct desc_struct idt_table[256] __attribute__((__section__(".data.idt"))) = { {0, 0}, };

#define _set_gate(gate_addr,type,dpl,addr) \
do { \
  int __d0, __d1; \
  __asm__ __volatile__ ("movw %%dx,%%ax\n\t" \
	"movw %4,%%dx\n\t" \
	"movl %%eax,%0\n\t" \
	"movl %%edx,%1" \
	:"=m" (*((long *) (gate_addr))), \
	 "=m" (*(1+(long *) (gate_addr))), "=&a" (__d0), "=&d" (__d1) \
	:"i" ((short) (0x8000+(dpl<<13)+(type<<8))), \
	 "3" ((char *) (addr)),"2" (__KERNEL_CS << 16)); \
} while (0)


#define _set_tssldt_desc(n,addr,limit,type) \
__asm__ __volatile__ ("movw %w3,0(%2)\n\t" \
	"movw %%ax,2(%2)\n\t" \
	"rorl $16,%%eax\n\t" \
	"movb %%al,4(%2)\n\t" \
	"movb %4,5(%2)\n\t" \
	"movb $0,6(%2)\n\t" \
	"movb %%ah,7(%2)\n\t" \
	"rorl $16,%%eax" \
	: "=m"(*(n)) : "a" (addr), "r"(n), "ir"(limit), "i"(type))

void set_intr_gate(unsigned int n, void *addr)
{
	_set_gate(idt_table+n,14,0,addr);
}

static void __init set_trap_gate(unsigned int n, void *addr)
{
	_set_gate(idt_table+n,15,0,addr);
}

static void __init set_system_gate(unsigned int n, void *addr)
{
	_set_gate(idt_table+n,15,3,addr);
}

static void __init set_call_gate(void *a, void *addr)
{
	_set_gate(a,12,3,addr);
}

void set_tss_desc(unsigned int n, void *addr)
{
	_set_tssldt_desc(gdt_table+__TSS(n), (int)addr, 235, 0x89);
}

void set_ldt_desc(unsigned int n, void *addr, unsigned int size)
{
	_set_tssldt_desc(gdt_table+__LDT(n), (int)addr, ((size << 3)-1), 0x82);
}

void __init trap_init(void){
    printk("trap init ...\n");
    return;
}