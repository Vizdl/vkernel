#include <vkernel/linkage.h>
#include <vkernel/init.h>
#include <vkernel/kernel.h>
#include <vkernel/bootmem.h>
#include <vkernel/mmzone.h>

void __init setup_arch(void)
{
    contig_page_data.bdata->node_boot_start = 1;
    printk("setup arch...\n");
    return;
}