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

//------------------------------------------------------------------
// Micro VU - Pass 2 Functions
//------------------------------------------------------------------

//------------------------------------------------------------------
// FMAC1 - Normal FMAC Opcodes
//------------------------------------------------------------------

#define getReg(reg, _reg_) {										\
	mVUloadReg(reg, (uptr)&mVU->regs->VF[_reg_].UL[0], _X_Y_Z_W);	\
	if (CHECK_VU_EXTRA_OVERFLOW) mVUclamp2(reg, xmmT2, _X_Y_Z_W);	\
}

#define getZero(reg) {														\
	if (_W)	{ mVUloadReg(reg, (uptr)&mVU->regs->VF[0].UL[0], _X_Y_Z_W); }	\
	else	{ SSE_XORPS_XMM_to_XMM(reg, reg); }								\
}

#define getReg6(reg, _reg_) {			\
	if (!_reg_)	{ getZero(reg); }		\
	else		{ getReg(reg, _reg_); }	\
}

microVUt(void) mVUallocFMAC1a(mV, int& Fd, int& Fs, int& Ft) {
	Fs = xmmFs;
	Ft = xmmFt;
	Fd = xmmFs;
	getReg6(Fs, _Fs_);
	if (_Ft_ == _Fs_) { Ft = Fs; }
	else			  { getReg6(Ft, _Ft_); }
}

microVUt(void) mVUallocFMAC1b(mV, int& Fd) {
	if (!_Fd_) return;
	if (CHECK_VU_OVERFLOW) mVUclamp1(Fd, xmmT1, _X_Y_Z_W);
	mVUsaveReg(Fd, (uptr)&mVU->regs->VF[_Fd_].UL[0], _X_Y_Z_W, 1);
}

//------------------------------------------------------------------
// FMAC2 - ABS/FTOI/ITOF Opcodes
//------------------------------------------------------------------

microVUt(void) mVUallocFMAC2a(mV, int& Fs, int& Ft) {
	Fs = xmmFs;
	Ft = xmmFs;
	getReg6(Fs, _Fs_);
}

microVUt(void) mVUallocFMAC2b(mV, int& Ft) {
	if (!_Ft_) return;
	//if (CHECK_VU_OVERFLOW) mVUclamp1<vuIndex>(Ft, xmmT1, _X_Y_Z_W);
	mVUsaveReg(Ft, (uptr)&mVU->regs->VF[_Ft_].UL[0], _X_Y_Z_W, 1);
}

//------------------------------------------------------------------
// FMAC3 - BC(xyzw) FMAC Opcodes
//------------------------------------------------------------------

#define getReg3SS(reg, _reg_) {												\
	mVUloadReg(reg, (uptr)&mVU->regs->VF[_reg_].UL[0], (1 << (3 - _bc_)));  \
	if (CHECK_VU_EXTRA_OVERFLOW) mVUclamp2(reg, xmmT2, (1 << (3 - _bc_)));  \
}

#define getReg3(reg, _reg_) {												\
	mVUloadReg(reg, (uptr)&mVU->regs->VF[_reg_].UL[0], (1 << (3 - _bc_)));  \
	if (CHECK_VU_EXTRA_OVERFLOW) mVUclamp2(reg, xmmT2, (1 << (3 - _bc_)));  \
	mVUunpack_xyzw(reg, reg, 0);											\
}

#define getZero3SS(reg) {												\
	if (_bc_w) { mVUloadReg(reg, (uptr)&mVU->regs->VF[0].UL[0], 1); }	\
	else { SSE_XORPS_XMM_to_XMM(reg, reg); }							\
}

#define getZero3(reg) {										\
	if (_bc_w)	{											\
		mVUloadReg(reg, (uptr)&mVU->regs->VF[0].UL[0], 1);	\
		mVUunpack_xyzw(reg, reg, 0);						\
	}														\
	else { SSE_XORPS_XMM_to_XMM(reg, reg); }				\
}

