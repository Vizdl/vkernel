
#include <vkernel/kernel.h>

void panic(char* filename, int line, const char* func)
{
    printk("panic : filename is %s, function is %s, line is 0x%d\n", filename, func, line);
    while(1);
}