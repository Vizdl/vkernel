#ifndef _LINUX_INIT_H
#define _LINUX_INIT_H
/*
 * Mark functions and data as being only used at initialization
 * or exit time.
 */
#define __init		__attribute__ ((__section__ (".text.init")))
#define __exit		__attribute__ ((unused, __section__(".text.exit")))
#define __initdata	__attribute__ ((__section__ (".data.init")))
#define __exitdata	__attribute__ ((unused, __section__ (".data.exit")))
#define __initsetup	__attribute__ ((unused,__section__ (".setup.init")))
#define __init_call	__attribute__ ((unused,__section__ (".initcall.init")))
#define __exit_call	__attribute__ ((unused,__section__ (".exitcall.exit")))

/* For assembly routines */
#define __INIT		.section	".text.init","ax"
#define __FINIT		.previous
#define __INITDATA	.section	".data.init","aw"
#endif