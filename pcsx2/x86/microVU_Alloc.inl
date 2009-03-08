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

//------------------------------------------------------------------
// FMAC1 - Normal FMAC Opcodes
//------------------------------------------------------------------

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

microVUt(void) mVUallocFMAC1a(int& Fd, int& Fs, int& Ft) {
	microVU* mVU = mVUx;
	Fs = xmmFs;
	Ft = xmmFt;
	Fd = xmmFs;
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
}

microVUt(void) mVUallocFMAC1b(int& Fd) {
	microVU* mVU = mVUx;
	if (!_Fd_) return;
	if (CHECK_VU_OVERFLOW) mVUclamp1<vuIndex>(Fd, xmmT1, _X_Y_Z_W);
	mVUsaveReg<vuIndex>(Fd, (uptr)&mVU->regs->VF[_Fd_].UL[0], _X_Y_Z_W);
}

//------------------------------------------------------------------
// FMAC2 - ABS/FTOI/ITOF Opcodes
//------------------------------------------------------------------

microVUt(void) mVUallocFMAC2a(int& Fs, int& Ft) {
	microVU* mVU = mVUx;
	Fs = xmmFs;
	Ft = xmmFs;
	if (_XYZW_SS) {
		if (!_Fs_)	{ getZeroSS(Fs); }
		else		{ getReg(Fs, _Fs_); }
	}
	else {
		if (!_Fs_)	{ getZero(Fs); }
		else		{ getReg(Fs, _Fs_); }
	}
}

microVUt(void) mVUallocFMAC2b(int& Ft) {
	microVU* mVU = mVUx;
	if (!_Ft_) return;
	//if (CHECK_VU_OVERFLOW) mVUclamp1<vuIndex>(Ft, xmmT1, _X_Y_Z_W);
	mVUsaveReg<vuIndex>(Ft, (uptr)&mVU->regs->VF[_Ft_].UL[0], _X_Y_Z_W);
}

//------------------------------------------------------------------
// FMAC3 - BC(xyzw) FMAC Opcodes
//------------------------------------------------------------------

#define getReg3SS(reg, _reg_) {  \
	mVUloadReg<vuIndex>(reg, (uptr)&mVU->regs->VF[_reg_].UL[0], (1 << (3 - _bc_)));  \
	if (CHECK_VU_EXTRA_OVERFLOW) mVUclamp2<vuIndex>(reg, xmmT1, (1 << (3 - _bc_)));  \
}

#define getReg3(reg, _reg_) {  \
	mVUloadReg<vuIndex>(reg, (uptr)&mVU->regs->VF[_reg_].UL[0], (1 << (3 - _bc_)));  \
	if (CHECK_VU_EXTRA_OVERFLOW) mVUclamp2<vuIndex>(reg, xmmT1, (1 << (3 - _bc_)));  \
	mVUunpack_xyzw<vuIndex>(reg, reg, _bc_);  \
}

#define getZero3SS(reg) {  \
	if (_bc_w) { mVUloadReg<vuIndex>(reg, (uptr)&mVU->regs->VF[0].UL[0], 1); }  \
	else { SSE_XORPS_XMM_to_XMM(reg, reg); }  \
}

#define getZero3(reg) {  \
	if (_bc_w)	{  \
		mVUloadReg<vuIndex>(reg, (uptr)&mVU->regs->VF[0].UL[0], 1);  \
		mVUunpack_xyzw<vuIndex>(reg, reg, _bc_);  \
	}  \
	else { SSE_XORPS_XMM_to_XMM(reg, reg); }  \
}

microVUt(void) mVUallocFMAC3a(int& Fd, int& Fs, int& Ft) {
	microVU* mVU = mVUx;
	Fs = xmmFs;
	Ft = xmmFt;
	Fd = xmmFs;
	if (_XYZW_SS) {
		if (!_Fs_)	{ getZeroSS(Fs); }
		else		{ getReg(Fs, _Fs_); }

		if ( (_Ft_ == _Fs_) && ((_X && _bc_x) || (_Y && _bc_y) || (_Z && _bc_w) || (_W && _bc_w)) ) {
			Ft = Fs; 
		}
		else {
			if (!_Ft_)	{ getZero3SS(Ft); }
			else		{ getReg3SS(Ft, _Ft_); }
		}
	}
	else {
		if (!_Fs_)	{ getZero(Fs); }
		else		{ getReg(Fs, _Fs_); }

		if (!_Ft_)	{ getZero3(Ft); } 
		else		{ getReg3(Ft, _Ft_); }
	}
}

