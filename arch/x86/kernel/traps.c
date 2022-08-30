#include <vkernel/init.h>
#include <vkernel/kernel.h>
#include <vkernel/linkage.h>
#include <asm/ptrace.h>
#include <asm/hw_irq.h>
#include <asm/desc.h>

extern void __init cpu_init (void);

asmlinkage void divide_error(void) ;
asmlinkage void debug(void);
asmlinkage void nmi(void);
asmlinkage void int3(void);
asmlinkage void overflow(void);
asmlinkage void bounds(void);
asmlinkage void invalid_op(void);
asmlinkage void device_not_available(void);
asmlinkage void double_fault(void);
asmlinkage void coprocessor_segment_overrun(void);
asmlinkage void invalid_TSS(void);
asmlinkage void segment_not_present(void);
asmlinkage void stack_segment(void);
asmlinkage void general_protection(void);
asmlinkage void page_fault(void);
asmlinkage void coprocessor_error(void);
asmlinkage void simd_coprocessor_error(void);
asmlinkage void alignment_check(void);
asmlinkage void spurious_interrupt_bug(void);
asmlinkage void machine_check(void);
asmlinkage void system_call(void);


void do_system_call(struct pt_regs *regs, unsigned long error_code)
{
    printk("system_call ...\n");
}

void do_divide_error(struct pt_regs *regs, unsigned long error_code)
{
    printk("do_divide_error ...\n");
}

void do_debug(struct pt_regs *regs, unsigned long error_code)
{
    printk("do_debug ...\n");
}

void do_nmi(struct pt_regs *regs, unsigned long error_code)
{
    printk("do_nmi ...\n");
}

void do_int3(struct pt_regs *regs, unsigned long error_code)
{
    printk("do_int3 ...\n");
}

void do_overflow(struct pt_regs *regs, unsigned long error_code)
{
    printk("do_overflow ...\n");
}

void do_bounds(struct pt_regs *regs, unsigned long error_code)
{
    printk("do_bounds ...\n");
}

void do_invalid_op(struct pt_regs *regs, unsigned long error_code)
{
    printk("do_invalid_op ...\n");
}

void do_device_not_available(struct pt_regs *regs, unsigned long error_code)
{
    printk("do_device_not_available ...\n");
}

void do_double_fault(struct pt_regs *regs, unsigned long error_code)
{
    printk("do_double_fault ...\n");
}

void do_coprocessor_segment_overrun(struct pt_regs *regs, unsigned long error_code)
{
    printk("do_coprocessor_segment_overrun ...\n");
}

void do_invalid_TSS(struct pt_regs *regs, unsigned long error_code)
{
    printk("do_invalid_TSS ...\n");
}

void do_segment_not_present(struct pt_regs *regs, unsigned long error_code)
{
    printk("do_segment_not_present ...\n");
}

void do_stack_segment(struct pt_regs *regs, unsigned long error_code)
{
    printk("do_stack_segment ...\n");
}

void do_general_protection(struct pt_regs *regs, unsigned long error_code)
{
    printk("do_general_protection ...\n");
}

void do_page_fault(struct pt_regs *regs, unsigned long error_code)
{
    printk("do_page_fault ...\n");
}

void do_coprocessor_error(struct pt_regs *regs, unsigned long error_code)
{
    printk("do_coprocessor_error ...\n");
}

void do_simd_coprocessor_error(struct pt_regs *regs, unsigned long error_code)
{
    printk("do_simd_coprocessor_error ...\n");
}

void do_alignment_check(struct pt_regs *regs, unsigned long error_code)
{
    printk("do_alignment_check ...\n");
}

void do_spurious_interrupt_bug(struct pt_regs *regs, unsigned long error_code)
{
    printk("do_spurious_interrupt_bug ...\n");
}

void do_machine_check(struct pt_regs *regs, unsigned long error_code)
{
    printk("do_machine_check ...\n");
}

/**
 * @brief 初始化系统默认的 idt
 * 
 */
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