#ifndef _I386_PAGE_H
#define _I386_PAGE_H

// 页偏移量, 4^12,所以 PAGE_SHIFT = 12
#define PAGE_SHIFT	12
// 1 << 12, 2^12 = 4k
#define PAGE_SIZE	(1UL << PAGE_SHIFT)
// ~(2^12-1) = ~(0x0000ffff) = 0xffff0000
#define PAGE_MASK	(~(PAGE_SIZE-1))

#define PTE_MASK	PAGE_MASK

// 线性映射偏移量, 3GB
#define __PAGE_OFFSET   (0xC0000000)

#define PAGE_OFFSET		((unsigned long)__PAGE_OFFSET)

#ifndef __ASSEMBLY__

// 按页面对齐
#define __page_aligned __attribute__((__aligned__(PAGE_SIZE)))

/**
 * @brief 
 * 这里之所以要将 unsigned long 封装在 结构体内
 * 1. 起到类型保护的作用,避免被不恰当的使用(例如直接加减)
 * 2. 可拓展(后续如若有更改则对外提供的接口与数据类型不需改变)
 */
// 页全局目录, 内部的地址是物理地址。
typedef struct { unsigned long pte_low; } pte_t;
// 页中间目录, 内部的地址是物理地址。
typedef struct { unsigned long pmd; } pmd_t;
// 页表项, 内部的地址是物理地址。
typedef struct { unsigned long pgd; } pgd_t;
// 存储保护位
typedef struct { unsigned long pgprot; } pgprot_t;

/**
 * @brief	通过 pte_t/pmd_t/pgt_t/pgprot_t 结构体提取出内部的 unsigned long
 * @param	结构体
 * @return	内部的 unsigned long
 */
#define pte_val(x)	((x).pte_low)
#define pmd_val(x)	((x).pmd)
#define pgd_val(x)	((x).pgd)
#define pgprot_val(x)	((x).pgprot)

/**
 * @brief	将 unsigned long 转换为 pte_t/pmd_t/pgt_t/pgprot_t 结构体
 * @param	地址
 * @return	转换后的结构体
 */
#define __pte(x) ((pte_t) { (x) } )
#define __pmd(x) ((pmd_t) { (x) } )
#define __pgd(x) ((pgd_t) { (x) } )
#define __pgprot(x)	((pgprot_t) { (x) } )

/**
 * @brief	将一个地址按页面对齐
 * @param	地址
 * @return	页面对齐后的地址
 */
#define PAGE_ALIGN(addr)    (((addr)+PAGE_SIZE-1)&PAGE_MASK)

/**
 * @brief	根据虚拟地址计算物理地址
 * @param	虚拟地址
 * @return	物理地址
 */
#define __pa(x)			((unsigned long)(x)-PAGE_OFFSET)
/**
 * @brief	根据物理地址计算虚拟地址
 * @param	物理地址
 * @return	虚拟地址
 */
#define __va(x)			((void *)((unsigned long)(x)+PAGE_OFFSET))

#define virt_to_page(kaddr)	(mem_map + (__pa(kaddr) >> PAGE_SHIFT))
#define VALID_PAGE(page)	((page - mem_map) < max_mapnr)

#endif /* __ASSEMBLY__ */
#endif /* _I386_PAGE_H */