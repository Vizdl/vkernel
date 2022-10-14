#include <vkernel/kernel.h>
#include <vkernel/linkage.h>

asmlinkage long sys_exit(int error_code)
{
    printk("sys_exit : %d\n", error_code);
	return 0;
}