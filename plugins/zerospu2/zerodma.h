#ifndef ZERODMA_H_INCLUDED
#define ZERODMA_H_INCLUDED

#include <stdio.h>

#define SPU2defs
#include "PS2Edefs.h"

#include "reg.h"
#include "misc.h"

static __forceinline bool irq_test1(u32 ch, u32 addr)
{
	return ((addr + 0x2400) <= C_IRQA(ch) &&  (addr + 0x2500) >= C_IRQA(ch));
}

static __forceinline bool irq_test2(u32 ch, u32 addr)
{
	return ((addr + 0x2600) <= C_IRQA(ch) &&  (addr + 0x2700) >= C_IRQA(ch));
}


#endif // ZERODMA_H_INCLUDED
