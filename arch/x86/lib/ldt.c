#include <asm/desc.h>

/**
 * @brief 在 GDT 上设置一个 LDT 描述符
 * 
 * @param n GDT index
 * @param addr 线性基地址
 * @param size 段大小
 */
void set_ldt_desc(unsigned int n, void *addr, unsigned int size)
{
	_set_tssldt_desc(gdt_table+__LDT(n), (int)addr, ((size << 3)-1), 0x82);
}
