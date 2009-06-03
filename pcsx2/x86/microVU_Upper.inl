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
// mVUupdateFlags() - Updates status/mac flags
//------------------------------------------------------------------

#define AND_XYZW			((_XYZW_SS && modXYZW) ? (1) : (mFLAG.doFlag ? (_X_Y_Z_W) : (flipMask[_X_Y_Z_W])))
#define ADD_XYZW			((_XYZW_SS && modXYZW) ? (_X ? 3 : (_Y ? 2 : (_Z ? 1 : 0))) : 0)
#define SHIFT_XYZW(gprReg)	{ if (_XYZW_SS && modXYZW && !_W) { SHL32ItoR(gprReg, ADD_XYZW); } }

// Note: If modXYZW is true, then it adjusts XYZW for Single Scalar operations
microVUt(void) mVUupdateFlags(mV, int reg, int regT1, int regT2, int xyzw, bool modXYZW) {
	int sReg, mReg = gprT1;
	static const u16 flipMask[16] = {0, 8, 4, 12, 2, 10, 6, 14, 1, 9, 5, 13, 3, 11, 7, 15};

	//SysPrintf("Status = %d; Mac = %d\n", sFLAG.doFlag, mFLAG.doFlag);
	if (mVUsFlagHack) { sFLAG.doFlag = 0; }
	if (!sFLAG.doFlag && !mFLAG.doFlag)			{ return; }
	if (!mFLAG.doFlag || (_XYZW_SS && modXYZW))	{ regT1 = reg; }
	else { SSE2_PSHUFD_XMM_to_XMM(regT1, reg, 0x1B); } // Flip wzyx to xyzw
	if (sFLAG.doFlag) {
		getFlagReg(sReg, sFLAG.write); // Set sReg to valid GPR by Cur Flag Instance
		mVUallocSFLAGa(sReg, sFLAG.lastWrite); // Get Prev Status Flag
		AND32ItoR(sReg, 0xffffff00); // Keep Sticky and D/I flags
	}

	//-------------------------Check for Signed flags------------------------------

	// The following code makes sure the Signed Bit isn't set with Negative Zero
	SSE_XORPS_XMM_to_XMM(regT2, regT2); // Clear regT2
	SSE_CMPEQPS_XMM_to_XMM(regT2, regT1); // Set all F's if each vector is zero
	SSE_MOVMSKPS_XMM_to_R32(gprT2, regT2); // Used for Zero Flag Calculation
	SSE_ANDNPS_XMM_to_XMM(regT2, regT1);

	SSE_MOVMSKPS_XMM_to_R32(mReg, regT2); // Move the sign bits of the t1reg

	AND32ItoR(mReg, AND_XYZW);  // Grab "Is Signed" bits from the previous calculation
	SHL32ItoR(mReg, 4 + ADD_XYZW);

	//-------------------------Check for Zero flags------------------------------

	AND32ItoR(gprT2, AND_XYZW);  // Grab "Is Zero" bits from the previous calculation
	if (mFLAG.doFlag) { SHIFT_XYZW(gprT2); }
	OR32RtoR(mReg, gprT2);	

	//-------------------------Write back flags------------------------------

	if (mFLAG.doFlag) mVUallocMFLAGb(mVU, mReg, mFLAG.write); // Set Mac Flag
	if (sFLAG.doFlag) {
		OR32RtoR (sReg, mReg);
		SHL32ItoR(mReg, 8);
		OR32RtoR (sReg, mReg);
	}
}

//------------------------------------------------------------------
// Helper Macros
//------------------------------------------------------------------