microVUt(void) mVUallocFMAC3a(mV, int& Fd, int& Fs, int& Ft) {
	Fs = xmmFs;
	Ft = xmmFt;
	Fd = xmmFs;
	if (_XYZW_SS) {
		getReg6(Fs, _Fs_);
		if ( (_Ft_ == _Fs_) && ((_X && _bc_x) || (_Y && _bc_y) || (_Z && _bc_z) || (_W && _bc_w)) ) {
			Ft = Fs; 
		}
		else if (!_Ft_)	{ getZero3SS(Ft); }
		else			{ getReg3SS(Ft, _Ft_); }
	}
	else {
		getReg6(Fs, _Fs_);
		if (!_Ft_)	{ getZero3(Ft); } 
		else		{ getReg3(Ft, _Ft_); }
	}
}

microVUt(void) mVUallocFMAC3b(mV, int& Fd) {
	mVUallocFMAC1b(mVU, Fd);
}

//------------------------------------------------------------------
// FMAC4 - FMAC Opcodes Storing Result to ACC
//------------------------------------------------------------------

#define getReg4(reg, _reg_) {										\
	mVUloadReg(reg, (uptr)&mVU->regs->VF[_reg_].UL[0], _xyzw_ACC);  \
	if (CHECK_VU_EXTRA_OVERFLOW) mVUclamp2(reg, xmmT2, _xyzw_ACC);  \
}

#define getZero4(reg) {														\
	if (_W)	{ mVUloadReg(reg, (uptr)&mVU->regs->VF[0].UL[0], _xyzw_ACC); }  \
	else	{ SSE_XORPS_XMM_to_XMM(reg, reg); }								\
}

microVUt(void) mVUallocFMAC4a(mV, int& ACC, int& Fs, int& Ft) {
	ACC = xmmACC;
	Fs = (_X_Y_Z_W == 15) ? xmmACC : xmmFs;
	Ft = xmmFt;
	if (_X_Y_Z_W == 8) {
		getReg6(Fs, _Fs_);
		if (_Ft_ == _Fs_) { Ft = Fs; }
		else			  { getReg6(Ft, _Ft_); }
	}
	else {
		if (!_Fs_)		  { getZero4(Fs); }
		else			  { getReg4(Fs, _Fs_); }

		if (_Ft_ == _Fs_) { Ft = Fs; }
		else if (!_Ft_)	  { getZero4(Ft); } 
		else			  { getReg4(Ft, _Ft_); }
	}
}

microVUt(void) mVUallocFMAC4b(mV, int& ACC, int& Fs) {
	if (CHECK_VU_OVERFLOW) mVUclamp1(Fs, xmmT1, _xyzw_ACC);
	mVUmergeRegs(ACC, Fs, _X_Y_Z_W);
}

//------------------------------------------------------------------
// FMAC5 - FMAC BC(xyzw) Opcodes Storing Result to ACC
//------------------------------------------------------------------

microVUt(void) mVUallocFMAC5a(mV, int& ACC, int& Fs, int& Ft) {
	ACC = xmmACC;
	Fs = (_X_Y_Z_W == 15) ? xmmACC : xmmFs;
	Ft = xmmFt;
	if (_X_Y_Z_W == 8) {
		getReg6(Fs, _Fs_);
		if ((_Ft_ == _Fs_) && _bc_x) { Ft = Fs; }
		else if (!_Ft_)				 { getZero3SS(Ft); }
		else						 { getReg3SS(Ft, _Ft_); }
	}
	else {
		if (!_Fs_)	{ getZero4(Fs); }
		else		{ getReg4(Fs, _Fs_); }

		if (!_Ft_)	{ getZero3(Ft); } 
		else		{ getReg3(Ft, _Ft_); }
	}
}

microVUt(void) mVUallocFMAC5b(mV, int& ACC, int& Fs) {
	mVUallocFMAC4b(mVU, ACC, Fs);
}

//------------------------------------------------------------------
// FMAC6 - Normal FMAC Opcodes (I Reg)
//------------------------------------------------------------------

