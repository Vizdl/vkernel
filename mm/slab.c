#include <asm/page.h>
#include <vkernel/mm.h>
#include <vkernel/smp.h>
#include <vkernel/irq.h>
#include <vkernel/list.h>
#include <vkernel/slab.h>
#include <vkernel/init.h>
#include <vkernel/types.h>
#include <vkernel/debug.h>
#include <vkernel/cache.h>
#include <vkernel/string.h>
#include <vkernel/spinlock.h>

typedef unsigned int kmem_bufctl_t;

// 空闲内存集合表示
typedef struct slab_s {
	struct list_head	list;		// 将 slab 链入一个 kmem_cache_t
	unsigned long		colouroff;	// 本 slab 着色区的大小
	void			*s_mem;			// 指向对象区的起点
	unsigned int		inuse;		// 已分配对象的计数器
	// 从这里开始到特定长度,都是空闲列表指针
	kmem_bufctl_t		free;		// 空闲对象链中的第一个对象的下标,需要乘以对象大小才能得到真正的空闲对象,如若为空,则表示为 BUFCTL_END
} slab_t;

#define BUFCTL_END 0xffffFFFF
#define	SLAB_LIMIT 0xffffFFFE
#define slab_bufctl(slabp) \
	((kmem_bufctl_t *)(((slab_t*)slabp)+1))

typedef struct kmem_cache_s kmem_cache_t;

#define CACHE_NAMELEN	20	/* max name length for a slab cache */

/* internal c_flags */
#define	CFLGS_OFF_SLAB	0x010000UL	/* slab management in own cache */
#define	CFLGS_OPTIMIZE	0x020000UL	/* optimized slab lookup */

/* c_dflags (dynamic flags). Need to hold the spinlock to access this member */
#define	DFLGS_GROWN	0x000001UL	/* don't reap a recently grown */

#define	OFF_SLAB(x)	((x)->flags & CFLGS_OFF_SLAB)
#define	OPTIMIZE(x)	((x)->flags & CFLGS_OPTIMIZE)
#define	GROWN(x)	((x)->dlags & DFLGS_GROWN)

#define	SET_PAGE_CACHE(pg,x)  ((pg)->list.next = (struct list_head *)(x))
#define	GET_PAGE_CACHE(pg)    ((kmem_cache_t *)(pg)->list.next)
#define	SET_PAGE_SLAB(pg,x)   ((pg)->list.prev = (struct list_head *)(x))
#define	GET_PAGE_SLAB(pg)     ((slab_t *)(pg)->list.prev)

# define CHECK_PAGE(pg)	do { } while (0)

// 统计宏
#define	STATS_INC_ACTIVE(x)	do { } while (0)
#define	STATS_DEC_ACTIVE(x)	do { } while (0)
#define	STATS_INC_ALLOCED(x)	do { } while (0)
#define	STATS_INC_GROWN(x)	do { } while (0)
#define	STATS_INC_REAPED(x)	do { } while (0)
#define	STATS_SET_HIGH(x)	do { } while (0)
#define	STATS_INC_ERR(x)	do { } while (0)

#define	MAX_OBJ_ORDER	5	/* 32 pages */

#define	BREAK_GFP_ORDER_HI	2
#define	BREAK_GFP_ORDER_LO	1
static int slab_break_gfp_order = BREAK_GFP_ORDER_LO;

#define	MAX_GFP_ORDER	5	/* 32 pages */

#define	BYTES_PER_WORD		sizeof(void *)

#define cache_chain (cache_cache.next)

# define CREATE_MASK	(SLAB_HWCACHE_ALIGN | SLAB_NO_REAP | SLAB_CACHE_DMA)

// 专用缓冲区
struct kmem_cache_s {
	struct list_head	slabs;			// slab 链表
	struct list_head	*firstnotfull;	// 第一块有空闲对象的 slab 的指针
	unsigned int		objsize;		// 每个对象的大小
	unsigned int	 	flags;	        /* constant flags */
	unsigned int		num;			// 每个 slab 的对象数
	spinlock_t		    spinlock;

