#ifndef _VKERNEL_TIMER_H
#define _VKERNEL_TIMER_H

#include <vkernel/list.h>
#include <vkernel/stddef.h>

struct timer_list {
	struct list_head list;
	unsigned long expires;  // 期待定时器发生的时间
	unsigned long data;     // 定时器回调上下文
	void (*function)(unsigned long); // 定时器回调函数
};

/**
 * @brief 初始化定时器
 * 
 * @param timer 待初始化的定时器
 */
static inline void init_timer(struct timer_list * timer)
{
	timer->list.next = timer->list.prev = NULL;
}

/**
 * @brief 判断定时器是否已经被添加
 * 
 * @param timer 需要判断的定时器
 * @return int 判断结果
 */
static inline int timer_pending (const struct timer_list * timer)
{
	return timer->list.next != NULL;
}

/**
 * @brief 添加定时器
 * 
 * @param timer 待添加的定时器 
 */
extern void add_timer(struct timer_list * timer);

/**
 * @brief 删除定时器
 * 
 * @param timer 待删除的定时器
 * @return int 0 表示失败,非 0 表示成功
 */
extern int del_timer(struct timer_list * timer);

/**
 * @brief 修改定时器的 expires 值
 * 
 * @param timer 待修改的定时器
 * @param expires 想要填入的 expires
 * @return int 0 表示失败,非 0 表示成功
 */
int mod_timer(struct timer_list *timer, unsigned long expires);

#endif