#define getIreg(reg, modXYZW) {															\
	MOV32MtoR(gprT1, (uptr)&mVU->regs->VI[REG_I].UL);									\
	SSE2_MOVD_R_to_XMM(reg, gprT1);														\
	if (CHECK_VU_EXTRA_OVERFLOW) mVUclamp2(reg, xmmT2, 8);								\
	if (!((_XYZW_SS && modXYZW) || (_X_Y_Z_W == 8))) { mVUunpack_xyzw(reg, reg, 0); }	\
}

microVUt(void) mVUallocFMAC6a(mV, int& Fd, int& Fs, int& Ft) {
	Fs = xmmFs;
	Ft = xmmFt;
	Fd = xmmFs;
	getIreg(Ft, 1);
	getReg6(Fs, _Fs_);
}

microVUt(void) mVUallocFMAC6b(mV, int& Fd) {
	mVUallocFMAC1b(mVU, Fd);
}

//------------------------------------------------------------------
// FMAC7 - FMAC Opcodes Storing Result to ACC (I Reg)
//------------------------------------------------------------------

microVUt(void) mVUallocFMAC7a(mV, int& ACC, int& Fs, int& Ft) {
	ACC = xmmACC;
	Fs = (_X_Y_Z_W == 15) ? xmmACC : xmmFs;
	Ft = xmmFt;
	getIreg(Ft, 0);
	if (_X_Y_Z_W == 8)	{ getReg6(Fs, _Fs_); }
	else if (!_Fs_)		{ getZero4(Fs); }
	else				{ getReg4(Fs, _Fs_); }
}

microVUt(void) mVUallocFMAC7b(mV, int& ACC, int& Fs) {
	mVUallocFMAC4b(mVU, ACC, Fs);
}

//------------------------------------------------------------------
// FMAC8 - MADD FMAC Opcode Storing Result to Fd
//------------------------------------------------------------------

microVUt(void) mVUallocFMAC8a(mV, int& Fd, int& ACC, int& Fs, int& Ft) {
	Fs = xmmFs;
	Ft = xmmFt;
	Fd = xmmFs;
	ACC = xmmACC;
	if (_X_Y_Z_W == 8) {
		getReg6(Fs, _Fs_);
		if (_Ft_ == _Fs_) { Ft = Fs; }
		else			  { getReg6(Ft, _Ft_); }
	}
	else {
		if (!_Fs_)		  { getZero4(Fs); }
		else			  { getReg4(Fs, _Fs_); }

		if (_Ft_ == _Fs_) { Ft = Fs; }
		else if (!_Ft_)	  { getZero4(Ft); } 
		else			  { getReg4(Ft, _Ft_); }
	}
}

microVUt(void) mVUallocFMAC8b(mV, int& Fd) {
	if (!_Fd_) return;
	if (CHECK_VU_OVERFLOW) mVUclamp1(Fd, xmmT1, _xyzw_ACC);
	mVUsaveReg(Fd, (uptr)&mVU->regs->VF[_Fd_].UL[0], _X_Y_Z_W, 0);
}

//------------------------------------------------------------------
// FMAC9 - MSUB FMAC Opcode Storing Result to Fd
//------------------------------------------------------------------

microVUt(void) mVUallocFMAC9a(mV, int& Fd, int& ACC, int& Fs, int& Ft) {
	Fs = xmmFs;
	Ft = xmmFt;
	Fd = xmmT1;
	ACC = xmmT1;
	if (_X_Y_Z_W == 8) {
		getReg6(Fs, _Fs_);
		if (_Ft_ == _Fs_) { Ft = Fs; }
		else			  { getReg6(Ft, _Ft_); }
	}
	else {
		if (!_Fs_)		  { getZero4(Fs); }
		else			  { getReg4(Fs, _Fs_); }

		if (_Ft_ == _Fs_) { Ft = Fs; }
		else if (!_Ft_)	  { getZero4(Ft); } 
		else			  { getReg4(Ft, _Ft_); }
	}
	SSE_MOVAPS_XMM_to_XMM(ACC, xmmACC);
}