microVUt(void) mVUallocFMAC3b(int& Fd) {
	mVUallocFMAC1b<vuIndex>(Fd);
}

//------------------------------------------------------------------
// FMAC4 - FMAC Opcodes Storing Result to ACC
//------------------------------------------------------------------

#define getReg4(reg, _reg_) {  \
	mVUloadReg<vuIndex>(reg, (uptr)&mVU->regs->VF[_reg_].UL[0], _xyzw_ACC);  \
	if (CHECK_VU_EXTRA_OVERFLOW) mVUclamp2<vuIndex>(reg, xmmT1, _xyzw_ACC);  \
}

#define getZero4(reg) {  \
	if (_W)	{ mVUloadReg<vuIndex>(reg, (uptr)&mVU->regs->VF[0].UL[0], _xyzw_ACC); }  \
	else	{ SSE_XORPS_XMM_to_XMM(reg, reg); }  \
}

#define getACC(reg) {  \
	reg = xmmACC0 + writeACC;  \
	if (_X_Y_Z_W != 15) { SSE_MOVAPS_XMM_to_XMM(reg, (xmmACC0 + prevACC)); }  \
}

microVUt(void) mVUallocFMAC4a(int& ACC, int& Fs, int& Ft) {
	microVU* mVU = mVUx;
	Fs = xmmFs;
	Ft = xmmFt;
	getACC(ACC);
	if (_XYZW_SS && _X) {
		if (!_Fs_)	{ getZeroSS(Fs); }
		else		{ getReg(Fs, _Fs_); }

		if (_Ft_ == _Fs_) { Ft = Fs; }
		else {
			if (!_Ft_)	{ getZeroSS(Ft); }
			else		{ getReg(Ft, _Ft_); }
		}
	}
	else {
		if (!_Fs_)	{ getZero4(Fs); }
		else		{ getReg4(Fs, _Fs_); }

		if (_Ft_ == _Fs_) { Ft = Fs; }
		else {
			if (!_Ft_)	{ getZero4(Ft); } 
			else		{ getReg4(Ft, _Ft_); }
		}
	}
}

microVUt(void) mVUallocFMAC4b(int& ACC, int& Fs) {
	microVU* mVU = mVUx;
	if (CHECK_VU_OVERFLOW) mVUclamp1<vuIndex>(Fs, xmmT1, _xyzw_ACC);
	mVUmergeRegs<vuIndex>(ACC, Fs, _X_Y_Z_W);
}

//------------------------------------------------------------------
// FMAC5 - FMAC BC(xyzw) Opcodes Storing Result to ACC
//------------------------------------------------------------------

microVUt(void) mVUallocFMAC5a(int& ACC, int& Fs, int& Ft) {
	microVU* mVU = mVUx;
	Fs = xmmFs;
	Ft = xmmFt;
	getACC(ACC);
	if (_XYZW_SS && _X) {
		if (!_Fs_)	{ getZeroSS(Fs); }
		else		{ getReg(Fs, _Fs_); }

		if ( (_Ft_ == _Fs_) && _bc_x) {
			Ft = Fs; 
		}
		else {
			if (!_Ft_)	{ getZero3SS(Ft); }
			else		{ getReg3SS(Ft, _Ft_); }
		}
	}
	else {
		if (!_Fs_)	{ getZero4(Fs); }
		else		{ getReg4(Fs, _Fs_); }

		if (!_Ft_)	{ getZero3(Ft); } 
		else		{ getReg3(Ft, _Ft_); }
	}
}

microVUt(void) mVUallocFMAC5b(int& ACC, int& Fs) {
	mVUallocFMAC4b<vuIndex>(ACC, Fs);
}

//------------------------------------------------------------------
// FMAC6 - Normal FMAC Opcodes (I Reg)
//------------------------------------------------------------------

#define getIreg(reg) {  \
	MOV32ItoR(gprT1, mVU->iReg);  \
	SSE2_MOVD_R_to_XMM(reg, gprT1);  \
	if (CHECK_VU_EXTRA_OVERFLOW) mVUclamp2<vuIndex>(reg, xmmT1, 8);  \
	if (!_XYZW_SS) { mVUunpack_xyzw<vuIndex>(reg, reg, 0); }  \
}

