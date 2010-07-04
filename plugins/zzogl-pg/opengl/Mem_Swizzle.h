/*  ZeroGS KOSMOS
 *  Copyright (C) 2005-2006 zerofrog@gmail.com
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

#ifndef MEM_SWIZZLE_H_INCLUDED
#define MEM_SWIZZLE_H_INCLUDED

#include "GS.h"
#include "Mem.h"
#include "x86.h"

extern __forceinline void SwizzleBlock32(u8 *dst, u8 *src, int pitch, u32 WriteMask = 0xffffffff);
extern __forceinline void SwizzleBlock16(u8 *dst, u8 *src, int pitch, u32 WriteMask = 0xffffffff);
extern __forceinline void SwizzleBlock8(u8 *dst, u8 *src, int pitch, u32 WriteMask = 0xffffffff);
extern __forceinline void SwizzleBlock4(u8 *dst, u8 *src, int pitch, u32 WriteMask = 0xffffffff);
extern __forceinline void SwizzleBlock32u(u8 *dst, u8 *src, int pitch, u32 WriteMask = 0xffffffff);
extern __forceinline void SwizzleBlock16u(u8 *dst, u8 *src, int pitch, u32 WriteMask = 0xffffffff);
extern __forceinline void SwizzleBlock8u(u8 *dst, u8 *src, int pitch, u32 WriteMask = 0xffffffff);
extern __forceinline void SwizzleBlock4u(u8 *dst, u8 *src, int pitch, u32 WriteMask = 0xffffffff);

extern __forceinline void __fastcall SwizzleBlock32_c(u8* dst, u8* src, int srcpitch, u32 WriteMask = 0xffffffff);
extern __forceinline void __fastcall SwizzleBlock24_c(u8* dst, u8* src, int srcpitch, u32 WriteMask = 0xffffffff);
extern __forceinline void __fastcall SwizzleBlock16_c(u8* dst, u8* src, int srcpitch, u32 WriteMask = 0xffffffff);
extern __forceinline void __fastcall SwizzleBlock8_c(u8* dst, u8* src, int srcpitch, u32 WriteMask = 0xffffffff);
extern __forceinline void __fastcall SwizzleBlock4_c(u8* dst, u8* src, int srcpitch, u32 WriteMask = 0xffffffff);

// special swizzle macros - which I converted to functions.
extern __forceinline void SwizzleBlock24(u8 *dst, u8 *src, int pitch, u32 WriteMask = 0xffffffff);
extern __forceinline void SwizzleBlock8H(u8 *dst, u8 *src, int pitch, u32 WriteMask = 0xffffffff);
extern __forceinline void SwizzleBlock4HH(u8 *dst, u8 *src, int pitch, u32 WriteMask = 0xffffffff);
extern __forceinline void SwizzleBlock4HL(u8 *dst, u8 *src, int pitch, u32 WriteMask = 0xffffffff);
#define SwizzleBlock24u SwizzleBlock24
#define SwizzleBlock8Hu SwizzleBlock8H
#define SwizzleBlock4HHu SwizzleBlock4HH
#define SwizzleBlock4HLu SwizzleBlock4HL

#endif // MEM_SWIZZLE_H_INCLUDED
