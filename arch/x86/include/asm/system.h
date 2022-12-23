#ifndef __ASM_SYSTEM_H
#define __ASM_SYSTEM_H

#include <vkernel/config.h>
#include <vkernel/kernel.h>

#ifdef __KERNEL__

struct task_struct;	/* one of the stranger aspects of C forward declarations.. */
extern void FASTCALL(__switch_to(struct task_struct *prev, struct task_struct *next));

// 
// prev next 是怎么压入 __switch_to 函数栈内的?
// 通过 regparm(FASTCALL) 调用方式 将 eax edx ebx 分别设为 prev next prev 传参进去的
//
#define switch_to(prev,next,last) do {	\
	asm volatile("pushl %%esi\n\t"		\
		     "pushl %%edi\n\t"			\
		     "pushl %%ebp\n\t"			\
		     "movl %%esp,%0\n\t"	/* prev->thread.esp = esp */		\
		     "movl %3,%%esp\n\t"	/* esp = next->thread.esp */		\
		     "movl $1f,%1\n\t"		/* prev->thread.eip = label 1(1:) */		\
			 /* 将 next->thread.eip 压入 next 内核栈内,模拟之前调用过 */	\
			 /* 如若第一次被调度,则会跳到 ret_from_fork, 否则跳转到 label 1(1:) */	\
		     "pushl %4\n\t"										\
		     "jmp __switch_to\n"						\
		     "1:\t"										\
		     "popl %%ebp\n\t"							\
		     "popl %%edi\n\t"							\
		     "popl %%esi\n\t"							\
			 /* 输出参数,这三个参数会被修改 */ \
		     :"=m" (prev->thread.esp),"=m" (prev->thread.eip),	\
		      "=b" (last)										\
			 /* 输入参数, 将 eax edx ebx 分别设为 prev next prev */ \
		     :"m" (next->thread.esp),"m" (next->thread.eip),	\
		      "a" (prev), "d" (next),							\
		      "b" (prev));										\
} while (0)

/**
 * @brief 加载物理地址 pgdir 进入 cr3 寄存器
 * @param pgdir 物理地址
 * @return 
 */
#define load_cr3(pgdir) asm volatile("movl %0,%%cr3": :"r" (pgdir))

#define load_gdtr( gdtr ) asm volatile ("lgdt %0" : : "m" (gdtr))
#define load_idtr( idtr ) asm volatile ("lidt %0" : : "m" (idtr))

// 通过跳转刷新 cs 寄存器
#define flush_cs( cs ) asm volatile ( "ljmp %0, $fake_label%1\n\t fake_label%1: \n\t" :: "i"(cs), "i"(__LINE__))
#define flush_ds( ds ) asm volatile ("mov %0, %%ax;mov %%ax, %%ds" : : "i" (ds))
#define flush_ss( ss ) asm volatile ("mov %0, %%ax;mov %%ax, %%ss" : : "i" (ss))
#define flush_gs( gs ) asm volatile ("mov %0, %%ax;mov %%ax, %%gs" : : "i" (gs))

#define read_cr0() ({ \
	unsigned int __dummy; \
	__asm__( \
		"movl %%cr0,%0\n\t" \
		:"=r" (__dummy)); \
	__dummy; \
})

#define write_cr0(x) \
	__asm__("movl %0,%%cr0": :"r" (x));

#define paging() write_cr0(0x80000000 | read_cr0())

#endif	/* __KERNEL__ */

#define nop() __asm__ __volatile__ ("nop")

#define xchg(ptr,v) ((__typeof__(*(ptr)))__xchg((unsigned long)(v),(ptr),sizeof(*(ptr))))

struct __xchg_dummy { unsigned long a[100]; };
#define __xg(x) ((struct __xchg_dummy *)(x))
/*
 * Note: no "lock" prefix even on SMP: xchg always implies lock anyway
 * Note 2: xchg has side effect, so that attribute volatile is necessary,
 *	  but generally the primitive is invalid, *ptr is output argument. --ANK
 */
static inline unsigned long __xchg(unsigned long x, volatile void * ptr, int size)
{
	switch (size) {
		case 1:
			__asm__ __volatile__("xchgb %b0,%1"
				:"=q" (x)
				:"m" (*__xg(ptr)), "0" (x)
				:"memory");
			break;
		case 2:
			__asm__ __volatile__("xchgw %w0,%1"
				:"=r" (x)
				:"m" (*__xg(ptr)), "0" (x)
				:"memory");
			break;
		case 4:
			__asm__ __volatile__("xchgl %0,%1"
				:"=r" (x)
				:"m" (*__xg(ptr)), "0" (x)
				:"memory");
			break;
	}
	return x;
}

/* interrupt control.. */
#define __save_flags(x)		__asm__ __volatile__("pushfl ; popl %0":"=g" (x): /* no input */)
#define __restore_flags(x) 	__asm__ __volatile__("pushl %0 ; popfl": /* no output */ :"g" (x):"memory")
#define __cli() 		__asm__ __volatile__("cli": : :"memory")
#define __sti()			__asm__ __volatile__("sti": : :"memory")
/* used in the idle loop; sti takes one instruction cycle to complete */
#define safe_halt()		__asm__ __volatile__("sti; hlt": : :"memory")

/* For spinlocks etc */
#define local_irq_save(x)	__asm__ __volatile__("pushfl ; popl %0 ; cli":"=g" (x): /* no input */ :"memory")
#define local_irq_restore(x)	__restore_flags(x)
#define local_irq_disable()	__cli()
#define local_irq_enable()	__sti()

#endif