	unsigned int		gfporder;		// 单次申请 2^gfporder 大小个物理页
	unsigned int		gfpflags;		// 申请物理页时使用的 flags

	size_t			    colour;		    /* cache colouring range */
	unsigned int		colour_off;	    /* colour offset */
	unsigned int		colour_next;	/* cache colouring */
	kmem_cache_t		*slabp_cache;	// 申请 slab 的专用缓冲区(并非链表之类的结构)
	unsigned int		growing;		// 剩余的可成长个数,单位是 slab
	unsigned int		dflags;		    /* dynamic flags */

	/* constructor func */
	void (*ctor)(void *, kmem_cache_t *, unsigned long);

	/* de-constructor func */
	void (*dtor)(void *, kmem_cache_t *, unsigned long);

	unsigned long		failures;

    /* 3) cache creation/removal */
	char			name[CACHE_NAMELEN];
	struct list_head	next;
};

typedef struct cache_sizes {
	size_t		 cs_size;
	kmem_cache_t	*cs_cachep;
	kmem_cache_t	*cs_dmacachep;
} cache_sizes_t;

void * kmem_cache_alloc (kmem_cache_t *cachep, int flags);
void kmem_cache_free (kmem_cache_t *cachep, void *objp);

// 提供给非专用缓冲区使用
static cache_sizes_t cache_sizes[] = {
#if PAGE_SIZE == 4096
	{    32,	NULL, NULL},
#endif
	{    64,	NULL, NULL},
	{   128,	NULL, NULL},
	{   256,	NULL, NULL},
	{   512,	NULL, NULL},
	{  1024,	NULL, NULL},
	{  2048,	NULL, NULL},
	{  4096,	NULL, NULL},
	{  8192,	NULL, NULL},
	{ 16384,	NULL, NULL},
	{ 32768,	NULL, NULL},
	{ 65536,	NULL, NULL},
	{131072,	NULL, NULL},
	{     0,	NULL, NULL}
};

// kmem_cache_t 数据结构的专用缓冲区
static kmem_cache_t cache_cache = {
	slabs:		LIST_HEAD_INIT(cache_cache.slabs),
	firstnotfull:	&cache_cache.slabs,
	objsize:	sizeof(kmem_cache_t),
	flags:		SLAB_NO_REAP,
	spinlock:	SPIN_LOCK_UNLOCKED,
	colour_off:	L1_CACHE_BYTES,
	name:		"kmem_cache",
};

/* Place maintainer for reaping. */
static kmem_cache_t *clock_searchp = &cache_cache;

static unsigned long offslab_limit;

/**
 * @brief 计算给对象大小
 * 
 * @param gfporder 
 * @param size 
 * @param flags 
 * @param left_over 
 * @param num 
 */
static void kmem_cache_estimate (unsigned long gfporder, size_t size,
		 int flags, size_t *left_over, unsigned int *num)
{
	int i;
	size_t wastage = PAGE_SIZE<<gfporder;
	size_t extra = 0;
	size_t base = 0;

	if (!(flags & CFLGS_OFF_SLAB)) {
		base = sizeof(slab_t);
		extra = sizeof(kmem_bufctl_t);
	}
	i = 0;
	while (i*size + L1_CACHE_ALIGN(base+i*extra) <= wastage)
		i++;
	if (i > 0)
		i--;

	if (i > SLAB_LIMIT)
		i = SLAB_LIMIT;

	*num = i;
	wastage -= i*size;
	wastage -= L1_CACHE_ALIGN(base+i*extra);
	*left_over = wastage;
}

