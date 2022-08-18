#ifndef _I386_PGTABLE_H
#define _I386_PGTABLE_H

#ifndef _I386_BITOPS_H
#include <asm/bitops.h>
#endif

/**
 * @brief 加载物理地址 pgdir 进入 cr3 寄存器
 * @param pgdir 物理地址
 * @return 
 */
#define load_cr3(pgdir) \
	asm volatile("movl %0,%%cr3": :"r" (pgdir));

/* Caches aren't brain-dead on the intel. */
#define flush_cache_all()					do { } while (0)
#define flush_cache_mm(mm)					do { } while (0)
#define flush_cache_range(mm, start, end)	do { } while (0)
#define flush_cache_page(vma, vmaddr)		do { } while (0)
#define flush_page_to_ram(page)				do { } while (0)
#define flush_dcache_page(page)				do { } while (0)
#define flush_icache_range(start, end)		do { } while (0)
#define flush_icache_page(vma,pg)			do { } while (0)

#define __flush_tlb()							\
	do {								\
		unsigned int tmpreg;					\
									\
		__asm__ __volatile__(					\
			"movl %%cr3, %0;  # flush TLB \n"		\
			"movl %0, %%cr3;              \n"		\
			: "=r" (tmpreg)					\
			:: "memory");					\
	} while (0)

/*
 * Global pages have to be flushed a bit differently. Not a real
 * performance problem because this does not happen often.
 */
#define __flush_tlb_global()						\
	do {								\
		unsigned int tmpreg;					\
									\
		__asm__ __volatile__(					\
			"movl %1, %%cr4;  # turn off PGE     \n"	\
			"movl %%cr3, %0;  # flush TLB        \n"	\
			"movl %0, %%cr3;                     \n"	\
			"movl %2, %%cr4;  # turn PGE back on \n"	\
			: "=&r" (tmpreg)				\
			: "r" (mmu_cr4_features & ~X86_CR4_PGE),	\
			  "r" (mmu_cr4_features)			\
			: "memory");					\
	} while (0)

// 在x86 体系结构中 PTE 中的状态标志位的 bit 位下标
#define _PAGE_BIT_PRESENT	0
#define _PAGE_BIT_RW		1
#define _PAGE_BIT_USER		2
#define _PAGE_BIT_PWT		3
#define _PAGE_BIT_PCD		4
#define _PAGE_BIT_ACCESSED	5
#define _PAGE_BIT_DIRTY		6
#define _PAGE_BIT_PSE		7	/* 4 MB (or 2MB) page, Pentium+, if present.. */
#define _PAGE_BIT_GLOBAL	8	/* Global TLB entry PPro+ */

// 在x86 体系结构中 PTE 中的状态标志位的值, pte & _PAGE_XXX 就可知 _PAGE_XXX 是否被设置。
// 页面在内存中并且没有被换出
#define _PAGE_PRESENT	    0x001
// 页面可写
#define _PAGE_RW	        0x002
// 页面可以从用户空间访问
#define _PAGE_PWT	        0x008
#define _PAGE_USER	        0x004
#define _PAGE_PCD	        0x010
// 页面被访问过
#define _PAGE_ACCESSED	    0x020
// 页面被写过
#define _PAGE_DIRTY	        0x040
#define _PAGE_PSE	        0x080	/* 4 MB (or 2MB) page, Pentium+, if present.. */
#define _PAGE_GLOBAL	    0x100	/* Global TLB entry PPro+ */
// 页面在内存中, 但是不可访问
#define _PAGE_PROTNONE	    0x080	/* If not present */

// 复合属性
#define _PAGE_TABLE	(_PAGE_PRESENT | _PAGE_RW | _PAGE_USER | _PAGE_ACCESSED | _PAGE_DIRTY)
#define _KERNPG_TABLE	(_PAGE_PRESENT | _PAGE_RW | _PAGE_ACCESSED | _PAGE_DIRTY)
#define _PAGE_CHG_MASK	(PTE_MASK | _PAGE_ACCESSED | _PAGE_DIRTY)

