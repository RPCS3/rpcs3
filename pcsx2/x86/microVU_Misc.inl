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

#pragma once
#ifdef PCSX2_MICROVU

//------------------------------------------------------------------
// Micro VU - Clamp Functions
//------------------------------------------------------------------

// Used for Result Clamping
microVUx(void) mVUclamp1(int reg, int regT1, int xyzw) {
	switch (xyzw) {
		case 1: case 2: case 4: case 8:
			SSE_MINSS_M32_to_XMM(reg, (uptr)mVU_maxvals);
			SSE_MAXSS_M32_to_XMM(reg, (uptr)mVU_minvals);
			break;
		default:
			SSE_MINPS_M128_to_XMM(reg, (uptr)mVU_maxvals);
			SSE_MAXPS_M128_to_XMM(reg, (uptr)mVU_minvals);
			break;
	}
}

// Used for Operand Clamping
microVUx(void) mVUclamp2(int reg, int regT1, int xyzw) {
	if (CHECK_VU_SIGN_OVERFLOW) {
		switch (xyzw) {
			case 1: case 2: case 4: case 8:
				SSE_MOVSS_XMM_to_XMM(regT1, reg);
				SSE_ANDPS_M128_to_XMM(regT1, (uptr)mVU_signbit);
				SSE_MINSS_M32_to_XMM(reg, (uptr)mVU_maxvals);
				SSE_MAXSS_M32_to_XMM(reg, (uptr)mVU_minvals);
				SSE_ORPS_XMM_to_XMM(reg, regT1);
				break;
			default:
				SSE_MOVAPS_XMM_to_XMM(regT1, reg);
				SSE_ANDPS_M128_to_XMM(regT1, (uptr)mVU_signbit);
				SSE_MINPS_M128_to_XMM(reg, (uptr)mVU_maxvals);
				SSE_MAXPS_M128_to_XMM(reg, (uptr)mVU_minvals);
				SSE_ORPS_XMM_to_XMM(reg, regT1);
				break;
		}
	}
	else mVUclamp1<vuIndex>(reg, regT1, xyzw);
}

//------------------------------------------------------------------
// Micro VU - Misc Functions
//------------------------------------------------------------------

microVUx(void) mVUunpack_xyzw(int dstreg, int srcreg, int xyzw) {
	switch ( xyzw ) {
		case 0: SSE2_PSHUFD_XMM_to_XMM(dstreg, srcreg, 0x00); break;
		case 1: SSE2_PSHUFD_XMM_to_XMM(dstreg, srcreg, 0x55); break;
		case 2: SSE2_PSHUFD_XMM_to_XMM(dstreg, srcreg, 0xaa); break;
		case 3: SSE2_PSHUFD_XMM_to_XMM(dstreg, srcreg, 0xff); break;
	}
}

microVUx(void) mVUloadReg(int reg, u32 offset, int xyzw) {
	switch( xyzw ) {
		case 8:		SSE_MOVSS_M32_to_XMM(reg, offset);		break; // X
		case 4:		SSE_MOVSS_M32_to_XMM(reg, offset+4);	break; // Y
		case 2:		SSE_MOVSS_M32_to_XMM(reg, offset+8);	break; // Z
		case 1:		SSE_MOVSS_M32_to_XMM(reg, offset+12);	break; // W
		case 3:		SSE_MOVHPS_M64_to_XMM(reg, offset+8);	break; // ZW (not sure if this is faster than default)
		case 12:	SSE_MOVLPS_M64_to_XMM(reg, offset);		break; // XY (not sure if this is faster than default)
		default:	SSE_MOVAPS_M128_to_XMM(reg, offset);	break;
	}
}

