/*  Pcsx2 - Pc Ps2 Emulator
*  Copyright (C) 2009  Pcsx2-Playground Team
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

#include "PrecompiledHeader.h"
#include "microVU.h"
#ifdef PCSX2_MICROVU

extern PCSX2_ALIGNED16(microVU microVU0);
extern PCSX2_ALIGNED16(microVU microVU1);

//------------------------------------------------------------------
// Micro VU - Misc Functions
//------------------------------------------------------------------

microVUx(void) mVUsaveReg(u32 code, int reg, u32 offset) {
	switch ( _X_Y_Z_W ) {
		case 1: // W
			//SSE2_PSHUFD_XMM_to_XMM(xmmT1, reg, 0x27);
			//SSE_MOVSS_XMM_to_M32(offset+12, xmmT1);
			SSE_MOVSS_XMM_to_M32(offset+12, reg);
			break;
		case 2: // Z
			//SSE_MOVHLPS_XMM_to_XMM(xmmT1, reg);
			//SSE_MOVSS_XMM_to_M32(offset+8, xmmT1);
			SSE_MOVSS_XMM_to_M32(offset+8, reg);
			break;
		case 3: // ZW
			SSE_MOVHPS_XMM_to_M64(offset+8, reg);
			break;
		case 4: // Y
			//SSE2_PSHUFLW_XMM_to_XMM(xmmT1, reg, 0x4e);
			//SSE_MOVSS_XMM_to_M32(offset+4, xmmT1);
			SSE_MOVSS_XMM_to_M32(offset+4, reg);
			break;
		case 5: // YW
			SSE_SHUFPS_XMM_to_XMM(reg, reg, 0xB1);
			SSE_MOVHLPS_XMM_to_XMM(xmmT1, reg);
			SSE_MOVSS_XMM_to_M32(offset+4, reg);
			SSE_MOVSS_XMM_to_M32(offset+12, xmmT1);
			SSE_SHUFPS_XMM_to_XMM(reg, reg, 0xB1);
			break;
		case 6: // YZ
			SSE2_PSHUFD_XMM_to_XMM(xmmT1, reg, 0xc9);
			SSE_MOVLPS_XMM_to_M64(offset+4, xmmT1);
			break;
		case 7: // YZW
			SSE2_PSHUFD_XMM_to_XMM(xmmT1, reg, 0x93); //ZYXW
			SSE_MOVHPS_XMM_to_M64(offset+4, xmmT1);
			SSE_MOVSS_XMM_to_M32(offset+12, xmmT1);
			break;
		case 8: // X
			SSE_MOVSS_XMM_to_M32(offset, reg);
			break;
		case 9: // XW
			SSE_MOVHLPS_XMM_to_XMM(xmmT1, reg);
			SSE_MOVSS_XMM_to_M32(offset, reg);
			if ( cpucaps.hasStreamingSIMD3Extensions ) SSE3_MOVSLDUP_XMM_to_XMM(xmmT1, xmmT1);
			else SSE_SHUFPS_XMM_to_XMM(xmmT1, xmmT1, 0x55);
			SSE_MOVSS_XMM_to_M32(offset+12, xmmT1);
			break;
		case 10: //XZ
			SSE_MOVHLPS_XMM_to_XMM(xmmT1, reg);
			SSE_MOVSS_XMM_to_M32(offset, reg);
			SSE_MOVSS_XMM_to_M32(offset+8, xmmT1);
			break;
		case 11: //XZW
			SSE_MOVSS_XMM_to_M32(offset, reg);
			SSE_MOVHPS_XMM_to_M64(offset+8, reg);
			break;
		case 12: // XY
			SSE_MOVLPS_XMM_to_M64(offset, reg);
			break;
		case 13: // XYW
			SSE2_PSHUFD_XMM_to_XMM(xmmT1, reg, 0x4b); //YXZW				
			SSE_MOVHPS_XMM_to_M64(offset, xmmT1);
			SSE_MOVSS_XMM_to_M32(offset+12, xmmT1);
			break;
		case 14: // XYZ
			SSE_MOVHLPS_XMM_to_XMM(xmmT1, reg);
			SSE_MOVLPS_XMM_to_M64(offset, reg);
			SSE_MOVSS_XMM_to_M32(offset+8, xmmT1);
			break;
		case 15: // XYZW				
			if( offset & 15 ) SSE_MOVUPS_XMM_to_M128(offset, reg);
			else SSE_MOVAPS_XMM_to_M128(offset, reg);
			break;
	}
}

#endif //PCSX2_MICROVU