#define PAGE_NONE	__pgprot(_PAGE_PROTNONE | _PAGE_ACCESSED)
#define PAGE_SHARED	__pgprot(_PAGE_PRESENT | _PAGE_RW | _PAGE_USER | _PAGE_ACCESSED)
#define PAGE_COPY	__pgprot(_PAGE_PRESENT | _PAGE_USER | _PAGE_ACCESSED)
#define PAGE_READONLY	__pgprot(_PAGE_PRESENT | _PAGE_USER | _PAGE_ACCESSED)

#define __PAGE_KERNEL \
	(_PAGE_PRESENT | _PAGE_RW | _PAGE_DIRTY | _PAGE_ACCESSED)
#define __PAGE_KERNEL_NOCACHE \
	(_PAGE_PRESENT | _PAGE_RW | _PAGE_DIRTY | _PAGE_PCD | _PAGE_ACCESSED)
#define __PAGE_KERNEL_RO \
	(_PAGE_PRESENT | _PAGE_DIRTY | _PAGE_ACCESSED)

#define PAGE_KERNEL MAKE_GLOBAL(__PAGE_KERNEL)
#define PAGE_KERNEL_RO MAKE_GLOBAL(__PAGE_KERNEL_RO)
#define PAGE_KERNEL_NOCACHE MAKE_GLOBAL(__PAGE_KERNEL_NOCACHE)

# define MAKE_GLOBAL(x)						\
	({							\
		pgprot_t __ret;					\
		__ret = __pgprot(x);			\
		__ret;						\
	})

#include <asm/pgtable-2level.h>

#define PMD_SIZE	(1UL << PMD_SHIFT)
#define PMD_MASK	(~(PMD_SIZE-1))
#define PGDIR_SIZE	(1UL << PGDIR_SHIFT)
#define PGDIR_MASK	(~(PGDIR_SIZE-1))

// 判断 pte 是否可用(不在内存中或不可访问都为不可用)
#define pte_present(x)	((x).pte_low & (_PAGE_PRESENT | _PAGE_PROTNONE))
// 将 xp 的内容设置为 0
#define pte_clear(xp)	do { set_pte(xp, __pte(0)); } while (0)
// 判断 pmd 是否
#define pmd_none(x)	(!pmd_val(x))
#define pmd_present(x)	(pmd_val(x) & _PAGE_PRESENT)
#define pmd_clear(xp)	do { set_pmd(xp, __pmd(0)); } while (0)
#define	pmd_bad(x)	((pmd_val(x) & (~PAGE_MASK & ~_PAGE_USER)) != _KERNPG_TABLE)


// 判断 pte 是否 可读可/执行/已写/已读/可写
static inline int pte_read(pte_t pte)		{ return (pte).pte_low & _PAGE_USER; }
static inline int pte_exec(pte_t pte)		{ return (pte).pte_low & _PAGE_USER; }
static inline int pte_dirty(pte_t pte)		{ return (pte).pte_low & _PAGE_DIRTY; }
static inline int pte_young(pte_t pte)		{ return (pte).pte_low & _PAGE_ACCESSED; }
static inline int pte_write(pte_t pte)		{ return (pte).pte_low & _PAGE_RW; }
// 设置 pte 不可读/不可执行/未写/未读/不可写
static inline pte_t pte_rdprotect(pte_t pte)	{ (pte).pte_low &= ~_PAGE_USER; return pte; }
static inline pte_t pte_exprotect(pte_t pte)	{ (pte).pte_low &= ~_PAGE_USER; return pte; }
static inline pte_t pte_mkclean(pte_t pte)	{ (pte).pte_low &= ~_PAGE_DIRTY; return pte; }
static inline pte_t pte_mkold(pte_t pte)	{ (pte).pte_low &= ~_PAGE_ACCESSED; return pte; }
static inline pte_t pte_wrprotect(pte_t pte)	{ (pte).pte_low &= ~_PAGE_RW; return pte; }
// 设置 pte 可读/可执行/已写/已读/可写
static inline pte_t pte_mkread(pte_t pte)	{ (pte).pte_low |= _PAGE_USER; return pte; }
static inline pte_t pte_mkexec(pte_t pte)	{ (pte).pte_low |= _PAGE_USER; return pte; }
static inline pte_t pte_mkdirty(pte_t pte)	{ (pte).pte_low |= _PAGE_DIRTY; return pte; }
static inline pte_t pte_mkyoung(pte_t pte)	{ (pte).pte_low |= _PAGE_ACCESSED; return pte; }
static inline pte_t pte_mkwrite(pte_t pte)	{ (pte).pte_low |= _PAGE_RW; return pte; }
// pte 原子操作
static inline  int ptep_test_and_clear_dirty(pte_t *ptep)	{ return test_and_clear_bit(_PAGE_BIT_DIRTY, ptep); }
static inline  int ptep_test_and_clear_young(pte_t *ptep)	{ return test_and_clear_bit(_PAGE_BIT_ACCESSED, ptep); }
static inline void ptep_set_wrprotect(pte_t *ptep)		{ clear_bit(_PAGE_BIT_RW, ptep); }
static inline void ptep_mkdirty(pte_t *ptep)			{ set_bit(_PAGE_BIT_DIRTY, ptep); }