kmem_cache_t * kmem_find_general_cachep (size_t size, int gfpflags)
{
	cache_sizes_t *csizep = cache_sizes;

	/* This function could be moved to the header file, and
	 * made inline so consumers can quickly determine what
	 * cache pointer they require.
	 */
	for ( ; csizep->cs_size; csizep++) {
		if (size > csizep->cs_size)
			continue;
		break;
	}
	return (gfpflags & GFP_DMA) ? csizep->cs_dmacachep : csizep->cs_cachep;
}

/**
 * @brief 创建一个专用缓冲区
 * 
 * @param name 专用缓冲区名
 * @param size 对象大小
 * @param offset 
 * @param flags 标志位
 * @param ctor 
 * @param dtor 
 * @return kmem_cache_t* 
 */
kmem_cache_t *
kmem_cache_create (const char *name, size_t size, size_t offset,
	unsigned long flags, void (*ctor)(void*, kmem_cache_t *, unsigned long),
	void (*dtor)(void*, kmem_cache_t *, unsigned long))
{
	const char *func_nm = "KERNEL ERROR -> kmem_create: ";
	size_t left_over, align, slab_size;
	kmem_cache_t *cachep = NULL;

	/*
	 * Sanity checks... these are all serious usage bugs.
	 */
	if ((!name) ||
		((strlen(name) >= CACHE_NAMELEN - 1)) ||
		in_interrupt() ||
		(size < BYTES_PER_WORD) ||
		(size > (1<<MAX_OBJ_ORDER)*PAGE_SIZE) ||
		(dtor && !ctor) ||
		(offset < 0 || offset > size))
			BUG();

	/*
	 * Always checks flags, a caller might be expecting debug
	 * support which isn't available.
	 */
	if (flags & ~CREATE_MASK)
		BUG();

	// 申请缓冲区描述对象
	cachep = (kmem_cache_t *) kmem_cache_alloc(&cache_cache, SLAB_KERNEL);
	if (!cachep)
		goto opps;
	memset(cachep, 0, sizeof(kmem_cache_t));

	/* Check that size is in terms of words.  This is needed to avoid
	 * unaligned accesses for some archs when redzoning is used, and makes
	 * sure any on-slab bufctl's are also correctly aligned.
	 */
	if (size & (BYTES_PER_WORD-1)) {
		size += (BYTES_PER_WORD-1);
		size &= ~(BYTES_PER_WORD-1);
		printk("%sForcing size word alignment - %s\n", func_nm, name);
	}
	
	align = BYTES_PER_WORD;
	if (flags & SLAB_HWCACHE_ALIGN)
		align = L1_CACHE_BYTES;

	/* 计算专用缓冲区的大小 */
	if (size >= (PAGE_SIZE>>3))
		/*
		 * Size is large, assume best to place the slab management obj
		 * off-slab (should allow better packing of objs).
		 */
		flags |= CFLGS_OFF_SLAB;

	if (flags & SLAB_HWCACHE_ALIGN) {
		/* Need to adjust size so that objs are cache aligned. */
		/* Small obj size, can get at least two per cache line. */
		/* FIXME: only power of 2 supported, was better */
		while (size < align/2)
			align /= 2;
		size = (size+align-1)&(~(align-1));
	}

	/* Cal size (in pages) of slabs, and the num of objs per slab.
	 * This could be made much more intelligent.  For now, try to avoid
	 * using high page-orders for slabs.  When the gfp() funcs are more
	 * friendly towards high-order requests, this should be changed.
	 */
	do {
		unsigned int break_flag = 0;
cal_wastage:
		kmem_cache_estimate(cachep->gfporder, size, flags,
						&left_over, &cachep->num);
		if (break_flag)
			break;
		if (cachep->gfporder >= MAX_GFP_ORDER)
			break;
		if (!cachep->num)
			goto next;
		if (flags & CFLGS_OFF_SLAB && cachep->num > offslab_limit) {
			/* Oops, this num of objs will cause problems. */
			cachep->gfporder--;
			break_flag++;
			goto cal_wastage;
		}

		/*
		 * Large num of objs is good, but v. large slabs are currently
		 * bad for the gfp()s.
		 */
		if (cachep->gfporder >= slab_break_gfp_order)
			break;

		if ((left_over*8) <= (PAGE_SIZE<<cachep->gfporder))
			break;	/* Acceptable internal fragmentation. */
next:
		cachep->gfporder++;
	} while (1);

	if (!cachep->num) {
		printk("kmem_cache_create: couldn't create cache %s.\n", name);
		kmem_cache_free(&cache_cache, cachep);
		cachep = NULL;
		goto opps;
	}
	slab_size = L1_CACHE_ALIGN(cachep->num*sizeof(kmem_bufctl_t)+sizeof(slab_t));

	/*
	 * If the slab has been placed off-slab, and we have enough space then
	 * move it on-slab. This is at the expense of any extra colouring.
	 */
	if (flags & CFLGS_OFF_SLAB && left_over >= slab_size) {
		flags &= ~CFLGS_OFF_SLAB;
		left_over -= slab_size;
	}

	/* Offset must be a multiple of the alignment. */
	offset += (align-1);
	offset &= ~(align-1);
	if (!offset)
		offset = L1_CACHE_BYTES;
	cachep->colour_off = offset;
	cachep->colour = left_over/offset;

	/* init remaining fields */
	if (!cachep->gfporder && !(flags & CFLGS_OFF_SLAB))
		flags |= CFLGS_OPTIMIZE;

	cachep->flags = flags;
	cachep->gfpflags = 0;
	if (flags & SLAB_CACHE_DMA)
		cachep->gfpflags |= GFP_DMA;
	spin_lock_init(&cachep->spinlock);
	cachep->objsize = size;
	INIT_LIST_HEAD(&cachep->slabs);
	cachep->firstnotfull = &cachep->slabs;

	if (flags & CFLGS_OFF_SLAB)
		cachep->slabp_cache = kmem_find_general_cachep(slab_size,0);
	cachep->ctor = ctor;
	cachep->dtor = dtor;
	/* Copy name over so we don't have problems with unloaded modules */
	strcpy(cachep->name, name);

	/* Need the semaphore to access the chain. */
	// down(&cache_chain_sem);
	{
		struct list_head *p;

		list_for_each(p, &cache_chain) {
			kmem_cache_t *pc = list_entry(p, kmem_cache_t, next);

			/* The name field is constant - no lock needed. */
			if (!strcmp(pc->name, name))
				BUG();
		}
	}

	/* There is no reason to lock our new cache before we
	 * link it in - no one knows about it yet...
	 */
	list_add(&cachep->next, &cache_chain);
	// up(&cache_chain_sem);
opps:
	return cachep;
}