microVUt(void) mVUallocFMAC9b(mV, int& Fd) {
	if (!_Fd_) return;
	if (CHECK_VU_OVERFLOW) mVUclamp1(Fd, xmmFt, _xyzw_ACC);
	mVUsaveReg(Fd, (uptr)&mVU->regs->VF[_Fd_].UL[0], _X_Y_Z_W, 0);
}

//------------------------------------------------------------------
// FMAC10 - MADD FMAC BC(xyzw) Opcode Storing Result to Fd
//------------------------------------------------------------------

microVUt(void) mVUallocFMAC10a(mV, int& Fd, int& ACC, int& Fs, int& Ft) {
	Fs = xmmFs;
	Ft = xmmFt;
	Fd = xmmFs;
	ACC = xmmACC;
	if (_X_Y_Z_W == 8) {
		getReg6(Fs, _Fs_);
		if ( (_Ft_ == _Fs_) && _bc_x) { Ft = Fs; }
		else if (!_Ft_)				  { getZero3SS(Ft); }
		else						  { getReg3SS(Ft, _Ft_); }
	}
	else {
		if (!_Fs_)	{ getZero4(Fs); }
		else		{ getReg4(Fs, _Fs_); }

		if (!_Ft_)	{ getZero3(Ft); } 
		else		{ getReg3(Ft, _Ft_); }
	}
}

microVUt(void) mVUallocFMAC10b(mV, int& Fd) {
	mVUallocFMAC8b(mVU, Fd);
}

//------------------------------------------------------------------
// FMAC11 - MSUB FMAC BC(xyzw) Opcode Storing Result to Fd
//------------------------------------------------------------------

microVUt(void) mVUallocFMAC11a(mV, int& Fd, int& ACC, int& Fs, int& Ft) {
	Fs = xmmFs;
	Ft = xmmFt;
	Fd = xmmT1;
	ACC = xmmT1;
	if (_X_Y_Z_W == 8) {
		getReg6(Fs, _Fs_);
		if ( (_Ft_ == _Fs_) && _bc_x) { Ft = Fs; }
		else if (!_Ft_)				  { getZero3SS(Ft); }
		else						  { getReg3SS(Ft, _Ft_); }
	}
	else {
		if (!_Fs_)	{ getZero4(Fs); }
		else		{ getReg4(Fs, _Fs_); }

		if (!_Ft_)	{ getZero3(Ft); } 
		else		{ getReg3(Ft, _Ft_); }
	}
	SSE_MOVAPS_XMM_to_XMM(ACC, xmmACC);
}

microVUt(void) mVUallocFMAC11b(mV, int& Fd) {
	mVUallocFMAC9b(mVU, Fd);
}

//------------------------------------------------------------------
// FMAC12 - MADD FMAC Opcode Storing Result to Fd (I Reg)
//------------------------------------------------------------------

microVUt(void) mVUallocFMAC12a(mV, int& Fd, int& ACC, int& Fs, int& Ft) {
	Fs = xmmFs;
	Ft = xmmFt;
	Fd = xmmFs;
	ACC = xmmACC;
	getIreg(Ft, 0);
	if (_X_Y_Z_W == 8)	{ getReg6(Fs, _Fs_); }
	else if (!_Fs_)		{ getZero4(Fs); }
	else				{ getReg4(Fs, _Fs_); }
}

microVUt(void) mVUallocFMAC12b(mV, int& Fd) {
	mVUallocFMAC8b(mVU, Fd);
}

//------------------------------------------------------------------
// FMAC13 - MSUB FMAC Opcode Storing Result to Fd (I Reg)
//------------------------------------------------------------------

