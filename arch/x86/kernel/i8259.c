#include <asm/io.h>
#include <asm/delay.h>
#include <asm/desc.h>
#include <vkernel/irq.h>
#include <vkernel/init.h>
#include <vkernel/kernel.h>
#include <vkernel/spinlock.h>

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


// idt 回调函数列表
void (*interrupt[NR_IRQS])(void) = {
	IRQLIST_16(0x0),
};

#define __byte(x,y) 	(((unsigned char *)&(y))[x])
#define cached_21	(__byte(0,cached_irq_mask))
#define cached_A1	(__byte(1,cached_irq_mask))
// i8259A 芯片操作锁,避免并行设置
spinlock_t i8259A_lock = SPIN_LOCK_UNLOCKED;
// 中断掩码缓存
static unsigned int cached_irq_mask = 0xffff;

/**
 * @brief 核心函数 : 获取isr寄存器，判断是否是真实的中断
 * 
 * @param irq 中断号
 * @return int 0 表示不是,非 0 表示是。
 */
static inline int i8259A_irq_real(unsigned int irq)
{
    int value;
    int irqmask = 1<<irq;

    if (irq < 8) {
        outb(0x0B,0x20);		/* ISR register */
        value = inb(0x20) & irqmask;
        outb(0x0A,0x20);		/* back to the IRR register */
        return value;
    }
    outb(0x0B,0xA0);		/* ISR register */
    value = inb(0xA0) & (irqmask >> 8);
    outb(0x0A,0xA0);		/* back to the IRR register */
    return value;
}

/**
 * @brief 核心函数 : 禁用中断
 * 
 * @param irq 中断号
 */
void disable_8259A_irq(unsigned int irq)
{
	unsigned int mask = 1 << irq;
	unsigned long flags;

	spin_lock_irqsave(&i8259A_lock, flags);
	cached_irq_mask |= mask;
	if (irq & 8)
		outb(cached_A1,0xA1);
	else
		outb(cached_21,0x21);
	spin_unlock_irqrestore(&i8259A_lock, flags);
}

/**
 * @brief 核心函数 : 启用中断
 * 
 * @param irq 中断号
 */
void enable_8259A_irq(unsigned int irq)
{
	unsigned int mask = ~(1 << irq);
	unsigned long flags;

	spin_lock_irqsave(&i8259A_lock, flags);
	cached_irq_mask &= mask;
	if (irq & 8)
		outb(cached_A1,0xA1);
	else
		outb(cached_21,0x21);
	spin_unlock_irqrestore(&i8259A_lock, flags);
}

/**
 * @brief 中断结束操作
 * 
 * @param irq 中断号
 */
static void end_8259A_irq (unsigned int irq)
{
	if (!(irq_desc[irq].status & (IRQ_DISABLED|IRQ_INPROGRESS)))
		enable_8259A_irq(irq);
}

#define shutdown_8259A_irq	disable_8259A_irq

/**
 * @brief 中断安装操作
 * 
 * @param irq 中断号
 * @return 是否处于 pending
 */
static unsigned int startup_8259A_irq(unsigned int irq)
{ 
	enable_8259A_irq(irq);
	return 0; /* never anything pending */
}

/**
 * @brief 中断回复函数
 * 
 * @param irq 中断号
 */
void mask_and_ack_8259A(unsigned int irq)
{
    unsigned int irqmask = 1 << irq;
    unsigned long flags;

    spin_lock_irqsave(&i8259A_lock, flags);
	// 如若中断已禁用
    if (cached_irq_mask & irqmask)
        goto spurious_8259A_irq;
    cached_irq_mask |= irqmask;

handle_real_irq:
    if (irq & 8) {
        inb(0xA1);		/* DUMMY - (do we need this?) */
        outb(cached_A1,0xA1);
        outb(0x60+(irq&7),0xA0);/* 'Specific EOI' to slave */
        outb(0x62,0x20);	/* 'Specific EOI' to master-IRQ2 */
    } else {
        inb(0x21);		/* DUMMY - (do we need this?) */
        outb(cached_21,0x21);
        outb(0x60+irq,0x20);	/* 'Specific EOI' to master */
    }
    spin_unlock_irqrestore(&i8259A_lock, flags);
    return;
spurious_8259A_irq:
    /*
     * this is the slow path - should happen rarely.
     */
    if (i8259A_irq_real(irq))
        /*
         * oops, the IRQ _is_ in service according to the
         * 8259A - not spurious, go handle it.
         */
        goto handle_real_irq;

    {
        static int spurious_irq_mask;
        /*
         * At this point we can be sure the IRQ is spurious,
         * lets ACK and report it. [once per IRQ]
         */
        if (!(spurious_irq_mask & irqmask)) {
            printk("spurious 8259A interrupt: IRQ%d.\n", irq);
            spurious_irq_mask |= irqmask;
        }
        irq_err_count++;
        /*
         * Theoretically we do not have to handle this IRQ,
         * but in Linux this does not cause problems and is
         * simpler for us.
         */
        goto handle_real_irq;
    }
}

