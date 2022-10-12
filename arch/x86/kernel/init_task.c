#include <asm/processor.h>
#include <vkernel/cache.h>
#include <vkernel/sched.h>
#include <vkernel/threads.h>

struct mm_struct init_mm = INIT_MM(init_mm);

// init_task 的 栈 与 task_struct 的结合体
union task_union init_task_union 
	__attribute__((__section__(".data.init_task"))) =
		{ INIT_TASK(init_task_union.task) };

// per cpu 的任务上下文
struct tss_struct init_tss[NR_CPUS] __cacheline_aligned = { [0 ... NR_CPUS-1] = INIT_TSS };