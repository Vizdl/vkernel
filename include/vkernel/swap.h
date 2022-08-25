#ifndef _LINUX_SWAP_H
#define _LINUX_SWAP_H

#ifdef __KERNEL__
#define PAGE_AGE_START 2

extern int memory_pressure;

typedef struct freepages_v1
{
	unsigned int	min;
	unsigned int	low;
	unsigned int	high;
} freepages_v1;
typedef freepages_v1 freepages_t;
extern freepages_t freepages;

#endif /* __KERNEL__*/

#endif /* _LINUX_SWAP_H */