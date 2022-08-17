#ifndef _LINUX_BOOTMEM_H
#define _LINUX_BOOTMEM_H

#include <vkernel/init.h>
#include <vkernel/mmzone.h>

extern unsigned long max_low_pfn;
extern unsigned long min_low_pfn;

typedef struct bootmem_data {
	unsigned long node_boot_start;	// bootmem 管理的起始地址(物理地址)
	unsigned long node_low_pfn;		// bootmem 管理的最后一个页帧
	void *node_bootmem_map;			// 位图指针
	unsigned long last_offset;
	unsigned long last_pos;
} bootmem_data_t;

/**
 * @brief 初始化 bootmem, 并设置管理的内存范围
 * 
 * @param addr bootmem 管理的起始页帧
 * @param memend bootmem 管理的终止页帧
 * @return unsigned 位图的 bit 数
 */
extern unsigned long __init init_bootmem (unsigned long addr, unsigned long memend);

/**
 * @brief 初始化 bootmem
 * 
 * @param pgdat 
 * @param freepfn 位图的存储页帧
 * @param startpfn bootmem 管理的起始页帧
 * @param endpfn bootmem 管理的终止页帧
 * @return unsigned 位图的 bit 数
 */
extern unsigned long __init init_bootmem_node (pg_data_t *pgdat, unsigned long freepfn, unsigned long startpfn, unsigned long endpfn);

/**
 * @brief 将起始地址为 addr,大小为 size byte 的物理地址初始化为可用
 * 
 * @param addr 物理地址
 * @param size 单位 byte
 */
extern void __init reserve_bootmem (unsigned long addr, unsigned long size);

/**
 * @brief 将起始地址为 addr,大小为 size byte 的物理地址初始化为可用 
 * 
 * @param pgdat
 * @param physaddr 物理地址
 * @param size 单位 byte
 */
extern void __init reserve_bootmem_node (pg_data_t *pgdat, unsigned long physaddr, unsigned long size);

/**
 * @brief 
 * 
 * @param long 
 * @return unsigned 
 */
extern unsigned long __init bootmem_bootmap_pages (unsigned long);

/**
 * @brief 申请起始物理地址为 goal,对齐为 align,大小为 size 的物理地址并计算出其虚拟地址
 * 
 * @param size 需要申请的内存大小(单位 byte)
 * @param align 对齐,eg: 4k 对齐, align = (2^12)
 * @param goal 分配的起始地址(物理地址)
 * @return void* 虚拟地址
 */
extern void * __init __alloc_bootmem (unsigned long size, unsigned long align, unsigned long goal);

/**
 * @brief 向指定 pg_data_t 内,申请起始物理地址为 goal,对齐为 align,大小为 size 的物理地址并计算出其虚拟地址
 * 
 * @param pgdat
 * @param size 需要申请的内存大小(单位 byte)
 * @param align 对齐,eg: 4k 对齐, align = (2^12)
 * @param goal 分配的起始地址(物理地址)
 * @return void* 虚拟地址
 */
extern void * __init __alloc_bootmem_node (pg_data_t *pgdat, unsigned long size, unsigned long align, unsigned long goal);

/**
 * @brief 向指定 bootmem_data_t 释放起始物理地址为 addr 大小为 size byte 的内存块
 * 
 * @param bdata 
 * @param addr 物理地址
 * @param size 释放的大小,单位为 byte
 */
extern void __init free_bootmem (unsigned long addr, unsigned long size);

/**
 * @brief 从物理地址 0 寻找页面对齐大小为 x byte 的物理页
 * @param x 要申请的内存大小
 * @return void* 虚拟地址
 */
#define alloc_bootmem_low_pages(x) \
	__alloc_bootmem((x), PAGE_SIZE, 0)

#endif // _LINUX_BOOTMEM_H