#ifndef _I386_CURRENT_H
#define _I386_CURRENT_H

struct task_struct;

/**
 * @brief 获取当前 cpu 执行的 task
 * 
 * @return struct task_struct* 当前 cpu 执行的 task
 */
static inline struct task_struct * get_current(void)
{
	struct task_struct *current;
	// .data.init_task 是 8192 对齐的, 且 task_union 的 stack 最大为 8192
	// 故而 esp & (~8191UL) 等于 task_struct 的地址
	__asm__("andl %%esp,%0; ":"=r" (current) : "0" (~8191UL));
	return current;
}
 
#define current get_current()

#endif /* !(_I386_CURRENT_H) */
