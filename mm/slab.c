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
#include <vkernel/interrupt.h>

typedef unsigned int kmem_bufctl_t;

typedef struct slab_s {
	struct list_head	list;		// 将 slab 链入一个 kmem_cache_t
	unsigned long		colouroff;	// 本 slab 着色区的大小
	void				*s_mem;		// 指向对象区的起点,对象内存初始化为着色区之后
	unsigned int		inuse;		// 已分配对象的计数器
	// 从这里开始到特定长度,都是空闲列表指针
	kmem_bufctl_t		free;		// 空闲对象链中的第一个对象的下标,需要乘以对象大小才能得到真正的空闲对象,如若为空,则表示为 BUFCTL_END
} slab_t;

// 核查页
# define CHECK_PAGE(pg)	do { } while (0)

// 标志位
#define	CFLGS_OFF_SLAB	0x010000UL	// slab 管理结构内存是通过 kmem_cache_alloc 申请的,还是在 slab 管理页内。
#define	CFLGS_OPTIMIZE	0x020000UL
#define	OFF_SLAB(x)	((x)->flags & CFLGS_OFF_SLAB)
#define	OPTIMIZE(x)	((x)->flags & CFLGS_OPTIMIZE)

#define	DFLGS_GROWN	0x000001UL
#define	GROWN(x)	((x)->dlags & DFLGS_GROWN)

// kmem_bufctl_t
#define BUFCTL_END 0xffffFFFF
#define	SLAB_LIMIT 0xffffFFFE
#define slab_bufctl(slabp) \
	((kmem_bufctl_t *)(((slab_t*)slabp)+1))

// 专用缓冲区最小的大小(单位:字节)
#define	BYTES_PER_WORD		sizeof(void *)
// kmalloc 最大可申请对象为 2^5 = 32 个物理页
#define	MAX_OBJ_ORDER	5	/* 32 pages */
// kmem_cache_t 最大页帧的 order
#define	MAX_GFP_ORDER	5	/* 32 pages */
// kmem_cache_t 管理结构名最大长度
#define CACHE_NAMELEN	20
// 设置和获得物理页所属的 kmem_cache_t
#define	SET_PAGE_CACHE(pg,x)  ((pg)->list.next = (struct list_head *)(x))
#define	GET_PAGE_CACHE(pg)    ((kmem_cache_t *)(pg)->list.next)
// 设置和获得物理页所属的 slab
#define	SET_PAGE_SLAB(pg,x)   ((pg)->list.prev = (struct list_head *)(x))
#define	GET_PAGE_SLAB(pg)     ((slab_t *)(pg)->list.prev)

#define	BREAK_GFP_ORDER_HI	2
#define	BREAK_GFP_ORDER_LO	1
static int slab_break_gfp_order = BREAK_GFP_ORDER_LO;

# define CREATE_MASK	(SLAB_HWCACHE_ALIGN | SLAB_NO_REAP | SLAB_CACHE_DMA)

// 专用缓冲区
typedef struct kmem_cache_s kmem_cache_t;
struct kmem_cache_s {
	struct list_head	slabs;			// slab 链表(该缓冲区管理的所有 slab 都会在这里)
	struct list_head	*firstnotfull;	// 第一块有空闲对象的 slab 的指针,当这个指针指向 slabs 则表示没有空闲对象的 slab。
	unsigned int		objsize;		// 每个对象的大小
	unsigned int	 	flags;	        /* constant flags */
	unsigned int		num;			// 每个 slab 的对象数
	spinlock_t		    spinlock;

	unsigned int		gfporder;		// 单次申请 2^gfporder 大小个物理页
	unsigned int		gfpflags;		// 申请物理页时使用的 flags

	// 通过设置着色区使每个 slab 对象都按高速缓存中的缓存行大小对齐
	// slab 的内存本来就是页对齐的,所以也是缓存行大小对齐
	// 但是为了使不同 slab 上同一项对位置的对象的起始地址在高速缓存中互相错开
	// (这样可以改善高速缓存的效率),所以尽可能将着色区安排成不同的大小。
	size_t			    colour;		    /* cache colouring range */
	unsigned int		colour_off;	    /* colour offset */
	unsigned int		colour_next;	// slab->colouroff = colour_off * colour_next, colour_next 每申请一个 slab 自增,超过 colour 时变回 0。
	kmem_cache_t		*slabp_cache;	// 申请 slab 的专用缓冲区(并非链表之类的结构)
	unsigned int		growing;		// 该缓冲区的 slab 的个数
	unsigned int		dflags;		    /* dynamic flags */

	// 初始化缓冲内部结构
	void (*ctor)(void *, kmem_cache_t *, unsigned long);

