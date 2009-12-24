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
 
#ifndef __VIFDMA_INTERNAL_H__
#define __VIFDMA_INTERNAL_H__

#include "VifDma.h"

enum VifModes
{
	VIF_NORMAL_TO_MEM_MODE = 0,
	VIF_NORMAL_FROM_MEM_MODE = 1,
	VIF_CHAIN_MODE = 2
};

// Generic constants
static const unsigned int VIF0intc = 4;
static const unsigned int VIF1intc = 5;

typedef void (__fastcall *UNPACKFUNCTYPE)(u32 *dest, u32 *data);
typedef void (__fastcall *UNPACKFUNCTYPE_ODD)(u32 *dest, u32 *data, int size);
typedef int (*UNPACKPARTFUNCTYPESSE)(u32 *dest, u32 *data, int size);

#define create_unpack_u_type(bits) typedef void (__fastcall *UNPACKFUNCTYPE_U##bits)(u32 *dest, u##bits *data);
#define create_unpack_odd_u_type(bits) typedef void (__fastcall *UNPACKFUNCTYPE_ODD_U##bits)(u32 *dest, u##bits *data, int size);
#define create_unpack_s_type(bits) typedef void (__fastcall *UNPACKFUNCTYPE_S##bits)(u32 *dest, s##bits *data);
#define create_unpack_odd_s_type(bits) typedef void (__fastcall *UNPACKFUNCTYPE_ODD_S##bits)(u32 *dest, s##bits *data, int size);

#define create_some_unpacks(bits) \
    create_unpack_u_type(bits); \
    create_unpack_odd_u_type(bits); \
    create_unpack_s_type(bits); \
    create_unpack_odd_s_type(bits);

create_some_unpacks(32);
create_some_unpacks(16);
create_some_unpacks(8);

struct VIFUnpackFuncTable
{
    UNPACKFUNCTYPE  funcU;
    UNPACKFUNCTYPE  funcS;

	UNPACKFUNCTYPE_ODD  oddU;		// needed for old-style vif only, remove when old vif is removed.
	UNPACKFUNCTYPE_ODD  oddS;		// needed for old-style vif only, remove when old vif is removed.

	u8 bsize; // currently unused
	u8 dsize; // byte size of one channel
	u8 gsize; // size of data in bytes used for each write cycle
	u8 qsize; // used for unpack parts, num of vectors that
	// will be decompressed from data for 1 cycle
};

extern const __aligned16 VIFUnpackFuncTable VIFfuncTable[32];

extern __aligned16 u32 g_vif0Masks[64], g_vif1Masks[64];
extern u32 g_vif0HasMask3[4], g_vif1HasMask3[4];
extern int g_vifCycles;
extern u8 s_maskwrite[256];
extern vifStruct *vif;

template<const u32 VIFdmanum> void ProcessMemSkip(u32 size, u32 unpackType);
template<const u32 VIFdmanum> u32 VIFalign(u32 *data, vifCode *v, u32 size);
template<const u32 VIFdmanum> void VIFunpack(u32 *data, vifCode *v, u32 size);
template<const u32 VIFdmanum> void vuExecMicro(u32 addr);
extern void vif0FLUSH();
extern void vif1FLUSH();

static __forceinline u32 vif_size(u8 num)
{
    return (num == 0) ? 0x1000 : 0x4000;
}

// All defines are enabled with '1' or disabled with '0'

#define newVif		0	// Enable 'newVif' Code (if the below macros are not defined, it will use old non-sse code)
#define newVif1		0	// Use New Code for Vif1 Unpacks (needs newVif defined)
#define newVif0		0	// Use New Code for Vif0 Unpacks (not implemented)

#if newVif
extern int nVifUnpack(int idx, u8 *data);
#else
//#	define NON_SSE_UNPACKS  // Turns off SSE Unpacks (slower)
#endif

#endif
