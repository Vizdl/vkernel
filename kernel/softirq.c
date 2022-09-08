#include <asm/hardirq.h>
#include <vkernel/irq_cpustat.h>
#include <asm/system.h>
#include <vkernel/linkage.h>
#include <vkernel/spinlock.h>
#include <vkernel/threads.h>
#include <vkernel/interrupt.h>
#include <asm/types.h>
#include <asm/softirq.h>
#include <vkernel/kernel.h>
#include <vkernel/tqueue.h>
#include <vkernel/stddef.h>
/* Old style BHs */

static void (*bh_base[32])(void);
struct tasklet_struct bh_task_vec[32];
irq_cpustat_t irq_stat[NR_CPUS];
static spinlock_t softirq_mask_lock;
static struct softirq_action softirq_vec[32] __cacheline_aligned;

void init_bh(int nr, void (*routine)(void))
{
    bh_base[nr] = routine;
}

asmlinkage void do_softirq(void)
{
    int cpu = 0;
    uint32_t active, mask;

    if (in_interrupt())
        return;

    local_bh_disable();

    local_irq_disable();
    mask = softirq_mask();
    active = softirq_active() & mask;

    if (active) {
        struct softirq_action *h;

restart:
        /* Reset active bitmask before enabling irqs */
        softirq_active() &= ~active;

        local_irq_enable();

        h = softirq_vec;
        mask &= ~active;

        do {
            if (active & 1)
                h->action(h);
            h++;
            active >>= 1;
        } while (active);

        local_irq_disable();

        active = softirq_active();
        if ((active &= mask) != 0)
            goto retry;
    }

    local_bh_enable();

    /* Leave with locally disabled hard irqs. It is critical to close
     * window for infinite recursion, while we help local bh count,
     * it protected us. Now we are defenceless.
     */
    return;
retry:
    goto restart;
}

void tasklet_init(struct tasklet_struct *t,
                  void (*func)(unsigned long), unsigned long data)
{
    t->func = func;
    t->data = data;
    t->state = 0;
    atomic_set(&t->count, 0);
}

spinlock_t global_bh_lock = SPIN_LOCK_UNLOCKED;

static void bh_action(unsigned long nr)
{
    int cpu = 0;
    int num;
    if (!spin_trylock(&global_bh_lock)) {
        goto resched;
    }


    if (!hardirq_trylock())
        goto resched_unlock;

    if (bh_base[nr])
        bh_base[nr]();

    hardirq_endlock(cpu);
    global_bh_lock.lock = 1;
    return;

resched_unlock:
    spin_unlock(&global_bh_lock);
resched:
    mark_bh(nr);
}

void open_softirq(int nr, void (*action)(struct softirq_action*), void *data)
{
    unsigned long flags;
    int i;

    spin_lock_irqsave(&softirq_mask_lock, flags);
    softirq_vec[nr].data = data;
    softirq_vec[nr].action = action;

    for (i=0; i<NR_CPUS; i++)
        softirq_mask() |= (1<<nr);
    spin_unlock_irqrestore(&softirq_mask_lock, flags);
}
/* Tasklets */

struct tasklet_head tasklet_vec[NR_CPUS] __cacheline_aligned;

static void tasklet_action(struct softirq_action *a)
{
    int cpu = 0;
    struct tasklet_struct *list;

    local_irq_disable();
    list = tasklet_vec[cpu].list;
    tasklet_vec[cpu].list = NULL;
    local_irq_enable();

    while (list != NULL) {
        struct tasklet_struct *t = list;

        list = list->next;
        if (atomic_read(&t->count) == 0) {
            clear_bit(TASKLET_STATE_SCHED, &t->state);
            t->func(t->data);
            continue;
        }
        local_irq_disable();
        t->next = tasklet_vec[cpu].list;
        tasklet_vec[cpu].list = t;
        __cpu_raise_softirq(cpu, TASKLET_SOFTIRQ);
        local_irq_enable();
    }
}
struct tasklet_head tasklet_hi_vec[NR_CPUS] __cacheline_aligned;
static void tasklet_hi_action(struct softirq_action *a)
{
    int cpu = 0;
    struct tasklet_struct *list;

    local_irq_disable();
    list = tasklet_hi_vec[cpu].list;
    tasklet_hi_vec[cpu].list = NULL;
    local_irq_enable();

    while (list != NULL) {
        struct tasklet_struct *t = list;

        list = list->next;
        if (atomic_read(&t->count) == 0) {
            clear_bit(TASKLET_STATE_SCHED, &t->state);
            t->func(t->data);
            continue;
        }
        local_irq_disable();
        t->next = tasklet_hi_vec[cpu].list;
        tasklet_hi_vec[cpu].list = t;
        __cpu_raise_softirq(cpu, HI_SOFTIRQ);
        local_irq_enable();
    }
}
void softirq_init()
{
    int i;

    for (i=0; i<32; i++)
        tasklet_init(bh_task_vec+i, bh_action, i);

    open_softirq(TASKLET_SOFTIRQ, tasklet_action, NULL);
    open_softirq(HI_SOFTIRQ, tasklet_hi_action, NULL);
}

void __run_task_queue(task_queue *list)
{
    struct list_head head, *next;
    unsigned long flags;

    spin_lock_irqsave(&tqueue_lock, flags);
    list_add(&head, list);
    list_del_init(list);
    spin_unlock_irqrestore(&tqueue_lock, flags);

    next = head.next;
    while (next != &head) {
        void (*f) (void *);
        struct tq_struct *p;
        void *data;

        p = list_entry(next, struct tq_struct, list);
        next = next->next;
        f = p->routine;
        data = p->data;
        p->sync = 0;
        if (f)
            f(data);
    }
}
