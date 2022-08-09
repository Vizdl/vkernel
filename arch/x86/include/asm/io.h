#ifndef _ASM_IO_H
#define _ASM_IO_H

#ifdef __KERNEL__


/*
 * Change virtual addresses to physical addresses and vv.
 * These are pretty trivial
 */
extern inline unsigned long virt_to_phys(volatile void * address)
{
	return __pa(address);
}

extern inline void * phys_to_virt(unsigned long address)
{
	return __va(address);
}

#endif /* __KERNEL__ */

#endif
