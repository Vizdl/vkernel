#ifndef _I386_PGTABLE_H
#define _I386_PGTABLE_H

/*
 * The 4MB page is guessing..  Detailed in the infamous "Chapter H"
 * of the Pentium details, but assuming intel did the straightforward
 * thing, this bit set in the page directory entry just means that
 * the page directory entry points directly to a 4MB-aligned block of
 * memory. 
 */
#define _PAGE_BIT_PRESENT	0
#define _PAGE_BIT_RW		1
#define _PAGE_BIT_USER		2
#define _PAGE_BIT_PWT		3
#define _PAGE_BIT_PCD		4
#define _PAGE_BIT_ACCESSED	5
#define _PAGE_BIT_DIRTY		6
#define _PAGE_BIT_PSE		7	/* 4 MB (or 2MB) page, Pentium+, if present.. */
#define _PAGE_BIT_GLOBAL	8	/* Global TLB entry PPro+ */

// 页面在内存中并且没有被换出
#define _PAGE_PRESENT	    0x001
// 页面可写
#define _PAGE_RW	        0x002
// 页面可以从用户空间访问
#define _PAGE_USER	        0x004
#define _PAGE_PWT	        0x008
#define _PAGE_PCD	        0x010
// 页面被访问过
#define _PAGE_ACCESSED	    0x020
// 页面被写过
#define _PAGE_DIRTY	        0x040
#define _PAGE_PSE	        0x080	/* 4 MB (or 2MB) page, Pentium+, if present.. */
#define _PAGE_GLOBAL	    0x100	/* Global TLB entry PPro+ */
// 页面在内存中, 但是不可访问
#define _PAGE_PROTNONE	    0x080	/* If not present */

#endif /* _I386_PGTABLE_H */