microVUt(void) mVUallocFMAC6a(int& Fd, int& Fs, int& Ft) {
	microVU* mVU = mVUx;
	Fs = xmmFs;
	Ft = xmmFt;
	Fd = xmmFs;
	getIreg(Ft);
	if (_XYZW_SS) {
		if (!_Fs_)	{ getZeroSS(Fs); }
		else		{ getReg(Fs, _Fs_); }
	}
	else {
		if (!_Fs_)	{ getZero(Fs); }
		else		{ getReg(Fs, _Fs_); }
	}
}

microVUt(void) mVUallocFMAC6b(int& Fd) {
	mVUallocFMAC1b<vuIndex>(Fd);
}

//------------------------------------------------------------------
// FMAC7 - FMAC Opcodes Storing Result to ACC (I Reg)
//------------------------------------------------------------------

microVUt(void) mVUallocFMAC7a(int& ACC, int& Fs, int& Ft) {
	microVU* mVU = mVUx;
	Fs = xmmFs;
	Ft = xmmFt;
	getACC(ACC);
	getIreg(Ft);
	if (_XYZW_SS && _X) {
		if (!_Fs_)	{ getZeroSS(Fs); }
		else		{ getReg(Fs, _Fs_); }
	}
	else {
		if (!_Fs_)	{ getZero4(Fs); }
		else		{ getReg4(Fs, _Fs_); }
	}
}

microVUt(void) mVUallocFMAC7b(int& ACC, int& Fs) {
	mVUallocFMAC4b<vuIndex>(ACC, Fs);
}

//------------------------------------------------------------------
// FMAC8 - MADD FMAC Opcode Storing Result to Fd
//------------------------------------------------------------------

microVUt(void) mVUallocFMAC8a(int& Fd, int&ACC, int& Fs, int& Ft) {
	microVU* mVU = mVUx;
	Fs = xmmFs;
	Ft = xmmFt;
	Fd = xmmFs;
	ACC = xmmACC0 + readACC;
	if (_XYZW_SS && _X) {
		if (!_Fs_)	{ getZeroSS(Fs); }
		else		{ getReg(Fs, _Fs_); }

		if (_Ft_ == _Fs_) { Ft = Fs; }
		else {
			if (!_Ft_)	{ getZeroSS(Ft); }
			else		{ getReg(Ft, _Ft_); }
		}
	}
	else {
		if (!_Fs_)	{ getZero4(Fs); }
		else		{ getReg4(Fs, _Fs_); }

		if (_Ft_ == _Fs_) { Ft = Fs; }
		else {
			if (!_Ft_)	{ getZero4(Ft); } 
			else		{ getReg4(Ft, _Ft_); }
		}
	}
}

microVUt(void) mVUallocFMAC8b(int& Fd) {
	microVU* mVU = mVUx;
	if (!_Fd_) return;
	if (CHECK_VU_OVERFLOW) mVUclamp1<vuIndex>(Fd, xmmT1, _xyzw_ACC);
	mVUsaveReg<vuIndex>(Fd, (uptr)&mVU->regs->VF[_Fd_].UL[0], _X_Y_Z_W);
}

//------------------------------------------------------------------
// FMAC9 - MSUB FMAC Opcode Storing Result to Fd
//------------------------------------------------------------------

microVUt(void) mVUallocFMAC9a(int& Fd, int&ACC, int& Fs, int& Ft) {
	microVU* mVU = mVUx;
	Fs = xmmFs;
	Ft = xmmFt;
	Fd = xmmT1;
	ACC = xmmT1;
	SSE_MOVAPS_XMM_to_XMM(ACC, xmmACC0 + readACC);
	if (_XYZW_SS && _X) {
		if (!_Fs_)	{ getZeroSS(Fs); }
		else		{ getReg(Fs, _Fs_); }

		if (_Ft_ == _Fs_) { Ft = Fs; }
		else {
			if (!_Ft_)	{ getZeroSS(Ft); }
			else		{ getReg(Ft, _Ft_); }
		}
	}
	else {
		if (!_Fs_)	{ getZero4(Fs); }
		else		{ getReg4(Fs, _Fs_); }

		if (_Ft_ == _Fs_) { Ft = Fs; }
		else {
			if (!_Ft_)	{ getZero4(Ft); } 
			else		{ getReg4(Ft, _Ft_); }
		}
	}
}

