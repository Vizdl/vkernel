#include <vkernel/bootmem.h>
#include <vkernel/mmzone.h>

static bootmem_data_t contig_bootmem_data;
pg_data_t contig_page_data = { bdata: &contig_bootmem_data };