microVUt(void) mVUallocFMAC13a(mV, int& Fd, int& ACC, int& Fs, int& Ft) {
	Fs = xmmFs;
	Ft = xmmFt;
	Fd = xmmT1;
	ACC = xmmT1;
	getIreg(Ft, 0);
	if (_X_Y_Z_W == 8)	{ getReg6(Fs, _Fs_); }
	else if (!_Fs_)		{ getZero4(Fs); }
	else				{ getReg4(Fs, _Fs_); }
	SSE_MOVAPS_XMM_to_XMM(ACC, xmmACC);
}

microVUt(void) mVUallocFMAC13b(mV, int& Fd) {
	mVUallocFMAC9b(mVU, Fd);
}

//------------------------------------------------------------------
// FMAC14 - MADDA/MSUBA FMAC Opcodes
//------------------------------------------------------------------

microVUt(void) mVUallocFMAC14a(mV, int& ACCw, int& ACCr, int& Fs, int& Ft) {
	Fs = xmmFs;
	Ft = xmmFt;
	ACCw = xmmACC;
	ACCr = ((_X_Y_Z_W == 15) || (_X_Y_Z_W == 8)) ? xmmACC : xmmT1;
	
	if (_X_Y_Z_W == 8) {
		getReg6(Fs, _Fs_);
		if (_Ft_ == _Fs_) { Ft = Fs; }
		else			  { getReg6(Ft, _Ft_); }
	}
	else {
		if (!_Fs_)		  { getZero4(Fs); }
		else			  { getReg4(Fs, _Fs_); }

		if (_Ft_ == _Fs_) { Ft = Fs; }
		else if (!_Ft_)	  { getZero4(Ft); } 
		else			  { getReg4(Ft, _Ft_); }
	}
	SSE_MOVAPS_XMM_to_XMM(ACCr, xmmACC);
}

microVUt(void) mVUallocFMAC14b(mV, int& ACCw, int& ACCr) {
	if (CHECK_VU_OVERFLOW) mVUclamp1(ACCr, xmmFt, _xyzw_ACC);
	mVUmergeRegs(ACCw, ACCr, _X_Y_Z_W);
}

//------------------------------------------------------------------
// FMAC15 - MADDA/MSUBA BC(xyzw) FMAC Opcode
//------------------------------------------------------------------

microVUt(void) mVUallocFMAC15a(mV, int& ACCw, int& ACCr, int& Fs, int& Ft) {
	Fs = xmmFs;
	Ft = xmmFt;
	ACCw = xmmACC;
	ACCr = ((_X_Y_Z_W == 15) || (_X_Y_Z_W == 8)) ? xmmACC : xmmT1;
	
	if (_X_Y_Z_W == 8) {
		getReg6(Fs, _Fs_);
		if ((_Ft_ == _Fs_) && _bc_x) { Ft = Fs; }
		else if (!_Ft_)				 { getZero3SS(Ft); }
		else						 { getReg3SS(Ft, _Ft_); }
	}
	else {
		if (!_Fs_)	{ getZero4(Fs); }
		else		{ getReg4(Fs, _Fs_); }

		if (!_Ft_)	{ getZero3(Ft); } 
		else		{ getReg3(Ft, _Ft_); }
	}
	SSE_MOVAPS_XMM_to_XMM(ACCr, xmmACC);
}

microVUt(void) mVUallocFMAC15b(mV, int& ACCw, int& ACCr) {
	mVUallocFMAC14b(mVU, ACCw, ACCr);
}

//------------------------------------------------------------------
// FMAC16 - MADDA/MSUBA FMAC Opcode (I Reg)
//------------------------------------------------------------------

microVUt(void) mVUallocFMAC16a(mV, int& ACCw, int& ACCr, int& Fs, int& Ft) {
	Fs = xmmFs;
	Ft = xmmFt;
	ACCw = xmmACC;
	ACCr = ((_X_Y_Z_W == 15) || (_X_Y_Z_W == 8)) ? xmmACC : xmmT1;
	getIreg(Ft, 0);
	if (_X_Y_Z_W == 8)	{ getReg6(Fs, _Fs_); }
	else if (!_Fs_)		{ getZero4(Fs); }
	else				{ getReg4(Fs, _Fs_); }
	SSE_MOVAPS_XMM_to_XMM(ACCr, xmmACC);
}

