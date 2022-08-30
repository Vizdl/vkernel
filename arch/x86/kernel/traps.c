#include <asm/desc.h>
#include <asm/segment.h>
#include <vkernel/irq.h>
#include <vkernel/init.h>
#include <vkernel/kernel.h>
#include <vkernel/linkage.h>

struct desc_struct idt_table[256] __attribute__((__section__(".data.idt"))) = { {0, 0}, };
extern void cpu_init (void);

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

asmlinkage int system_call(void)
{
    printk("system_call ...\n");
}

asmlinkage void divide_error(void) 
{
    printk("divide_error ...\n");
}

asmlinkage void debug(void) 
{
    printk("debug ...\n");
}

asmlinkage void nmi(void) 
{
    printk("nmi ...\n");
}

asmlinkage void int3(void)
{
    printk("int3 ...\n");
}

asmlinkage void overflow(void)
{
    printk("overflow ...\n");
}

asmlinkage void bounds(void)
{
    printk("bounds ...\n");
}

asmlinkage void invalid_op(void)
{
    printk("invalid_op ...\n");
}

asmlinkage void device_not_available(void)
{
    printk("device_not_available ...\n");
}

asmlinkage void double_fault(void)
{
    printk("double_fault ...\n");
}

asmlinkage void coprocessor_segment_overrun(void)
{
    printk("coprocessor_segment_overrun ...\n");
}

asmlinkage void invalid_TSS(void)
{
    printk("invalid_TSS ...\n");
}

asmlinkage void segment_not_present(void)
{
    printk("segment_not_present ...\n");
}

asmlinkage void stack_segment(void)
{
    printk("stack_segment ...\n");
}

asmlinkage void general_protection(void)
{
    printk("general_protection ...\n");
}

asmlinkage void page_fault(void)
{
    printk("page_fault ...\n");
}

asmlinkage void coprocessor_error(void)
{
    printk("coprocessor_error ...\n");
}

asmlinkage void simd_coprocessor_error(void)
{
    printk("simd_coprocessor_error ...\n");
}

asmlinkage void alignment_check(void)
{
    printk("alignment_check ...\n");
}

asmlinkage void spurious_interrupt_bug(void)
{
    printk("spurious_interrupt_bug ...\n");
}

asmlinkage void machine_check(void)
{
    printk("machine_check ...\n");
}

void __init trap_init(void){
    printk("trap init ...\n");
	set_trap_gate(0,&divide_error);
	set_trap_gate(1,&debug);
	set_intr_gate(2,&nmi);
	set_system_gate(3,&int3);	/* int3-5 can be called from all */
	set_system_gate(4,&overflow);
	set_system_gate(5,&bounds);
	set_trap_gate(6,&invalid_op);
	set_trap_gate(7,&device_not_available);
	set_trap_gate(8,&double_fault);
	set_trap_gate(9,&coprocessor_segment_overrun);
	set_trap_gate(10,&invalid_TSS);
	set_trap_gate(11,&segment_not_present);
	set_trap_gate(12,&stack_segment);
	set_trap_gate(13,&general_protection);
	set_trap_gate(14,&page_fault);
	set_trap_gate(15,&spurious_interrupt_bug);
	set_trap_gate(16,&coprocessor_error);
	set_trap_gate(17,&alignment_check);
	set_trap_gate(18,&machine_check);
	set_trap_gate(19,&simd_coprocessor_error);

	set_system_gate(SYSCALL_VECTOR,&system_call);

    cpu_init();
    return;
}