microVUt(void) mVUallocFMAC9b(int& Fd) {
	microVU* mVU = mVUx;
	if (!_Fd_) return;
	if (CHECK_VU_OVERFLOW) mVUclamp1<vuIndex>(Fd, xmmFt, _xyzw_ACC);
	mVUsaveReg<vuIndex>(Fd, (uptr)&mVU->regs->VF[_Fd_].UL[0], _X_Y_Z_W);
}

//------------------------------------------------------------------
// FMAC10 - MADD FMAC BC(xyzw) Opcode Storing Result to Fd
//------------------------------------------------------------------

microVUt(void) mVUallocFMAC10a(int& Fd, int& ACC, int& Fs, int& Ft) {
	microVU* mVU = mVUx;
	Fs = xmmFs;
	Ft = xmmFt;
	Fd = xmmFs;
	ACC = xmmACC0 + readACC;
	if (_XYZW_SS && _X) {
		if (!_Fs_)	{ getZeroSS(Fs); }
		else		{ getReg(Fs, _Fs_); }

		if ( (_Ft_ == _Fs_) && _bc_x) {
			Ft = Fs; 
		}
		else {
			if (!_Ft_)	{ getZero3SS(Ft); }
			else		{ getReg3SS(Ft, _Ft_); }
		}
	}
	else {
		if (!_Fs_)	{ getZero4(Fs); }
		else		{ getReg4(Fs, _Fs_); }

		if (!_Ft_)	{ getZero3(Ft); } 
		else		{ getReg3(Ft, _Ft_); }
	}
}

microVUt(void) mVUallocFMAC10b(int& Fd) {
	mVUallocFMAC8b<vuIndex>(Fd);
}

//------------------------------------------------------------------
// FMAC11 - MSUB FMAC BC(xyzw) Opcode Storing Result to Fd
//------------------------------------------------------------------

microVUt(void) mVUallocFMAC11a(int& Fd, int& ACC, int& Fs, int& Ft) {
	microVU* mVU = mVUx;
	Fs = xmmFs;
	Ft = xmmFt;
	Fd = xmmT1;
	ACC = xmmT1;
	SSE_MOVAPS_XMM_to_XMM(ACC, xmmACC0 + readACC);
	if (_XYZW_SS && _X) {
		if (!_Fs_)	{ getZeroSS(Fs); }
		else		{ getReg(Fs, _Fs_); }

		if ( (_Ft_ == _Fs_) && _bc_x) {
			Ft = Fs; 
		}
		else {
			if (!_Ft_)	{ getZero3SS(Ft); }
			else		{ getReg3SS(Ft, _Ft_); }
		}
	}
	else {
		if (!_Fs_)	{ getZero4(Fs); }
		else		{ getReg4(Fs, _Fs_); }

		if (!_Ft_)	{ getZero3(Ft); } 
		else		{ getReg3(Ft, _Ft_); }
	}
}

microVUt(void) mVUallocFMAC11b(int& Fd) {
	mVUallocFMAC9b<vuIndex>(Fd);
}

//------------------------------------------------------------------
// FMAC12 - MADD FMAC Opcode Storing Result to Fd (I Reg)
//------------------------------------------------------------------

microVUt(void) mVUallocFMAC12a(int& Fd, int&ACC, int& Fs, int& Ft) {
	microVU* mVU = mVUx;
	Fs = xmmFs;
	Ft = xmmFt;
	Fd = xmmFs;
	ACC = xmmACC0 + readACC;
	getIreg(Ft);
	if (_XYZW_SS && _X) {
		if (!_Fs_)	{ getZeroSS(Fs); }
		else		{ getReg(Fs, _Fs_); }
	}
	else {
		if (!_Fs_)	{ getZero4(Fs); }
		else		{ getReg4(Fs, _Fs_); }
	}
}

microVUt(void) mVUallocFMAC12b(int& Fd) {
	mVUallocFMAC8b<vuIndex>(Fd);
}

//------------------------------------------------------------------
// FMAC13 - MSUB FMAC Opcode Storing Result to Fd (I Reg)
//------------------------------------------------------------------

