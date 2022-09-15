#include <asm/page.h>
#include <vkernel/mm.h>
#include <vkernel/swap.h>
#include <vkernel/init.h>
#include <vkernel/debug.h>
#include <vkernel/mmzone.h>
#include <vkernel/threads.h>
#include <vkernel/spinlock.h>
#include <vkernel/kernel.h>
#include <vkernel/string.h>
#include <vkernel/bootmem.h>

pg_data_t *pgdat_list;

static char *zone_names[MAX_NR_ZONES] = { "DMA", "Normal", "HighMem" };

#define memlist_init(x) INIT_LIST_HEAD(x)
#define memlist_add_head list_add
#define memlist_add_tail list_add_tail
#define memlist_del list_del
#define memlist_entry list_entry
#define memlist_next(x) ((x)->next)
#define memlist_prev(x) ((x)->prev)

/*
 * Temporary debugging check.
 */
#define BAD_RANGE(zone,x) (((zone) != (x)->zone) || (((x)-mem_map) < (zone)->offset) || (((x)-mem_map) >= (zone)->offset+(zone)->size))

static void FASTCALL(__free_pages_ok (struct page *page, unsigned long order));

/**
 * @brief 释放 2^order 大小的物理页
 * 
 * @param page 要释放的第一个物理页
 * @param order 2^order
 */
static void __free_pages_ok (struct page *page, unsigned long order)
{
	unsigned long index, page_idx, mask, flags;
	free_area_t *area;
	struct page *base;
	zone_t *zone;

	if (!VALID_PAGE(page))
		BUG();
	if (PageSwapCache(page))
		BUG();
	if (PageLocked(page))
		BUG();
	if (PageDecrAfter(page))
		BUG();
	if (PageActive(page))
		BUG();
	if (PageInactiveDirty(page))
		BUG();
	if (PageInactiveClean(page))
		BUG();

	page->flags &= ~((1<<PG_referenced) | (1<<PG_dirty));
	page->age = PAGE_AGE_START;
	
	zone = page->zone;
	// 0bffff fff0
	mask = (~0UL) << order;
	// 找到 zone 第一个物理页
	base = mem_map + zone->offset;
	// 计算 page 相对于 zone 第一个物理页 的偏移量
	page_idx = page - base;
	if (page_idx & ~mask)
		BUG();
	index = page_idx >> (1 + order);
	// 根据阶数找到对应的 free_area
	area = zone->free_area + order;

	spin_lock_irqsave(&zone->lock, flags);

	zone->free_pages -= mask;

	while (mask + (1 << (MAX_ORDER-1))) {
		struct page *buddy1, *buddy2;

		if (area >= zone->free_area + MAX_ORDER)
			BUG();
		// 判断是否能合成,如若不能则 break
		if (!test_and_change_bit(index, area->map))
			/*
			 * the buddy page is still allocated.
			 */
			break;

		// buddy
		buddy1 = base + (page_idx ^ -mask);
		// 当前要释放的物理页
		buddy2 = base + page_idx;
		if (BAD_RANGE(zone,buddy1))
			BUG();
		if (BAD_RANGE(zone,buddy2))
			BUG();
		// 将 buddy 释放了
		memlist_del(&buddy1->list);
		mask <<= 1;
		area++;
		index >>= 1;
		page_idx &= mask;
	}
	// 添加合成后的物理页到 free_list
	memlist_add_head(&(base + page_idx)->list, &area->free_list);

	spin_unlock_irqrestore(&zone->lock, flags);

	/*
	 * We don't want to protect this variable from race conditions
	 * since it's nothing important, but we do want to make sure
	 * it never gets negative.
	 */
	if (memory_pressure > NR_CPUS)
		memory_pressure--;
}

#define MARK_USED(index, order, area) \
	change_bit((index) >> (1+(order)), (area)->map)

/**
 * @brief 拆分 page 数组,并将拆分下来的放到 area->free_list 上。
 * 
 * @param zone 申请到的 page 的 zone
 * @param page 申请到的 page 数组(长度为 high) 的第一个
 * @param index page 相对 zone->offset 的偏移量
 * @param low 需要的 page 的 order
 * @param high 当前 page 的 order
 * @param area page 的 area
 * @return 申请的第一个 page
 */
static inline struct page * expand (zone_t *zone, struct page *page,
	 unsigned long index, int low, int high, free_area_t * area)
{
	unsigned long size = 1 << high;
	// 不断拆分,直到当前 page 大小(high)等于 low
	while (high > low) {
		if (BAD_RANGE(zone,page))
			BUG();
		area--;
		high--;
		size >>= 1;
		// 将拆分下来的放到 area->free_list 上
		memlist_add_head(&(page)->list, &(area)->free_list);
		MARK_USED(index, high, area);
		index += size;
		// 两半取后面一半
		page += size;
	}
	if (BAD_RANGE(zone,page))
		BUG();
	return page;
}