microVUt(void) mVUallocFMAC16b(mV, int& ACCw, int& ACCr) {
	mVUallocFMAC14b(mVU, ACCw, ACCr);
}

//------------------------------------------------------------------
// FMAC17 - CLIP FMAC Opcode
//------------------------------------------------------------------

#define getReg9(reg, _reg_) {								\
	mVUloadReg(reg, (uptr)&mVU->regs->VF[_reg_].UL[0], 1);  \
	if (CHECK_VU_EXTRA_OVERFLOW) mVUclamp2(reg, xmmT2, 1);  \
	mVUunpack_xyzw(reg, reg, 0);							\
}

microVUt(void) mVUallocFMAC17a(mV, int& Fs, int& Ft) {
	Fs = xmmFs;
	Ft = xmmFt;
	getReg6(Fs, _Fs_);
	getReg9(Ft, _Ft_);
}

//------------------------------------------------------------------
// FMAC18 - OPMULA FMAC Opcode
//------------------------------------------------------------------

microVUt(void) mVUallocFMAC18a(mV, int& ACC, int& Fs, int& Ft) {
	Fs = xmmFs;
	Ft = xmmFt;
	ACC = xmmACC;

	if (!_Fs_)	{ getZero4(Fs); }
	else		{ getReg4(Fs, _Fs_); }

	if (!_Ft_)	{ getZero4(Ft); } 
	else		{ getReg4(Ft, _Ft_); }

	SSE2_PSHUFD_XMM_to_XMM(Fs, Fs, 0xC9); // WXZY
	SSE2_PSHUFD_XMM_to_XMM(Ft, Ft, 0xD2); // WYXZ
}

microVUt(void) mVUallocFMAC18b(mV, int& ACC, int& Fs) {
	mVUallocFMAC4b(mVU, ACC, Fs);
}

//------------------------------------------------------------------
// FMAC19 - OPMSUB FMAC Opcode
//------------------------------------------------------------------

microVUt(void) mVUallocFMAC19a(mV, int& Fd, int& ACC, int& Fs, int& Ft) {
	Fs = xmmFs;
	Ft = xmmFt;
	Fd = xmmT1;
	ACC = xmmT1;
	
	if (!_Fs_)	{ getZero4(Fs); }
	else		{ getReg4(Fs, _Fs_); }

	if (!_Ft_)	{ getZero4(Ft); } 
	else		{ getReg4(Ft, _Ft_); }

	SSE2_PSHUFD_XMM_to_XMM(Fs, Fs, 0xC9); // WXZY
	SSE2_PSHUFD_XMM_to_XMM(Ft, Ft, 0xD2); // WYXZ
	SSE_MOVAPS_XMM_to_XMM(ACC, xmmACC);
}

microVUt(void) mVUallocFMAC19b(mV, int& Fd) {
	mVUallocFMAC9b(mVU, Fd);
}

//------------------------------------------------------------------
// FMAC22 - Normal FMAC Opcodes (Q Reg)
//------------------------------------------------------------------

#define getQreg(reg) {														\
	mVUunpack_xyzw(reg, xmmPQ, mVUinfo.readQ);								\
	/*if (CHECK_VU_EXTRA_OVERFLOW) mVUclamp2<vuIndex>(reg, xmmT1, 15);*/	\
}

microVUt(void) mVUallocFMAC22a(mV, int& Fd, int& Fs, int& Ft) {
	Fs = xmmFs;
	Ft = xmmFt;
	Fd = xmmFs;
	getQreg(Ft);
	getReg6(Fs, _Fs_);
}

microVUt(void) mVUallocFMAC22b(mV, int& Fd) {
	mVUallocFMAC1b(mVU, Fd);
}

//------------------------------------------------------------------
// FMAC23 - FMAC Opcodes Storing Result to ACC (Q Reg)
//------------------------------------------------------------------