	// 释放缓冲内部结构
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

#define cache_chain (cache_cache.next)

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

/**
 * @brief 根据 size 和 flags 判断使用 cache_sizes 中 哪个 kmem_cache_t
 * 
 * @param size 申请的内存大小
 * @param gfpflags 申请内存的 flags
 * @return kmem_cache_t* 选中的 kmem_cache_t
 */
kmem_cache_t * kmem_find_general_cachep (size_t size, int gfpflags)
{
	cache_sizes_t *csizep = cache_sizes;

	// 根据大小选择对应的 cache_sizes_t
	for ( ; csizep->cs_size; csizep++) {
		if (size > csizep->cs_size)
			continue;
		break;
	}
	return (gfpflags & GFP_DMA) ? csizep->cs_dmacachep : csizep->cs_cachep;
}

/**
 * @brief 创建一个专用缓冲区并返回
 * 
 * @param name 专用缓冲区名
 * @param size 专用缓冲区对象大小
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

	// 1. 参数检查
	if ((!name) ||
		((strlen(name) >= CACHE_NAMELEN - 1)) ||
		in_interrupt() ||
		(size < BYTES_PER_WORD) ||
		(size > (1<<MAX_OBJ_ORDER)*PAGE_SIZE) ||
		(dtor && !ctor) ||
		(offset < 0 || offset > size))
			BUG();

	if (flags & ~CREATE_MASK)
		BUG();

	// 2. 申请缓冲区描述对象并初始化
	cachep = (kmem_cache_t *) kmem_cache_alloc(&cache_cache, SLAB_KERNEL);
	if (!cachep)
		goto opps;
	memset(cachep, 0, sizeof(kmem_cache_t));


	// 3. 设置专用缓冲区参数
	if (size & (BYTES_PER_WORD-1)) {
		size += (BYTES_PER_WORD-1);
		size &= ~(BYTES_PER_WORD-1);
		printk("%sForcing size word alignment - %s\n", func_nm, name);
	}
	
	align = BYTES_PER_WORD;
	if (flags & SLAB_HWCACHE_ALIGN)
		align = L1_CACHE_BYTES;

	if (size >= (PAGE_SIZE>>3))
		flags |= CFLGS_OFF_SLAB;

	if (flags & SLAB_HWCACHE_ALIGN) {
		while (size < align/2)
			align /= 2;
		size = (size+align-1)&(~(align-1));
	}

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
	
	strcpy(cachep->name, name);
	// 4. 判断是否有 name 冲突
	{
		struct list_head *p;

		list_for_each(p, &cache_chain) {
			kmem_cache_t *pc = list_entry(p, kmem_cache_t, next);

			/* The name field is constant - no lock needed. */
			if (!strcmp(pc->name, name))
				BUG();
		}
	}

	// 5. 将新生成的缓冲区插入链表
	list_add(&cachep->next, &cache_chain);
opps:
	return cachep;
}



/**
 * @brief 申请虚拟内存(2^cachep->gfporder 个物理页大小)
 * 
 * @param cachep 缓冲区
 * @param flags 标志位
 * @return void* 申请到的内存的虚拟地址
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
 * @brief 创建 slab 管理结构
 * 
 * @param cachep slab 所属的专用缓冲区
 * @param objp 该 slab 管理的虚拟内存指针
 * @param colour_off 
 * @param local_flags 
 * @return slab_t* 对应的 slab
 */
static inline slab_t * kmem_cache_slabmgmt (kmem_cache_t *cachep,
			void *objp, int colour_off, int local_flags)
{
	slab_t *slabp;
	// 如若 slab 结构不在 slab 所属管理页内
	if (OFF_SLAB(cachep)) {
		slabp = kmem_cache_alloc(cachep->slabp_cache, local_flags);
		if (!slabp)
			return NULL;
	} else {
		slabp = objp+colour_off;
		colour_off += L1_CACHE_ALIGN(cachep->num *
				sizeof(kmem_bufctl_t) + sizeof(slab_t));
	}
	slabp->inuse = 0;
	slabp->colouroff = colour_off;
	slabp->s_mem = objp+colour_off;

	return slabp;
}

/**
 * @brief 给专用缓冲区添加 slab
 * 
 * @param cachep 待添加 slab 的专用缓冲区
 * @param flags 
 * @return int 
 */
