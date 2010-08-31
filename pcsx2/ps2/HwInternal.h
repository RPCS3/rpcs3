/*  PCSX2 - PS2 Emulator for PCs
 *  Copyright (C) 2002-2010  PCSX2 Dev Team
 *
 *  PCSX2 is free software: you can redistribute it and/or modify it under the terms
 *  of the GNU Lesser General Public License as published by the Free Software Found-
 *  ation, either version 3 of the License, or (at your option) any later version.
 *
 *  PCSX2 is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 *  without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 *  PURPOSE.  See the GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along with PCSX2.
 *  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include "Hw.h"

// --------------------------------------------------------------------------------------
//  IsPageFor() / iswitch() / icase()   [macros!]
// --------------------------------------------------------------------------------------
// Page-granulated switch helpers: In order for the compiler to optimize hardware register
// handlers, which dispatch registers along a series of switches, the compiler needs to know
// that the case entry applies to the current page only.  Under MSVC, I tried all manners of
// bitmasks against the templated page value, and this was the only one that worked:
//
// Note: MSVC 2008 actually fails to optimize "switch" properly due to being overly aggressive
// about trying to use its clever BSP-tree logic for long switches.  It adds the BSP tree logic,
// even though most of the "tree" is empty (resulting in several compare/jumps that do nothing).
// Explained: Even though only one or two of the switch entires are valid, MSVC will still
// compile in its BSP tree check (which divides the switch into 2 or 4 ranges of values).  Three
// of the ranges just link to "RET", while the fourth range contains the handler for the one
// register operation contained in the templated page.

#define IsPageFor(_mem)	((page<<12) == (_mem&(0xf<<12)))
#define icase(ugh)		if(IsPageFor(ugh) && (mem==ugh))
#define iswitch(mem)

// hw read functions
template< uint page > extern mem8_t  __fastcall hwRead8  (u32 mem);
template< uint page > extern mem16_t __fastcall hwRead16 (u32 mem);
template< uint page > extern mem32_t __fastcall hwRead32 (u32 mem);
template< uint page > extern void    __fastcall hwRead64 (u32 mem, mem64_t* out );
template< uint page > extern void    __fastcall hwRead128(u32 mem, mem128_t* out);

// Internal hwRead32 which does not log reads, used by hwWrite8/16 to perform
// read-modify-write operations.
template< uint page, bool intcstathack >
mem32_t __fastcall _hwRead32(u32 mem);

extern mem16_t __fastcall hwRead16_page_0F_INTC_HACK(u32 mem);
extern mem32_t __fastcall hwRead32_page_0F_INTC_HACK(u32 mem);


// hw write functions
template<uint page> extern void __fastcall hwWrite8  (u32 mem, u8  value);
template<uint page> extern void __fastcall hwWrite16 (u32 mem, u16 value);

template<uint page> extern void __fastcall hwWrite32 (u32 mem, mem32_t value);
template<uint page> extern void __fastcall hwWrite64 (u32 mem, const mem64_t* srcval);
template<uint page> extern void __fastcall hwWrite128(u32 mem, const mem128_t* srcval);

// --------------------------------------------------------------------------------------
//  Hardware FIFOs (128 bit access only!)
// --------------------------------------------------------------------------------------
// VIF0   -- 0x10004000 -- eeHw[0x4000]
// VIF1   -- 0x10005000 -- eeHw[0x5000]
// GIF    -- 0x10006000 -- eeHw[0x6000]
// IPUout -- 0x10007000 -- eeHw[0x7000]
// IPUin  -- 0x10007010 -- eeHw[0x7010]

extern void __fastcall ReadFIFO_VIF1(mem128_t* out);
extern void __fastcall ReadFIFO_IPUout(mem128_t* out);

extern void __fastcall WriteFIFO_VIF0(const mem128_t* value);
extern void __fastcall WriteFIFO_VIF1(const mem128_t* value);
extern void __fastcall WriteFIFO_GIF(const mem128_t* value);
extern void __fastcall WriteFIFO_IPUin(const mem128_t* value);
