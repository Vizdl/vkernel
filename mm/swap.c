#include <vkernel/swap.h>

int memory_pressure;

freepages_t freepages = {
	0,	/* freepages.min */
	0,	/* freepages.low */
	0	/* freepages.high */
};