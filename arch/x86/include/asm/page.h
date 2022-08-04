#ifndef _I386_PAGE_H
#define _I386_PAGE_H

/* PAGE_SHIFT determines the page size */
#define PAGE_SHIFT	12                  // 页偏移量, 4^12,所以 PAGE_SHIFT = 12
#define PAGE_SIZE	(1UL << PAGE_SHIFT) // 1 << 12, 2^12 = 4k
#define PAGE_MASK	(~(PAGE_SIZE-1))    // ~(2^12-1) = ~(0x0000ffff) = 0xffff0000

/**
 * @brief 
 * 这里之所以要将 unsigned long 封装在 结构体内
 * 1. 起到类型保护的作用,避免被不恰当的使用(例如直接加减)
 * 2. 可拓展(后续如若有更改则对外提供的接口与数据类型不需改变)
 */
// 页全局目录
typedef struct { unsigned long pte_low; } pte_t;
// 页中间目录
typedef struct { unsigned long pmd; } pmd_t;
// 页表项
typedef struct { unsigned long pgd; } pgd_t;

// 通过结构体提取出里面的 unsigned long
#define pte_val(x)	((x).pte_low)
#define pmd_val(x)	((x).pmd)
#define pgd_val(x)	((x).pgd)
// 通过 unsigned long 获得 结构体
#define __pte(x) ((pte_t) { (x) } )
#define __pmd(x) ((pmd_t) { (x) } )
#define __pgd(x) ((pgd_t) { (x) } )

/* to align the pointer to the (next) page boundary */
// 将一个地址页面对齐
#define PAGE_ALIGN(addr)	(((addr)+PAGE_SIZE-1)&PAGE_MASK)

/*
 * This handles the memory map.. We could make this a config
 * option, but too many people screw it up, and too few need
 * it.
 *
 * A __PAGE_OFFSET of 0xC0000000 means that the kernel has
 * a virtual address space of one gigabyte, which limits the
 * amount of physical memory you can use to about 950MB. 
 *
 * If you want more physical memory than this then see the CONFIG_HIGHMEM4G
 * and CONFIG_HIGHMEM64G options in the kernel configuration.
 */
// 线性映射偏移量, 3GB
#define __PAGE_OFFSET		(0xC0000000)

#define PAGE_OFFSET		((unsigned long)__PAGE_OFFSET)
// 根据虚拟地址计算物理地址
#define __pa(x)			((unsigned long)(x)-PAGE_OFFSET)
// 根据物理地址计算虚拟地址
#define __va(x)			((void *)((unsigned long)(x)+PAGE_OFFSET))

#endif /* _I386_PAGE_H */