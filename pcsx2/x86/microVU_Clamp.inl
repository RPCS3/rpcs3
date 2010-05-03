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

//------------------------------------------------------------------
// Micro VU - Clamp Functions
//------------------------------------------------------------------

const __aligned16 u32 sse4_minvals[2][4] = {
   { 0xff7fffff, 0xffffffff, 0xffffffff, 0xffffffff }, //1000
   { 0xff7fffff, 0xff7fffff, 0xff7fffff, 0xff7fffff }, //1111
};
const __aligned16 u32 sse4_maxvals[2][4] = {
   { 0x7f7fffff, 0x7fffffff, 0x7fffffff, 0x7fffffff }, //1000
   { 0x7f7fffff, 0x7f7fffff, 0x7f7fffff, 0x7f7fffff }, //1111
};

// Used for Result Clamping
// Note: This function will not preserve NaN values' sign.
// The theory behind this is that when we compute a result, and we've
// gotten a NaN value, then something went wrong; and the NaN's sign
// is not to be trusted. Games like positive values better usually, 
// and its faster... so just always make NaNs into positive infinity.
void mVUclamp1(int reg, int regT1, int xyzw, bool bClampE = 0) {
	if ((!clampE && CHECK_VU_OVERFLOW) || (clampE && bClampE)) {
		switch (xyzw) {
			case 1: case 2: case 4: case 8:
				SSE_MINSS_M32_to_XMM(reg, (uptr)mVUglob.maxvals);
				SSE_MAXSS_M32_to_XMM(reg, (uptr)mVUglob.minvals);
				break;
			default:
				SSE_MINPS_M128_to_XMM(reg, (uptr)mVUglob.maxvals);
				SSE_MAXPS_M128_to_XMM(reg, (uptr)mVUglob.minvals);
				break;
		}
	}
}

// Used for Operand Clamping
// Note 1: If 'preserve sign' mode is on, it will preserve the sign of NaN values.
// Note 2: Using regalloc here seems to contaminate some regs in certain games.
// Must be some specific case I've overlooked (or I used regalloc improperly on an opcode)
// so we just use a temporary mem location for our backup for now... (non-sse4 version only)
void mVUclamp2(microVU* mVU, int reg, int regT1, int xyzw, bool bClampE = 0) {
	if ((!clampE && CHECK_VU_SIGN_OVERFLOW) || (clampE && bClampE && CHECK_VU_SIGN_OVERFLOW)) {
		if (x86caps.hasStreamingSIMD4Extensions) {
			int i = (xyzw==1||xyzw==2||xyzw==4||xyzw==8) ? 0: 1;
			SSE4_PMINSD_M128_to_XMM(reg, (uptr)&sse4_maxvals[i][0]);
			SSE4_PMINUD_M128_to_XMM(reg, (uptr)&sse4_minvals[i][0]);
			return;
		}
		int regT1b = 0;
		if (regT1 < 0) { 
			regT1b = 1; regT1=(reg+1)%8;
			SSE_MOVAPS_XMM_to_M128((uptr)mVU->xmmCTemp, regT1); 
			//regT1 = mVU->regAlloc->allocReg();
		}
		switch (xyzw) {
			case 1: case 2: case 4: case 8:
				SSE_MOVAPS_XMM_to_XMM(regT1, reg);
				SSE_ANDPS_M128_to_XMM(regT1, (uptr)mVUglob.signbit);
				SSE_MINSS_M32_to_XMM (reg,   (uptr)mVUglob.maxvals);
				SSE_MAXSS_M32_to_XMM (reg,   (uptr)mVUglob.minvals);
				SSE_ORPS_XMM_to_XMM  (reg, regT1);
				break;
			default:
				SSE_MOVAPS_XMM_to_XMM(regT1, reg);
				SSE_ANDPS_M128_to_XMM(regT1, (uptr)mVUglob.signbit);
				SSE_MINPS_M128_to_XMM(reg,   (uptr)mVUglob.maxvals);
				SSE_MAXPS_M128_to_XMM(reg,   (uptr)mVUglob.minvals);
				SSE_ORPS_XMM_to_XMM  (reg, regT1);
				break;
		}
		//if (regT1b) mVU->regAlloc->clearNeeded(regT1);
		if (regT1b) SSE_MOVAPS_M128_to_XMM(regT1, (uptr)mVU->xmmCTemp);
	}
	else mVUclamp1(reg, regT1, xyzw, bClampE);
}

// Used for operand clamping on every SSE instruction (add/sub/mul/div)
void mVUclamp3(microVU* mVU, int reg, int regT1, int xyzw) {
	if (clampE) mVUclamp2(mVU, reg, regT1, xyzw, 1);
}

// Used for result clamping on every SSE instruction (add/sub/mul/div)
// Note: Disabled in "preserve sign" mode because in certain cases it
// makes too much code-gen, and you get jump8-overflows in certain
// emulated opcodes (causing crashes). Since we're clamping the operands 
// with mVUclamp3, we should almost never be getting a NaN result, 
// but this clamp is just a precaution just-in-case.
void mVUclamp4(int reg, int regT1, int xyzw) {
	if (clampE && !CHECK_VU_SIGN_OVERFLOW) mVUclamp1(reg, regT1, xyzw, 1);
}
