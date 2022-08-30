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

restore_all:
	popl %ebx;
	popl %ecx;
	popl %edx;
	popl %esi;
	popl %edi;
	popl %ebp;
	popl %eax;
1:	popl %ds;
2:	popl %es;
	addl $4,%esp;
3:	iret;


	ALIGN
ret_from_exception:
ENTRY(ret_from_intr)
	GET_CURRENT(%ebx)
ALIGN
	movl EFLAGS(%esp),%eax		# mix EFLAGS and CS
	movb CS(%esp),%al
	jmp restore_all


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
	RESTORE_ALL

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

# 暂时仅输出
ENTRY(system_call)
	pushl $0
	pushl $ SYMBOL_NAME(do_system_call)
	jmp error_code