microVUx(void) mVUsaveReg(int reg, u32 offset, int xyzw) {
	switch ( xyzw ) {
		case 1: // W
			SSE_MOVSS_XMM_to_M32(offset+12, reg);
			break;
		case 2: // Z
			SSE_MOVSS_XMM_to_M32(offset+8, reg);
			break;
		case 3: // ZW
			SSE_MOVHPS_XMM_to_M64(offset+8, reg);
			break;
		case 4: // Y
			SSE_MOVSS_XMM_to_M32(offset+4, reg);
			break;
		case 5: // YW
			SSE_SHUFPS_XMM_to_XMM(reg, reg, 0xB1);
			SSE_MOVHLPS_XMM_to_XMM(xmmT1, reg);
			SSE_MOVSS_XMM_to_M32(offset+4, reg);
			SSE_MOVSS_XMM_to_M32(offset+12, xmmT1);
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
			SSE_MOVAPS_XMM_to_M128(offset, reg);
			break;
	}
}

// Modifies the Source Reg!
microVUx(void) mVUmergeRegs(int dest, int src, int xyzw) {
	xyzw &= 0xf;
	if ( (dest != src) && (xyzw != 0) ) {
		if ( cpucaps.hasStreamingSIMD4Extensions && (xyzw != 0x8) && (xyzw != 0xf) ) {
			xyzw = ((xyzw & 1) << 3) | ((xyzw & 2) << 1) | ((xyzw & 4) >> 1) | ((xyzw & 8) >> 3); 
			SSE4_BLENDPS_XMM_to_XMM(dest, src, xyzw);
		}
		else {
			switch (xyzw) {
				case 1:  SSE_MOVHLPS_XMM_to_XMM(src, dest);
						 SSE_SHUFPS_XMM_to_XMM(dest, src, 0xc4);
						 break;
				case 2:  SSE_MOVHLPS_XMM_to_XMM(src, dest);
						 SSE_SHUFPS_XMM_to_XMM(dest, src, 0x64);
						 break;
				case 3:	 SSE_SHUFPS_XMM_to_XMM(dest, src, 0xe4);
						 break;
				case 4:	 SSE_MOVSS_XMM_to_XMM(src, dest);
						 SSE2_MOVSD_XMM_to_XMM(dest, src);
						 break;
				case 5:	 SSE_SHUFPS_XMM_to_XMM(dest, src, 0xd8);
						 SSE_SHUFPS_XMM_to_XMM(dest, dest, 0xd8);
						 break;
				case 6:	 SSE_SHUFPS_XMM_to_XMM(dest, src, 0x9c);
						 SSE_SHUFPS_XMM_to_XMM(dest, dest, 0x78);
						 break;
				case 7:	 SSE_MOVSS_XMM_to_XMM(src, dest);
						 SSE_MOVAPS_XMM_to_XMM(dest, src);
						 break;
				case 8:	 SSE_MOVSS_XMM_to_XMM(dest, src);
						 break;
				case 9:	 SSE_SHUFPS_XMM_to_XMM(dest, src, 0xc9);
						 SSE_SHUFPS_XMM_to_XMM(dest, dest, 0xd2);
						 break;
				case 10: SSE_SHUFPS_XMM_to_XMM(dest, src, 0x8d);
						 SSE_SHUFPS_XMM_to_XMM(dest, dest, 0x72);
						 break;
				case 11: SSE_MOVSS_XMM_to_XMM(dest, src);
						 SSE_SHUFPS_XMM_to_XMM(dest, src, 0xe4);
						 break;
				case 12: SSE2_MOVSD_XMM_to_XMM(dest, src);
						 break;
				case 13: SSE_MOVHLPS_XMM_to_XMM(dest, src);
						 SSE_SHUFPS_XMM_to_XMM(src, dest, 0x64);
						 SSE_MOVAPS_XMM_to_XMM(dest, src);
						 break;
				case 14: SSE_MOVHLPS_XMM_to_XMM(dest, src);
						 SSE_SHUFPS_XMM_to_XMM(src, dest, 0xc4);
						 SSE_MOVAPS_XMM_to_XMM(dest, src);
						 break;
				default: SSE_MOVAPS_XMM_to_XMM(dest, src); 
						 break;
			}
		}
	}
}

#endif //PCSX2_MICROVU