/**
 * @brief 申请物理页
 * 
 * @param cachep 
 * @param flags 
 * @return void* 
 */
static inline void * kmem_getpages (kmem_cache_t *cachep, unsigned long flags)
{
	void	*addr;
	flags |= cachep->gfpflags;
	addr = (void*) __get_free_pages(flags, cachep->gfporder);
	return addr;
}

/**
 * @brief 给专用缓冲区内释放首虚拟地址为 addr,大小为 2^cachep->gfporder 的物理页
 * 
 * @param cachep 专用缓冲区
 * @param addr 要释放的第一个物理页的虚拟地址
 */
static inline void kmem_freepages (kmem_cache_t *cachep, void *addr)
{
	unsigned long i = (1<<cachep->gfporder);
	struct page *page = virt_to_page(addr);
	// 将物理页清除 slab 标志
	while (i--) {
		PageClearSlab(page);
		page++;
	}
	free_pages((unsigned long)addr, cachep->gfporder);
}

/**
 * @brief 初始化 slab 的所有对象
 * 
 * @param cachep 专用缓冲区
 * @param slabp slab 指针
 * @param ctor_flags 对象构造标志位
 */
static inline void kmem_cache_init_objs (kmem_cache_t * cachep,
			slab_t * slabp, unsigned long ctor_flags)
{
	int i;
	// 初始化一个 slab 的所有对象
	for (i = 0; i < cachep->num; i++) {
		void* objp = slabp->s_mem+cachep->objsize*i;
		if (cachep->ctor)
			cachep->ctor(objp, cachep, ctor_flags);
            
		slab_bufctl(slabp)[i] = i+1;
	}
	slab_bufctl(slabp)[i-1] = BUFCTL_END;
	slabp->free = 0;
}


