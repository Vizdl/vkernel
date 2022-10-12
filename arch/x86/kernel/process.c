
#include <asm/system.h>
#include <asm/processor.h>
#include <vkernel/smp.h>
#include <vkernel/sched.h>
#include <vkernel/string.h>

void __switch_to(struct task_struct *prev_p, struct task_struct *next_p)
{
	struct thread_struct *prev = &prev_p->thread,
				 *next = &next_p->thread;
	struct tss_struct *tss = init_tss + smp_processor_id();
    // 修改 tss 内容,这样如若从内核态回去才能回到正确的进程。
	tss->esp0 = next->esp0;
}