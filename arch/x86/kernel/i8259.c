#include <vkernel/init.h>
#include <vkernel/kernel.h>
#include <asm/irq.h>
#include <asm/desc.h>
#include <asm/hw_irq.h>

// 定义 common_interrupt
BUILD_COMMON_IRQ()


// 定义 IRQ#x#y#_interrupt 格式的单个函数, eg : IRQ0x00_interrupt
#define BI(x,y) \
	BUILD_IRQ(x##y)

// 定义 IRQ#x#y#_interrupt 格式的 16 个函数
#define BUILD_16_IRQS(x) \
	BI(x,0) BI(x,1) BI(x,2) BI(x,3) \
	BI(x,4) BI(x,5) BI(x,6) BI(x,7) \
	BI(x,8) BI(x,9) BI(x,a) BI(x,b) \
	BI(x,c) BI(x,d) BI(x,e) BI(x,f)

// 定义 IRQ0x00_interrupt, IRQ0x01_interrupt, ...
// 目的是为了让所有的中断跳转到 common_interrupt
BUILD_16_IRQS(0x0)

// 声明 IRQ#x#y#_interrupt 格式的单个函数, eg : IRQ0x00_interrupt
#define IRQ(x,y) \
	IRQ##x##y##_interrupt

#define IRQLIST_16(x) \
	IRQ(x,0), IRQ(x,1), IRQ(x,2), IRQ(x,3), \
	IRQ(x,4), IRQ(x,5), IRQ(x,6), IRQ(x,7), \
	IRQ(x,8), IRQ(x,9), IRQ(x,a), IRQ(x,b), \
	IRQ(x,c), IRQ(x,d), IRQ(x,e), IRQ(x,f)


void (*interrupt[NR_IRQS])(void) = {
	IRQLIST_16(0x0),
};

/**
 * @brief 初始化 外设 等自定义 IDT
 * 
 */
void __init init_IRQ(void)
{
	int i;
	
    printk("init_IRQ...\n");

	for (i = 0; i < NR_IRQS; i++) {
		// 从 idt 0x20 开始初始化起
		int vector = FIRST_EXTERNAL_VECTOR + i;
		// 除去系统调用 0x80
		if (vector != SYSCALL_VECTOR) 
			set_intr_gate(vector, interrupt[i]);
	}

    return ;
}