/**
 * @brief 根据专用缓冲区与对象指针获取对应的 slab
 * 
 * @param cachep 专用缓冲区
 * @param objp 对象指针
 * @param colour_off 
 * @param local_flags 
 * @return slab_t* 对应的 slab
 */
static inline slab_t * kmem_cache_slabmgmt (kmem_cache_t *cachep,
			void *objp, int colour_off, int local_flags)
{
	slab_t *slabp;
	
	if (OFF_SLAB(cachep)) {
		/* Slab management obj is off-slab. */
		slabp = kmem_cache_alloc(cachep->slabp_cache, local_flags);
		if (!slabp)
			return NULL;
	} else {
		/* FIXME: change to
			slabp = objp
		 * if you enable OPTIMIZE
		 */
		slabp = objp+colour_off;
		colour_off += L1_CACHE_ALIGN(cachep->num *
				sizeof(kmem_bufctl_t) + sizeof(slab_t));
	}
	slabp->inuse = 0;
	slabp->colouroff = colour_off;
	slabp->s_mem = objp+colour_off;

	return slabp;
}

static int kmem_cache_grow (kmem_cache_t * cachep, int flags)
{
	slab_t	*slabp;
	struct page	*page;
	void		*objp;
	size_t		 offset;
	unsigned int	 i, local_flags;
	unsigned long	 ctor_flags;
	unsigned long	 save_flags;

	/* Be lazy and only check for valid flags here,
 	 * keeping it out of the critical path in kmem_cache_alloc().
	 */
	if (flags & ~(SLAB_DMA|SLAB_LEVEL_MASK|SLAB_NO_GROW))
		BUG();
	if (flags & SLAB_NO_GROW)
		return 0;

	/*
	 * The test for missing atomic flag is performed here, rather than
	 * the more obvious place, simply to reduce the critical path length
	 * in kmem_cache_alloc(). If a caller is seriously mis-behaving they
	 * will eventually be caught here (where it matters).
	 */
	if (in_interrupt() && (flags & SLAB_LEVEL_MASK) != SLAB_ATOMIC)
		BUG();

	ctor_flags = SLAB_CTOR_CONSTRUCTOR;
	local_flags = (flags & SLAB_LEVEL_MASK);
	if (local_flags == SLAB_ATOMIC)
		/*
		 * Not allowed to sleep.  Need to tell a constructor about
		 * this - it might need to know...
		 */
		ctor_flags |= SLAB_CTOR_ATOMIC;

	/* About to mess with non-constant members - lock. */
	spin_lock_irqsave(&cachep->spinlock, save_flags);

	/* Get colour for the slab, and cal the next value. */
	offset = cachep->colour_next;
	cachep->colour_next++;
	if (cachep->colour_next >= cachep->colour)
		cachep->colour_next = 0;
	offset *= cachep->colour_off;
	cachep->dflags |= DFLGS_GROWN;

	cachep->growing++;
	spin_unlock_irqrestore(&cachep->spinlock, save_flags);

	/* A series of memory allocations for a new slab.
	 * Neither the cache-chain semaphore, or cache-lock, are
	 * held, but the incrementing c_growing prevents this
	 * cache from being reaped or shrunk.
	 * Note: The cache could be selected in for reaping in
	 * kmem_cache_reap(), but when the final test is made the
	 * growing value will be seen.
	 */

	/* Get mem for the objs. */
	if (!(objp = kmem_getpages(cachep, flags)))
		goto failed;

	/* Get slab management. */
	if (!(slabp = kmem_cache_slabmgmt(cachep, objp, offset, local_flags)))
		goto opps1;

	// 设置物理页标志位
	i = 1 << cachep->gfporder;
	page = virt_to_page(objp);
	do {
		SET_PAGE_CACHE(page, cachep);
		SET_PAGE_SLAB(page, slabp);
		PageSetSlab(page);
		page++;
	} while (--i);
	// 初始化 slab 的所有对象
	kmem_cache_init_objs(cachep, slabp, ctor_flags);

	spin_lock_irqsave(&cachep->spinlock, save_flags);
	cachep->growing--;

	/* Make slab active. */
	list_add_tail(&slabp->list,&cachep->slabs);
	if (cachep->firstnotfull == &cachep->slabs)
		cachep->firstnotfull = &slabp->list;
	STATS_INC_GROWN(cachep);
	cachep->failures = 0;

	spin_unlock_irqrestore(&cachep->spinlock, save_flags);
	return 1;
opps1:
	kmem_freepages(cachep, objp);
failed:
	spin_lock_irqsave(&cachep->spinlock, save_flags);
	cachep->growing--;
	spin_unlock_irqrestore(&cachep->spinlock, save_flags);
	return 0;
}

