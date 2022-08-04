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
 * @brief	TODO
 * @param	pgd_t
 * @return	
 */
#define pgd_page(pgd) \
((unsigned long) __va(pgd_val(pgd) & PAGE_MASK))

/**
 * @brief	TODO
 * @param	TODO
 * @param	TODO
 * @return	
 */
extern inline pmd_t * pmd_offset(pgd_t * dir, unsigned long address)
{
	return (pmd_t *) dir;
}

#endif /* _I386_PGTABLE_2LEVEL_H */
