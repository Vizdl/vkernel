#include <vkernel/debug.h>
#include <vkernel/sched.h>
#include <vkernel/kernel.h>
#include <vkernel/linkage.h>

NORET_TYPE void do_exit(long code)
{
	printk("do_exit : %d\n", code);
	schedule();
	BUG();
}

asmlinkage long sys_exit(int error_code)
{
	do_exit((error_code&0xff)<<8);
	return 0;
}