/**
 * @brief 向专用缓冲区的指定 slab 申请内存
 * 
 * @param cachep 专用缓冲区
 * @param slabp slab指针
 * @return void* 申请到的内存,虚拟地址
 */
static inline void * kmem_cache_alloc_one_tail (kmem_cache_t *cachep,
							 slab_t *slabp)
{
	void *objp;

	STATS_INC_ALLOCED(cachep);
	STATS_INC_ACTIVE(cachep);
	STATS_SET_HIGH(cachep);

	/* get obj pointer */
	slabp->inuse++;
	// 获取对象指针
	objp = slabp->s_mem + slabp->free*cachep->objsize;
	// 更新空闲内存链表
	slabp->free=slab_bufctl(slabp)[slabp->free];

	if (slabp->free == BUFCTL_END)
		/* slab now full: move to next slab for next alloc */
		cachep->firstnotfull = slabp->list.next;
	return objp;
}


/**
 * @brief 向专用缓冲区申请内存
 * @param cachep 专用缓冲区
 */
#define kmem_cache_alloc_one(cachep)				\
({								\
	slab_t	*slabp;					\
								\
	/* Get slab alloc is to come from. */			\
	{							\
		struct list_head* p = cachep->firstnotfull;	\
		if (p == &cachep->slabs)			\
			goto alloc_new_slab;			\
		slabp = list_entry(p,slab_t, list);	\
	}							\
	kmem_cache_alloc_one_tail(cachep, slabp);		\
})

static inline void kmem_cache_alloc_head(kmem_cache_t *cachep, int flags) {}

/**
 * @brief 向特定 kmem_cache_t 申请内存
 * 
 * @param cachep 
 * @param flags 
 * @return void* 申请到的内存,如若失败则为 NULL
 */
static inline void * __kmem_cache_alloc (kmem_cache_t *cachep, int flags)
{
	unsigned long save_flags;
	void* objp;

	kmem_cache_alloc_head(cachep, flags);
// 申请内存
try_again:
	local_irq_save(save_flags);
	objp = kmem_cache_alloc_one(cachep);
	local_irq_restore(save_flags);
	return objp;
// 创建一个新 slab
alloc_new_slab:
	local_irq_restore(save_flags);
	if (kmem_cache_grow(cachep, flags))
		goto try_again;
	return NULL;
}

/**
 * @brief 向专用缓冲区释放内存
 * 
 * @param cachep 专用缓冲区
 * @param objp 待释放的内存的虚拟地址
 */
