#include <asm/io.h>
#include <asm/page.h>
#include <asm/system.h>
#include <asm/pgtable.h>
#include <vkernel/mm.h>
#include <vkernel/init.h>
#include <vkernel/debug.h>
#include <vkernel/mmzone.h>
#include <vkernel/kernel.h>
#include <vkernel/bootmem.h>

pgd_t swapper_pg_dir[PTRS_PER_PGD] __page_aligned;
static unsigned long totalram_pages;

static void __init pagetable_init (void)
{
	unsigned long vaddr, end;
	pgd_t *pgd, *pgd_base;
	int i, j, k;
	pmd_t *pmd;
	pte_t *pte;
    printk("pagetable_init...\n");

	/*
	 * This can be zero as well - no problem, in that case we exit
	 * the loops anyway due to the PTRS_PER_* conditions.
	 */
	end = (unsigned long)__va(max_low_pfn*PAGE_SIZE);
    // 设置 pgd 为内核空间的第一个 pgd
	pgd_base = swapper_pg_dir;
	i = __pgd_offset(PAGE_OFFSET);
	pgd = pgd_base + i;
    // 遍历内核空间所有的 pgd
	for (; i < PTRS_PER_PGD; pgd++, i++) {
		vaddr = i*PGDIR_SIZE;
		if (end && (vaddr >= end))
			break;
		pmd = (pmd_t *)pgd;
		if (pmd != pmd_offset(pgd, 0))
			BUG();
        // 遍历内核空间所有 pmd
		for (j = 0; j < PTRS_PER_PMD; pmd++, j++) {
			vaddr = i*PGDIR_SIZE + j*PMD_SIZE;
			if (end && (vaddr >= end))
				break;
            // 创建一页大小的内存作为 pte 数组
			pte = (pte_t *) alloc_bootmem_low_pages(PAGE_SIZE);
            // pmd 内设置的是物理地址
			set_pmd(pmd, __pmd(_KERNPG_TABLE + __pa(pte)));

			if (pte != pte_offset(pmd, 0))
				BUG();
            // 遍历 pte 表
			for (k = 0; k < PTRS_PER_PTE; pte++, k++) {
                // 计算出当前 pte 虚拟地址
				vaddr = i*PGDIR_SIZE + j*PMD_SIZE + k*PAGE_SIZE;
				if (end && (vaddr >= end))
					break;
				*pte = mk_pte_phys(__pa(vaddr), PAGE_KERNEL);
			}
		}
	}

    return;
}

void __init paging_init(void)
{
    printk("paging_init...\n");
    pagetable_init();
    load_cr3(__pa(swapper_pg_dir));
    __flush_tlb();
	// 计算每个 zone 的大小并且初始化 buddy 系统
	{
		unsigned long zones_size[MAX_NR_ZONES] = {0, 0, 0};
		unsigned int max_dma, low;

		max_dma = virt_to_phys((char *)MAX_DMA_ADDRESS) >> PAGE_SHIFT;
		low = max_low_pfn;

		if (low < max_dma)
			zones_size[ZONE_DMA] = low;
		else {
			zones_size[ZONE_DMA] = max_dma;
			zones_size[ZONE_NORMAL] = low - max_dma;
		}
		free_area_init(zones_size);
	}
    return;
}

void __init mem_init(void){
	max_mapnr = num_physpages = max_low_pfn;
	/* this will put all low memory onto the freelists */
	totalram_pages += free_all_bootmem();
}