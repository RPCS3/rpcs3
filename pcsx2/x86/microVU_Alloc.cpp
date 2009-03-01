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
// Micro VU - recPass 0 Functions
//------------------------------------------------------------------

//------------------------------------------------------------------
// Micro VU - recPass 1 Functions
//------------------------------------------------------------------
/*
#define setFd (mVU->prog.prog[mVU->prog.cur].allocInfo.info[pc] & (1<<7))
#define getFd (mVU->prog.prog[mVU->prog.cur].allocInfo.info[pc] & (1<<1))
#define getFs (mVU->prog.prog[mVU->prog.cur].allocInfo.info[pc] & (1<<2))
#define getFt (mVU->prog.prog[mVU->prog.cur].allocInfo.info[pc] & (1<<3))
*/
#define makeFdFd (makeFd == 0)
#define makeFdFs (makeFd == 1)
#define makeFdFt (makeFd == 2)

microVUt(void) mVUallocFMAC1a(u32 code, int& Fd, int& Fs, int& Ft, const int makeFd) {
	microVU* mVU = mVUx;
	if (_Fs_ == 0) { Fs = xmmZ; } else { Fs = xmmFs; }
	if (_Ft_ == 0) { Ft = xmmZ; } else { Ft = xmmFt; }
	if		(makeFdFd) {Fd = xmmFd;}
	else if (makeFdFs) {Fd = Fs;}
	else if (makeFdFt) {Fd = Ft;}

	if (_Fs_) SSE_MOVAPS_M128_to_XMM(Fs, (uptr)&mVU->regs->VF[_Fs_].UL[0]);
	if (_Ft_ == _Ft_) SSE_MOVAPS_M128_to_XMM(Ft, (uptr)&mVU->regs->VF[_Ft_].UL[0]);
}

microVUt(void) mVUallocFMAC1b(u32 code, u32 pc, int& Fd) {
	microVU* mVU = mVUx;
	if (_Fd_ == 0) return;
	else mVUsaveReg<vuIndex>(code, Fd, (uptr)&mVU->regs->VF[_Fd_].UL[0]);
}

#endif //PCSX2_MICROVU
