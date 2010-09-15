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

struct vifStruct;

typedef void (__fastcall *UNPACKFUNCTYPE)(void* dest, const void* src);

#define create_unpack_u_type(bits)		typedef void (__fastcall *UNPACKFUNCTYPE_u##bits)(u32* dest, const u##bits* src);
#define create_unpack_s_type(bits)		typedef void (__fastcall *UNPACKFUNCTYPE_s##bits)(u32* dest, const s##bits* src);

#define create_some_unpacks(bits)		\
		create_unpack_u_type(bits);		\
		create_unpack_s_type(bits);		\

create_some_unpacks(32);
create_some_unpacks(16);
create_some_unpacks(8);

extern __aligned16 const u8 nVifT[16];

// Array sub-dimension order: [vifidx] [mode] (VN * VL * USN * doMask)
extern __aligned16 const UNPACKFUNCTYPE VIFfuncTable[2][3][(4 * 4 * 2 * 2)];

_vifT extern int  nVifUnpack (const u8* data);
extern void resetNewVif(int idx);

template< int idx >
extern void vifUnpackSetup(const u32* data);