microVUt(void) mVUallocFMAC23a(mV, int& ACC, int& Fs, int& Ft) {
	ACC = xmmACC;
	Fs = (_X_Y_Z_W == 15) ? xmmACC : xmmFs;
	Ft = xmmFt;
	getQreg(Ft);
	if (_X_Y_Z_W == 8)	{ getReg6(Fs, _Fs_); }
	else if (!_Fs_)		{ getZero4(Fs); }
	else				{ getReg4(Fs, _Fs_); }
}

microVUt(void) mVUallocFMAC23b(mV, int& ACC, int& Fs) {
	mVUallocFMAC4b(mVU, ACC, Fs);
}

//------------------------------------------------------------------
// FMAC24 - MADD FMAC Opcode Storing Result to Fd (Q Reg)
//------------------------------------------------------------------

microVUt(void) mVUallocFMAC24a(mV, int& Fd, int& ACC, int& Fs, int& Ft) {
	Fs = xmmFs;
	Ft = xmmFt;
	Fd = xmmFs;
	ACC = xmmACC;
	getQreg(Ft);
	if (_X_Y_Z_W == 8)	{ getReg6(Fs, _Fs_); }
	else if (!_Fs_)		{ getZero4(Fs); }
	else				{ getReg4(Fs, _Fs_); }
}

microVUt(void) mVUallocFMAC24b(mV, int& Fd) {
	mVUallocFMAC8b(mVU, Fd);
}

//------------------------------------------------------------------
// FMAC25 - MSUB FMAC Opcode Storing Result to Fd (Q Reg)
//------------------------------------------------------------------

microVUt(void) mVUallocFMAC25a(mV, int& Fd, int& ACC, int& Fs, int& Ft) {
	Fs = xmmFs;
	Ft = xmmFt;
	Fd = xmmT1;
	ACC = xmmT1;
	getQreg(Ft);
	if (_X_Y_Z_W == 8)	{ getReg6(Fs, _Fs_); }
	else if (!_Fs_)		{ getZero4(Fs); }
	else				{ getReg4(Fs, _Fs_); }
	SSE_MOVAPS_XMM_to_XMM(ACC, xmmACC);
}

microVUt(void) mVUallocFMAC25b(mV, int& Fd) {
	mVUallocFMAC9b(mVU, Fd);
}

//------------------------------------------------------------------
// FMAC26 - MADDA/MSUBA FMAC Opcode (Q Reg)
//------------------------------------------------------------------

microVUt(void) mVUallocFMAC26a(mV, int& ACCw, int& ACCr, int& Fs, int& Ft) {
	Fs = xmmFs;
	Ft = xmmFt;
	ACCw = xmmACC;
	ACCr = ((_X_Y_Z_W == 15) || (_X_Y_Z_W == 8)) ? xmmACC : xmmT1;
	getQreg(Ft);
	if (_X_Y_Z_W == 8)	{ getReg6(Fs, _Fs_); }
	else if (!_Fs_)		{ getZero4(Fs); }
	else				{ getReg4(Fs, _Fs_); }
	SSE_MOVAPS_XMM_to_XMM(ACCr, xmmACC);
}

microVUt(void) mVUallocFMAC26b(mV, int& ACCw, int& ACCr) {
	mVUallocFMAC14b(mVU, ACCw, ACCr);
}

//------------------------------------------------------------------
// Flag Allocators
//------------------------------------------------------------------

#define getFlagReg(regX, fInst) {														\
	switch (fInst) {																	\
		case 0: regX = gprF0;	break;													\
		case 1: regX = gprF1;	break;													\
		case 2: regX = gprF2;	break;													\
		case 3: regX = gprF3;	break;													\
		default:																		\
			Console::Error("microVU: Flag Instance Error (fInst = %d)", params fInst);	\
			regX = gprF0;																\
			break;																		\
	}																					\
}

microVUt(void) mVUallocSFLAGa(int reg, int fInstance) {
	getFlagReg(fInstance, fInstance);
	MOVZX32R16toR(reg, fInstance);
}

