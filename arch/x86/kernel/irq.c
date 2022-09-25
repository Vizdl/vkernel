#include <asm/errno.h>
#include <asm/ptrace.h>
#include <asm/signal.h>
#include <vkernel/irq.h>
#include <vkernel/smp.h>
#include <vkernel/slab.h>
#include <vkernel/kernel.h>
#include <vkernel/stddef.h>
#include <vkernel/linkage.h>
#include <vkernel/interrupt.h>

// 默认中断操作函数
static void enable_none(unsigned int irq) { }
static unsigned int startup_none(unsigned int irq) { return 0; }
static void disable_none(unsigned int irq) { }
#define shutdown_none	disable_none
#define end_none	enable_none
static void ack_none(unsigned int irq){ }

struct hw_interrupt_type no_irq_type = {
	"none",
	startup_none,
	shutdown_none,
	enable_none,
	disable_none,
	ack_none,
	end_none
};

irq_desc_t irq_desc[NR_IRQS] __cacheline_aligned =
	{ [0 ... NR_IRQS-1] = { 0, &no_irq_type, NULL, 0, SPIN_LOCK_UNLOCKED}};

/**
 * @brief 向 irq 添加一个 irqaction
 * 
 * @param irq 中断向量号
 * @param new 中断处理 action
 * @return int 状态码
 */
int setup_irq(unsigned int irq, struct irqaction * new)
{
	int shared = 0;
	unsigned long flags;
	struct irqaction *old, **p;
	// 找到 irq 对应的 irq_desc
	irq_desc_t *desc = irq_desc + irq;

	/*
	 * 以下代码块必须以原子方式执行
	 */
	spin_lock_irqsave(&desc->lock,flags);
	p = &desc->action;
	if ((old = *p) != NULL) {
		/* Can't share interrupts unless both agree to */
		if (!(old->flags & new->flags & SA_SHIRQ)) {
			spin_unlock_irqrestore(&desc->lock,flags);
			return -EBUSY;
		}

		/* add new interrupt at end of irq queue */
		do {
			p = &old->next;
			old = *p;
		} while (old);
		shared = 1;
	}

	/* 添加 new irqaction 到队列尾部(p指向最后一个 irqaction) */
	*p = new;

	if (!shared) {
		desc->depth = 0;
		desc->status &= ~(IRQ_DISABLED | IRQ_AUTODETECT | IRQ_WAITING);
		desc->handler->startup(irq);
	}
	spin_unlock_irqrestore(&desc->lock,flags);

	return 0;
}

/**
 * @brief 分配中断线
 * 
 * @param irq 要分配的中断线
 * @param handler IRQ发生时要调用的函数
 * @param irqflags 中断类型标志
 * @param devname 声明设备的ascii名称
 * @param dev_id 传递回处理函数的cookie
 * @return int 
 */
int request_irq(unsigned int irq, 
		void (*handler)(int, void *, struct pt_regs *),
		unsigned long irqflags, 
		const char * devname,
		void *dev_id)
{
	int retval;
	struct irqaction * action;

#if 1
	/*
	 * Sanity-check: shared interrupts should REALLY pass in
	 * a real dev-ID, otherwise we'll have trouble later trying
	 * to figure out which interrupt is which (messes up the
	 * interrupt freeing logic etc).
	 */
	if (irqflags & SA_SHIRQ) {
		if (!dev_id)
			printk("Bad boy: %s (at 0x%x) called us without a dev_id!\n", devname, (&irq)[-1]);
	}
#endif

	if (irq >= NR_IRQS)
		return -EINVAL;
	if (!handler)
		return -EINVAL;

	action = (struct irqaction *)
			kmalloc(sizeof(struct irqaction), GFP_KERNEL);
	if (!action)
		return -ENOMEM;

	action->handler = handler;
	action->flags = irqflags;
	action->mask = 0;
	action->name = devname;
	action->next = NULL;
	action->dev_id = dev_id;

	retval = setup_irq(irq, action);
	if (retval)
		kfree(action);
	return retval;
}

/**
 * @brief 释放中断
 * 
 * @param irq 需要释放的中断向量号
 * @param dev_id 需要释放的设备 id
 */
void free_irq(unsigned int irq, void *dev_id)
{
	irq_desc_t *desc;
	struct irqaction **p;
	unsigned long flags;

	if (irq >= NR_IRQS)
		return;

	desc = irq_desc + irq;
	spin_lock_irqsave(&desc->lock,flags);
	p = &desc->action;
	for (;;) {
		struct irqaction * action = *p;
		if (action) {
			struct irqaction **pp = p;
			p = &action->next;
			if (action->dev_id != dev_id)
				continue;

			/* Found it - now remove it from the list of entries */
			*pp = action->next;
			if (!desc->action) {
				desc->status |= IRQ_DISABLED;
				desc->handler->shutdown(irq);
			}
			spin_unlock_irqrestore(&desc->lock,flags);

			kfree(action);
			return;
		}
		printk("Trying to free free IRQ%d\n",irq);
		spin_unlock_irqrestore(&desc->lock,flags);
		return;
	}
}