/**
 * @brief 在 zone 申请 2^order 个 struct page(物理页)
 * 
 * @param zone 需要申请的区域
 * @param order 申请 2^order 个 struct page(物理页)
 * @return 申请的第一个 page
 */
static FASTCALL(struct page * rmqueue(zone_t *zone, unsigned long order));
static struct page * rmqueue(zone_t *zone, unsigned long order)
{
	free_area_t * area = zone->free_area + order;
	unsigned long curr_order = order;
	struct list_head *head, *curr;
	unsigned long flags;
	struct page *page;

	spin_lock_irqsave(&zone->lock, flags);
	// order -> MAX_ORDER - 1,寻找有空闲列表的一项。
	do {
		head = &area->free_list;
		curr = memlist_next(head);
		// 当前 free_list 不为空
		if (curr != head) {
			unsigned int index;

			page = memlist_entry(curr, struct page, list);
			if (BAD_RANGE(zone,page))
				BUG();
			memlist_del(curr);
			index = (page - mem_map) - zone->offset;
			MARK_USED(index, curr_order, area);
			zone->free_pages -= 1 << order;

			page = expand(zone, page, index, order, curr_order, area);
			spin_unlock_irqrestore(&zone->lock, flags);

			set_page_count(page, 1);
			if (BAD_RANGE(zone,page))
				BUG();
			//DEBUG_ADD_PAGE
			return page;	
		}
		curr_order++;
		area++;
	} while (curr_order < MAX_ORDER);
	spin_unlock_irqrestore(&zone->lock, flags);

	return NULL;
}

void __free_pages(struct page *page, unsigned long order)
{
	if (!PageReserved(page) && put_page_testzero(page))
		__free_pages_ok(page, order);
}

void free_pages(unsigned long addr, unsigned long order)
{
	struct page *fpage;

	fpage = virt_to_page(addr);
	if (VALID_PAGE(fpage))
		__free_pages(fpage, order);
}

/**
 * @brief 向 zonelist 里的 zone 依次申请 2^order 个物理页
 * 
 * @param zonelist 
 * @param order 
 * @return struct page* 
 */
struct page * __alloc_pages(zonelist_t *zonelist, unsigned long order)
{
	zone_t **zone;
	struct page * page;

	// 首先，看看是否有任何具有大量空闲内存的区域。我们首先分配空闲内存，因为它不包含任何数据…废话！
	zone = zonelist->zones;
	// 遍历所有区域,查看是否能申请到内存
	for (;;) {
		zone_t *z = *(zone++);
		if (!z)
			break;
		if (!z->size)
			BUG();
		page = rmqueue(z, order);
		if (page)
			return page;
	}

	/* No luck.. */
	printk("error : __alloc_pages: %lu-order allocation failed.\n", order);
	return NULL;
}

/*
 * Builds allocation fallback zone lists.
 */
static inline void build_zonelists(pg_data_t *pgdat)
{
	int i, j, k;

	for (i = 0; i < NR_GFPINDEX; i++) {
		zonelist_t *zonelist;
		zone_t *zone;

		zonelist = pgdat->node_zonelists + i;
		memset(zonelist, 0, sizeof(*zonelist));

		zonelist->gfp_mask = i;
		j = 0;
		k = ZONE_NORMAL;
		if (i & __GFP_DMA)
			k = ZONE_DMA;

		switch (k) {
			default:
				BUG();
			case ZONE_NORMAL:
				zone = pgdat->node_zones + ZONE_NORMAL;
				if (zone->size)
					zonelist->zones[j++] = zone;
			case ZONE_DMA:
				zone = pgdat->node_zones + ZONE_DMA;
				if (zone->size)
					zonelist->zones[j++] = zone;
		}
		zonelist->zones[j++] = NULL;
	} 
}

#define LONG_ALIGN(x) (((x)+(sizeof(long))-1)&~((sizeof(long))-1))

/**
 * @brief 初始化 buddy
 * 
 * @param nid cpu id?
 * @param pgdat 
 * @param gmap 全局物理页数组
 * @param zones_size 每个zone的物理页个数
 * @param zone_start_paddr 第一个 zone 开始地址
 * @param zholes_size 每个zone的空洞物理页个数
 * @param lmem_map 
 */
