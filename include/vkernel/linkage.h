#ifndef _LINUX_LINKAGE_H
#define _LINUX_LINKAGE_H

#include <vkernel/config.h>

#ifdef __cplusplus
#define CPP_ASMLINKAGE extern "C"
#else
#define CPP_ASMLINKAGE
#endif
#define SYMBOL_NAME_STR(X) #X
#define __ALIGN_STR ".align 4,0x90"

#define asmlinkage CPP_ASMLINKAGE __attribute__((regparm(0)))

#define __ALIGN .align 4,0x90

#define ALIGN __ALIGN

#define ENTRY(name) \
  .globl name; \
  ALIGN; \
  name:

#endif
