#include <linux/init.h>
#include <linux/bootmem.h>
#include <asm/page.h>>
#include <asm/pgtable.h>

static void __init pagetable_init (void) {
	unsigned long vaddr, end;
	pgd_t *pgd, *pgd_base;
	int i, j, k;
	pmd_t *pmd;
	pte_t *pte;

    // 获取最大帧虚拟地址
    end = (unsigned long)__va(max_low_pfn*PAGE_SIZE);

    pgd_base = swapper_pg_dir;
    // 获取内核空间的第一个页全局目录项
    i = __pgd_offset(PAGE_OFFSET);
	pgd = pgd_base + i;
    // 遍历内核空间的所有页全局目录项
    for (; i < PTRS_PER_PGD; pgd++, i++) {
		vaddr = i*PGDIR_SIZE;
        // 如若超越了最大帧
		if (end && (vaddr >= end))
			break;
        // 创建 pmd
#if CONFIG_X86_PAE
		pmd = (pmd_t *) alloc_bootmem_low_pages(PAGE_SIZE);
		set_pgd(pgd, __pgd(__pa(pmd) + 0x1));
#else
        // 如若是二级页表,则 pmd 等于 pgd
		pmd = (pmd_t *)pgd;
#endif
        // 如若 pmd 不是 页中间目录表表头的话
		if (pmd != pmd_offset(pgd, 0))
			BUG();
            
        // 遍历整个页中间目录表
		for (j = 0; j < PTRS_PER_PMD; pmd++, j++) {
			vaddr = i*PGDIR_SIZE + j*PMD_SIZE;
			if (end && (vaddr >= end))
				break;
			if (cpu_has_pse) {
				unsigned long __pe;

				set_in_cr4(X86_CR4_PSE);
				boot_cpu_data.wp_works_ok = 1;
				__pe = _KERNPG_TABLE + _PAGE_PSE + __pa(vaddr);
				/* Make it "global" too if supported */
				if (cpu_has_pge) {
					set_in_cr4(X86_CR4_PGE);
					__pe += _PAGE_GLOBAL;
				}
				set_pmd(pmd, __pmd(__pe));
				continue;
			}

			pte = (pte_t *) alloc_bootmem_low_pages(PAGE_SIZE);
			set_pmd(pmd, __pmd(_KERNPG_TABLE + __pa(pte)));

			if (pte != pte_offset(pmd, 0))
				BUG();

			for (k = 0; k < PTRS_PER_PTE; pte++, k++) {
				vaddr = i*PGDIR_SIZE + j*PMD_SIZE + k*PAGE_SIZE;
				if (end && (vaddr >= end))
					break;
				*pte = mk_pte_phys(__pa(vaddr), PAGE_KERNEL);
			}
		}

    }
    return;
}