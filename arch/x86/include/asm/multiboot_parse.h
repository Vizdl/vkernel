#ifndef _LINUX_MULTIBOOT_PARSE_H
#define _LINUX_MULTIBOOT_PARSE_H
#include <asm/multiboot2.h>

extern struct multiboot_tag * multiboot_memory_map;

void multiboot_save (unsigned long addr);

#endif // _LINUX_MULTIBOOT_PARSE_H