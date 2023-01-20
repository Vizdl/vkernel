#include <asm/segment.h>
#include <vkernel/linkage.h>

EBX		= 0x00
ECX		= 0x04
EDX		= 0x08
ESI		= 0x0C
EDI		= 0x10
EBP		= 0x14
EAX		= 0x18
DS		= 0x1C
ES		= 0x20
ORIG_EAX	= 0x24
EIP		= 0x28
CS		= 0x2C
EFLAGS		= 0x30
OLDESP		= 0x34
OLDSS		= 0x38

CF_MASK		= 0x00000001
TF_MASK		= 0x00000100
IF_MASK		= 0x00000200
DF_MASK		= 0x00000400
NT_MASK		= 0x00004000
VM_MASK		= 0x00020000

/*
 * 属性在 task_struct 的偏移量
 */
state		=  0
flags		=  4
sigpending	=  8
addr_limit	= 12
exec_domain	= 16
need_resched = 20
tsk_ptrace	= 24
processor	= 52

#define SAVE_ALL \
	cld; \
	pushl %es; \
	pushl %ds; \
	pushl %eax; \
	pushl %ebp; \
	pushl %edi; \
	pushl %esi; \
	pushl %edx; \
	pushl %ecx; \
	pushl %ebx; \
	movl $(__KERNEL_DS),%edx; \
	movl %edx,%ds; \
	movl %edx,%es;

#define RESTORE_ALL	\
	popl %ebx;	\
	popl %ecx;	\
	popl %edx;	\
	popl %esi;	\
	popl %edi;	\
	popl %ebp;	\
	popl %eax;	\
1:	popl %ds;	\
2:	popl %es;	\
	addl $4,%esp;	\
3:	iret;		\

#define GET_CURRENT(reg) \
	movl $-8192, reg; \
	andl %esp, reg

.text

	ALIGN
ret_from_exception:
ENTRY(ret_from_intr)
	GET_CURRENT(%ebx)
	movl EFLAGS(%esp),%eax		# mix EFLAGS and CS
	movb CS(%esp),%al
	testl $(VM_MASK | 3),%eax	# return to VM86 mode or non-supervisor?
	# 无论是什么 cpu 运行级别都进行重调度
	jmp ret_with_reschedule
	jmp restore_all

	ALIGN
handle_softirq:
	call SYMBOL_NAME(do_softirq)
	jmp ret_from_intr

	ALIGN
reschedule:
	call SYMBOL_NAME(schedule)    # test
	jmp ret_from_sys_call

ENTRY(divide_error)
	pushl $0		# no error code
	pushl $ SYMBOL_NAME(do_divide_error)
	ALIGN
error_code:
	pushl %ds
	pushl %eax
	xorl %eax,%eax
	pushl %ebp
	pushl %edi
	pushl %esi
	pushl %edx
	decl %eax			# eax = -1
	pushl %ecx
	pushl %ebx
	cld
	movl %es,%ecx
	movl ORIG_EAX(%esp), %esi	# get the error code
	movl ES(%esp), %edi		# get the function address
	movl %eax, ORIG_EAX(%esp)
	movl %ecx, ES(%esp)
	movl %esp,%edx
	pushl %esi			# push the error code
	pushl %edx			# push the pt_regs pointer
	movl $(__KERNEL_DS),%edx
	movl %edx,%ds
	movl %edx,%es
	GET_CURRENT(%ebx)
	call *%edi
	addl $8,%esp
	jmp ret_from_exception


ENTRY(coprocessor_error)
	pushl $0
	pushl $ SYMBOL_NAME(do_coprocessor_error)
	jmp error_code

ENTRY(simd_coprocessor_error)
	pushl $0
	pushl $ SYMBOL_NAME(do_simd_coprocessor_error)
	jmp error_code

# 进行了更改仅输出
ENTRY(device_not_available)
	pushl $0
	pushl $ SYMBOL_NAME(do_device_not_available)
	jmp error_code

ENTRY(debug)
	pushl $0
	pushl $ SYMBOL_NAME(do_debug)
	jmp error_code

ENTRY(nmi)
	pushl %eax
	SAVE_ALL
	movl %esp,%edx
	pushl $0
	pushl %edx
	call SYMBOL_NAME(do_nmi)
	addl $8,%esp
	jmp restore_all

ENTRY(int3)
	pushl $0
	pushl $ SYMBOL_NAME(do_int3)
	jmp error_code

ENTRY(overflow)
	pushl $0
	pushl $ SYMBOL_NAME(do_overflow)
	jmp error_code

ENTRY(bounds)
	pushl $0
	pushl $ SYMBOL_NAME(do_bounds)
	jmp error_code

ENTRY(invalid_op)
	pushl $0
	pushl $ SYMBOL_NAME(do_invalid_op)
	jmp error_code

ENTRY(coprocessor_segment_overrun)
	pushl $0
	pushl $ SYMBOL_NAME(do_coprocessor_segment_overrun)
	jmp error_code

ENTRY(double_fault)
	pushl $ SYMBOL_NAME(do_double_fault)
	jmp error_code

ENTRY(invalid_TSS)
	pushl $ SYMBOL_NAME(do_invalid_TSS)
	jmp error_code

ENTRY(segment_not_present)
	pushl $ SYMBOL_NAME(do_segment_not_present)
	jmp error_code

ENTRY(stack_segment)
	pushl $ SYMBOL_NAME(do_stack_segment)
	jmp error_code

ENTRY(general_protection)
	pushl $ SYMBOL_NAME(do_general_protection)
	jmp error_code

ENTRY(alignment_check)
	pushl $ SYMBOL_NAME(do_alignment_check)
	jmp error_code

ENTRY(page_fault)
	pushl $ SYMBOL_NAME(do_page_fault)
	jmp error_code

ENTRY(machine_check)
	pushl $0
	pushl $ SYMBOL_NAME(do_machine_check)
	jmp error_code

ENTRY(spurious_interrupt_bug)
	pushl $0
	pushl $ SYMBOL_NAME(do_spurious_interrupt_bug)
	jmp error_code

ENTRY(ret_from_fork)
	pushl %ebx
	call SYMBOL_NAME(schedule_tail)
	addl $4, %esp
	jmp	ret_from_sys_call

ENTRY(system_call)
	pushl %eax			# save orig_eax
	SAVE_ALL
	GET_CURRENT(%ebx)
	call *SYMBOL_NAME(sys_call_table)(,%eax,4)
	movl %eax,EAX(%esp)		# save the return value

# 从系统调用返回
ENTRY(ret_from_sys_call)
	# 判断是否有软中断发生
	movl SYMBOL_NAME(irq_stat),%ecx		# softirq_active
	testl SYMBOL_NAME(irq_stat)+4,%ecx	# softirq_mask
	jne   handle_softirq

ret_with_reschedule:
	cmpl $0,need_resched(%ebx)
	jne reschedule

restore_all:
	RESTORE_ALL
	
.data
# 函数调用表.data
ENTRY(sys_call_table)
	.long SYMBOL_NAME(sys_exit)
	.long SYMBOL_NAME(sys_clone)