// FMAC1 - Normal FMAC Opcodes
#define mVU_FMAC1(operation, OPname) {								\
	pass1 { mVUanalyzeFMAC1(mVU, _Fd_, _Fs_, _Ft_); }				\
	pass2 {															\
		int Fd, Fs, Ft;												\
		mVUallocFMAC1a(mVU, Fd, Fs, Ft);							\
		if (_XYZW_SS) SSE_##operation##SS_XMM_to_XMM(Fs, Ft);		\
		else		  SSE_##operation##PS_XMM_to_XMM(Fs, Ft);		\
		mVUupdateFlags(mVU, Fd, xmmT1, xmmT2, _X_Y_Z_W, 1);			\
		mVUallocFMAC1b(mVU, Fd);									\
	}																\
	pass3 {	mVUlog(OPname);	mVUlogFd(); mVUlogFt();	}				\
}
// FMAC3 - BC(xyzw) FMAC Opcodes
#define mVU_FMAC3(operation, OPname) {								\
	pass1 { mVUanalyzeFMAC3(mVU, _Fd_, _Fs_, _Ft_); }				\
	pass2 {															\
		int Fd, Fs, Ft;												\
		mVUallocFMAC3a(mVU, Fd, Fs, Ft);							\
		if (_XYZW_SS) SSE_##operation##SS_XMM_to_XMM(Fs, Ft);		\
		else		  SSE_##operation##PS_XMM_to_XMM(Fs, Ft);		\
		mVUupdateFlags(mVU, Fd, xmmT1, xmmT2, _X_Y_Z_W, 1);			\
		mVUallocFMAC3b(mVU, Fd);									\
	}																\
	pass3 { mVUlog(OPname); mVUlogFd(); mVUlogBC(); }				\
}
// FMAC4 - FMAC Opcodes Storing Result to ACC
#define mVU_FMAC4(operation, OPname) {								\
	pass1 { mVUanalyzeFMAC1(mVU, 0, _Fs_, _Ft_); }					\
	pass2 {															\
		int ACC, Fs, Ft;											\
		mVUallocFMAC4a(mVU, ACC, Fs, Ft);							\
		if (_X_Y_Z_W == 8)	SSE_##operation##SS_XMM_to_XMM(Fs, Ft);	\
		else				SSE_##operation##PS_XMM_to_XMM(Fs, Ft);	\
		mVUupdateFlags(mVU, Fs, xmmT1, xmmT2, _X_Y_Z_W, 0);			\
		mVUallocFMAC4b(mVU, ACC, Fs);								\
	}																\
	pass3 { mVUlog(OPname); mVUlogACC(); mVUlogFt(); }				\
}
// FMAC5 - FMAC BC(xyzw) Opcodes Storing Result to ACC
#define mVU_FMAC5(operation, OPname) {								\
	pass1 { mVUanalyzeFMAC3(mVU, 0, _Fs_, _Ft_); }					\
	pass2 {															\
		int ACC, Fs, Ft;											\
		mVUallocFMAC5a(mVU, ACC, Fs, Ft);							\
		if (_X_Y_Z_W == 8)	SSE_##operation##SS_XMM_to_XMM(Fs, Ft);	\
		else				SSE_##operation##PS_XMM_to_XMM(Fs, Ft);	\
		mVUupdateFlags(mVU, Fs, xmmT1, xmmT2, _X_Y_Z_W, 0);			\
		mVUallocFMAC5b(mVU, ACC, Fs);								\
	}																\
	pass3 { mVUlog(OPname); mVUlogACC(); mVUlogBC(); }				\
}
// FMAC6 - Normal FMAC Opcodes (I Reg)
#define mVU_FMAC6(operation, OPname) {								\
	pass1 { mVUanalyzeFMAC1(mVU, _Fd_, _Fs_, 0); }					\
	pass2 {															\
		int Fd, Fs, Ft;												\
		mVUallocFMAC6a(mVU, Fd, Fs, Ft);							\
		if (_XYZW_SS) SSE_##operation##SS_XMM_to_XMM(Fs, Ft);		\
		else		  SSE_##operation##PS_XMM_to_XMM(Fs, Ft);		\
		mVUupdateFlags(mVU, Fd, xmmT1, xmmT2, _X_Y_Z_W, 1);			\
		mVUallocFMAC6b(mVU, Fd);									\
	}																\
	pass3 { mVUlog(OPname); mVUlogFd(); mVUlogI(); }				\
}
// FMAC7 - FMAC Opcodes Storing Result to ACC (I Reg)
#define mVU_FMAC7(operation, OPname) {								\
	pass1 { mVUanalyzeFMAC1(mVU, 0, _Fs_, 0); }						\
	pass2 {															\
		int ACC, Fs, Ft;											\
		mVUallocFMAC7a(mVU, ACC, Fs, Ft);							\
		if (_X_Y_Z_W == 8)	SSE_##operation##SS_XMM_to_XMM(Fs, Ft);	\
		else				SSE_##operation##PS_XMM_to_XMM(Fs, Ft);	\
		mVUupdateFlags(mVU, Fs, xmmT1, xmmT2, _X_Y_Z_W, 0);			\
		mVUallocFMAC7b(mVU, ACC, Fs);								\
	}																\
	pass3 { mVUlog(OPname); mVUlogACC(); mVUlogI(); }				\
}
// FMAC8 - MADD FMAC Opcode Storing Result to Fd
#define mVU_FMAC8(operation, OPname) {								\
	pass1 { mVUanalyzeFMAC1(mVU, _Fd_, _Fs_, _Ft_); }				\
	pass2 {															\
		int Fd, ACC, Fs, Ft;										\
		mVUallocFMAC8a(mVU, Fd, ACC, Fs, Ft);						\
		if (_X_Y_Z_W == 8) {										\
			SSE_MULSS_XMM_to_XMM(Fs, Ft);							\
			SSE_##operation##SS_XMM_to_XMM(Fs, ACC);				\
		}															\
		else {														\
			SSE_MULPS_XMM_to_XMM(Fs, Ft);							\
			SSE_##operation##PS_XMM_to_XMM(Fs, ACC);				\
		}															\
		mVUupdateFlags(mVU, Fd, xmmT1, xmmT2, _X_Y_Z_W, 0);			\
		mVUallocFMAC8b(mVU, Fd);									\
	}																\
	pass3 { mVUlog(OPname); mVUlogFd(); mVUlogFt(); }				\
}
// FMAC9 - MSUB FMAC Opcode Storing Result to Fd
#define mVU_FMAC9(operation, OPname) {								\
	pass1 { mVUanalyzeFMAC1(mVU, _Fd_, _Fs_, _Ft_); }				\
	pass2 {															\
		int Fd, ACC, Fs, Ft;										\
		mVUallocFMAC9a(mVU, Fd, ACC, Fs, Ft);						\
		if (_X_Y_Z_W == 8) {										\
			SSE_MULSS_XMM_to_XMM(Fs, Ft);							\
			SSE_##operation##SS_XMM_to_XMM(ACC, Fs);				\
		}															\
		else {														\
			SSE_MULPS_XMM_to_XMM(Fs, Ft);							\
			SSE_##operation##PS_XMM_to_XMM(ACC, Fs);				\
		}															\
		mVUupdateFlags(mVU, Fd, Fs, xmmT2, _X_Y_Z_W, 0);			\
		mVUallocFMAC9b(mVU, Fd);									\
	}																\
	pass3 { mVUlog(OPname); mVUlogFd(); mVUlogFt(); }				\
}
// FMAC10 - MADD FMAC BC(xyzw) Opcode Storing Result to Fd
#define mVU_FMAC10(operation, OPname) {								\
	pass1 { mVUanalyzeFMAC3(mVU, _Fd_, _Fs_, _Ft_); }				\
	pass2 {															\
		int Fd, ACC, Fs, Ft;										\
		mVUallocFMAC10a(mVU, Fd, ACC, Fs, Ft);						\
		if (_X_Y_Z_W == 8) {										\
			SSE_MULSS_XMM_to_XMM(Fs, Ft);							\
			SSE_##operation##SS_XMM_to_XMM(Fs, ACC);				\
		}															\
		else {														\
			SSE_MULPS_XMM_to_XMM(Fs, Ft);							\
			SSE_##operation##PS_XMM_to_XMM(Fs, ACC);				\
		}															\
		mVUupdateFlags(mVU, Fd, xmmT1, xmmT2, _X_Y_Z_W, 0);			\
		mVUallocFMAC10b(mVU, Fd);									\
	}																\
	pass3 { mVUlog(OPname); mVUlogFd(); mVUlogBC(); }				\
}
// FMAC11 - MSUB FMAC BC(xyzw) Opcode Storing Result to Fd
#define mVU_FMAC11(operation, OPname) {								\
	pass1 { mVUanalyzeFMAC3(mVU, _Fd_, _Fs_, _Ft_); }				\
	pass2 {															\
		int Fd, ACC, Fs, Ft;										\
		mVUallocFMAC11a(mVU, Fd, ACC, Fs, Ft);						\
		if (_X_Y_Z_W == 8) {										\
			SSE_MULSS_XMM_to_XMM(Fs, Ft);							\
			SSE_##operation##SS_XMM_to_XMM(ACC, Fs);				\
		}															\
		else {														\
			SSE_MULPS_XMM_to_XMM(Fs, Ft);							\
			SSE_##operation##PS_XMM_to_XMM(ACC, Fs);				\
		}															\
		mVUupdateFlags(mVU, Fd, Fs, xmmT2, _X_Y_Z_W, 0);			\
		mVUallocFMAC11b(mVU, Fd);									\
	}																\
	pass3 { mVUlog(OPname); mVUlogFd(); mVUlogBC(); }				\
}
// FMAC12 - MADD FMAC Opcode Storing Result to Fd (I Reg)
#define mVU_FMAC12(operation, OPname) {								\
	pass1 { mVUanalyzeFMAC1(mVU, _Fd_, _Fs_, 0); }					\
	pass2 {															\
		int Fd, ACC, Fs, Ft;										\
		mVUallocFMAC12a(mVU, Fd, ACC, Fs, Ft);						\
		if (_X_Y_Z_W == 8) {										\
			SSE_MULSS_XMM_to_XMM(Fs, Ft);							\
			SSE_##operation##SS_XMM_to_XMM(Fs, ACC);				\
		}															\
		else {														\
			SSE_MULPS_XMM_to_XMM(Fs, Ft);							\
			SSE_##operation##PS_XMM_to_XMM(Fs, ACC);				\
		}															\
		mVUupdateFlags(mVU, Fd, xmmT1, xmmT2, _X_Y_Z_W, 0);			\
		mVUallocFMAC12b(mVU, Fd);									\
	}																\
	pass3 { mVUlog(OPname); mVUlogFd(); mVUlogI(); }				\
}
// FMAC13 - MSUB FMAC Opcode Storing Result to Fd (I Reg)
#define mVU_FMAC13(operation, OPname) {								\
	pass1 { mVUanalyzeFMAC1(mVU, _Fd_, _Fs_, 0); }					\
	pass2 {															\
		int Fd, ACC, Fs, Ft;										\
		mVUallocFMAC13a(mVU, Fd, ACC, Fs, Ft);						\
		if (_X_Y_Z_W == 8) {										\
			SSE_MULSS_XMM_to_XMM(Fs, Ft);							\
			SSE_##operation##SS_XMM_to_XMM(ACC, Fs);				\
		}															\
		else {														\
			SSE_MULPS_XMM_to_XMM(Fs, Ft);							\
			SSE_##operation##PS_XMM_to_XMM(ACC, Fs);				\
		}															\
		mVUupdateFlags(mVU, Fd, Fs, xmmT2, _X_Y_Z_W, 0);			\
		mVUallocFMAC13b(mVU, Fd);									\
	}																\
	pass3 { mVUlog(OPname); mVUlogFd(); mVUlogI(); }				\
}
// FMAC14 - MADDA/MSUBA FMAC Opcode
#define mVU_FMAC14(operation, OPname) {								\
	pass1 { mVUanalyzeFMAC1(mVU, 0, _Fs_, _Ft_); }					\
	pass2 {															\
		int ACCw, ACCr, Fs, Ft;										\
		mVUallocFMAC14a(mVU, ACCw, ACCr, Fs, Ft);					\
		if (_X_Y_Z_W == 8) {										\
			SSE_MULSS_XMM_to_XMM(Fs, Ft);							\
			SSE_##operation##SS_XMM_to_XMM(ACCr, Fs);				\
		}															\
		else {														\
			SSE_MULPS_XMM_to_XMM(Fs, Ft);							\
			SSE_##operation##PS_XMM_to_XMM(ACCr, Fs);				\
		}															\
		mVUupdateFlags(mVU, ACCr, Fs, xmmT2, _X_Y_Z_W, 0);			\
		mVUallocFMAC14b(mVU, ACCw, ACCr);							\
	}																\
	pass3 { mVUlog(OPname); mVUlogACC(); mVUlogFt(); }				\
}
// FMAC15 - MADDA/MSUBA BC(xyzw) FMAC Opcode
#define mVU_FMAC15(operation, OPname) {								\
	pass1 { mVUanalyzeFMAC3(mVU, 0, _Fs_, _Ft_); }					\
	pass2 {															\
		int ACCw, ACCr, Fs, Ft;										\
		mVUallocFMAC15a(mVU, ACCw, ACCr, Fs, Ft);					\
		if (_X_Y_Z_W == 8) {										\
			SSE_MULSS_XMM_to_XMM(Fs, Ft);							\
			SSE_##operation##SS_XMM_to_XMM(ACCr, Fs);				\
		}															\
		else {														\
			SSE_MULPS_XMM_to_XMM(Fs, Ft);							\
			SSE_##operation##PS_XMM_to_XMM(ACCr, Fs);				\
		}															\
		mVUupdateFlags(mVU, ACCr, Fs, xmmT2, _X_Y_Z_W, 0);			\
		mVUallocFMAC15b(mVU, ACCw, ACCr);							\
	}																\
	pass3 { mVUlog(OPname); mVUlogACC(); mVUlogBC(); }				\
}
// FMAC16 - MADDA/MSUBA FMAC Opcode (I Reg)
#define mVU_FMAC16(operation, OPname) {								\
	pass1 { mVUanalyzeFMAC1(mVU, 0, _Fs_, 0); }						\
	pass2 {															\
		int ACCw, ACCr, Fs, Ft;										\
		mVUallocFMAC16a(mVU, ACCw, ACCr, Fs, Ft);					\
		if (_X_Y_Z_W == 8) {										\
			SSE_MULSS_XMM_to_XMM(Fs, Ft);							\
			SSE_##operation##SS_XMM_to_XMM(ACCr, Fs);				\
		}															\
		else {														\
			SSE_MULPS_XMM_to_XMM(Fs, Ft);							\
			SSE_##operation##PS_XMM_to_XMM(ACCr, Fs);				\
		}															\
		mVUupdateFlags(mVU, ACCr, Fs, xmmT2, _X_Y_Z_W, 0);			\
		mVUallocFMAC16b(mVU, ACCw, ACCr);							\
	}																\
	pass3 { mVUlog(OPname); mVUlogACC(); mVUlogI(); }				\
}
// FMAC18 - OPMULA FMAC Opcode
#define mVU_FMAC18(operation, OPname) {								\
	pass1 { mVUanalyzeFMAC1(mVU, 0, _Fs_, _Ft_); }					\
	pass2 {															\
		int ACC, Fs, Ft;											\
		mVUallocFMAC18a(mVU, ACC, Fs, Ft);							\
		SSE_##operation##PS_XMM_to_XMM(Fs, Ft);						\
		mVUupdateFlags(mVU, Fs, xmmT1, xmmT2, _X_Y_Z_W, 0);			\
		mVUallocFMAC18b(mVU, ACC, Fs);								\
	}																\
	pass3 { mVUlog(OPname); mVUlogACC(); mVUlogFt(); }				\
}
// FMAC19 - OPMSUB FMAC Opcode
#define mVU_FMAC19(operation, OPname) {								\
	pass1 { mVUanalyzeFMAC1(mVU, _Fd_, _Fs_, _Ft_); }				\
	pass2 {															\
		int Fd, ACC, Fs, Ft;										\
		mVUallocFMAC19a(mVU, Fd, ACC, Fs, Ft);						\
		SSE_MULPS_XMM_to_XMM(Fs, Ft);								\
		SSE_##operation##PS_XMM_to_XMM(ACC, Fs);					\
		mVUupdateFlags(mVU, Fd, Fs, xmmT2, _X_Y_Z_W, 0);			\
		mVUallocFMAC19b(mVU, Fd);									\
	}																\
	pass3 { mVUlog(OPname); mVUlogFd(); mVUlogFt(); }				\
}
// FMAC22 - Normal FMAC Opcodes (Q Reg)
#define mVU_FMAC22(operation, OPname) {								\
	pass1 { mVUanalyzeFMAC1(mVU, _Fd_, _Fs_, 0); }					\
	pass2 {															\
		int Fd, Fs, Ft;												\
		mVUallocFMAC22a(mVU, Fd, Fs, Ft);							\
		if (_XYZW_SS) SSE_##operation##SS_XMM_to_XMM(Fs, Ft);		\
		else		  SSE_##operation##PS_XMM_to_XMM(Fs, Ft);		\
		mVUupdateFlags(mVU, Fd, xmmT1, xmmT2, _X_Y_Z_W, 1);			\
		mVUallocFMAC22b(mVU, Fd);									\
	}																\
	pass3 { mVUlog(OPname); mVUlogFd(); mVUlogQ(); }				\
}
// FMAC23 - FMAC Opcodes Storing Result to ACC (Q Reg)
#define mVU_FMAC23(operation, OPname) {								\
	pass1 { mVUanalyzeFMAC1(mVU, 0, _Fs_, 0); }						\
	pass2 {															\
		int ACC, Fs, Ft;											\
		mVUallocFMAC23a(mVU, ACC, Fs, Ft);							\
		if (_X_Y_Z_W == 8)	SSE_##operation##SS_XMM_to_XMM(Fs, Ft);	\
		else				SSE_##operation##PS_XMM_to_XMM(Fs, Ft);	\
		mVUupdateFlags(mVU, Fs, xmmT1, xmmT2, _X_Y_Z_W, 0);			\
		mVUallocFMAC23b(mVU, ACC, Fs);								\
	}																\
	pass3 { mVUlog(OPname); mVUlogACC(); mVUlogQ();}				\
}
// FMAC24 - MADD FMAC Opcode Storing Result to Fd (Q Reg)
#define mVU_FMAC24(operation, OPname) {								\
	pass1 { mVUanalyzeFMAC1(mVU, _Fd_, _Fs_, 0); }					\
	pass2 {															\
		int Fd, ACC, Fs, Ft;										\
		mVUallocFMAC24a(mVU, Fd, ACC, Fs, Ft);						\
		if (_X_Y_Z_W == 8) {										\
			SSE_MULSS_XMM_to_XMM(Fs, Ft);							\
			SSE_##operation##SS_XMM_to_XMM(Fs, ACC);				\
		}															\
		else {														\
			SSE_MULPS_XMM_to_XMM(Fs, Ft);							\
			SSE_##operation##PS_XMM_to_XMM(Fs, ACC);				\
		}															\
		mVUupdateFlags(mVU, Fd, xmmT1, xmmT2, _X_Y_Z_W, 0);			\
		mVUallocFMAC24b(mVU, Fd);									\
	}																\
	pass3 { mVUlog(OPname); mVUlogFd(); mVUlogQ(); }				\
}
// FMAC25 - MSUB FMAC Opcode Storing Result to Fd (Q Reg)
#define mVU_FMAC25(operation, OPname) {								\
	pass1 { mVUanalyzeFMAC1(mVU, _Fd_, _Fs_, 0); }					\
	pass2 {															\
		int Fd, ACC, Fs, Ft;										\
		mVUallocFMAC25a(mVU, Fd, ACC, Fs, Ft);						\
		if (_X_Y_Z_W == 8) {										\
			SSE_MULSS_XMM_to_XMM(Fs, Ft);							\
			SSE_##operation##SS_XMM_to_XMM(ACC, Fs);				\
		}															\
		else {														\
			SSE_MULPS_XMM_to_XMM(Fs, Ft);							\
			SSE_##operation##PS_XMM_to_XMM(ACC, Fs);				\
		}															\
		mVUupdateFlags(mVU, Fd, Fs, xmmT2, _X_Y_Z_W, 0);			\
		mVUallocFMAC25b(mVU, Fd);									\
	}																\
	pass3 { mVUlog(OPname); mVUlogFd(); mVUlogQ(); }				\
}
// FMAC26 - MADDA/MSUBA FMAC Opcode (Q Reg)
#define mVU_FMAC26(operation, OPname) {								\
	pass1 { mVUanalyzeFMAC1(mVU, 0, _Fs_, 0); }						\
	pass2 {															\
		int ACCw, ACCr, Fs, Ft;										\
		mVUallocFMAC26a(mVU, ACCw, ACCr, Fs, Ft);					\
		if (_X_Y_Z_W == 8) {										\
			SSE_MULSS_XMM_to_XMM(Fs, Ft);							\
			SSE_##operation##SS_XMM_to_XMM(ACCr, Fs);				\
		}															\
		else {														\
			SSE_MULPS_XMM_to_XMM(Fs, Ft);							\
			SSE_##operation##PS_XMM_to_XMM(ACCr, Fs);				\
		}															\
		mVUupdateFlags(mVU, ACCr, Fs, xmmT2, _X_Y_Z_W, 0);			\
		mVUallocFMAC26b(mVU, ACCw, ACCr);							\
	}																\
	pass3 { mVUlog(OPname); mVUlogACC(); mVUlogQ(); }				\
}