// i8259 硬中断处理方法
static struct hw_interrupt_type i8259A_irq_type = {
	"XT-PIC",
	startup_8259A_irq,
	shutdown_8259A_irq,
	enable_8259A_irq,
	disable_8259A_irq,
	mask_and_ack_8259A,
	end_8259A_irq,
	NULL
};

/**
 * @brief 初始化 8259A 可编程中断芯片
 * 
 * @param auto_eoi 
 */
void __init init_8259A(int auto_eoi)
{
	unsigned long flags;

	spin_lock_irqsave(&i8259A_lock, flags);

	outb(0xff, 0x21);	/* mask all of 8259A-1 */
	outb(0xff, 0xA1);	/* mask all of 8259A-2 */

	/*
	 * outb_p - this has to work on a wide range of PC hardware.
	 */
	outb_p(0x11, 0x20);	/* ICW1: select 8259A-1 init */
	outb_p(0x20 + 0, 0x21);	/* ICW2: 8259A-1 IR0-7 mapped to 0x20-0x27 */
	outb_p(0x04, 0x21);	/* 8259A-1 (the master) has a slave on IR2 */
	if (auto_eoi)
		outb_p(0x03, 0x21);	/* master does Auto EOI */
	else
		outb_p(0x01, 0x21);	/* master expects normal EOI */

	outb_p(0x11, 0xA0);	/* ICW1: select 8259A-2 init */
	outb_p(0x20 + 8, 0xA1);	/* ICW2: 8259A-2 IR0-7 mapped to 0x28-0x2f */
	outb_p(0x02, 0xA1);	/* 8259A-2 is a slave on master's IR2 */
	outb_p(0x01, 0xA1);	/* (slave's support for AEOI in flat mode
				    is to be investigated) */

	if (auto_eoi)
		/*
		 * in AEOI mode we just have to mask the interrupt
		 * when acking.
		 */
		i8259A_irq_type.ack = disable_8259A_irq;
	else
		i8259A_irq_type.ack = mask_and_ack_8259A;

	udelay(100);		/* wait for 8259A to initialize */

	outb(cached_21, 0x21);	/* restore master IRQ mask */
	outb(cached_A1, 0xA1);	/* restore slave IRQ mask */

	spin_unlock_irqrestore(&i8259A_lock, flags);
}

/**
 * @brief 初始化 8259A 可编程中断芯片
 * 
 */
void __init init_ISA_irqs (void)
{
	int i;

	init_8259A(0);

	for (i = 0; i < NR_IRQS; i++) {
		irq_desc[i].status = IRQ_DISABLED;
		irq_desc[i].action = 0;
		irq_desc[i].depth = 1;

		if (i < 16) {
			/*
			 * 16 old-style INTA-cycle interrupts:
			 */
			irq_desc[i].handler = &i8259A_irq_type;
		} else {
			/*
			 * 'high' PCI IRQs filled in on demand
			 */
			irq_desc[i].handler = &no_irq_type;
		}
	}
}

/**
 * @brief 初始化 外设 等自定义 IDT
 * 
 */
void __init init_IRQ(void)
{
	int i;
	
    printk("init_IRQ...\n");
	init_ISA_irqs();
	for (i = 0; i < NR_IRQS; i++) {
		// 从 idt 0x20 开始初始化起
		int vector = FIRST_EXTERNAL_VECTOR + i;
		// 除去系统调用 0x80
		if (vector != SYSCALL_VECTOR) 
			set_intr_gate(vector, interrupt[i]);
	}

    return ;
}