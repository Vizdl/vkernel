#ifndef _ASM_DMA_H
#define _ASM_DMA_H

#include <vkernel/config.h>

/* The maximum address that we can perform a DMA transfer to on this platform */
#define MAX_DMA_ADDRESS      (PAGE_OFFSET+0x1000000)

#endif /* _ASM_DMA_H */