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

#include "PrecompiledHeader.h"
#include "Common.h"

#include <cmath>
#include <float.h>

#include "VUmicro.h"

/*****************************************/
/*          NEW FLAGS                    */ //By asadr. Thnkx F|RES :p
/*****************************************/


void vuUpdateDI(VURegs * VU) {
//	u32 Flag_S = 0;
//	u32 Flag_I = 0;
//	u32 Flag_D = 0;
//
//	/*
//	FLAG D - I
//	*/
//	Flag_I = (VU->statusflag >> 4) & 0x1;
//	Flag_D = (VU->statusflag >> 5) & 0x1;
//
//	VU->statusflag|= (Flag_I | (VU0.VI[REG_STATUS_FLAG].US[0] >> 4)) << 10;
//	VU->statusflag|= (Flag_D | (VU0.VI[REG_STATUS_FLAG].US[0] >> 5)) << 11;
}

static __releaseinline u32 VU_MAC_UPDATE( int shift, VURegs * VU, float f )
{
	u32 v = *(u32*)&f;
	int exp = (v >> 23) & 0xff;
	u32 s = v & 0x80000000;

	if (s)
		VU->macflag |= 0x0010<<shift;
	else
		VU->macflag &= ~(0x0010<<shift);

	if( f == 0 )
	{
		VU->macflag = (VU->macflag & ~(0x1100<<shift)) | (0x0001<<shift);
		return v;
	}

	switch(exp)
	{
		case 0:
			VU->macflag = (VU->macflag&~(0x1000<<shift)) | (0x0101<<shift);
			return s;
		case 255:
			VU->macflag = (VU->macflag&~(0x0100<<shift)) | (0x1000<<shift);
			return s|0x7f7fffff; /* max allowed */
		default:
			VU->macflag = (VU->macflag & ~(0x1101<<shift));
			return v;
	}
}

__forceinline u32 VU_MACx_UPDATE(VURegs * VU, float x)
{
	return VU_MAC_UPDATE(3, VU, x);
}

__forceinline u32 VU_MACy_UPDATE(VURegs * VU, float y)
{
	return VU_MAC_UPDATE(2, VU, y);
}

__forceinline u32 VU_MACz_UPDATE(VURegs * VU, float z)
{
	return VU_MAC_UPDATE(1, VU, z);
}

__forceinline u32 VU_MACw_UPDATE(VURegs * VU, float w)
{
	return VU_MAC_UPDATE(0, VU, w);
}

__forceinline void VU_MACx_CLEAR(VURegs * VU)
{
	VU->macflag&= ~(0x1111<<3);
}

__forceinline void VU_MACy_CLEAR(VURegs * VU)
{
	VU->macflag&= ~(0x1111<<2);
}

__forceinline void VU_MACz_CLEAR(VURegs * VU)
{
	VU->macflag&= ~(0x1111<<1);
}

__forceinline void VU_MACw_CLEAR(VURegs * VU)
{
	VU->macflag&= ~(0x1111<<0);
}

__releaseinline void VU_STAT_UPDATE(VURegs * VU) {
	int newflag = 0 ;
	if (VU->macflag & 0x000F) newflag = 0x1;
	if (VU->macflag & 0x00F0) newflag |= 0x2;
	if (VU->macflag & 0x0F00) newflag |= 0x4;
	if (VU->macflag & 0xF000) newflag |= 0x8;
	VU->statusflag = (VU->statusflag&0xc30)|newflag|((VU->statusflag&0xf)<<6);
}