static int kmem_cache_grow (kmem_cache_t * cachep, int flags)
{
	slab_t	*slabp;
	struct page	*page;
	void		*objp;
	size_t		 offset;
	unsigned int	 i, local_flags;
	unsigned long	 ctor_flags;
	unsigned long	 save_flags;

	if (flags & ~(SLAB_DMA|SLAB_LEVEL_MASK|SLAB_NO_GROW))
		BUG();
	if (flags & SLAB_NO_GROW)
		return 0;

	if (in_interrupt() && (flags & SLAB_LEVEL_MASK) != SLAB_ATOMIC)
		BUG();

	ctor_flags = SLAB_CTOR_CONSTRUCTOR;
	local_flags = (flags & SLAB_LEVEL_MASK);
	if (local_flags == SLAB_ATOMIC)
		ctor_flags |= SLAB_CTOR_ATOMIC;

	spin_lock_irqsave(&cachep->spinlock, save_flags);

	offset = cachep->colour_next;
	cachep->colour_next++;
	if (cachep->colour_next >= cachep->colour)
		cachep->colour_next = 0;
	offset *= cachep->colour_off;
	cachep->dflags |= DFLGS_GROWN;

	cachep->growing++;
	spin_unlock_irqrestore(&cachep->spinlock, save_flags);

	// 获得虚拟内存
	if (!(objp = kmem_getpages(cachep, flags)))
		goto failed;

	// 创建 slab 管理结构并初始化
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
try_again:
	local_irq_save(save_flags);
	// 申请内存
	objp = kmem_cache_alloc_one(cachep);
	local_irq_restore(save_flags);
	return objp;
alloc_new_slab:
	local_irq_restore(save_flags);
	// 创建一个新 slab
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
	// 根据虚拟内存地址找到 slab 管理结构
	slabp = GET_PAGE_SLAB(virt_to_page(objp));
	{
		// 计算出这块内存的偏移量
		unsigned int objnr = (objp-slabp->s_mem)/cachep->objsize;
		// 更新空闲内存表
		slab_bufctl(slabp)[objnr] = slabp->free;
		slabp->free = objnr;
	}
	
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
	// 1. 遍历 cache_sizes 找到大小最接近的缓冲区
	for (; csizep->cs_size; csizep++) {
		if (size > csizep->cs_size)
			continue;
		// 1.1 通过大小最接近的缓冲区申请内存并返回
		return __kmem_cache_alloc(flags & GFP_DMA ?
			 csizep->cs_dmacachep : csizep->cs_cachep, flags);
	}
	// 2. 如若申请的内存比 cache_sizes 最大的缓冲区还大
	BUG();
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

/**
 * @brief 销毁缓冲区 cachep 的 slab
 * 
 * @param cachep 
 * @param slabp 
 */
static void kmem_slab_destroy (kmem_cache_t *cachep, slab_t *slabp)
{
	// 1. 释放每块缓冲内部结构
	if (cachep->dtor
	) {
		int i;
		for (i = 0; i < cachep->num; i++) {
			void* objp = slabp->s_mem+cachep->objsize*i;
			if (cachep->dtor)
				(cachep->dtor)(objp, cachep, 0);
		}
	}
	// 2. 释放 slab 管理的内存页
	kmem_freepages(cachep, slabp->s_mem-slabp->colouroff);
	// 3. 释放缓冲区内存(根据 slab 管理结构的内存位置决定是否要释放)
	if (OFF_SLAB(cachep))
		kmem_cache_free(cachep->slabp_cache, slabp);
}

/**
 * @brief 销毁缓冲区内部结构
 * 
 * @param cachep 待销毁缓冲区
 * @return int 返回码
 */
static int __kmem_cache_shrink(kmem_cache_t *cachep)
{
	slab_t *slabp;
	int ret;

	drain_cpu_caches(cachep);

	spin_lock_irq(&cachep->spinlock);

	// 销毁所有的 slab
	while (!cachep->growing) {
		struct list_head *p;
		// 获得最后一个 slab
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
		// 销毁 slab
		kmem_slab_destroy(cachep, slabp);
		spin_lock_irq(&cachep->spinlock);
	}
	ret = !list_empty(&cachep->slabs);
	spin_unlock_irq(&cachep->spinlock);
	return ret;
}

/**
 * @brief 销毁指定缓冲区
 * 
 * @param cachep 待销毁的缓冲区
 * @return int 返回码
 */
int kmem_cache_destroy (kmem_cache_t * cachep)
{
	if (!cachep || in_interrupt() || cachep->growing)
		BUG();

	// 1. 将 cachep 从链表中删除
	if (clock_searchp == cachep)
		clock_searchp = list_entry(cachep->next.next,
						kmem_cache_t, next);
	list_del(&cachep->next);
	// 2. 销毁缓冲区内部结构
	if (__kmem_cache_shrink(cachep)) {
		printk("kernel error -> kmem_cache_destroy: Can't free all objects %p\n",
		       cachep);
		list_add(&cachep->next,&cache_chain);
		return 1;
	}
	// 3. 释放 kmem_cache_t 内存
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

	if (num_physpages > (32 << 20) >> PAGE_SHIFT)
		slab_break_gfp_order = BREAK_GFP_ORDER_HI;
	// 1. 遍历 cache_sizes 数组, 并挨个 cache_sizes_t 进行初始化。
	do {
		sprintf(name,"size-%Zd",sizes->cs_size);
		// 1.1 为 cache_sizes_t 创建专用缓冲区(使用普通内存)
		if (!(sizes->cs_cachep =
			kmem_cache_create(name, sizes->cs_size,
					0, SLAB_HWCACHE_ALIGN, NULL, NULL))) {
			BUG();
		}

		if (!(OFF_SLAB(sizes->cs_cachep))) {
			offslab_limit = sizes->cs_size-sizeof(slab_t);
			offslab_limit /= 2;
		}
		sprintf(name, "size-%Zd(DMA)",sizes->cs_size);
		// 1.2 建立 dma 专用缓冲区(使用 DMA 内存)
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
	// 1. 初始化 cache_cache
	INIT_LIST_HEAD(&cache_chain);
	kmem_cache_estimate(0, cache_cache.objsize, 0,
			&left_over, &cache_cache.num);
	if (!cache_cache.num)
		BUG();
	cache_cache.colour = left_over/cache_cache.colour_off;
	cache_cache.colour_next = 0;
}