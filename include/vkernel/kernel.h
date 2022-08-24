#ifndef __KERNEL_H
#define __KERNEL_H

#include <asm/types.h>

typedef char* va_list;
extern int sprintf(char * buf, const char *fmt, ...);
extern int vsprintf(char *buf, const char *fmt, va_list args);
extern void print_int(int num);
extern void print_hex(unsigned int num);
extern void put_char(char ch);
extern void printk(const char* format, ...);
extern void print_str(char *str);
#define va_start(ap, v) ap = (va_list)&v
#define va_arg(ap, t) *((t*)(ap += 4))
#define va_end(ap) ap = NULL

#define FASTCALL(x)	x 

#endif /* __KERNEL_H */
