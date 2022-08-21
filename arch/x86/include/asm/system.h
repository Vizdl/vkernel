#ifndef __ASM_SYSTEM_H
#define __ASM_SYSTEM_H

#include <vkernel/config.h>

#ifdef __KERNEL__

/**
 * @brief 加载物理地址 pgdir 进入 cr3 寄存器
 * @param pgdir 物理地址
 * @return 
 */
#define load_cr3(pgdir) asm volatile("movl %0,%%cr3": :"r" (pgdir))

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
