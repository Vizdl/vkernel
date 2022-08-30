#include <asm/desc.h>

/**
 * @brief 在 GDT 上设置一个 TSS 描述符
 * 
 * @param n GDT index
 * @param addr 线性基地址
 */
void set_tss_desc(unsigned int n, void *addr)
{
	_set_tssldt_desc(gdt_table+__TSS(n), (int)addr, 235, 0x89);
}