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
// Micro VU - recPass 0 Functions
//------------------------------------------------------------------

//------------------------------------------------------------------
// Micro VU - recPass 1 Functions
//------------------------------------------------------------------

#define makeFdFd (makeFd == 0)
#define makeFdFs (makeFd == 1)

#define getReg(reg, _reg_) {  \
	mVUloadReg<vuIndex>(reg, (uptr)&mVU->regs->VF[_reg_].UL[0], _X_Y_Z_W);  \
	if (CHECK_VU_EXTRA_OVERFLOW) mVUclamp2<vuIndex>(reg, xmmT1, _X_Y_Z_W);  \
}

#define getZeroSS(reg) {  \
	if (_W)	{ mVUloadReg<vuIndex>(reg, (uptr)&mVU->regs->VF[0].UL[0], _X_Y_Z_W); }  \
	else	{ SSE_XORPS_XMM_to_XMM(reg, reg); }  \
}

#define getZero(reg) {  \
	if (_W)	{ mVUloadReg<vuIndex>(reg, (uptr)&mVU->regs->VF[0].UL[0], _X_Y_Z_W); }  \
	else	{ SSE_XORPS_XMM_to_XMM(reg, reg); }  \
}

// Note: If _Ft_ is 0, then don't modify xmm reg Ft, because its equal to xmmZ (unless _XYZW_SS, then you can modify xmm reg Ft)
microVUt(void) mVUallocFMAC1a(int& Fd, int& Fs, int& Ft, const bool makeFd) {
	microVU* mVU = mVUx;
	Fs = xmmFs;
	Ft = xmmFt;
	if (_XYZW_SS) {
		if (!_Fs_)	{ getZeroSS(Fs); }
		else		{ getReg(Fs, _Fs_); }

		if (_Ft_ == _Fs_) { Ft = Fs; }
		else {
			if (!_Ft_)	{ getZeroSS(Ft); }
			else		{ getReg(Ft, _Ft_); }
		}
	}
	else {
		if (!_Fs_)	{ getZero(Fs); }
		else		{ getReg(Fs, _Fs_); }
		
		if (_Ft_ == _Fs_) { Ft = Fs; }
		else {
			if (!_Ft_)	{ getZero(Ft); } 
			else		{ getReg(Ft, _Ft_); }
		}
	}
	if (makeFdFs) {Fd = Fs;}
	else		  {Fd = xmmFd;}
}

microVUt(void) mVUallocFMAC1b(int& Fd) {
	microVU* mVU = mVUx;
	if (!_Fd_) return;
	if (CHECK_VU_OVERFLOW) mVUclamp1<vuIndex>(Fd, xmmT1, _X_Y_Z_W);
	mVUsaveReg<vuIndex>(Fd, (uptr)&mVU->regs->VF[_Fd_].UL[0], _X_Y_Z_W);
}

#endif //PCSX2_MICROVU
