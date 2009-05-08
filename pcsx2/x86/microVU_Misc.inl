/*  Pcsx2 - Pc Ps2 Emulator
*  Copyright (C) 2009  Pcsx2 Team
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
			SSE_MINSS_XMM_to_XMM(reg, xmmMax);
			SSE_MAXSS_XMM_to_XMM(reg, xmmMin);
			break;
		default:
			SSE_MINPS_XMM_to_XMM(reg, xmmMax);
			SSE_MAXPS_XMM_to_XMM(reg, xmmMin);
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
				SSE_MINSS_XMM_to_XMM(reg, xmmMax);
				SSE_MAXSS_XMM_to_XMM(reg, xmmMin);
				SSE_ORPS_XMM_to_XMM(reg, regT1);
				break;
			default:
				SSE_MOVAPS_XMM_to_XMM(regT1, reg);
				SSE_ANDPS_M128_to_XMM(regT1, (uptr)mVU_signbit);
				SSE_MINPS_XMM_to_XMM(reg, xmmMax);
				SSE_MAXPS_XMM_to_XMM(reg, xmmMin);
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

microVUx(void) mVUloadReg(int reg, uptr offset, int xyzw) {
	switch( xyzw ) {
		case 8:		SSE_MOVSS_M32_to_XMM(reg, offset);		break; // X
		case 4:		SSE_MOVSS_M32_to_XMM(reg, offset+4);	break; // Y
		case 2:		SSE_MOVSS_M32_to_XMM(reg, offset+8);	break; // Z
		case 1:		SSE_MOVSS_M32_to_XMM(reg, offset+12);	break; // W
		default:	SSE_MOVAPS_M128_to_XMM(reg, offset);	break;
	}
}

microVUx(void) mVUloadReg2(int reg, int gprReg, uptr offset, int xyzw) {
	switch( xyzw ) {
		case 8:		SSE_MOVSS_Rm_to_XMM(reg, gprReg, offset);	 break; // X
		case 4:		SSE_MOVSS_Rm_to_XMM(reg, gprReg, offset+4);  break; // Y
		case 2:		SSE_MOVSS_Rm_to_XMM(reg, gprReg, offset+8);  break; // Z
		case 1:		SSE_MOVSS_Rm_to_XMM(reg, gprReg, offset+12); break; // W
		default:	SSE_MOVAPSRmtoR(reg, gprReg, offset);		 break;
	}
}

// Modifies the Source Reg!
microVUx(void) mVUsaveReg(int reg, uptr offset, int xyzw, bool modXYZW) {
	/*SSE_MOVAPS_M128_to_XMM(xmmT2, offset);
	if (modXYZW && (xyzw == 8 || xyzw == 4 || xyzw == 2 || xyzw == 1)) {
		mVUunpack_xyzw<vuIndex>(reg, reg, 0);
	}
	mVUmergeRegs<vuIndex>(xmmT2, reg, xyzw);

	SSE_MOVAPS_XMM_to_M128(offset, xmmT2);
	return;*/

	switch ( xyzw ) {
		case 5:		SSE_SHUFPS_XMM_to_XMM(reg, reg, 0xe1); //WZXY
					SSE_MOVSS_XMM_to_M32(offset+4, reg);
					SSE_SHUFPS_XMM_to_XMM(reg, reg, 0xff); //WWWW
					SSE_MOVSS_XMM_to_M32(offset+12, reg);
					break; // YW
		case 6:		SSE_SHUFPS_XMM_to_XMM(reg, reg, 0xc9);
					SSE_MOVLPS_XMM_to_M64(offset+4, reg);
					break; // YZ
		case 7:		SSE_SHUFPS_XMM_to_XMM(reg, reg, 0x93); //ZYXW
					SSE_MOVHPS_XMM_to_M64(offset+4, reg);
					SSE_MOVSS_XMM_to_M32(offset+12, reg);
					break; // YZW
		case 9:		SSE_MOVSS_XMM_to_M32(offset, reg);
					SSE_SHUFPS_XMM_to_XMM(reg, reg, 0xff); //WWWW
					SSE_MOVSS_XMM_to_M32(offset+12, reg);
					break; // XW
		case 10:	SSE_MOVSS_XMM_to_M32(offset, reg);
					SSE_MOVHLPS_XMM_to_XMM(reg, reg);
					SSE_MOVSS_XMM_to_M32(offset+8, reg);
					break; //XZ
		case 11:	SSE_MOVSS_XMM_to_M32(offset, reg);
					SSE_MOVHPS_XMM_to_M64(offset+8, reg);
					break; //XZW
		case 13:	SSE_SHUFPS_XMM_to_XMM(reg, reg, 0x4b); //YXZW				
					SSE_MOVHPS_XMM_to_M64(offset, reg);
					SSE_MOVSS_XMM_to_M32(offset+12, reg);
					break; // XYW
		case 14:	SSE_MOVLPS_XMM_to_M64(offset, reg);
					SSE_MOVHLPS_XMM_to_XMM(reg, reg);
					SSE_MOVSS_XMM_to_M32(offset+8, reg);
					break; // XYZ
		case 4:		if (!modXYZW) mVUunpack_xyzw<vuIndex>(reg, reg, 1);
					SSE_MOVSS_XMM_to_M32(offset+4, reg);		 
					break; // Y
		case 2:		if (!modXYZW) mVUunpack_xyzw<vuIndex>(reg, reg, 2);
					SSE_MOVSS_XMM_to_M32(offset+8, reg);	
					break; // Z
		case 1:		if (!modXYZW) mVUunpack_xyzw<vuIndex>(reg, reg, 3);
					SSE_MOVSS_XMM_to_M32(offset+12, reg);	
					break; // W
		case 8:		SSE_MOVSS_XMM_to_M32(offset, reg);		break; // X
		case 12:	SSE_MOVLPS_XMM_to_M64(offset, reg);		break; // XY
		case 3:		SSE_MOVHPS_XMM_to_M64(offset+8, reg);	break; // ZW
		default:	SSE_MOVAPS_XMM_to_M128(offset, reg);	break; // XYZW
	}
}

