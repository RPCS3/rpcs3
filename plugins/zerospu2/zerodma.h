/*  ZeroSPU2
 *  Copyright (C) 2006-2010 zerofrog
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

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
