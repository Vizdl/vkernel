#ifndef _ASM_IO_H
#define _ASM_IO_H

#ifdef __KERNEL__

extern unsigned long virt_to_phys(volatile void * address);
extern void * phys_to_virt(unsigned long address);

extern  void outsb(unsigned short port, const void * addr, unsigned long count);
extern  void outsw(unsigned short port, const void * addr, unsigned long count);
extern  void outsl(unsigned short port, const void * addr, unsigned long count);

extern  void insb(unsigned short port, void * addr, unsigned long count);
extern  void insw(unsigned short port, void * addr, unsigned long count);
extern  void insl(unsigned short port, void * addr, unsigned long count);

extern  void outb(unsigned char value, unsigned short port);
extern  void outw(unsigned short value, unsigned short port);
extern  void outl(unsigned int value, unsigned short port);

extern  void outb_p(unsigned char value, unsigned short port);
extern  void outw_p(unsigned short value, unsigned short port);
extern  void outl_p(unsigned int value, unsigned short port);

extern  unsigned char inb(unsigned short port);
extern  unsigned short inw(unsigned short port);
extern  unsigned int inl(unsigned short port);

extern  unsigned char inb_p(unsigned short port);
extern  unsigned short inw_p(unsigned short port);
extern  unsigned int inl_p(unsigned short port);

#endif /* __KERNEL__ */

#endif
