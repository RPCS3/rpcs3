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

#define AND_XYZW			((_XYZW_SS && modXYZW) ? (1) : (doMac ? (_X_Y_Z_W) : (flipMask[_X_Y_Z_W])))
#define ADD_XYZW			((_XYZW_SS && modXYZW) ? (_X ? 3 : (_Y ? 2 : (_Z ? 1 : 0))) : 0)
#define SHIFT_XYZW(gprReg)	{ if (_XYZW_SS && modXYZW && !_W) { SHL32ItoR(gprReg, ADD_XYZW); } }

// Note: If modXYZW is true, then it adjusts XYZW for Single Scalar operations
microVUt(void) mVUupdateFlags(int reg, int regT1, int regT2, int xyzw, bool modXYZW) {
	microVU* mVU = mVUx;
	int sReg, mReg = gprT1;
	static u8 *pjmp, *pjmp2;
	static const u16 flipMask[16] = {0, 8, 4, 12, 2, 10, 6, 14, 1, 9, 5, 13, 3, 11, 7, 15};

	//SysPrintf("doStatus = %d; doMac = %d\n", doStatus>>9, doMac>>8);
	if (mVUsFlagHack) { mVUinfo &= ~_doStatus; }
	if (!doFlags) return;
	if (!doMac || (_XYZW_SS && modXYZW)) { regT1 = reg; }
	else { SSE2_PSHUFD_XMM_to_XMM(regT1, reg, 0x1B); } // Flip wzyx to xyzw
	if (doStatus) {
		getFlagReg(sReg, fsInstance); // Set sReg to valid GPR by Cur Flag Instance
		mVUallocSFLAGa<vuIndex>(sReg, fpsInstance); // Get Prev Status Flag
		AND32ItoR(sReg, 0xff0); // Keep Sticky and D/I flags
	}

	//-------------------------Check for Signed flags------------------------------

	// The following code makes sure the Signed Bit isn't set with Negative Zero
	SSE_XORPS_XMM_to_XMM(regT2, regT2); // Clear regT2
	SSE_CMPEQPS_XMM_to_XMM(regT2, regT1); // Set all F's if each vector is zero
	SSE_MOVMSKPS_XMM_to_R32(gprT2, regT2); // Used for Zero Flag Calculation
	SSE_ANDNPS_XMM_to_XMM(regT2, regT1);

	SSE_MOVMSKPS_XMM_to_R32(mReg, regT2); // Move the sign bits of the t1reg

	AND32ItoR(mReg, AND_XYZW);  // Grab "Is Signed" bits from the previous calculation
	if (doStatus) pjmp = JZ8(0); // Skip if none are
		if (doMac)	  SHL32ItoR(mReg, 4 + ADD_XYZW);
		if (doStatus) OR32ItoR(sReg, 0x82); // SS, S flags
		if (_XYZW_SS && doStatus) pjmp2 = JMP8(0); // If negative and not Zero, we can skip the Zero Flag checking
	if (doStatus) x86SetJ8(pjmp);

	//-------------------------Check for Zero flags------------------------------

	AND32ItoR(gprT2, AND_XYZW);  // Grab "Is Zero" bits from the previous calculation
	if (doStatus) pjmp = JZ8(0); // Skip if none are
		if (doMac)	  { SHIFT_XYZW(gprT2); OR32RtoR(mReg, gprT2); }	
		if (doStatus) { OR32ItoR(sReg, 0x41); } // ZS, Z flags		
	if (doStatus) x86SetJ8(pjmp);

	//-------------------------Write back flags------------------------------

	if (_XYZW_SS && doStatus) x86SetJ8(pjmp2); // If we skipped the Zero Flag Checking, return here

	if (doMac) mVUallocMFLAGb<vuIndex>(mReg, fmInstance); // Set Mac Flag
}

//------------------------------------------------------------------
// Helper Macros
//------------------------------------------------------------------

