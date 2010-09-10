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

typedef void (__fastcall *UNPACKFUNCTYPE)(u32 *dest, const u32 *data);
typedef int (*UNPACKPARTFUNCTYPESSE)(u32 *dest, const u32 *data, int size);

#define create_unpack_u_type(bits)		typedef void (__fastcall *UNPACKFUNCTYPE_U##bits)(u32 *dest, const u##bits *data);
#define create_unpack_odd_u_type(bits)	typedef void (__fastcall *UNPACKFUNCTYPE_ODD_U##bits)(u32 *dest, const u##bits *data, int size);
#define create_unpack_s_type(bits)		typedef void (__fastcall *UNPACKFUNCTYPE_S##bits)(u32 *dest, const s##bits *data);
#define create_unpack_odd_s_type(bits)	typedef void (__fastcall *UNPACKFUNCTYPE_ODD_S##bits)(u32 *dest, const s##bits *data, int size);

#define create_some_unpacks(bits)		\
		create_unpack_u_type(bits);		\
		create_unpack_odd_u_type(bits);	\
		create_unpack_s_type(bits);		\
		create_unpack_odd_s_type(bits);

create_some_unpacks(32);
create_some_unpacks(16);
create_some_unpacks(8);

struct VIFUnpackFuncTable
{
    UNPACKFUNCTYPE  funcU;
    UNPACKFUNCTYPE  funcS;

	u8 gsize; // size of data in bytes used for each write cycle
	u8 qsize; // used for unpack parts, num of vectors that
	// will be decompressed from data for 1 cycle
};

extern const __aligned16 VIFUnpackFuncTable VIFfuncTable[32];

extern int  nVifUnpack (int idx, const u8 *data);
extern void resetNewVif(int idx);

template< int idx >
extern void vifUnpackSetup(const u32 *data);
