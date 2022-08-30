#ifndef _LINUX_LDT_H
#define _LINUX_LDT_H


#ifndef __ASSEMBLY__

extern void set_ldt_desc(unsigned int n, void *addr, unsigned int size);

#endif /* !__ASSEMBLY__ */
#endif