microVUt(void) mVUallocFMAC13a(int& Fd, int&ACC, int& Fs, int& Ft) {
	microVU* mVU = mVUx;
	Fs = xmmFs;
	Ft = xmmFt;
	Fd = xmmT1;
	ACC = xmmT1;
	SSE_MOVAPS_XMM_to_XMM(ACC, xmmACC0 + readACC);
	getIreg(Ft);
	if (_XYZW_SS && _X) {
		if (!_Fs_)	{ getZeroSS(Fs); }
		else		{ getReg(Fs, _Fs_); }
	}
	else {
		if (!_Fs_)	{ getZero4(Fs); }
		else		{ getReg4(Fs, _Fs_); }
	}
}

microVUt(void) mVUallocFMAC13b(int& Fd) {
	mVUallocFMAC9b<vuIndex>(Fd);
}

//------------------------------------------------------------------
// FMAC14 - MADDA FMAC Opcode
//------------------------------------------------------------------

microVUt(void) mVUallocFMAC14a(int& ACCw, int&ACCr, int& Fs, int& Ft) {
	microVU* mVU = mVUx;
	getACC(ACCw);
	Fs = (_X_Y_Z_W == 15) ? ACCw : xmmFs;
	Ft = xmmFt;
	ACCr = xmmACC0 + readACC;
	if (_XYZW_SS && _X) {
		if (!_Fs_)	{ getZeroSS(Fs); }
		else		{ getReg(Fs, _Fs_); }

		if (_Ft_ == _Fs_) { Ft = Fs; }
		else {
			if (!_Ft_)	{ getZeroSS(Ft); }
			else		{ getReg(Ft, _Ft_); }
		}
	}
	else {
		if (!_Fs_)	{ getZero4(Fs); }
		else		{ getReg4(Fs, _Fs_); }

		if (_Ft_ == _Fs_) { Ft = Fs; }
		else {
			if (!_Ft_)	{ getZero4(Ft); } 
			else		{ getReg4(Ft, _Ft_); }
		}
	}
}

microVUt(void) mVUallocFMAC14b(int& ACCw, int& Fs) {
	microVU* mVU = mVUx;
	if (CHECK_VU_OVERFLOW) mVUclamp1<vuIndex>(Fs, xmmT1, _xyzw_ACC);
	mVUmergeRegs<vuIndex>(ACCw, Fs, _X_Y_Z_W);
}

//------------------------------------------------------------------
// FMAC15 - MSUBA FMAC Opcode
//------------------------------------------------------------------

microVUt(void) mVUallocFMAC15a(int& ACCw, int&ACCr, int& Fs, int& Ft) {
	mVUallocFMAC14a<vuIndex>(ACCw, ACCr, Fs, Ft);
	SSE_MOVAPS_XMM_to_XMM(xmmT1, ACCr);
	ACCr = xmmT1;
}

microVUt(void) mVUallocFMAC15b(int& ACCw, int& ACCr) {
	microVU* mVU = mVUx;
	if (CHECK_VU_OVERFLOW) mVUclamp1<vuIndex>(ACCr, xmmFt, _xyzw_ACC);
	mVUmergeRegs<vuIndex>(ACCw, ACCr, _X_Y_Z_W);
}

//------------------------------------------------------------------
// FMAC16 - MADDA BC(xyzw) FMAC Opcode
//------------------------------------------------------------------

microVUt(void) mVUallocFMAC16a(int& ACCw, int&ACCr, int& Fs, int& Ft) {
	microVU* mVU = mVUx;
	getACC(ACCw);
	Fs = (_X_Y_Z_W == 15) ? ACCw : xmmFs;
	Ft = xmmFt;
	ACCr = xmmACC0 + readACC;
	if (_XYZW_SS && _X) {
		if (!_Fs_)	{ getZeroSS(Fs); }
		else		{ getReg(Fs, _Fs_); }

		if ( (_Ft_ == _Fs_) && _bc_x) {
			Ft = Fs; 
		}
		else {
			if (!_Ft_)	{ getZero3SS(Ft); }
			else		{ getReg3SS(Ft, _Ft_); }
		}
	}
	else {
		if (!_Fs_)	{ getZero4(Fs); }
		else		{ getReg4(Fs, _Fs_); }

		if (!_Ft_)	{ getZero3(Ft); } 
		else		{ getReg3(Ft, _Ft_); }
	}
}