static inline void kmem_cache_free_one(kmem_cache_t *cachep, void *objp)
{
	slab_t* slabp;

	CHECK_PAGE(virt_to_page(objp));
	slabp = GET_PAGE_SLAB(virt_to_page(objp));
	{
		// 计算出这块内存的偏移量
		unsigned int objnr = (objp-slabp->s_mem)/cachep->objsize;
		// 更新空闲内存表
		slab_bufctl(slabp)[objnr] = slabp->free;
		slabp->free = objnr;
	}
	STATS_DEC_ACTIVE(cachep);
	
	/* fixup slab chain */
	if (slabp->inuse-- == cachep->num)
		goto moveslab_partial;
	if (!slabp->inuse)
		goto moveslab_free;
	return;

moveslab_partial:
	{
		struct list_head *t = cachep->firstnotfull;

		cachep->firstnotfull = &slabp->list;
		if (slabp->list.next == t)
			return;
		list_del(&slabp->list);
		list_add_tail(&slabp->list, t);
		return;
	}
moveslab_free:
	{
		struct list_head *t = cachep->firstnotfull->prev;

		list_del(&slabp->list);
		list_add_tail(&slabp->list, &cachep->slabs);
		if (cachep->firstnotfull == &slabp->list)
			cachep->firstnotfull = t->next;
		return;
	}
}

/**
 * @brief 向专用缓冲区释放内存
 * 
 * @param cachep 专用缓冲区
 * @param objp 待释放的内存的虚拟地址
 */
static inline void __kmem_cache_free (kmem_cache_t *cachep, void* objp)
{
	kmem_cache_free_one(cachep, objp);
}

/**
 * @brief 向特定 kmem_cache_t 申请内存
 * 
 * @param cachep 
 * @param flags 
 * @return void* 申请到的内存,如若失败则为 NULL
 */
void * kmem_cache_alloc (kmem_cache_t *cachep, int flags)
{
	return __kmem_cache_alloc(cachep, flags);
}

/**
 * @brief 向特定 kmem_cache_t 释放内存
 * 
 * @param cachep 专用缓冲区
 * @param objp 待释放内存的虚拟地址
 */
void kmem_cache_free (kmem_cache_t *cachep, void *objp)
{
	unsigned long flags;

	local_irq_save(flags);
	__kmem_cache_free(cachep, objp);
	local_irq_restore(flags);
}

/**
 * @brief 向 cache_sizes 申请内存(非指定缓冲区)
 * 
 * @param size 要申请的字节数
 * @param flags 
 * @return void* 虚拟地址
 */
void * kmalloc (size_t size, int flags)
{
	cache_sizes_t *csizep = cache_sizes;

	for (; csizep->cs_size; csizep++) {
		if (size > csizep->cs_size)
			continue;
		return __kmem_cache_alloc(flags & GFP_DMA ?
			 csizep->cs_dmacachep : csizep->cs_cachep, flags);
	}
	BUG(); // too big size
	return NULL;
}

/**
 * @brief 向 cache_sizes  释放内存
 * 
 * @param objp 要释放内存的虚拟地址
 */
void kfree (const void *objp)
{
	kmem_cache_t *c;
	unsigned long flags;

	if (!objp)
		return;
	local_irq_save(flags);
	CHECK_PAGE(virt_to_page(objp));
	c = GET_PAGE_CACHE(virt_to_page(objp));
	__kmem_cache_free(c, (void*)objp);
	local_irq_restore(flags);
}

#define drain_cpu_caches(cachep)	do { } while (0)

static void kmem_slab_destroy (kmem_cache_t *cachep, slab_t *slabp)
{
	if (cachep->dtor
	) {
		int i;
		for (i = 0; i < cachep->num; i++) {
			void* objp = slabp->s_mem+cachep->objsize*i;
			if (cachep->dtor)
				(cachep->dtor)(objp, cachep, 0);
		}
	}

	kmem_freepages(cachep, slabp->s_mem-slabp->colouroff);
	if (OFF_SLAB(cachep))
		kmem_cache_free(cachep->slabp_cache, slabp);
}