// FMAC27~29 - MAX/MINI FMAC Opcodes
#define mVU_FMAC27(operation, OPname) { mVU_FMAC1 (operation, OPname); pass1 { sFLAG.doFlag = 0; } }
#define mVU_FMAC28(operation, OPname) { mVU_FMAC6 (operation, OPname); pass1 { sFLAG.doFlag = 0; } }
#define mVU_FMAC29(operation, OPname) { mVU_FMAC3 (operation, OPname); pass1 { sFLAG.doFlag = 0; } }

//------------------------------------------------------------------
// Micro VU Micromode Upper instructions
//------------------------------------------------------------------

mVUop(mVU_ABS) {
	pass1 { mVUanalyzeFMAC2(mVU, _Fs_, _Ft_); }
	pass2 { 
		int Fs, Ft;
		mVUallocFMAC2a(mVU, Fs, Ft);
		SSE_ANDPS_M128_to_XMM(Fs, (uptr)mVU_absclip);
		mVUallocFMAC2b(mVU, Ft);
	}
	pass3 { mVUlog("ABS"); mVUlogFtFs(); }
}
mVUop(mVU_ADD)		{ mVU_FMAC1 (ADD,  "ADD");    }
mVUop(mVU_ADDi)		{ mVU_FMAC6 (ADD2, "ADDi");   }
mVUop(mVU_ADDq)		{ mVU_FMAC22(ADD,  "ADDq");   }
mVUop(mVU_ADDx)		{ mVU_FMAC3 (ADD,  "ADDx");   }
mVUop(mVU_ADDy)		{ mVU_FMAC3 (ADD,  "ADDy");   }
mVUop(mVU_ADDz)		{ mVU_FMAC3 (ADD,  "ADDz");   }
mVUop(mVU_ADDw)		{ mVU_FMAC3 (ADD,  "ADDw");   }
mVUop(mVU_ADDA)		{ mVU_FMAC4 (ADD,  "ADDA");   }
mVUop(mVU_ADDAi)	{ mVU_FMAC7 (ADD,  "ADDAi");  }
mVUop(mVU_ADDAq)	{ mVU_FMAC23(ADD,  "ADDAq");  }
mVUop(mVU_ADDAx)	{ mVU_FMAC5 (ADD,  "ADDAx");  }
mVUop(mVU_ADDAy)	{ mVU_FMAC5 (ADD,  "ADDAy");  }
mVUop(mVU_ADDAz)	{ mVU_FMAC5 (ADD,  "ADDAz");  }
mVUop(mVU_ADDAw)	{ mVU_FMAC5 (ADD,  "ADDAw");  }
mVUop(mVU_SUB)		{ mVU_FMAC1 (SUB,  "SUB");    }
mVUop(mVU_SUBi)		{ mVU_FMAC6 (SUB,  "SUBi");   }
mVUop(mVU_SUBq)		{ mVU_FMAC22(SUB,  "SUBq");   }
mVUop(mVU_SUBx)		{ mVU_FMAC3 (SUB,  "SUBx");   }
mVUop(mVU_SUBy)		{ mVU_FMAC3 (SUB,  "SUBy");   }
mVUop(mVU_SUBz)		{ mVU_FMAC3 (SUB,  "SUBz");   }
mVUop(mVU_SUBw)		{ mVU_FMAC3 (SUB,  "SUBw");   }
mVUop(mVU_SUBA)		{ mVU_FMAC4 (SUB,  "SUBA");   }
mVUop(mVU_SUBAi)	{ mVU_FMAC7 (SUB,  "SUBAi");  }
mVUop(mVU_SUBAq)	{ mVU_FMAC23(SUB,  "SUBAq");  }
mVUop(mVU_SUBAx)	{ mVU_FMAC5 (SUB,  "SUBAx");  }
mVUop(mVU_SUBAy)	{ mVU_FMAC5 (SUB,  "SUBAy");  }
mVUop(mVU_SUBAz)	{ mVU_FMAC5 (SUB,  "SUBAz");  }
mVUop(mVU_SUBAw)	{ mVU_FMAC5 (SUB,  "SUBAw");  }
mVUop(mVU_MUL)		{ mVU_FMAC1 (MUL,  "MUL");    }
mVUop(mVU_MULi)		{ mVU_FMAC6 (MUL,  "MULi");   }
mVUop(mVU_MULq)		{ mVU_FMAC22(MUL,  "MULq");   }
mVUop(mVU_MULx)		{ mVU_FMAC3 (MUL,  "MULx");   }
mVUop(mVU_MULy)		{ mVU_FMAC3 (MUL,  "MULy");   }
mVUop(mVU_MULz)		{ mVU_FMAC3 (MUL,  "MULz");   }
mVUop(mVU_MULw)		{ mVU_FMAC3 (MUL,  "MULw");   }
mVUop(mVU_MULA)		{ mVU_FMAC4 (MUL,  "MULA");   }
mVUop(mVU_MULAi)	{ mVU_FMAC7 (MUL,  "MULAi");  }
mVUop(mVU_MULAq)	{ mVU_FMAC23(MUL,  "MULAq");  }
mVUop(mVU_MULAx)	{ mVU_FMAC5 (MUL,  "MULAx");  }
mVUop(mVU_MULAy)	{ mVU_FMAC5 (MUL,  "MULAy");  }
mVUop(mVU_MULAz)	{ mVU_FMAC5 (MUL,  "MULAz");  }
mVUop(mVU_MULAw)	{ mVU_FMAC5 (MUL,  "MULAw");  }
mVUop(mVU_MADD)		{ mVU_FMAC8 (ADD,  "MADD");   }
mVUop(mVU_MADDi)	{ mVU_FMAC12(ADD,  "MADDi");  }
mVUop(mVU_MADDq)	{ mVU_FMAC24(ADD,  "MADDq");  }
mVUop(mVU_MADDx)	{ mVU_FMAC10(ADD,  "MADDx");  }
mVUop(mVU_MADDy)	{ mVU_FMAC10(ADD,  "MADDy");  }
mVUop(mVU_MADDz)	{ mVU_FMAC10(ADD,  "MADDz");  }
mVUop(mVU_MADDw)	{ mVU_FMAC10(ADD,  "MADDw");  }
mVUop(mVU_MADDA)	{ mVU_FMAC14(ADD,  "MADDA");  }
mVUop(mVU_MADDAi)	{ mVU_FMAC16(ADD,  "MADDAi"); }
mVUop(mVU_MADDAq)	{ mVU_FMAC26(ADD,  "MADDAq"); }
mVUop(mVU_MADDAx)	{ mVU_FMAC15(ADD,  "MADDAx"); }
mVUop(mVU_MADDAy)	{ mVU_FMAC15(ADD,  "MADDAy"); }
mVUop(mVU_MADDAz)	{ mVU_FMAC15(ADD,  "MADDAz"); }
mVUop(mVU_MADDAw)	{ mVU_FMAC15(ADD,  "MADDAw"); }
mVUop(mVU_MSUB)		{ mVU_FMAC9 (SUB,  "MSUB");   }
mVUop(mVU_MSUBi)	{ mVU_FMAC13(SUB,  "MSUBi");  }
mVUop(mVU_MSUBq)	{ mVU_FMAC25(SUB,  "MSUBq");  }
mVUop(mVU_MSUBx)	{ mVU_FMAC11(SUB,  "MSUBx");  }
mVUop(mVU_MSUBy)	{ mVU_FMAC11(SUB,  "MSUBy");  }
mVUop(mVU_MSUBz)	{ mVU_FMAC11(SUB,  "MSUBz");  }
mVUop(mVU_MSUBw)	{ mVU_FMAC11(SUB,  "MSUBw");  }
mVUop(mVU_MSUBA)	{ mVU_FMAC14(SUB,  "MSUBA");  }
mVUop(mVU_MSUBAi)	{ mVU_FMAC16(SUB,  "MSUBAi"); }
mVUop(mVU_MSUBAq)	{ mVU_FMAC26(SUB,  "MSUBAq"); }
mVUop(mVU_MSUBAx)	{ mVU_FMAC15(SUB,  "MSUBAx"); }
mVUop(mVU_MSUBAy)	{ mVU_FMAC15(SUB,  "MSUBAy"); }
mVUop(mVU_MSUBAz)	{ mVU_FMAC15(SUB,  "MSUBAz"); }
mVUop(mVU_MSUBAw)	{ mVU_FMAC15(SUB,  "MSUBAw"); }
mVUop(mVU_MAX)		{ mVU_FMAC27(MAX2, "MAX");    }
mVUop(mVU_MAXi)		{ mVU_FMAC28(MAX2, "MAXi");   }
mVUop(mVU_MAXx)		{ mVU_FMAC29(MAX2, "MAXx");   }
mVUop(mVU_MAXy)		{ mVU_FMAC29(MAX2, "MAXy");   }
mVUop(mVU_MAXz)		{ mVU_FMAC29(MAX2, "MAXz");   }
mVUop(mVU_MAXw)		{ mVU_FMAC29(MAX2, "MAXw");   }
mVUop(mVU_MINI)		{ mVU_FMAC27(MIN2, "MINI");   }
mVUop(mVU_MINIi)	{ mVU_FMAC28(MIN2, "MINIi");  }
mVUop(mVU_MINIx)	{ mVU_FMAC29(MIN2, "MINIx");  }
mVUop(mVU_MINIy)	{ mVU_FMAC29(MIN2, "MINIy");  }
mVUop(mVU_MINIz)	{ mVU_FMAC29(MIN2, "MINIz");  }
mVUop(mVU_MINIw)	{ mVU_FMAC29(MIN2, "MINIw");  }
mVUop(mVU_OPMULA)	{ mVU_FMAC18(MUL,  "OPMULA"); }
mVUop(mVU_OPMSUB)	{ mVU_FMAC19(SUB,  "OPMSUB"); }
mVUop(mVU_NOP)		{ pass3 { mVUlog("NOP"); }    }
void mVU_FTOIx(mP, uptr addr) {
	pass1 { mVUanalyzeFMAC2(mVU, _Fs_, _Ft_); }
	pass2 { 
		int Fs, Ft;
		mVUallocFMAC2a(mVU, Fs, Ft);

		// Note: For help understanding this algorithm see recVUMI_FTOI_Saturate()
		SSE_MOVAPS_XMM_to_XMM(xmmT1, Fs);
		if (addr) { SSE_MULPS_M128_to_XMM(Fs, addr); }
		SSE2_CVTTPS2DQ_XMM_to_XMM(Fs, Fs);
		SSE2_PXOR_M128_to_XMM(xmmT1, (uptr)mVU_signbit);
		SSE2_PSRAD_I8_to_XMM(xmmT1, 31);
		SSE_MOVAPS_XMM_to_XMM(xmmFt, Fs);
		SSE2_PCMPEQD_M128_to_XMM(xmmFt, (uptr)mVU_signbit);
		SSE_ANDPS_XMM_to_XMM(xmmT1, xmmFt);
		SSE2_PADDD_XMM_to_XMM(Fs, xmmT1);

		mVUallocFMAC2b(mVU, Ft);
	}
}
mVUop(mVU_FTOI0)  { mVU_FTOIx(mX, (uptr)0);				pass3 { mVUlog("FTOI0");  mVUlogFtFs(); } }
mVUop(mVU_FTOI4)  { mVU_FTOIx(mX, (uptr)mVU_FTOI_4);	pass3 { mVUlog("FTOI4");  mVUlogFtFs(); } }
mVUop(mVU_FTOI12) { mVU_FTOIx(mX, (uptr)mVU_FTOI_12);	pass3 { mVUlog("FTOI12"); mVUlogFtFs(); } }
mVUop(mVU_FTOI15) { mVU_FTOIx(mX, (uptr)mVU_FTOI_15);	pass3 { mVUlog("FTOI15"); mVUlogFtFs(); } }
void mVU_ITOFx(mP, uptr addr) {
	pass1 { mVUanalyzeFMAC2(mVU, _Fs_, _Ft_); }
	pass2 { 
		int Fs, Ft;
		mVUallocFMAC2a(mVU, Fs, Ft);

		SSE2_CVTDQ2PS_XMM_to_XMM(Ft, Fs);
		if (addr) { SSE_MULPS_M128_to_XMM(Ft, addr); }
		//mVUclamp2(Ft, xmmT1, 15); // Clamp (not sure if this is needed)
		
		mVUallocFMAC2b(mVU, Ft);
	}
}
mVUop(mVU_ITOF0)  { mVU_ITOFx(mX, (uptr)0);				pass3 { mVUlog("ITOF0");  mVUlogFtFs(); } }
mVUop(mVU_ITOF4)  { mVU_ITOFx(mX, (uptr)mVU_ITOF_4);	pass3 { mVUlog("ITOF4");  mVUlogFtFs(); } }
mVUop(mVU_ITOF12) { mVU_ITOFx(mX, (uptr)mVU_ITOF_12);	pass3 { mVUlog("ITOF12"); mVUlogFtFs(); } }
mVUop(mVU_ITOF15) { mVU_ITOFx(mX, (uptr)mVU_ITOF_15);	pass3 { mVUlog("ITOF15"); mVUlogFtFs(); } }
mVUop(mVU_CLIP) {
	pass1 { mVUanalyzeFMAC4(mVU, _Fs_, _Ft_); }
	pass2 {
		int Fs, Ft;
		mVUallocFMAC17a(mVU, Fs, Ft);
		mVUallocCFLAGa(mVU, gprT1, cFLAG.lastWrite);
		SHL32ItoR(gprT1, 6);

		SSE_ANDPS_M128_to_XMM(Ft, (uptr)mVU_absclip);
		SSE_MOVAPS_XMM_to_XMM(xmmT1, Ft);
		SSE_ORPS_M128_to_XMM(xmmT1, (uptr)mVU_signbit);

		SSE_CMPNLEPS_XMM_to_XMM(xmmT1, Fs);  //-w, -z, -y, -x
		SSE_CMPLTPS_XMM_to_XMM(Ft, Fs); //+w, +z, +y, +x

		SSE_MOVAPS_XMM_to_XMM(Fs, Ft); //Fs = +w, +z, +y, +x
		SSE_UNPCKLPS_XMM_to_XMM(Ft, xmmT1); //Ft = -y,+y,-x,+x
		SSE_UNPCKHPS_XMM_to_XMM(Fs, xmmT1); //Fs = -w,+w,-z,+z

		SSE_MOVMSKPS_XMM_to_R32(gprT2, Fs); // -w,+w,-z,+z
		AND32ItoR(gprT2, 0x3);
		SHL32ItoR(gprT2, 4);
		OR32RtoR (gprT1, gprT2);

		SSE_MOVMSKPS_XMM_to_R32(gprT2, Ft); // -y,+y,-x,+x
		AND32ItoR(gprT2, 0xf);
		OR32RtoR (gprT1, gprT2);
		AND32ItoR(gprT1, 0xffffff);

		mVUallocCFLAGb(mVU, gprT1, cFLAG.write);
	}
	pass3 { mVUlog("CLIP"); mVUlogCLIP(); }
}