// FMAC1 - Normal FMAC Opcodes
#define mVU_FMAC1(operation, OPname) {								\
	microVU* mVU = mVUx;											\
	pass1 { mVUanalyzeFMAC1<vuIndex>(_Fd_, _Fs_, _Ft_); }			\
	pass2 {															\
		int Fd, Fs, Ft;												\
		mVUallocFMAC1a<vuIndex>(Fd, Fs, Ft);						\
		if (_XYZW_SS) SSE_##operation##SS_XMM_to_XMM(Fs, Ft);		\
		else		  SSE_##operation##PS_XMM_to_XMM(Fs, Ft);		\
		mVUupdateFlags<vuIndex>(Fd, xmmT1, xmmT2, _X_Y_Z_W, 1);		\
		mVUallocFMAC1b<vuIndex>(Fd);								\
	}																\
	pass3 {	mVUlog(OPname);	mVUlogFd(); mVUlogFt();	}				\
}
// FMAC3 - BC(xyzw) FMAC Opcodes
#define mVU_FMAC3(operation, OPname) {								\
	microVU* mVU = mVUx;											\
	pass1 { mVUanalyzeFMAC3<vuIndex>(_Fd_, _Fs_, _Ft_); }			\
	pass2 {															\
		int Fd, Fs, Ft;												\
		mVUallocFMAC3a<vuIndex>(Fd, Fs, Ft);						\
		if (_XYZW_SS) SSE_##operation##SS_XMM_to_XMM(Fs, Ft);		\
		else		  SSE_##operation##PS_XMM_to_XMM(Fs, Ft);		\
		mVUupdateFlags<vuIndex>(Fd, xmmT1, xmmT2, _X_Y_Z_W, 1);		\
		mVUallocFMAC3b<vuIndex>(Fd);								\
	}																\
	pass3 { mVUlog(OPname); mVUlogFd(); mVUlogBC(); }				\
}
// FMAC4 - FMAC Opcodes Storing Result to ACC
#define mVU_FMAC4(operation, OPname) {								\
	microVU* mVU = mVUx;											\
	pass1 { mVUanalyzeFMAC1<vuIndex>(0, _Fs_, _Ft_); }				\
	pass2 {															\
		int ACC, Fs, Ft;											\
		mVUallocFMAC4a<vuIndex>(ACC, Fs, Ft);						\
		if (_X_Y_Z_W == 8)	SSE_##operation##SS_XMM_to_XMM(Fs, Ft);	\
		else				SSE_##operation##PS_XMM_to_XMM(Fs, Ft);	\
		mVUupdateFlags<vuIndex>(Fs, xmmT1, xmmT2, _X_Y_Z_W, 0);		\
		mVUallocFMAC4b<vuIndex>(ACC, Fs);							\
	}																\
	pass3 { mVUlog(OPname); mVUlogACC(); mVUlogFt(); }				\
}
// FMAC5 - FMAC BC(xyzw) Opcodes Storing Result to ACC
#define mVU_FMAC5(operation, OPname) {								\
	microVU* mVU = mVUx;											\
	pass1 { mVUanalyzeFMAC3<vuIndex>(0, _Fs_, _Ft_); }				\
	pass2 {															\
		int ACC, Fs, Ft;											\
		mVUallocFMAC5a<vuIndex>(ACC, Fs, Ft);						\
		if (_X_Y_Z_W == 8)	SSE_##operation##SS_XMM_to_XMM(Fs, Ft);	\
		else				SSE_##operation##PS_XMM_to_XMM(Fs, Ft);	\
		mVUupdateFlags<vuIndex>(Fs, xmmT1, xmmT2, _X_Y_Z_W, 0);		\
		mVUallocFMAC5b<vuIndex>(ACC, Fs);							\
	}																\
	pass3 { mVUlog(OPname); mVUlogACC(); mVUlogBC(); }				\
}
// FMAC6 - Normal FMAC Opcodes (I Reg)
#define mVU_FMAC6(operation, OPname) {								\
	microVU* mVU = mVUx;											\
	pass1 { mVUanalyzeFMAC1<vuIndex>(_Fd_, _Fs_, 0); }				\
	pass2 {															\
		int Fd, Fs, Ft;												\
		mVUallocFMAC6a<vuIndex>(Fd, Fs, Ft);						\
		if (_XYZW_SS) SSE_##operation##SS_XMM_to_XMM(Fs, Ft);		\
		else		  SSE_##operation##PS_XMM_to_XMM(Fs, Ft);		\
		mVUupdateFlags<vuIndex>(Fd, xmmT1, xmmT2, _X_Y_Z_W, 1);		\
		mVUallocFMAC6b<vuIndex>(Fd);								\
	}																\
	pass3 { mVUlog(OPname); mVUlogFd(); mVUlogI(); }				\
}
// FMAC7 - FMAC Opcodes Storing Result to ACC (I Reg)
#define mVU_FMAC7(operation, OPname) {								\
	microVU* mVU = mVUx;											\
	pass1 { mVUanalyzeFMAC1<vuIndex>(0, _Fs_, 0); }					\
	pass2 {															\
		int ACC, Fs, Ft;											\
		mVUallocFMAC7a<vuIndex>(ACC, Fs, Ft);						\
		if (_X_Y_Z_W == 8)	SSE_##operation##SS_XMM_to_XMM(Fs, Ft);	\
		else				SSE_##operation##PS_XMM_to_XMM(Fs, Ft);	\
		mVUupdateFlags<vuIndex>(Fs, xmmT1, xmmT2, _X_Y_Z_W, 0);		\
		mVUallocFMAC7b<vuIndex>(ACC, Fs);							\
	}																\
	pass3 { mVUlog(OPname); mVUlogACC(); mVUlogI(); }				\
}
// FMAC8 - MADD FMAC Opcode Storing Result to Fd
#define mVU_FMAC8(operation, OPname) {								\
	microVU* mVU = mVUx;											\
	pass1 { mVUanalyzeFMAC1<vuIndex>(_Fd_, _Fs_, _Ft_); }			\
	pass2 {															\
		int Fd, ACC, Fs, Ft;										\
		mVUallocFMAC8a<vuIndex>(Fd, ACC, Fs, Ft);					\
		if (_X_Y_Z_W == 8) {										\
			SSE_MULSS_XMM_to_XMM(Fs, Ft);							\
			SSE_##operation##SS_XMM_to_XMM(Fs, ACC);				\
		}															\
		else {														\
			SSE_MULPS_XMM_to_XMM(Fs, Ft);							\
			SSE_##operation##PS_XMM_to_XMM(Fs, ACC);				\
		}															\
		mVUupdateFlags<vuIndex>(Fd, xmmT1, xmmT2, _X_Y_Z_W, 0);		\
		mVUallocFMAC8b<vuIndex>(Fd);								\
	}																\
	pass3 { mVUlog(OPname); mVUlogFd(); mVUlogFt(); }				\
}
// FMAC9 - MSUB FMAC Opcode Storing Result to Fd
#define mVU_FMAC9(operation, OPname) {								\
	microVU* mVU = mVUx;											\
	pass1 { mVUanalyzeFMAC1<vuIndex>(_Fd_, _Fs_, _Ft_); }			\
	pass2 {															\
		int Fd, ACC, Fs, Ft;										\
		mVUallocFMAC9a<vuIndex>(Fd, ACC, Fs, Ft);					\
		if (_X_Y_Z_W == 8) {										\
			SSE_MULSS_XMM_to_XMM(Fs, Ft);							\
			SSE_##operation##SS_XMM_to_XMM(ACC, Fs);				\
		}															\
		else {														\
			SSE_MULPS_XMM_to_XMM(Fs, Ft);							\
			SSE_##operation##PS_XMM_to_XMM(ACC, Fs);				\
		}															\
		mVUupdateFlags<vuIndex>(Fd, Fs, xmmT2, _X_Y_Z_W, 0);		\
		mVUallocFMAC9b<vuIndex>(Fd);								\
	}																\
	pass3 { mVUlog(OPname); mVUlogFd(); mVUlogFt(); }				\
}
// FMAC10 - MADD FMAC BC(xyzw) Opcode Storing Result to Fd
#define mVU_FMAC10(operation, OPname) {								\
	microVU* mVU = mVUx;											\
	pass1 { mVUanalyzeFMAC3<vuIndex>(_Fd_, _Fs_, _Ft_); }			\
	pass2 {															\
		int Fd, ACC, Fs, Ft;										\
		mVUallocFMAC10a<vuIndex>(Fd, ACC, Fs, Ft);					\
		if (_X_Y_Z_W == 8) {										\
			SSE_MULSS_XMM_to_XMM(Fs, Ft);							\
			SSE_##operation##SS_XMM_to_XMM(Fs, ACC);				\
		}															\
		else {														\
			SSE_MULPS_XMM_to_XMM(Fs, Ft);							\
			SSE_##operation##PS_XMM_to_XMM(Fs, ACC);				\
		}															\
		mVUupdateFlags<vuIndex>(Fd, xmmT1, xmmT2, _X_Y_Z_W, 0);		\
		mVUallocFMAC10b<vuIndex>(Fd);								\
	}																\
	pass3 { mVUlog(OPname); mVUlogFd(); mVUlogBC(); }				\
}
// FMAC11 - MSUB FMAC BC(xyzw) Opcode Storing Result to Fd
#define mVU_FMAC11(operation, OPname) {								\
	microVU* mVU = mVUx;											\
	pass1 { mVUanalyzeFMAC3<vuIndex>(_Fd_, _Fs_, _Ft_); }			\
	pass2 {															\
		int Fd, ACC, Fs, Ft;										\
		mVUallocFMAC11a<vuIndex>(Fd, ACC, Fs, Ft);					\
		if (_X_Y_Z_W == 8) {										\
			SSE_MULSS_XMM_to_XMM(Fs, Ft);							\
			SSE_##operation##SS_XMM_to_XMM(ACC, Fs);				\
		}															\
		else {														\
			SSE_MULPS_XMM_to_XMM(Fs, Ft);							\
			SSE_##operation##PS_XMM_to_XMM(ACC, Fs);				\
		}															\
		mVUupdateFlags<vuIndex>(Fd, Fs, xmmT2, _X_Y_Z_W, 0);		\
		mVUallocFMAC11b<vuIndex>(Fd);								\
	}																\
	pass3 { mVUlog(OPname); mVUlogFd(); mVUlogBC(); }				\
}
// FMAC12 - MADD FMAC Opcode Storing Result to Fd (I Reg)
#define mVU_FMAC12(operation, OPname) {								\
	microVU* mVU = mVUx;											\
	pass1 { mVUanalyzeFMAC1<vuIndex>(_Fd_, _Fs_, 0); }				\
	pass2 {															\
		int Fd, ACC, Fs, Ft;										\
		mVUallocFMAC12a<vuIndex>(Fd, ACC, Fs, Ft);					\
		if (_X_Y_Z_W == 8) {										\
			SSE_MULSS_XMM_to_XMM(Fs, Ft);							\
			SSE_##operation##SS_XMM_to_XMM(Fs, ACC);				\
		}															\
		else {														\
			SSE_MULPS_XMM_to_XMM(Fs, Ft);							\
			SSE_##operation##PS_XMM_to_XMM(Fs, ACC);				\
		}															\
		mVUupdateFlags<vuIndex>(Fd, xmmT1, xmmT2, _X_Y_Z_W, 0);		\
		mVUallocFMAC12b<vuIndex>(Fd);								\
	}																\
	pass3 { mVUlog(OPname); mVUlogFd(); mVUlogI(); }				\
}
// FMAC13 - MSUB FMAC Opcode Storing Result to Fd (I Reg)
#define mVU_FMAC13(operation, OPname) {								\
	microVU* mVU = mVUx;											\
	pass1 { mVUanalyzeFMAC1<vuIndex>(_Fd_, _Fs_, 0); }				\
	pass2 {															\
		int Fd, ACC, Fs, Ft;										\
		mVUallocFMAC13a<vuIndex>(Fd, ACC, Fs, Ft);					\
		if (_X_Y_Z_W == 8) {										\
			SSE_MULSS_XMM_to_XMM(Fs, Ft);							\
			SSE_##operation##SS_XMM_to_XMM(ACC, Fs);				\
		}															\
		else {														\
			SSE_MULPS_XMM_to_XMM(Fs, Ft);							\
			SSE_##operation##PS_XMM_to_XMM(ACC, Fs);				\
		}															\
		mVUupdateFlags<vuIndex>(Fd, Fs, xmmT2, _X_Y_Z_W, 0);		\
		mVUallocFMAC13b<vuIndex>(Fd);								\
	}																\
	pass3 { mVUlog(OPname); mVUlogFd(); mVUlogI(); }				\
}
// FMAC14 - MADDA/MSUBA FMAC Opcode
#define mVU_FMAC14(operation, OPname) {								\
	microVU* mVU = mVUx;											\
	pass1 { mVUanalyzeFMAC1<vuIndex>(0, _Fs_, _Ft_); }				\
	pass2 {															\
		int ACCw, ACCr, Fs, Ft;										\
		mVUallocFMAC14a<vuIndex>(ACCw, ACCr, Fs, Ft);				\
		if (_X_Y_Z_W == 8) {										\
			SSE_MULSS_XMM_to_XMM(Fs, Ft);							\
			SSE_##operation##SS_XMM_to_XMM(ACCr, Fs);				\
		}															\
		else {														\
			SSE_MULPS_XMM_to_XMM(Fs, Ft);							\
			SSE_##operation##PS_XMM_to_XMM(ACCr, Fs);				\
		}															\
		mVUupdateFlags<vuIndex>(ACCr, Fs, xmmT2, _X_Y_Z_W, 0);		\
		mVUallocFMAC14b<vuIndex>(ACCw, ACCr);						\
	}																\
	pass3 { mVUlog(OPname); mVUlogACC(); mVUlogFt(); }				\
}
// FMAC15 - MADDA/MSUBA BC(xyzw) FMAC Opcode
#define mVU_FMAC15(operation, OPname) {								\
	microVU* mVU = mVUx;											\
	pass1 { mVUanalyzeFMAC3<vuIndex>(0, _Fs_, _Ft_); }				\
	pass2 {															\
		int ACCw, ACCr, Fs, Ft;										\
		mVUallocFMAC15a<vuIndex>(ACCw, ACCr, Fs, Ft);				\
		if (_X_Y_Z_W == 8) {										\
			SSE_MULSS_XMM_to_XMM(Fs, Ft);							\
			SSE_##operation##SS_XMM_to_XMM(ACCr, Fs);				\
		}															\
		else {														\
			SSE_MULPS_XMM_to_XMM(Fs, Ft);							\
			SSE_##operation##PS_XMM_to_XMM(ACCr, Fs);				\
		}															\
		mVUupdateFlags<vuIndex>(ACCr, Fs, xmmT2, _X_Y_Z_W, 0);		\
		mVUallocFMAC15b<vuIndex>(ACCw, ACCr);						\
	}																\
	pass3 { mVUlog(OPname); mVUlogACC(); mVUlogBC(); }				\
}
// FMAC16 - MADDA/MSUBA FMAC Opcode (I Reg)
#define mVU_FMAC16(operation, OPname) {								\
	microVU* mVU = mVUx;											\
	pass1 { mVUanalyzeFMAC1<vuIndex>(0, _Fs_, 0); }					\
	pass2 {															\
		int ACCw, ACCr, Fs, Ft;										\
		mVUallocFMAC16a<vuIndex>(ACCw, ACCr, Fs, Ft);				\
		if (_X_Y_Z_W == 8) {										\
			SSE_MULSS_XMM_to_XMM(Fs, Ft);							\
			SSE_##operation##SS_XMM_to_XMM(ACCr, Fs);				\
		}															\
		else {														\
			SSE_MULPS_XMM_to_XMM(Fs, Ft);							\
			SSE_##operation##PS_XMM_to_XMM(ACCr, Fs);				\
		}															\
		mVUupdateFlags<vuIndex>(ACCr, Fs, xmmT2, _X_Y_Z_W, 0);		\
		mVUallocFMAC16b<vuIndex>(ACCw, ACCr);						\
	}																\
	pass3 { mVUlog(OPname); mVUlogACC(); mVUlogI(); }				\
}
// FMAC18 - OPMULA FMAC Opcode
#define mVU_FMAC18(operation, OPname) {								\
	microVU* mVU = mVUx;											\
	pass1 { mVUanalyzeFMAC1<vuIndex>(0, _Fs_, _Ft_); }				\
	pass2 {															\
		int ACC, Fs, Ft;											\
		mVUallocFMAC18a<vuIndex>(ACC, Fs, Ft);						\
		SSE_##operation##PS_XMM_to_XMM(Fs, Ft);						\
		mVUupdateFlags<vuIndex>(Fs, xmmT1, xmmT2, _X_Y_Z_W, 0);		\
		mVUallocFMAC18b<vuIndex>(ACC, Fs);							\
	}																\
	pass3 { mVUlog(OPname); mVUlogACC(); mVUlogFt(); }				\
}
// FMAC19 - OPMSUB FMAC Opcode
#define mVU_FMAC19(operation, OPname) {								\
	microVU* mVU = mVUx;											\
	pass1 { mVUanalyzeFMAC1<vuIndex>(_Fd_, _Fs_, _Ft_); }			\
	pass2 {															\
		int Fd, ACC, Fs, Ft;										\
		mVUallocFMAC19a<vuIndex>(Fd, ACC, Fs, Ft);					\
		SSE_MULPS_XMM_to_XMM(Fs, Ft);								\
		SSE_##operation##PS_XMM_to_XMM(ACC, Fs);					\
		mVUupdateFlags<vuIndex>(Fd, Fs, xmmT2, _X_Y_Z_W, 0);		\
		mVUallocFMAC19b<vuIndex>(Fd);								\
	}																\
	pass3 { mVUlog(OPname); mVUlogFd(); mVUlogFt(); }				\
}
// FMAC22 - Normal FMAC Opcodes (Q Reg)
#define mVU_FMAC22(operation, OPname) {								\
	microVU* mVU = mVUx;											\
	pass1 { mVUanalyzeFMAC1<vuIndex>(_Fd_, _Fs_, 0); }				\
	pass2 {															\
		int Fd, Fs, Ft;												\
		mVUallocFMAC22a<vuIndex>(Fd, Fs, Ft);						\
		if (_XYZW_SS) SSE_##operation##SS_XMM_to_XMM(Fs, Ft);		\
		else		  SSE_##operation##PS_XMM_to_XMM(Fs, Ft);		\
		mVUupdateFlags<vuIndex>(Fd, xmmT1, xmmT2, _X_Y_Z_W, 1);		\
		mVUallocFMAC22b<vuIndex>(Fd);								\
	}																\
	pass3 { mVUlog(OPname); mVUlogFd(); mVUlogQ(); }				\
}
// FMAC23 - FMAC Opcodes Storing Result to ACC (Q Reg)
#define mVU_FMAC23(operation, OPname) {								\
	microVU* mVU = mVUx;											\
	pass1 { mVUanalyzeFMAC1<vuIndex>(0, _Fs_, 0); }					\
	pass2 {															\
		int ACC, Fs, Ft;											\
		mVUallocFMAC23a<vuIndex>(ACC, Fs, Ft);						\
		if (_X_Y_Z_W == 8)	SSE_##operation##SS_XMM_to_XMM(Fs, Ft);	\
		else				SSE_##operation##PS_XMM_to_XMM(Fs, Ft);	\
		mVUupdateFlags<vuIndex>(Fs, xmmT1, xmmT2, _X_Y_Z_W, 0);		\
		mVUallocFMAC23b<vuIndex>(ACC, Fs);							\
	}																\
	pass3 { mVUlog(OPname); mVUlogACC(); mVUlogQ();}				\
}
// FMAC24 - MADD FMAC Opcode Storing Result to Fd (Q Reg)
#define mVU_FMAC24(operation, OPname) {								\
	microVU* mVU = mVUx;											\
	pass1 { mVUanalyzeFMAC1<vuIndex>(_Fd_, _Fs_, 0); }				\
	pass2 {															\
		int Fd, ACC, Fs, Ft;										\
		mVUallocFMAC24a<vuIndex>(Fd, ACC, Fs, Ft);					\
		if (_X_Y_Z_W == 8) {										\
			SSE_MULSS_XMM_to_XMM(Fs, Ft);							\
			SSE_##operation##SS_XMM_to_XMM(Fs, ACC);				\
		}															\
		else {														\
			SSE_MULPS_XMM_to_XMM(Fs, Ft);							\
			SSE_##operation##PS_XMM_to_XMM(Fs, ACC);				\
		}															\
		mVUupdateFlags<vuIndex>(Fd, xmmT1, xmmT2, _X_Y_Z_W, 0);		\
		mVUallocFMAC24b<vuIndex>(Fd);								\
	}																\
	pass3 { mVUlog(OPname); mVUlogFd(); mVUlogQ(); }				\
}
// FMAC25 - MSUB FMAC Opcode Storing Result to Fd (Q Reg)
#define mVU_FMAC25(operation, OPname) {								\
	microVU* mVU = mVUx;											\
	pass1 { mVUanalyzeFMAC1<vuIndex>(_Fd_, _Fs_, 0); }				\
	pass2 {															\
		int Fd, ACC, Fs, Ft;										\
		mVUallocFMAC25a<vuIndex>(Fd, ACC, Fs, Ft);					\
		if (_X_Y_Z_W == 8) {										\
			SSE_MULSS_XMM_to_XMM(Fs, Ft);							\
			SSE_##operation##SS_XMM_to_XMM(ACC, Fs);				\
		}															\
		else {														\
			SSE_MULPS_XMM_to_XMM(Fs, Ft);							\
			SSE_##operation##PS_XMM_to_XMM(ACC, Fs);				\
		}															\
		mVUupdateFlags<vuIndex>(Fd, Fs, xmmT2, _X_Y_Z_W, 0);		\
		mVUallocFMAC25b<vuIndex>(Fd);								\
	}																\
	pass3 { mVUlog(OPname); mVUlogFd(); mVUlogQ(); }				\
}
// FMAC26 - MADDA/MSUBA FMAC Opcode (Q Reg)
#define mVU_FMAC26(operation, OPname) {								\
	microVU* mVU = mVUx;											\
	pass1 { mVUanalyzeFMAC1<vuIndex>(0, _Fs_, 0); }					\
	pass2 {															\
		int ACCw, ACCr, Fs, Ft;										\
		mVUallocFMAC26a<vuIndex>(ACCw, ACCr, Fs, Ft);				\
		if (_X_Y_Z_W == 8) {										\
			SSE_MULSS_XMM_to_XMM(Fs, Ft);							\
			SSE_##operation##SS_XMM_to_XMM(ACCr, Fs);				\
		}															\
		else {														\
			SSE_MULPS_XMM_to_XMM(Fs, Ft);							\
			SSE_##operation##PS_XMM_to_XMM(ACCr, Fs);				\
		}															\
		mVUupdateFlags<vuIndex>(ACCr, Fs, xmmT2, _X_Y_Z_W, 0);		\
		mVUallocFMAC26b<vuIndex>(ACCw, ACCr);						\
	}																\
	pass3 { mVUlog(OPname); mVUlogACC(); mVUlogQ(); }				\
}

