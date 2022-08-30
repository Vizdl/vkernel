
#ifndef _VKERNEL_IDT_H
#define _VKERNEL_IDT_H

#define IDT_TABLE_SIZE 256

#ifndef __ASSEMBLY__

struct idt_desc {
	unsigned long a,b;
};

/**
 * @brief 设置门描述符
 * 
 * @param gate_addr 门描述符地址
 * @param type 门类型
 * @param dpl 门 dpl
 * @param addr 门处理函数地址
 */
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

/**
 * @brief 设置陷阱门描述符(DPL=0)
 * 
 * @param n idt表下标
 * @param addr 回调函数指针
 */
extern void set_trap_gate(unsigned int n, void *addr);

/**
 * @brief 设置陷阱门描述符(DPL=3)
 * 
 * @param n 
 * @param addr 
 */
extern void set_system_gate(unsigned int n, void *addr);

/**
 * @brief 设置中断门描述符(外设中断)
 * 
 * @param n idt表下标
 * @param addr 回调函数指针
 */
extern void set_intr_gate(unsigned int n, void *addr);

/**
 * @brief 设置调用门
 * 
 * @param a 
 * @param addr 
 */
extern void set_call_gate(void *a, void *addr);

#endif /* !__ASSEMBLY__ */
#endif