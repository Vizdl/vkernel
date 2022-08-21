#ifndef _LINUX_MM_H
#define _LINUX_MM_H

#ifdef __KERNEL__

#include <asm/atomic.h>
#include <vkernel/kernel.h>
#include <vkernel/list.h>

typedef struct page {
	struct list_head list;
	unsigned long index;
	struct page *next_hash;
	atomic_t count;
	unsigned long flags;	/* atomic flags, some possibly updated asynchronously */
	struct list_head lru;
	unsigned long age;
	struct page **pprev_hash;
	void *virtual; /* non-NULL if kmapped */
	struct zone_struct *zone;
} mem_map_t;

#define page_count(p)		atomic_read(&(p)->count)
#define set_page_count(p,v) 	atomic_set(&(p)->count, v)

/* Page flag bit values */
#define PG_locked		 	0
#define PG_error		 	1
#define PG_referenced		2
#define PG_uptodate		 	3
#define PG_dirty		 	4
#define PG_decr_after		5
#define PG_active		 	6
#define PG_inactive_dirty	7
#define PG_slab			 	8
#define PG_swap_cache		9
#define PG_skip				10
#define PG_inactive_clean	11
#define PG_highmem			12
				/* bits 21-29 unused */
#define PG_arch_1			30
#define PG_reserved			31

/* Make it prettier to test the above... */
#define Page_Uptodate(page)	test_bit(PG_uptodate, &(page)->flags)
#define SetPageUptodate(page)	set_bit(PG_uptodate, &(page)->flags)
#define ClearPageUptodate(page)	clear_bit(PG_uptodate, &(page)->flags)
#define PageDirty(page)		test_bit(PG_dirty, &(page)->flags)
#define SetPageDirty(page)	set_bit(PG_dirty, &(page)->flags)
#define ClearPageDirty(page)	clear_bit(PG_dirty, &(page)->flags)
#define PageLocked(page)	test_bit(PG_locked, &(page)->flags)
#define LockPage(page)		set_bit(PG_locked, &(page)->flags)
#define TryLockPage(page)	test_and_set_bit(PG_locked, &(page)->flags)

#define PageError(page)		test_bit(PG_error, &(page)->flags)
#define SetPageError(page)	set_bit(PG_error, &(page)->flags)
#define ClearPageError(page)	clear_bit(PG_error, &(page)->flags)
#define PageReferenced(page)	test_bit(PG_referenced, &(page)->flags)
#define SetPageReferenced(page)	set_bit(PG_referenced, &(page)->flags)
#define ClearPageReferenced(page)	clear_bit(PG_referenced, &(page)->flags)
#define PageTestandClearReferenced(page)	test_and_clear_bit(PG_referenced, &(page)->flags)
#define PageDecrAfter(page)	test_bit(PG_decr_after, &(page)->flags)
#define SetPageDecrAfter(page)	set_bit(PG_decr_after, &(page)->flags)
#define PageTestandClearDecrAfter(page)	test_and_clear_bit(PG_decr_after, &(page)->flags)
#define PageSlab(page)		test_bit(PG_slab, &(page)->flags)
#define PageSwapCache(page)	test_bit(PG_swap_cache, &(page)->flags)
#define PageReserved(page)	test_bit(PG_reserved, &(page)->flags)

#define PageSetSlab(page)	set_bit(PG_slab, &(page)->flags)
#define PageSetSwapCache(page)	set_bit(PG_swap_cache, &(page)->flags)

#define PageTestandSetSwapCache(page)	test_and_set_bit(PG_swap_cache, &(page)->flags)

#define PageClearSlab(page)		clear_bit(PG_slab, &(page)->flags)
#define PageClearSwapCache(page)	clear_bit(PG_swap_cache, &(page)->flags)

#define PageTestandClearSwapCache(page)	test_and_clear_bit(PG_swap_cache, &(page)->flags)

#define PageActive(page)	test_bit(PG_active, &(page)->flags)
#define SetPageActive(page)	set_bit(PG_active, &(page)->flags)
#define ClearPageActive(page)	clear_bit(PG_active, &(page)->flags)

#define PageInactiveDirty(page)	test_bit(PG_inactive_dirty, &(page)->flags)
#define SetPageInactiveDirty(page)	set_bit(PG_inactive_dirty, &(page)->flags)
#define ClearPageInactiveDirty(page)	clear_bit(PG_inactive_dirty, &(page)->flags)

#define PageInactiveClean(page)	test_bit(PG_inactive_clean, &(page)->flags)
#define SetPageInactiveClean(page)	set_bit(PG_inactive_clean, &(page)->flags)
#define ClearPageInactiveClean(page)	clear_bit(PG_inactive_clean, &(page)->flags)

#define SetPageReserved(page)		set_bit(PG_reserved, &(page)->flags)
#define ClearPageReserved(page)		clear_bit(PG_reserved, &(page)->flags)

extern mem_map_t * mem_map;

#endif /* __KERNEL__ */

#endif /* _LINUX_MM_H */