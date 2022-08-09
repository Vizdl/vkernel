#ifndef __KERNEL_DEBUG_H
#define __KERNEL_DEBUG_H

extern void panic(char* filename, const char* func, int line);

#define PANIC() panic (__FILE__, __func__, __LINE__)

#define BUG() PANIC()

#endif // __KERNEL_DEBUG_H
