#include <asm/processor.h>
#include <vkernel/sched.h>

union task_union init_task_union
        __attribute__((__section__(".data.init_task"))) =
        { INIT_TASK(init_task_union.task) };

struct tss_struct init_tss[NR_CPUS] = { [0 ... NR_CPUS-1] = INIT_TSS };