/**
 * @brief 对外封装利用 irq_desc_t 中注册的 disable 函数实现
 * 
 * @param irq 中断号
 */
void inline disable_irq_nosync(unsigned int irq)
{
	irq_desc_t *desc = irq_desc + irq;
	unsigned long flags;

	spin_lock_irqsave(&desc->lock, flags);
	if (!desc->depth++) {
		desc->status |= IRQ_DISABLED;
		desc->handler->disable(irq);
	}
	spin_unlock_irqrestore(&desc->lock, flags);
}

/**
 * @brief 对外封装利用 irq_desc_t 中注册的 disable 函数实现
 * 
 * @param irq 中断号
 */
void disable_irq(unsigned int irq)
{
	disable_irq_nosync(irq);

	if (!local_irq_count(smp_processor_id())) {
		do {
			barrier();
		} while (irq_desc[irq].status & IRQ_INPROGRESS);
	}
}

/**
 * @brief 对外封装利用 irq_desc_t 中注册的 enable 函数实现
 * 
 * @param irq 中断号
 */
void enable_irq(unsigned int irq)
{
	irq_desc_t *desc = irq_desc + irq;
	unsigned long flags;

	spin_lock_irqsave(&desc->lock, flags);
	switch (desc->depth) {
	case 1: {
		unsigned int status = desc->status & ~IRQ_DISABLED;
		desc->status = status;
		if ((status & (IRQ_PENDING | IRQ_REPLAY)) == IRQ_PENDING) {
			desc->status = status | IRQ_REPLAY;
			hw_resend_irq(desc->handler,irq);
		}
		desc->handler->enable(irq);
		/* fall-through */
	}
	default:
		desc->depth--;
		break;
	case 0:
		printk("enable_irq(%u) unbalanced from %p\n", irq,
		       __builtin_return_address(0));
	}
	spin_unlock_irqrestore(&desc->lock, flags);
}

int handle_IRQ_event(unsigned int irq, struct pt_regs * regs, struct irqaction * action)
{
	int status;
	int cpu = smp_processor_id();

	irq_enter(cpu, irq);

	status = 1;	/* Force the "do bottom halves" bit */

	if (!(action->flags & SA_INTERRUPT))
		__sti();
	// 遍历 action 链表,执行所有处理函数。
	do {
		status |= action->flags;
		action->handler(irq, action->dev_id, regs);
		action = action->next;
	} while (action);
	__cli();

	irq_exit(cpu, irq);

	return status;
}


/**
 * @brief 总入口,中断分发函数。
 * 
 * @param regs 寄存器组
 */
asmlinkage unsigned int do_IRQ(struct pt_regs regs)
{	int irq = regs.orig_eax & 0xff; /* high bits used in ret_from_ code  */
	irq_desc_t *desc = irq_desc + irq;	// 确定描述符
	struct irqaction * action;
	unsigned int status;

	spin_lock(&desc->lock);
	desc->handler->ack(irq);
	/*
	   REPLAY is when Linux resends an IRQ that was dropped earlier
	   WAITING is used by probe to mark irqs that are being tested
	   */
	status = desc->status & ~(IRQ_REPLAY | IRQ_WAITING);
	status |= IRQ_PENDING; /* we _want_ to handle it */

	/*
	 * If the IRQ is disabled for whatever reason, we cannot
	 * use the action we have.
	 */
	action = NULL;
	if (!(status & (IRQ_DISABLED | IRQ_INPROGRESS))) {
		action = desc->action;
		status &= ~IRQ_PENDING; /* we commit to handling */
		status |= IRQ_INPROGRESS; /* we are handling it */
	}
	desc->status = status;

	/*
	 * If there is no IRQ handler or it was disabled, exit early.
	   Since we set PENDING, if another processor is handling
	   a different instance of this same irq, the other processor
	   will take care of it.
	 */
	if (!action)
		goto out;

	/*
	 * Edge triggered interrupts need to remember
	 * pending events.
	 * This applies to any hw interrupts that allow a second
	 * instance of the same irq to arrive while we are in do_IRQ
	 * or in the handler. But the code here only handles the _second_
	 * instance of the irq, not the third or fourth. So it is mostly
	 * useful for irq hardware that does not mask cleanly in an
	 * SMP environment.
	 */
	for (;;) {
		spin_unlock(&desc->lock);
		handle_IRQ_event(irq, &regs, action);
		spin_lock(&desc->lock);
		
		if (!(desc->status & IRQ_PENDING))
			break;
		desc->status &= ~IRQ_PENDING;
	}
	desc->status &= ~IRQ_INPROGRESS;
out:
	/*
	 * The ->end() handler has to deal with interrupts which got
	 * disabled while the handler was running.
	 */
	desc->handler->end(irq);
    return 0;
}