/*  PCSX2 - PS2 Emulator for PCs
 *  Copyright (C) 2002-2009  PCSX2 Dev Team
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

#ifndef __CACHE_H__
#define __CACHE_H__

#include "Common.h"

struct _u8bit_128 {
	u8	_8[16];

};

struct u8bit_128 {
	_u8bit_128 b8;

};

struct _cacheS {
	u32 tag[2];
	u8bit_128 data[2][4];
};

extern _cacheS pCache[64];

void writeCache8(u32 mem, u8 value);
void writeCache16(u32 mem, u16 value);
void writeCache32(u32 mem, u32 value);
void writeCache64(u32 mem, u64 value);
void writeCache128(u32 mem, u64 *value);
u8  *readCache(u32 mem);

// Fixme - these two functions do nothing, and the cache code relies on these two functions.	
static __forceinline u32 getMemR(s32 mem)
{
	return 0;//memLUTR[mem >> 12];
}

static __forceinline u32 getMemW(s32 mem)
{
	return 0;//memLUTW[mem>>12];
}

#endif /* __CACHE_H__ */
