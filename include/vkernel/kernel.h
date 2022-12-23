#ifndef __KERNEL_H
#define __KERNEL_H

#include <asm/types.h>

#define barrier() __asm__ __volatile__("": : :"memory")

#define	KERN_EMERG	"<0>"	/* system is unusable			*/
#define	KERN_ALERT	"<1>"	/* action must be taken immediately	*/
#define	KERN_CRIT	"<2>"	/* critical conditions			*/
#define	KERN_ERR	"<3>"	/* error conditions			*/
#define	KERN_WARNING	"<4>"	/* warning conditions			*/
#define	KERN_NOTICE	"<5>"	/* normal but significant condition	*/
#define	KERN_INFO	"<6>"	/* informational			*/
#define	KERN_DEBUG	"<7>"	/* debug-level messages			*/

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

#ifdef __i386__
// 使用寄存器调用栈的方式 : eax edx ebx
#define FASTCALL(x)	x __attribute__((regparm(3)))
#define __fastcall __attribute__((regparm(3)))
#else
#define FASTCALL(x)	x
#define __fastcall
#endif

#endif /* __KERNEL_H */