// FMAC27~29 - MAX/MINI FMAC Opcodes
#define mVU_FMAC27(operation, OPname) { mVU_FMAC1 (operation, OPname); pass1 { microVU* mVU = mVUx; mVUinfo &= ~_doStatus; } }
#define mVU_FMAC28(operation, OPname) { mVU_FMAC6 (operation, OPname); pass1 { microVU* mVU = mVUx; mVUinfo &= ~_doStatus; } }
#define mVU_FMAC29(operation, OPname) { mVU_FMAC3 (operation, OPname); pass1 { microVU* mVU = mVUx; mVUinfo &= ~_doStatus; } }

//------------------------------------------------------------------
// Micro VU Micromode Upper instructions
//------------------------------------------------------------------

microVUf(void) mVU_ABS(mF) {
	microVU* mVU = mVUx;
	pass1 { mVUanalyzeFMAC2<vuIndex>(_Fs_, _Ft_); }
	pass2 { 
		int Fs, Ft;
		mVUallocFMAC2a<vuIndex>(Fs, Ft);
		SSE_ANDPS_M128_to_XMM(Fs, (uptr)mVU_absclip);
		mVUallocFMAC2b<vuIndex>(Ft);
	}
	pass3 { mVUlog("ABS"); mVUlogFtFs(); }
}
microVUf(void) mVU_ADD(mF)		{ mVU_FMAC1 (ADD,  "ADD");    }
microVUf(void) mVU_ADDi(mF)		{ mVU_FMAC6 (ADD2, "ADDi");   }
microVUf(void) mVU_ADDq(mF)		{ mVU_FMAC22(ADD,  "ADDq");   }
microVUf(void) mVU_ADDx(mF)		{ mVU_FMAC3 (ADD,  "ADDx");   }
microVUf(void) mVU_ADDy(mF)		{ mVU_FMAC3 (ADD,  "ADDy");   }
microVUf(void) mVU_ADDz(mF)		{ mVU_FMAC3 (ADD,  "ADDz");   }
microVUf(void) mVU_ADDw(mF)		{ mVU_FMAC3 (ADD,  "ADDw");   }
microVUf(void) mVU_ADDA(mF)		{ mVU_FMAC4 (ADD,  "ADDA");   }
microVUf(void) mVU_ADDAi(mF)	{ mVU_FMAC7 (ADD,  "ADDAi");  }
microVUf(void) mVU_ADDAq(mF)	{ mVU_FMAC23(ADD,  "ADDAq");  }
microVUf(void) mVU_ADDAx(mF)	{ mVU_FMAC5 (ADD,  "ADDAx");  }
microVUf(void) mVU_ADDAy(mF)	{ mVU_FMAC5 (ADD,  "ADDAy");  }
microVUf(void) mVU_ADDAz(mF)	{ mVU_FMAC5 (ADD,  "ADDAz");  }
microVUf(void) mVU_ADDAw(mF)	{ mVU_FMAC5 (ADD,  "ADDAw");  }
microVUf(void) mVU_SUB(mF)		{ mVU_FMAC1 (SUB,  "SUB");    }
microVUf(void) mVU_SUBi(mF)		{ mVU_FMAC6 (SUB,  "SUBi");   }
microVUf(void) mVU_SUBq(mF)		{ mVU_FMAC22(SUB,  "SUBq");   }
microVUf(void) mVU_SUBx(mF)		{ mVU_FMAC3 (SUB,  "SUBx");   }
microVUf(void) mVU_SUBy(mF)		{ mVU_FMAC3 (SUB,  "SUBy");   }
microVUf(void) mVU_SUBz(mF)		{ mVU_FMAC3 (SUB,  "SUBz");   }
microVUf(void) mVU_SUBw(mF)		{ mVU_FMAC3 (SUB,  "SUBw");   }
microVUf(void) mVU_SUBA(mF)		{ mVU_FMAC4 (SUB,  "SUBA");   }
microVUf(void) mVU_SUBAi(mF)	{ mVU_FMAC7 (SUB,  "SUBAi");  }
microVUf(void) mVU_SUBAq(mF)	{ mVU_FMAC23(SUB,  "SUBAq");  }
microVUf(void) mVU_SUBAx(mF)	{ mVU_FMAC5 (SUB,  "SUBAx");  }
microVUf(void) mVU_SUBAy(mF)	{ mVU_FMAC5 (SUB,  "SUBAy");  }
microVUf(void) mVU_SUBAz(mF)	{ mVU_FMAC5 (SUB,  "SUBAz");  }
microVUf(void) mVU_SUBAw(mF)	{ mVU_FMAC5 (SUB,  "SUBAw");  }
microVUf(void) mVU_MUL(mF)		{ mVU_FMAC1 (MUL,  "MUL");    }
microVUf(void) mVU_MULi(mF)		{ mVU_FMAC6 (MUL,  "MULi");   }
microVUf(void) mVU_MULq(mF)		{ mVU_FMAC22(MUL,  "MULq");   }
microVUf(void) mVU_MULx(mF)		{ mVU_FMAC3 (MUL,  "MULx");   }
microVUf(void) mVU_MULy(mF)		{ mVU_FMAC3 (MUL,  "MULy");   }
microVUf(void) mVU_MULz(mF)		{ mVU_FMAC3 (MUL,  "MULz");   }
microVUf(void) mVU_MULw(mF)		{ mVU_FMAC3 (MUL,  "MULw");   }
microVUf(void) mVU_MULA(mF)		{ mVU_FMAC4 (MUL,  "MULA");   }
microVUf(void) mVU_MULAi(mF)	{ mVU_FMAC7 (MUL,  "MULAi");  }
microVUf(void) mVU_MULAq(mF)	{ mVU_FMAC23(MUL,  "MULAq");  }
microVUf(void) mVU_MULAx(mF)	{ mVU_FMAC5 (MUL,  "MULAx");  }
microVUf(void) mVU_MULAy(mF)	{ mVU_FMAC5 (MUL,  "MULAy");  }
microVUf(void) mVU_MULAz(mF)	{ mVU_FMAC5 (MUL,  "MULAz");  }
microVUf(void) mVU_MULAw(mF)	{ mVU_FMAC5 (MUL,  "MULAw");  }
microVUf(void) mVU_MADD(mF)		{ mVU_FMAC8 (ADD,  "MADD");   }
microVUf(void) mVU_MADDi(mF)	{ mVU_FMAC12(ADD,  "MADDi");  }
microVUf(void) mVU_MADDq(mF)	{ mVU_FMAC24(ADD,  "MADDq");  }
microVUf(void) mVU_MADDx(mF)	{ mVU_FMAC10(ADD,  "MADDx");  }
microVUf(void) mVU_MADDy(mF)	{ mVU_FMAC10(ADD,  "MADDy");  }
microVUf(void) mVU_MADDz(mF)	{ mVU_FMAC10(ADD,  "MADDz");  }
microVUf(void) mVU_MADDw(mF)	{ mVU_FMAC10(ADD,  "MADDw");  }
microVUf(void) mVU_MADDA(mF)	{ mVU_FMAC14(ADD,  "MADDA");  }
microVUf(void) mVU_MADDAi(mF)	{ mVU_FMAC16(ADD,  "MADDAi"); }
microVUf(void) mVU_MADDAq(mF)	{ mVU_FMAC26(ADD,  "MADDAq"); }
microVUf(void) mVU_MADDAx(mF)	{ mVU_FMAC15(ADD,  "MADDAx"); }
microVUf(void) mVU_MADDAy(mF)	{ mVU_FMAC15(ADD,  "MADDAy"); }
microVUf(void) mVU_MADDAz(mF)	{ mVU_FMAC15(ADD,  "MADDAz"); }
microVUf(void) mVU_MADDAw(mF)	{ mVU_FMAC15(ADD,  "MADDAw"); }
microVUf(void) mVU_MSUB(mF)		{ mVU_FMAC9 (SUB,  "MSUB");   }
microVUf(void) mVU_MSUBi(mF)	{ mVU_FMAC13(SUB,  "MSUBi");  }
microVUf(void) mVU_MSUBq(mF)	{ mVU_FMAC25(SUB,  "MSUBq");  }
microVUf(void) mVU_MSUBx(mF)	{ mVU_FMAC11(SUB,  "MSUBx");  }
microVUf(void) mVU_MSUBy(mF)	{ mVU_FMAC11(SUB,  "MSUBy");  }
microVUf(void) mVU_MSUBz(mF)	{ mVU_FMAC11(SUB,  "MSUBz");  }
microVUf(void) mVU_MSUBw(mF)	{ mVU_FMAC11(SUB,  "MSUBw");  }
microVUf(void) mVU_MSUBA(mF)	{ mVU_FMAC14(SUB,  "MSUBA");  }
microVUf(void) mVU_MSUBAi(mF)	{ mVU_FMAC16(SUB,  "MSUBAi"); }
microVUf(void) mVU_MSUBAq(mF)	{ mVU_FMAC26(SUB,  "MSUBAq"); }
microVUf(void) mVU_MSUBAx(mF)	{ mVU_FMAC15(SUB,  "MSUBAx"); }
microVUf(void) mVU_MSUBAy(mF)	{ mVU_FMAC15(SUB,  "MSUBAy"); }
microVUf(void) mVU_MSUBAz(mF)	{ mVU_FMAC15(SUB,  "MSUBAz"); }
microVUf(void) mVU_MSUBAw(mF)	{ mVU_FMAC15(SUB,  "MSUBAw"); }
microVUf(void) mVU_MAX(mF)		{ mVU_FMAC27(MAX2, "MAX");    }
microVUf(void) mVU_MAXi(mF)		{ mVU_FMAC28(MAX2, "MAXi");   }
microVUf(void) mVU_MAXx(mF)		{ mVU_FMAC29(MAX2, "MAXx");   }
microVUf(void) mVU_MAXy(mF)		{ mVU_FMAC29(MAX2, "MAXy");   }
microVUf(void) mVU_MAXz(mF)		{ mVU_FMAC29(MAX2, "MAXz");   }
microVUf(void) mVU_MAXw(mF)		{ mVU_FMAC29(MAX2, "MAXw");   }
microVUf(void) mVU_MINI(mF)		{ mVU_FMAC27(MIN2, "MINI");   }
microVUf(void) mVU_MINIi(mF)	{ mVU_FMAC28(MIN2, "MINIi");  }
microVUf(void) mVU_MINIx(mF)	{ mVU_FMAC29(MIN2, "MINIx");  }
microVUf(void) mVU_MINIy(mF)	{ mVU_FMAC29(MIN2, "MINIy");  }
microVUf(void) mVU_MINIz(mF)	{ mVU_FMAC29(MIN2, "MINIz");  }
microVUf(void) mVU_MINIw(mF)	{ mVU_FMAC29(MIN2, "MINIw");  }
microVUf(void) mVU_OPMULA(mF)	{ mVU_FMAC18(MUL,  "OPMULA"); }
microVUf(void) mVU_OPMSUB(mF)	{ mVU_FMAC19(SUB,  "OPMSUB"); }
microVUf(void) mVU_NOP(mF)		{ pass3 { mVUlog("NOP"); }    }
microVUq(void) mVU_FTOIx(uptr addr, int recPass) {
	microVU* mVU = mVUx;
	pass1 { mVUanalyzeFMAC2<vuIndex>(_Fs_, _Ft_); }
	pass2 { 
		int Fs, Ft;
		mVUallocFMAC2a<vuIndex>(Fs, Ft);

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

		mVUallocFMAC2b<vuIndex>(Ft);
	}
}
microVUf(void) mVU_FTOI0(mF)  { mVU_FTOIx<vuIndex>((uptr)0,			  recPass); pass3 { microVU* mVU = mVUx; mVUlog("FTOI0");  mVUlogFtFs(); } }
microVUf(void) mVU_FTOI4(mF)  { mVU_FTOIx<vuIndex>((uptr)mVU_FTOI_4,  recPass); pass3 { microVU* mVU = mVUx; mVUlog("FTOI4");  mVUlogFtFs(); } }
microVUf(void) mVU_FTOI12(mF) { mVU_FTOIx<vuIndex>((uptr)mVU_FTOI_12, recPass); pass3 { microVU* mVU = mVUx; mVUlog("FTOI12"); mVUlogFtFs(); } }
microVUf(void) mVU_FTOI15(mF) { mVU_FTOIx<vuIndex>((uptr)mVU_FTOI_15, recPass); pass3 { microVU* mVU = mVUx; mVUlog("FTOI15"); mVUlogFtFs(); } }
microVUq(void) mVU_ITOFx(uptr addr, int recPass) {
	microVU* mVU = mVUx;
	pass1 { mVUanalyzeFMAC2<vuIndex>(_Fs_, _Ft_); }
	pass2 { 
		int Fs, Ft;
		mVUallocFMAC2a<vuIndex>(Fs, Ft);

		SSE2_CVTDQ2PS_XMM_to_XMM(Ft, Fs);
		if (addr) { SSE_MULPS_M128_to_XMM(Ft, addr); }
		//mVUclamp2(Ft, xmmT1, 15); // Clamp (not sure if this is needed)
		
		mVUallocFMAC2b<vuIndex>(Ft);
	}
}
microVUf(void) mVU_ITOF0(mF)  { mVU_ITOFx<vuIndex>((uptr)0,			  recPass); pass3 { microVU* mVU = mVUx; mVUlog("ITOF0");  mVUlogFtFs(); } }
microVUf(void) mVU_ITOF4(mF)  { mVU_ITOFx<vuIndex>((uptr)mVU_ITOF_4,  recPass);	pass3 { microVU* mVU = mVUx; mVUlog("ITOF4");  mVUlogFtFs(); } }
microVUf(void) mVU_ITOF12(mF) { mVU_ITOFx<vuIndex>((uptr)mVU_ITOF_12, recPass);	pass3 { microVU* mVU = mVUx; mVUlog("ITOF12"); mVUlogFtFs(); } }
microVUf(void) mVU_ITOF15(mF) { mVU_ITOFx<vuIndex>((uptr)mVU_ITOF_15, recPass);	pass3 { microVU* mVU = mVUx; mVUlog("ITOF15"); mVUlogFtFs(); } }
microVUf(void) mVU_CLIP(mF) {
	microVU* mVU = mVUx;
	pass1 { mVUanalyzeFMAC4<vuIndex>(_Fs_, _Ft_); }
	pass2 {
		int Fs, Ft;
		mVUallocFMAC17a<vuIndex>(Fs, Ft);
		mVUallocCFLAGa<vuIndex>(gprT1, fpcInstance);
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

		mVUallocCFLAGb<vuIndex>(gprT1, fcInstance);
	}
	pass3 { mVUlog("CLIP"); mVUlogCLIP(); }
}