microVUt(void) mVUallocFMAC16b(int& ACCw, int& Fs) {
	mVUallocFMAC14b<vuIndex>(ACCw, Fs);
}

//------------------------------------------------------------------
// FMAC17 - MSUBA BC(xyzw) FMAC Opcode
//------------------------------------------------------------------

microVUt(void) mVUallocFMAC17a(int& ACCw, int&ACCr, int& Fs, int& Ft) {
	mVUallocFMAC16a<vuIndex>(ACCw, ACCr, Fs, Ft);
	SSE_MOVAPS_XMM_to_XMM(xmmT1, ACCr);
	ACCr = xmmT1;
}

microVUt(void) mVUallocFMAC17b(int& ACCw, int& ACCr) {
	mVUallocFMAC15b<vuIndex>(ACCw, ACCr);
}

//------------------------------------------------------------------
// FMAC18 - OPMULA FMAC Opcode
//------------------------------------------------------------------

microVUt(void) mVUallocFMAC18a(int& ACC, int& Fs, int& Ft) {
	microVU* mVU = mVUx;
	Fs = xmmFs;
	Ft = xmmFt;
	getACC(ACC);

	if (!_Fs_)	{ getZero4(Fs); }
	else		{ getReg4(Fs, _Fs_); }

	if (!_Ft_)	{ getZero4(Ft); } 
	else		{ getReg4(Ft, _Ft_); }

	SSE_SHUFPS_XMM_to_XMM(Fs, Fs, 0xC9); // WXZY
	SSE_SHUFPS_XMM_to_XMM(Ft, Ft, 0xD2); // WYXZ
}

microVUt(void) mVUallocFMAC18b(int& ACC, int& Fs) {
	mVUallocFMAC4b<vuIndex>(ACC, Fs);
}

//------------------------------------------------------------------
// FMAC19 - OPMULA FMAC Opcode
//------------------------------------------------------------------

microVUt(void) mVUallocFMAC19a(int& Fd, int&ACC, int& Fs, int& Ft) {
	microVU* mVU = mVUx;
	Fs = xmmFs;
	Ft = xmmFt;
	Fd = xmmT1;
	ACC = xmmT1;
	SSE_MOVAPS_XMM_to_XMM(ACC, xmmACC0 + readACC);

	if (!_Fs_)	{ getZero4(Fs); }
	else		{ getReg4(Fs, _Fs_); }

	if (!_Ft_)	{ getZero4(Ft); } 
	else		{ getReg4(Ft, _Ft_); }

	SSE_SHUFPS_XMM_to_XMM(Fs, Fs, 0xC9); // WXZY
	SSE_SHUFPS_XMM_to_XMM(Ft, Ft, 0xD2); // WYXZ
}

microVUt(void) mVUallocFMAC19b(int& Fd) {
	mVUallocFMAC9b<vuIndex>(Fd);
}

//------------------------------------------------------------------
// Flag Allocators
//------------------------------------------------------------------

#define getFlagReg(regX, fInst) {  \
	switch (fInst) { \
		case 0: regX = gprF0;	break;  \
		case 1: regX = gprF1;	break;  \
		case 2: regX = gprF2;	break;  \
		case 3: regX = gprF3;	break;  \
	}  \
}

microVUt(void) mVUallocSFLAGa(int reg, int fInstance) {
	getFlagReg(fInstance, fInstance);
	MOVZX32R16toR(reg, fInstance);
}

microVUt(void) mVUallocSFLAGb(int reg, int fInstance) {
	getFlagReg(fInstance, fInstance);
	MOV32RtoR(fInstance, reg);
}

microVUt(void) mVUallocMFLAGa(int reg, int fInstance) {
	getFlagReg(fInstance, fInstance);
	MOV32RtoR(reg, fInstance);
	SHR32ItoR(reg, 16);
}

microVUt(void) mVUallocMFLAGb(int reg, int fInstance) {
	getFlagReg(fInstance, fInstance);
	AND32ItoR(fInstance, 0xffff);
	SHL32ItoR(reg, 16);
	OR32RtoR(fInstance, reg);
}

#endif //PCSX2_MICROVU
