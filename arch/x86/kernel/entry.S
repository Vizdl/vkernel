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
 * these are offsets into the task-struct.
 */
state		=  0
flags		=  4
sigpending	=  8
addr_limit	= 12
exec_domain	= 16
need_resched	= 20
tsk_ptrace	= 24
processor	= 52

ENOSYS = 38

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

ENTRY(ret_from_intr)
	GET_CURRENT(%ebx)
ALIGN
	movl EFLAGS(%esp),%eax		# mix EFLAGS and CS
	movb CS(%esp),%al
	jmp restore_all