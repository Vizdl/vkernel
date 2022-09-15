#ifndef _ASM_HW_IRQ_H
#define _ASM_HW_IRQ_H

#include <vkernel/linkage.h>
#include <asm/segment.h>

#define FIRST_EXTERNAL_VECTOR	0x20
#define SYSCALL_VECTOR		    0x80

// IRQ_NAME2(0x1) ==> 0x1_interrupt(void)
#define IRQ_NAME2(nr) nr##_interrupt(void)
// IRQ_NAME(0x1) ==> IRQ_NAME2(IRQ0x1) ==> IRQ0x1_interrupt(void)
#define IRQ_NAME(nr) IRQ_NAME2(IRQ##nr)

#define __STR(x) #x
#define STR(x) __STR(x)

// 保存寄存器信息,切换 __KERNEL_DS 作为选择符
#define SAVE_ALL \
	"cld\n\t" \
	"pushl %es\n\t" \
	"pushl %ds\n\t" \
	"pushl %eax\n\t" \
	"pushl %ebp\n\t" \
	"pushl %edi\n\t" \
	"pushl %esi\n\t" \
	"pushl %edx\n\t" \
	"pushl %ecx\n\t" \
	"pushl %ebx\n\t" \
	"movl $" STR(__KERNEL_DS) ",%edx\n\t" \
	"movl %edx,%ds\n\t" \
	"movl %edx,%es\n\t"

// 中断汇聚到 common_interrupt 的辅助宏
#define BUILD_IRQ(nr) \
	asmlinkage void IRQ_NAME(nr); \
	__asm__( \
	"\n"__ALIGN_STR"\n" \
	SYMBOL_NAME_STR(IRQ) #nr "_interrupt:\n\t" \
	"pushl $"#nr"-256\n\t" \
	"jmp common_interrupt");

// 保存寄存器,保存函数,执行跳转到 do_IRQ 函数
#define BUILD_COMMON_IRQ() \
asmlinkage void call_do_IRQ(void); \
__asm__( \
	"\n" __ALIGN_STR"\n" \
	"common_interrupt:\n\t" \
	SAVE_ALL \
	"pushl $ret_from_intr\n\t" \
	SYMBOL_NAME_STR(call_do_IRQ)":\n\t" \
	"jmp "SYMBOL_NAME_STR(do_IRQ));

#endif /* _ASM_HW_IRQ_H */