void __init free_area_init_core(int nid, pg_data_t *pgdat, struct page **gmap,
	unsigned long *zones_size, unsigned long zone_start_paddr, 
	unsigned long *zholes_size, struct page *lmem_map)
{
	struct page *p;
	unsigned long i, j;
	unsigned long map_size;
	unsigned long totalpages, offset, realtotalpages;
	unsigned int cumulative = 0;

	// 计算所有 ZONE 的 pages 总和
	totalpages = 0;
	for (i = 0; i < MAX_NR_ZONES; i++) {
		unsigned long size = zones_size[i];
		totalpages += size;
	}
	realtotalpages = totalpages;
	if (zholes_size)
		for (i = 0; i < MAX_NR_ZONES; i++)
			realtotalpages -= zholes_size[i];
			
	printk("On node %d totalpages: %lu\n", nid, realtotalpages);

	// 创建 lmem_map struct page 数组
	map_size = (totalpages + 1)*sizeof(struct page);
	if (lmem_map == (struct page *)0) {
		lmem_map = (struct page *) alloc_bootmem_node(pgdat, map_size);
		lmem_map = (struct page *)(PAGE_OFFSET + 
			MAP_ALIGN((unsigned long)lmem_map - PAGE_OFFSET));
	}
	*gmap = pgdat->node_mem_map = lmem_map;
	pgdat->node_size = totalpages;
	pgdat->node_start_paddr = zone_start_paddr;
	pgdat->node_start_mapnr = (lmem_map - mem_map);

	// 最初，所有页面都是保留的,一旦早期引导过程完成，free_all_bootmem() 就会释放空闲页面。
	// 遍历所有 struct page
	for (p = lmem_map; p < lmem_map + totalpages; p++) {
		set_page_count(p, 0);
		SetPageReserved(p);
		memlist_init(&p->list);
	}
	// zone 的偏移量
	offset = lmem_map - mem_map;	
	// 遍历初始化 zone
	for (j = 0; j < MAX_NR_ZONES; j++) {
		zone_t *zone = pgdat->node_zones + j;
		unsigned long mask;
		unsigned long size, realsize;
		// 计算 zone 真实大小
		realsize = size = zones_size[j];
		if (zholes_size)
			realsize -= zholes_size[j];

		printk("zone(%lu): %lu pages.\n", j, size);
		zone->size = size;
		zone->name = zone_names[j];
		zone->lock = SPIN_LOCK_UNLOCKED;
		zone->zone_pgdat = pgdat;
		zone->free_pages = 0;
		if (!size)
			continue;

		zone->offset = offset;
		cumulative += size;

		zone->zone_mem_map = mem_map + offset;
		zone->zone_start_mapnr = offset;
		zone->zone_start_paddr = zone_start_paddr;

		for (i = 0; i < size; i++) {
			struct page *page = mem_map + offset + i;
			page->zone = zone;
			if (j != ZONE_HIGHMEM) {
				page->virtual = __va(zone_start_paddr);
				zone_start_paddr += PAGE_SIZE;
			}
		}
		// size 表示的是物理页个数
		offset += size;
		mask = -1;
		// 创建 free_area 数组
		for (i = 0; i < MAX_ORDER; i++) {
			unsigned long bitmap_size;
			// 初始化 free area list
			memlist_init(&zone->free_area[i].free_list);
			mask += mask;
			size = (size + ~mask) & mask;	// 对齐?
			// 计算 bitmap 的大小(针对不同阶有不同的大小), bitmap_size = zone_size(物理页总数) / 2^i
			// 假设有 1024 个物理页
			// 0 阶 free_area bitmap_size = 1024 bit
			// 1 阶 free_area bitmap_size = 512 bit
			// 2 阶 free_area bitmap_size = 256 bit
			// 3 阶 free_area bitmap_size = 128 bit
			// ...
			// 但是 alloc_bootmem_node 仅能申请物理页大小为单位的内存,而不是 bit, 所以 4k byte 是最小单位
			bitmap_size = size >> i;				// 一个bit能表示多少个物理页
			bitmap_size = (bitmap_size + 7) >> 3;	// 2^3 = 8
			bitmap_size = LONG_ALIGN(bitmap_size);	// 对齐
			zone->free_area[i].map = 
			  (unsigned int *) alloc_bootmem_node(pgdat, bitmap_size);
		}
	}
	build_zonelists(pgdat);
}

/**
 * @brief 通过释放 contig_page_data 内的 bootmem 来初始化 buddy
 * 
 * @param zones_size 每个 zones 物理页的数量
 */
void __init free_area_init(unsigned long *zones_size)
{
	free_area_init_core(0, &contig_page_data, &mem_map, zones_size, 0, 0, 0);
}