#define mk_pte(page, pgprot)	__mk_pte((page) - mem_map, (pgprot))

#define mk_pte_phys(physpage, pgprot)	__mk_pte((physpage) >> PAGE_SHIFT, pgprot)

static inline pte_t pte_modify(pte_t pte, pgprot_t newprot)
{
	pte.pte_low &= _PAGE_CHG_MASK;
	pte.pte_low |= pgprot_val(newprot);
	return pte;
}

/**
 * @brief 根据页中间目录项找到其页表项表头的虚拟地址
 * @param pmd_t
 * @return pte_t* ,页表项表头的虚拟地址
 */
#define pmd_page(pmd) \
((unsigned long) __va(pmd_val(pmd) & PAGE_MASK))

/**
 * @brief 获取虚拟地址在 pgd 表中的索引
 * @param 虚拟地址
 * @return 虚拟地址在 pgd 表中的索引
 */
#define pgd_index(address) ((address >> PGDIR_SHIFT) & (PTRS_PER_PGD-1))
#define __pgd_offset(address) pgd_index(address)

/**
 * @brief 从指定 mm_struct 对应的页表 获取 虚拟地址 对应的 页中间表项指针
 * @param struct mm_struct*
 * @param 虚拟地址
 * @return pmd_t *, 页中间表项指针
 */
#define pgd_offset(mm, address) ((mm)->pgd+pgd_index(address))

/**
 * @brief 获取 属于内核空间的虚拟地址 对应的 页中间表项指针
 * @param 虚拟地址
 * @return pmd_t *, 页中间表项指针
 */
#define pgd_offset_k(address) pgd_offset(&init_mm, address)

/**
 * @brief 获取虚拟地址在 pmd 表中的索引
 * @param 虚拟地址
 * @return 虚拟地址在 pmd 表中的索引
 */
#define __pmd_offset(address) \
		(((address) >> PMD_SHIFT) & (PTRS_PER_PMD-1))

/**
 * @brief 获取虚拟地址在 pte 表中的索引
 * @param 虚拟地址
 * @return 虚拟地址在 pte 表中的索引
 */
#define __pte_offset(address) \
		((address >> PAGE_SHIFT) & (PTRS_PER_PTE - 1))

/**
 * @brief 根据 页中间目录表项 和 虚拟地址 获取 对应的 页表项
 * @param pmd_t *, 页中间目录表项指针
 * @param unsigned long, 虚拟地址
 * @return pte_t *,页表项指针(虚拟地址)
 */
#define pte_offset(dir, address) ((pte_t *) pmd_page(*(dir)) + \
			__pte_offset(address))

extern pgd_t swapper_pg_dir[PTRS_PER_PGD];
extern void paging_init(void);

#endif /* _I386_PGTABLE_H */