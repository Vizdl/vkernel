#ifndef _LINUX_MMZONE_H
#define _LINUX_MMZONE_H

#ifdef __KERNEL__
#ifndef __ASSEMBLY__

#include <vkernel/config.h>
#include <vkernel/spinlock.h>
#include <vkernel/list.h>

#define MAX_ORDER 10

typedef struct free_area_struct {
	struct list_head	free_list;
	unsigned int		*map;
} free_area_t;

struct pglist_data;

typedef struct zone_struct {
	spinlock_t		lock;
	unsigned long		offset;
	unsigned long		free_pages;
	unsigned long		inactive_clean_pages;
	unsigned long		inactive_dirty_pages;
	unsigned long		pages_min, pages_low, pages_high;

	struct list_head	inactive_clean_list;
	free_area_t		free_area[MAX_ORDER];

	char			*name;
	unsigned long		size;

	struct pglist_data	*zone_pgdat;
	unsigned long		zone_start_paddr;
	unsigned long		zone_start_mapnr;
	struct page		*zone_mem_map;
} zone_t;

#define ZONE_DMA		0
#define ZONE_NORMAL		1
#define ZONE_HIGHMEM	2
#define MAX_NR_ZONES	3

typedef struct zonelist_struct {
	zone_t * zones [MAX_NR_ZONES+1]; // NULL delimited
	int gfp_mask;
} zonelist_t;

#define NR_GFPINDEX		0x100

struct bootmem_data;
typedef struct pglist_data {
	zone_t node_zones[MAX_NR_ZONES];
	zonelist_t node_zonelists[NR_GFPINDEX];
	struct page *node_mem_map;
	unsigned long *valid_addr_bitmap;
	struct bootmem_data *bdata;
	unsigned long node_start_paddr;
	unsigned long node_start_mapnr;
	unsigned long node_size;
	int node_id;
	struct pglist_data *node_next;
} pg_data_t;

extern pg_data_t *pgdat_list;
extern pg_data_t contig_page_data;

#define MAP_ALIGN(x)	((((x) % sizeof(mem_map_t)) == 0) ? (x) : ((x) + \
		sizeof(mem_map_t) - ((x) % sizeof(mem_map_t))))

#endif /* !__ASSEMBLY__ */
#endif /* __KERNEL__ */
#endif /* _LINUX_MMZONE_H */