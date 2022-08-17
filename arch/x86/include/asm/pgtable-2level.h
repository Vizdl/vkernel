#ifndef _I386_PGTABLE_2LEVEL_H
#define _I386_PGTABLE_2LEVEL_H

// 22 - 32(22+10)
#define PGDIR_SHIFT		22
// 每个 pgd 表最多有 1024(2^10) 个项
#define PTRS_PER_PGD	1024
// 22 - 22(22+0)
#define PMD_SHIFT		22
// 每个 pmd 表最多有 1(2^0) 个项
#define PTRS_PER_PMD	1
// 每个 pte 表最多有 1024(2^10) 个项 
#define PTRS_PER_PTE	1024

#define pgd_none(pgd)		(0)
#define pgd_bad(pgd)		(0)
#define pgd_present(pgd)	(1)
#define pgd_clear(xp)				do { } while (0)

/**
 * @brief	设置 pte_t* 的值
 * @param	pte_t*
 * @param	pte_t
 * @return
 */
#define set_pte(pteptr, pteval) (*(pteptr) = pteval)
#define set_pmd(pmdptr, pmdval) (*(pmdptr) = pmdval)
#define set_pgd(pgdptr, pgdval) (*(pgdptr) = pgdval)

/**
 * @brief	根据页全局目录项找到对应的 页中间目录表头(虚拟地址)
 * @param	pgd_t
 * @return	unsigned long, 页中间目录表头(虚拟地址)
 */
#define pgd_page(pgd) \
((unsigned long) __va(pgd_val(pgd) & PAGE_MASK))

/**
 * @brief	通过 页全局目录表项 和 虚拟地址 找到 虚拟地址所对应的页中间目录表项(虚拟地址)
 * @param	pgd_t* 页全局目录表项
 * @param	unsigned long 虚拟地址
 * @return	虚拟地址所对应的页中间目录表项
 */
#define pmd_offset(dir, address) ((pmd_t *)dir)

#define ptep_get_and_clear(xp)	__pte(xchg(&(xp)->pte_low, 0))
#define pte_same(a, b)		((a).pte_low == (b).pte_low)
#define pte_page(x)		(mem_map+((unsigned long)(((x).pte_low >> PAGE_SHIFT))))
#define pte_none(x)		(!(x).pte_low)
#define __mk_pte(page_nr,pgprot) __pte(((page_nr) << PAGE_SHIFT) | pgprot_val(pgprot))

#endif /* _I386_PGTABLE_2LEVEL_H */
