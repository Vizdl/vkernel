#ifndef __ASM_I386_PROCESSOR_H
#define __ASM_I386_PROCESSOR_H

#include <asm/desc.h>
#include <asm/unistd.h>
#include <asm/segment.h>
#include <asm/cpufeature.h>
#include <vkernel/threads.h>

#define IO_BITMAP_SIZE	32
#define IO_BITMAP_OFFSET offsetof(struct tss_struct,io_bitmap)
#define INVALID_IO_BITMAP_OFFSET 0x8000


#define init_task	(init_task_union.task)
#define init_stack	(init_task_union.stack)

// tss 任务上下文
struct tss_struct {
	unsigned short	back_link,__blh;
	unsigned long	esp0;
	unsigned short	ss0,__ss0h;
	unsigned long	esp1;
	unsigned short	ss1,__ss1h;
	unsigned long	esp2;
	unsigned short	ss2,__ss2h;
	unsigned long	__cr3;
	unsigned long	eip;
	unsigned long	eflags;
	unsigned long	eax,ecx,edx,ebx;
	unsigned long	esp;
	unsigned long	ebp;
	unsigned long	esi;
	unsigned long	edi;
	unsigned short	es, __esh;
	unsigned short	cs, __csh;
	unsigned short	ss, __ssh;
	unsigned short	ds, __dsh;
	unsigned short	fs, __fsh;
	unsigned short	gs, __gsh;
	unsigned short	ldt, __ldth;
	unsigned short	trace, bitmap;
	unsigned long	io_bitmap[IO_BITMAP_SIZE+1];
	/*
	 * pads the TSS to be cacheline-aligned (size is 0x100)
	 */
	unsigned long __cacheline_filler[5];
};

// 内核态线程切换的上下文
struct thread_struct {
	unsigned long	esp0;
	unsigned long	eip;
	unsigned long	esp;
	unsigned long	fs;
	unsigned long	gs;
/* Hardware debugging registers */
	unsigned long	debugreg[8];  /* %%db0-7 debug registers */
/* fault info */
	unsigned long	cr2, trap_no, error_code;
	unsigned long		screen_bitmap;
	unsigned long		v86flags, v86mask, v86mode, saved_esp0;
/* IO permissions */
	int		ioperm;
	unsigned long	io_bitmap[IO_BITMAP_SIZE+1];
};

#define INIT_THREAD  {						\
	0,							\
	0, 0, 0, 0, 						\
	{ [0 ... 7] = 0 },	/* debugging registers */	\
	0, 0, 0,						\
	0,0,0,0,0,						\
	0,{~0,}			/* io permissions */		\
}

#define INIT_TSS  {						\
	0,0, /* back_link, __blh */				\
	sizeof(init_stack) + (long) &init_stack, /* esp0 */	\
	__KERNEL_DS, 0, /* ss0 */				\
	0,0,0,0,0,0, /* stack1, stack2 */			\
	0, /* cr3 */						\
	0,0, /* eip,eflags */					\
	0,0,0,0, /* eax,ecx,edx,ebx */				\
	0,0,0,0, /* esp,ebp,esi,edi */				\
	0,0,0,0,0,0, /* es,cs,ss */				\
	0,0,0,0,0,0, /* ds,fs,gs */				\
	__LDT(0),0, /* ldt */					\
	0, INVALID_IO_BITMAP_OFFSET, /* tace, bitmap */		\
	{~0, } /* ioperm */					\
}

extern struct tss_struct init_tss[NR_CPUS];

struct cpuinfo_x86 {
	__u8	x86;		/* CPU family */
	__u8	x86_vendor;	/* CPU vendor */
	__u8	x86_model;
	__u8	x86_mask;
	char	wp_works_ok;	/* It doesn't on 386's */
	char	hlt_works_ok;	/* Problems on some 486Dx4's and old 386's */
	char	hard_math;
	char	rfu;
       	int	cpuid_level;	/* Maximum supported CPUID level, -1=no CPUID */
	__u32	x86_capability[NCAPINTS];
	char	x86_vendor_id[16];
	char	x86_model_id[64];
	int 	x86_cache_size;  /* in KB - valid for CPUS which support this
				    call  */
	int	fdiv_bug;
	int	f00f_bug;
	int	coma_bug;
	unsigned long loops_per_jiffy;
	unsigned long *pgd_quick;
	unsigned long *pmd_quick;
	unsigned long *pte_quick;
	unsigned long pgtable_cache_sz;
};

extern struct cpuinfo_x86 boot_cpu_data;
#define current_cpu_data boot_cpu_data

extern int kernel_thread(int (*fn)(void *), void * arg, unsigned long flags);

#endif