static int __kmem_cache_shrink(kmem_cache_t *cachep)
{
	slab_t *slabp;
	int ret;

	drain_cpu_caches(cachep);

	spin_lock_irq(&cachep->spinlock);

	/* If the cache is growing, stop shrinking. */
	while (!cachep->growing) {
		struct list_head *p;

		p = cachep->slabs.prev;
		if (p == &cachep->slabs)
			break;

		slabp = list_entry(cachep->slabs.prev, slab_t, list);
		if (slabp->inuse)
			break;

		list_del(&slabp->list);
		if (cachep->firstnotfull == &slabp->list)
			cachep->firstnotfull = &cachep->slabs;

		spin_unlock_irq(&cachep->spinlock);
		kmem_slab_destroy(cachep, slabp);
		spin_lock_irq(&cachep->spinlock);
	}
	ret = !list_empty(&cachep->slabs);
	spin_unlock_irq(&cachep->spinlock);
	return ret;
}

int kmem_cache_destroy (kmem_cache_t * cachep)
{
	if (!cachep || in_interrupt() || cachep->growing)
		BUG();

	/* Find the cache in the chain of caches. */
	// down(&cache_chain_sem);
	/* the chain is never empty, cache_cache is never destroyed */
	if (clock_searchp == cachep)
		clock_searchp = list_entry(cachep->next.next,
						kmem_cache_t, next);
	list_del(&cachep->next);
	// up(&cache_chain_sem);

	if (__kmem_cache_shrink(cachep)) {
		printk("kernel error -> kmem_cache_destroy: Can't free all objects %p\n",
		       cachep);
		// down(&cache_chain_sem);
		list_add(&cachep->next,&cache_chain);
		// up(&cache_chain_sem);
		return 1;
	}
	kmem_cache_free(&cache_cache, cachep);

	return 0;
}

/**
 * @brief 初始化非专用缓冲区列表 cache_sizes
 * 
 */
void __init kmem_cache_sizes_init(void)
{
	cache_sizes_t *sizes = cache_sizes;
	char name[20];
	/*
	 * Fragmentation resistance on low memory - only use bigger
	 * page orders on machines with more than 32MB of memory.
	 */
	if (num_physpages > (32 << 20) >> PAGE_SHIFT)
		slab_break_gfp_order = BREAK_GFP_ORDER_HI;
	do {
		/* For performance, all the general caches are L1 aligned.
		 * This should be particularly beneficial on SMP boxes, as it
		 * eliminates "false sharing".
		 * Note for systems short on memory removing the alignment will
		 * allow tighter packing of the smaller caches. */
		sprintf(name,"size-%Zd",sizes->cs_size);
		if (!(sizes->cs_cachep =
			kmem_cache_create(name, sizes->cs_size,
					0, SLAB_HWCACHE_ALIGN, NULL, NULL))) {
			BUG();
		}

		/* Inc off-slab bufctl limit until the ceiling is hit. */
		if (!(OFF_SLAB(sizes->cs_cachep))) {
			offslab_limit = sizes->cs_size-sizeof(slab_t);
			offslab_limit /= 2;
		}
		sprintf(name, "size-%Zd(DMA)",sizes->cs_size);
		sizes->cs_dmacachep = kmem_cache_create(name, sizes->cs_size, 0,
			      SLAB_CACHE_DMA|SLAB_HWCACHE_ALIGN, NULL, NULL);
		if (!sizes->cs_dmacachep)
			BUG();
		sizes++;
	} while (sizes->cs_size);
}

void __init kmem_cache_init(void)
{
	size_t left_over;

	// init_MUTEX(&cache_chain_sem);
	INIT_LIST_HEAD(&cache_chain);

	kmem_cache_estimate(0, cache_cache.objsize, 0,
			&left_over, &cache_cache.num);
	if (!cache_cache.num)
		BUG();

	cache_cache.colour = left_over/cache_cache.colour_off;
	cache_cache.colour_next = 0;
}