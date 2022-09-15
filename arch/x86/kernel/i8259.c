#include <vkernel/init.h>
#include <vkernel/kernel.h>

/**
 * @brief 初始化 外设 等自定义 IDT
 * 
 */
void __init init_IRQ(void)
{
    printk("init_IRQ...\n");
    return ;
}