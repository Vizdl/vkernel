#include <asm/page.h>

extern inline unsigned long virt_to_phys(volatile void * address)
{
	return __pa(address);
}

extern inline void * phys_to_virt(unsigned long address)
{
	return __va(address);
}

void outsb(unsigned short port, const void * addr, unsigned long count)
{
    __asm__ __volatile__ ("rep ; outsb": "=S" (addr), "=c" (count) : "d" (port),"0" (addr),"1" (count));
}
void outsw(unsigned short port, const void * addr, unsigned long count)
{
    __asm__ __volatile__ ("rep ; outsw": "=S" (addr), "=c" (count) : "d" (port),"0" (addr),"1" (count));
}
void outsl(unsigned short port, const void * addr, unsigned long count)
{
    __asm__ __volatile__ ("rep ; outsl": "=S" (addr), "=c" (count) : "d" (port),"0" (addr),"1" (count));
}


void insb(unsigned short port, void * addr, unsigned long count)
{
    __asm__ __volatile__ ("rep ; insb": "=D" (addr), "=c" (count) : "d" (port),"0" (addr),"1" (count));
}
void insw(unsigned short port, void * addr, unsigned long count)
{
    __asm__ __volatile__ ("rep ; insw": "=D" (addr), "=c" (count) : "d" (port),"0" (addr),"1" (count));
}
void insl(unsigned short port, void * addr, unsigned long count)
{
    __asm__ __volatile__ ("rep ; insl": "=D" (addr), "=c" (count) : "d" (port),"0" (addr),"1" (count));
}

unsigned char inb(unsigned short port)
{
    unsigned char _v;
    __asm__ __volatile__ ("inb %w1,%0": "=a" (_v) : "Nd" (port));
    return _v;
}
unsigned short inw(unsigned short port)
{
    unsigned short _v;
    __asm__ __volatile__ ("inw %w1,%0": "=a" (_v) : "Nd" (port));
    return _v;
}
unsigned int inl(unsigned short port)
{
    unsigned int _v;
    __asm__ __volatile__ ("inl %w1,%0": "=a" (_v) : "Nd" (port));
    return _v;
}

unsigned char inb_p(unsigned short port)
{
    unsigned char _v;
    __asm__ __volatile__ ("inb %w1,%0;\njmp 1f;\n1:\tjmp 1f;\n1:": "=a" (_v) : "Nd" (port));
    return _v;
}
unsigned short inw_p(unsigned short port)
{
    unsigned short _v;
    __asm__ __volatile__ ("inw %w1,%0;\njmp 1f;\n1:\tjmp 1f;\n1:": "=a" (_v) : "Nd" (port));
    return _v;
}
unsigned int inl_p(unsigned short port)
{
    unsigned int _v;
    __asm__ __volatile__ ("inl %w1,%0;\njmp 1f;\n1:\tjmp 1f;\n1:": "=a" (_v) : "Nd" (port));
    return _v;
}

void outb(unsigned char value, unsigned short port)
{
    __asm__ __volatile__ ("outb %b0,%w1": : "a" (value), "Nd" (port));
}

void outw(unsigned short value, unsigned short port)
{
    __asm__ __volatile__ ("outw %w0,%w1": : "a" (value), "Nd" (port));
}

void outl(unsigned int value, unsigned short port)
{
    __asm__ __volatile__ ("outl %0,%w1": : "a" (value), "Nd" (port));
}

void outb_p(unsigned char value, unsigned short port)
{
    __asm__ __volatile__ ("outb %b0,%w1;\njmp 1f;\n1:\tjmp 1f;\n1:": : "a" (value), "Nd" (port));
}

void outw_p(unsigned short value, unsigned short port)
{
    __asm__ __volatile__ ("outw %w0,%w1;\njmp 1f;\n1:\tjmp 1f;\n1:": : "a" (value), "Nd" (port));
}

void outl_p(unsigned int value, unsigned short port)
{
    __asm__ __volatile__ ("outl %0,%w1;\njmp 1f;\n1:\tjmp 1f;\n1:": : "a" (value), "Nd" (port));
}
