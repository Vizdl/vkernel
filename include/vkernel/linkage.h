#ifndef _LINUX_LINKAGE_H
#define _LINUX_LINKAGE_H

#include <vkernel/config.h>

#ifdef __cplusplus
#define CPP_ASMLINKAGE extern "C"
#else
#define CPP_ASMLINKAGE
#endif

#define asmlinkage CPP_ASMLINKAGE __attribute__((regparm(0)))

#endif
