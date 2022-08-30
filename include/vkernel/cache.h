#ifndef __LINUX_CACHE_H
#define __LINUX_CACHE_H

#include <asm/cache.h>

#ifndef L1_CACHE_ALIGN
#define L1_CACHE_ALIGN(x) (((x)+(L1_CACHE_BYTES-1))&~(L1_CACHE_BYTES-1))
#endif

#ifndef SMP_CACHE_BYTES
#define SMP_CACHE_BYTES L1_CACHE_BYTES

#define ____cacheline_aligned __attribute__((__aligned__(SMP_CACHE_BYTES)))

#define __cacheline_aligned ____cacheline_aligned

#endif

#endif /* __LINUX_CACHE_H */