// Modifies the Source Reg!
microVUx(void) mVUsaveReg2(int reg, int gprReg, u32 offset, int xyzw) {
	/*SSE_MOVAPSRmtoR(xmmT2, gprReg, offset);
	if (xyzw == 8 || xyzw == 4 || xyzw == 2 || xyzw == 1) {
		mVUunpack_xyzw<vuIndex>(reg, reg, 0);
	}
	mVUmergeRegs<vuIndex>(xmmT2, reg, xyzw);
	SSE_MOVAPSRtoRm(gprReg, xmmT2, offset);
	return;*/

	switch ( xyzw ) {
		case 5:		SSE_SHUFPS_XMM_to_XMM(reg, reg, 0xe1); //WZXY
					SSE_MOVSS_XMM_to_Rm(gprReg, reg, offset+4);
					SSE_SHUFPS_XMM_to_XMM(reg, reg, 0xff); //WWWW
					SSE_MOVSS_XMM_to_Rm(gprReg, reg, offset+12);
					break; // YW
		case 6:		SSE_SHUFPS_XMM_to_XMM(reg, reg, 0xc9);
					SSE_MOVLPS_XMM_to_Rm(gprReg, reg, offset+4);
					break; // YZ
		case 7:		SSE_SHUFPS_XMM_to_XMM(reg, reg, 0x93); //ZYXW
					SSE_MOVHPS_XMM_to_Rm(gprReg, reg, offset+4);
					SSE_MOVSS_XMM_to_Rm(gprReg, reg, offset+12);
					break; // YZW
		case 9:		SSE_MOVSS_XMM_to_Rm(gprReg, reg, offset);
					SSE_SHUFPS_XMM_to_XMM(reg, reg, 0xff); //WWWW
					SSE_MOVSS_XMM_to_Rm(gprReg, reg, offset+12);
					break; // XW
		case 10:	SSE_MOVSS_XMM_to_Rm(gprReg, reg, offset);
					SSE_MOVHLPS_XMM_to_XMM(reg, reg);
					SSE_MOVSS_XMM_to_Rm(gprReg, reg, offset+8);
					break; //XZ
		case 11:	SSE_MOVSS_XMM_to_Rm(gprReg, reg, offset);
					SSE_MOVHPS_XMM_to_Rm(gprReg, reg, offset+8);
					break; //XZW
		case 13:	SSE_SHUFPS_XMM_to_XMM(reg, reg, 0x4b); //YXZW				
					SSE_MOVHPS_XMM_to_Rm(gprReg, reg, offset);
					SSE_MOVSS_XMM_to_Rm(gprReg, reg, offset+12);
					break; // XYW
		case 14:	SSE_MOVLPS_XMM_to_Rm(gprReg, reg, offset);
					SSE_MOVHLPS_XMM_to_XMM(reg, reg);
					SSE_MOVSS_XMM_to_Rm(gprReg,  reg, offset+8);
					break; // XYZ
		case 8:		SSE_MOVSS_XMM_to_Rm(gprReg, reg, offset);	 break; // X
		case 4:		SSE_MOVSS_XMM_to_Rm(gprReg, reg, offset+4);	 break; // Y
		case 2:		SSE_MOVSS_XMM_to_Rm(gprReg, reg, offset+8);	 break; // Z
		case 1:		SSE_MOVSS_XMM_to_Rm(gprReg, reg, offset+12); break; // W
		case 12:	SSE_MOVLPS_XMM_to_Rm(gprReg, reg, offset);	 break; // XY
		case 3:		SSE_MOVHPS_XMM_to_Rm(gprReg, reg, offset+8); break; // ZW
		default:	SSE_MOVAPSRtoRm(gprReg, reg, offset);		 break; // XYZW
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
						 SSE2_PSHUFD_XMM_to_XMM(dest, dest, 0xd8);
						 break;
				case 6:	 SSE_SHUFPS_XMM_to_XMM(dest, src, 0x9c);
						 SSE2_PSHUFD_XMM_to_XMM(dest, dest, 0x78);
						 break;
				case 7:	 SSE_MOVSS_XMM_to_XMM(src, dest);
						 SSE_MOVAPS_XMM_to_XMM(dest, src);
						 break;
				case 8:	 SSE_MOVSS_XMM_to_XMM(dest, src);
						 break;
				case 9:	 SSE_SHUFPS_XMM_to_XMM(dest, src, 0xc9);
						 SSE2_PSHUFD_XMM_to_XMM(dest, dest, 0xd2);
						 break;
				case 10: SSE_SHUFPS_XMM_to_XMM(dest, src, 0x8d);
						 SSE2_PSHUFD_XMM_to_XMM(dest, dest, 0x72);
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

// Transforms the Address in gprReg to valid VU0/VU1 Address
microVUt(void) mVUaddrFix(int gprReg) {
	if (vuIndex) {
		AND32ItoR(gprReg, 0x3ff); // wrap around
		SHL32ItoR(gprReg, 4);
	}
	else {
		u8 *jmpA, *jmpB; 
		CMP32ItoR(gprReg, 0x400);
		jmpA = JL8(0); // if addr >= 0x4000, reads VU1's VF regs and VI regs
			AND32ItoR(gprReg, 0x43f); // ToDo: theres a potential problem if VU0 overrides VU1's VF0/VI0 regs!
			jmpB = JMP8(0);
		x86SetJ8(jmpA);
			AND32ItoR(gprReg, 0xff); // if addr < 0x4000, wrap around
		x86SetJ8(jmpB);
		SHL32ItoR(gprReg, 4); // multiply by 16 (shift left by 4)
	}
}

// Backup Volatile Regs (EAX, ECX, EDX, MM0~7, XMM0~7, are all volatile according to 32bit Win/Linux ABI)
microVUt(void) mVUbackupRegs() {
	microVU* mVU = mVUx;
	SSE_MOVAPS_XMM_to_M128((uptr)&mVU->regs->ACC.UL[0], xmmACC);
	SSE_MOVAPS_XMM_to_M128((uptr)&mVU->xmmPQb[0], xmmPQ);
	PUSH32R(gprR); // Backup EDX
}

// Restore Volatile Regs
microVUt(void) mVUrestoreRegs() {
	microVU* mVU = mVUx;
	SSE_MOVAPS_M128_to_XMM(xmmACC, (uptr)&mVU->regs->ACC.UL[0]);
	SSE_MOVAPS_M128_to_XMM(xmmPQ,  (uptr)&mVU->xmmPQb[0]);
	SSE_MOVAPS_M128_to_XMM(xmmMax, (uptr)mVU_maxvals);
	SSE_MOVAPS_M128_to_XMM(xmmMin, (uptr)mVU_minvals);
	POP32R(gprR); // Restore EDX
}

#endif //PCSX2_MICROVU