microVUt(void) mVUallocSFLAGb(int reg, int fInstance) {
	getFlagReg(fInstance, fInstance);
	//AND32ItoR(reg, 0xffff);
	MOV32RtoR(fInstance, reg);
}

microVUt(void) mVUallocMFLAGa(mV, int reg, int fInstance) {
	MOVZX32M16toR(reg, (uptr)&mVU->macFlag[fInstance]);
}

microVUt(void) mVUallocMFLAGb(mV, int reg, int fInstance) {
	//AND32ItoR(reg, 0xffff);
	MOV32RtoM((uptr)&mVU->macFlag[fInstance], reg);
}

microVUt(void) mVUallocCFLAGa(mV, int reg, int fInstance) {
	MOV32MtoR(reg, (uptr)&mVU->clipFlag[fInstance]);
}

microVUt(void) mVUallocCFLAGb(mV, int reg, int fInstance) {
	MOV32RtoM((uptr)&mVU->clipFlag[fInstance], reg);
}

//------------------------------------------------------------------
// VI Reg Allocators
//------------------------------------------------------------------

microVUt(void) mVUallocVIa(mV, int GPRreg, int _reg_) {
	if (!_reg_)				{ XOR32RtoR(GPRreg, GPRreg); }
	else if (isMMX(_reg_))	{ MOVD32MMXtoR(GPRreg, mmVI(_reg_)); }
	else					{ MOVZX32Rm16toR(GPRreg, gprR, (_reg_ - 9) * 16); }
}

microVUt(void) mVUallocVIb(mV, int GPRreg, int _reg_) {
	if (mVUlow.backupVI) { // Backs up reg to memory (used when VI is modified b4 a branch)
		MOVZX32M16toR(gprR, (uptr)&mVU->regs->VI[_reg_].UL);
		MOV32RtoM((uptr)&mVU->VIbackup, gprR);
		MOV32ItoR(gprR, Roffset);
	}
	if (_reg_ == 0)			{ return; }
	else if (isMMX(_reg_))	{ MOVD32RtoMMX(mmVI(_reg_), GPRreg); }
	else if (_reg_ < 16)	{ MOV16RtoRm(gprR, GPRreg, (_reg_ - 9) * 16); }
}

//------------------------------------------------------------------
// P Reg Allocator
//------------------------------------------------------------------

#define getPreg(reg) {											\
	mVUunpack_xyzw(reg, xmmPQ, (2 + mVUinfo.readP));			\
	/*if (CHECK_VU_EXTRA_OVERFLOW) mVUclamp2(reg, xmmT1, 15);*/	\
}

//------------------------------------------------------------------
// Lower Instruction Allocator Helpers
//------------------------------------------------------------------

#define getReg5(reg, _reg_, _fxf_) {												\
	if (!_reg_) {																	\
		if (_fxf_ < 3) { SSE_XORPS_XMM_to_XMM(reg, reg); }							\
		else { mVUloadReg(reg, (uptr)&mVU->regs->VF[_reg_].UL[0], 1); }				\
	}																				\
	else {																			\
		mVUloadReg(reg, (uptr)&mVU->regs->VF[_reg_].UL[0], (1 << (3 - _fxf_)));		\
		if (CHECK_VU_EXTRA_OVERFLOW) mVUclamp2(reg, xmmT2, (1 << (3 - _fxf_)));		\
	}																				\
}

// Doesn't Clamp
#define getReg7(reg, _reg_) {														\
	if (!_reg_)	{ getZero(reg); }													\
	else		{ mVUloadReg(reg, (uptr)&mVU->regs->VF[_reg_].UL[0], _X_Y_Z_W); }	\
}

// VF to GPR
#define getReg8(GPRreg, _reg_, _fxf_) {														\
	if (!_reg_ && (_fxf_ < 3))	{ XOR32RtoR(GPRreg, GPRreg); }								\
	else						{ MOV32MtoR(GPRreg, (uptr)&mVU->regs->VF[_reg_].UL[0]); }	\
}
