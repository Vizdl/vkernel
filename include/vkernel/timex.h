#ifndef _LINUX_TIMEX_H
#define _LINUX_TIMEX_H

#include <asm/param.h>
#include <asm/timex.h>

#define LATCH  ((CLOCK_TICK_RATE + HZ/2) / HZ)	/* For divider */

#endif