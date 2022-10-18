#include <vkernel/sched.h>

static LIST_HEAD(runqueue_head);	// 就绪态进程运行队列

/**
 * @brief 将进程添加到 runqueue_head
 * 
 * @param p 待添加 runqueue_head 的进程
 */
void add_to_runqueue(struct task_struct * p)
{
	list_add(&p->run_list, &runqueue_head);
	nr_running++;
}

/**
 * @brief 将 task p 移动到 runqueue 的结尾
 * 
 * @param p 待移动的进程
 */
void move_last_runqueue(struct task_struct * p)
{
	list_del(&p->run_list);
	list_add_tail(&p->run_list, &runqueue_head);
}

/**
 * @brief 将 task p 移动到 runqueue 的开头
 * 
 * @param p 待移动的进程
 */
void move_first_runqueue(struct task_struct * p)
{
	list_del(&p->run_list);
	list_add(&p->run_list, &runqueue_head);
}
