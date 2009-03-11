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
// mVUupdateFlags() - Updates status/mac flags
//------------------------------------------------------------------

#define AND_XYZW ((_XYZW_SS && modXYZW) ? (1) : (doMac ? (_X_Y_Z_W) : (flipMask[_X_Y_Z_W])))

// Note: If modXYZW is true, then it adjusts XYZW for Single Scalar operations
microVUt(void) mVUupdateFlags(int reg, int regT1, int regT2, int xyzw, bool modXYZW) {
	microVU* mVU = mVUx;
	int sReg, mReg = gprT1;
	static u8 *pjmp, *pjmp2;
	static const int flipMask[16] = {0, 8, 4, 12, 2, 10, 6, 14, 1, 9, 5, 13, 3, 11, 7, 15};

	//SysPrintf ("mVUupdateFlags\n");
	if( !(doFlags) ) return;

	if (!doMac) { regT1 = reg; }
	else SSE2_PSHUFD_XMM_to_XMM(regT1, reg, 0x1B); // Flip wzyx to xyzw
	if (doStatus) {
		getFlagReg(sReg, fsInstance); // Set sReg to valid GPR by Cur Flag Instance
		mVUallocSFLAGa<vuIndex>(sReg, fpsInstance); // Get Prev Status Flag
		AND16ItoR(sReg, 0xff0); // Keep Sticky and D/I flags
	}

	//-------------------------Check for Signed flags------------------------------

	// The following code makes sure the Signed Bit isn't set with Negative Zero
	SSE_XORPS_XMM_to_XMM(regT2, regT2); // Clear regT2
	SSE_CMPEQPS_XMM_to_XMM(regT2, regT1); // Set all F's if each vector is zero
	SSE_MOVMSKPS_XMM_to_R32(gprT2, regT2); // Used for Zero Flag Calculation
	SSE_ANDNPS_XMM_to_XMM(regT2, regT1);

	SSE_MOVMSKPS_XMM_to_R32(mReg, regT2); // Move the sign bits of the t1reg

	AND16ItoR(mReg, AND_XYZW );  // Grab "Is Signed" bits from the previous calculation
	pjmp = JZ8(0); // Skip if none are
		if (doMac)	  SHL16ItoR(mReg, 4);
		if (doStatus) OR16ItoR(sReg, 0x82); // SS, S flags
		if (_XYZW_SS) pjmp2 = JMP8(0); // If negative and not Zero, we can skip the Zero Flag checking
	x86SetJ8(pjmp);

	//-------------------------Check for Zero flags------------------------------

	AND16ItoR(gprT2, AND_XYZW );  // Grab "Is Zero" bits from the previous calculation
	pjmp = JZ8(0); // Skip if none are
		if (doMac)	  OR32RtoR(mReg, gprT2);	
		if (doStatus) OR16ItoR(sReg, 0x41); // ZS, Z flags		
	x86SetJ8(pjmp);

	//-------------------------Finally: Send the Flags to the Mac Flag Address------------------------------

	if (_XYZW_SS) x86SetJ8(pjmp2); // If we skipped the Zero Flag Checking, return here

	if (doMac) mVUallocMFLAGb<vuIndex>(mReg, fmInstance); // Set Mac Flag
}

//------------------------------------------------------------------
// Helper Macros
//------------------------------------------------------------------

// FMAC1 - Normal FMAC Opcodes
#define mVU_FMAC1(operation) {										\
	microVU* mVU = mVUx;											\
	if (recPass == 0) {}											\
	else {															\
		int Fd, Fs, Ft;												\
		mVUallocFMAC1a<vuIndex>(Fd, Fs, Ft);						\
		if (_XYZW_SS) SSE_##operation##SS_XMM_to_XMM(Fs, Ft);		\
		else		  SSE_##operation##PS_XMM_to_XMM(Fs, Ft);		\
		mVUupdateFlags<vuIndex>(Fd, xmmT1, Ft, _X_Y_Z_W, 1);		\
		mVUallocFMAC1b<vuIndex>(Fd);								\
	}																\
}
// FMAC3 - BC(xyzw) FMAC Opcodes
#define mVU_FMAC3(operation) {										\
	microVU* mVU = mVUx;											\
	if (recPass == 0) {}											\
	else {															\
		int Fd, Fs, Ft;												\
		mVUallocFMAC3a<vuIndex>(Fd, Fs, Ft);						\
		if (_XYZW_SS) SSE_##operation##SS_XMM_to_XMM(Fs, Ft);		\
		else		  SSE_##operation##PS_XMM_to_XMM(Fs, Ft);		\
		mVUupdateFlags<vuIndex>(Fd, xmmT1, Ft, _X_Y_Z_W, 1);		\
		mVUallocFMAC3b<vuIndex>(Fd);								\
	}																\
}
// FMAC4 - FMAC Opcodes Storing Result to ACC
#define mVU_FMAC4(operation) {										\
	microVU* mVU = mVUx;											\
	if (recPass == 0) {}											\
	else {															\
		int ACC, Fs, Ft;											\
		mVUallocFMAC4a<vuIndex>(ACC, Fs, Ft);						\
		if (_XYZW_SS && _X) SSE_##operation##SS_XMM_to_XMM(Fs, Ft);	\
		else				SSE_##operation##PS_XMM_to_XMM(Fs, Ft);	\
		mVUupdateFlags<vuIndex>(Fs, xmmT1, Ft, _X_Y_Z_W, 0);		\
		mVUallocFMAC4b<vuIndex>(ACC, Fs);							\
	}																\
}
// FMAC5 - FMAC BC(xyzw) Opcodes Storing Result to ACC
#define mVU_FMAC5(operation) {										\
	microVU* mVU = mVUx;											\
	if (recPass == 0) {}											\
	else {															\
		int ACC, Fs, Ft;											\
		mVUallocFMAC5a<vuIndex>(ACC, Fs, Ft);						\
		if (_XYZW_SS && _X) SSE_##operation##SS_XMM_to_XMM(Fs, Ft);	\
		else				SSE_##operation##PS_XMM_to_XMM(Fs, Ft);	\
		mVUupdateFlags<vuIndex>(Fs, xmmT1, Ft, _X_Y_Z_W, 0);		\
		mVUallocFMAC5b<vuIndex>(ACC, Fs);							\
	}																\
}
// FMAC6 - Normal FMAC Opcodes (I Reg)
#define mVU_FMAC6(operation) {										\
	microVU* mVU = mVUx;											\
	if (recPass == 0) {}											\
	else {															\
		int Fd, Fs, Ft;												\
		mVUallocFMAC6a<vuIndex>(Fd, Fs, Ft);						\
		if (_XYZW_SS) SSE_##operation##SS_XMM_to_XMM(Fs, Ft);		\
		else		  SSE_##operation##PS_XMM_to_XMM(Fs, Ft);		\
		mVUupdateFlags<vuIndex>(Fd, xmmT1, Ft, _X_Y_Z_W, 1);		\
		mVUallocFMAC6b<vuIndex>(Fd);								\
	}																\
}
// FMAC7 - FMAC Opcodes Storing Result to ACC (I Reg)
#define mVU_FMAC7(operation) {										\
	microVU* mVU = mVUx;											\
	if (recPass == 0) {}											\
	else {															\
		int ACC, Fs, Ft;											\
		mVUallocFMAC7a<vuIndex>(ACC, Fs, Ft);						\
		if (_XYZW_SS && _X) SSE_##operation##SS_XMM_to_XMM(Fs, Ft);	\
		else				SSE_##operation##PS_XMM_to_XMM(Fs, Ft);	\
		mVUupdateFlags<vuIndex>(Fs, xmmT1, Ft, _X_Y_Z_W, 0);		\
		mVUallocFMAC7b<vuIndex>(ACC, Fs);							\
	}																\
}
// FMAC8 - MADD FMAC Opcode Storing Result to Fd
#define mVU_FMAC8(operation) {										\
	microVU* mVU = mVUx;											\
	if (recPass == 0) {}											\
	else {															\
		int Fd, ACC, Fs, Ft;										\
		mVUallocFMAC8a<vuIndex>(Fd, ACC, Fs, Ft);					\
		if (_XYZW_SS && _X) {										\
			SSE_MULSS_XMM_to_XMM(Fs, Ft);							\
			SSE_##operation##SS_XMM_to_XMM(Fs, ACC);				\
		}															\
		else {														\
			SSE_MULPS_XMM_to_XMM(Fs, Ft);							\
			SSE_##operation##PS_XMM_to_XMM(Fs, ACC);				\
		}															\
		mVUupdateFlags<vuIndex>(Fd, xmmT1, Ft, _X_Y_Z_W, 0);		\
		mVUallocFMAC8b<vuIndex>(Fd);								\
	}																\
}
// FMAC9 - MSUB FMAC Opcode Storing Result to Fd
#define mVU_FMAC9(operation) {										\
	microVU* mVU = mVUx;											\
	if (recPass == 0) {}											\
	else {															\
		int Fd, ACC, Fs, Ft;										\
		mVUallocFMAC9a<vuIndex>(Fd, ACC, Fs, Ft);					\
		if (_XYZW_SS && _X) {										\
			SSE_MULSS_XMM_to_XMM(Fs, Ft);							\
			SSE_##operation##SS_XMM_to_XMM(ACC, Fs);				\
		}															\
		else {														\
			SSE_MULPS_XMM_to_XMM(Fs, Ft);							\
			SSE_##operation##PS_XMM_to_XMM(ACC, Fs);				\
		}															\
		mVUupdateFlags<vuIndex>(Fd, Fs, Ft, _X_Y_Z_W, 0);			\
		mVUallocFMAC9b<vuIndex>(Fd);								\
	}																\
}
// FMAC10 - MADD FMAC BC(xyzw) Opcode Storing Result to Fd
#define mVU_FMAC10(operation) {										\
	microVU* mVU = mVUx;											\
	if (recPass == 0) {}											\
	else {															\
		int Fd, ACC, Fs, Ft;										\
		mVUallocFMAC10a<vuIndex>(Fd, ACC, Fs, Ft);					\
		if (_XYZW_SS && _X) {										\
			SSE_MULSS_XMM_to_XMM(Fs, Ft);							\
			SSE_##operation##SS_XMM_to_XMM(Fs, ACC);				\
		}															\
			else {													\
			SSE_MULPS_XMM_to_XMM(Fs, Ft);							\
			SSE_##operation##PS_XMM_to_XMM(Fs, ACC);				\
		}															\
		mVUupdateFlags<vuIndex>(Fd, xmmT1, Ft, _X_Y_Z_W, 0);		\
		mVUallocFMAC10b<vuIndex>(Fd);								\
	}																\
}
// FMAC11 - MSUB FMAC BC(xyzw) Opcode Storing Result to Fd
#define mVU_FMAC11(operation) {										\
	microVU* mVU = mVUx;											\
	if (recPass == 0) {}											\
	else {															\
		int Fd, ACC, Fs, Ft;										\
		mVUallocFMAC11a<vuIndex>(Fd, ACC, Fs, Ft);					\
		if (_XYZW_SS && _X) {										\
			SSE_MULSS_XMM_to_XMM(Fs, Ft);							\
			SSE_##operation##SS_XMM_to_XMM(ACC, Fs);				\
		}															\
		else {														\
			SSE_MULPS_XMM_to_XMM(Fs, Ft);							\
			SSE_##operation##PS_XMM_to_XMM(ACC, Fs);				\
		}															\
		mVUupdateFlags<vuIndex>(Fd, Fs, Ft, _X_Y_Z_W, 0);			\
		mVUallocFMAC11b<vuIndex>(Fd);								\
	}																\
}
// FMAC12 - MADD FMAC Opcode Storing Result to Fd (I Reg)
#define mVU_FMAC12(operation) {										\
	microVU* mVU = mVUx;											\
	if (recPass == 0) {}											\
	else {															\
		int Fd, ACC, Fs, Ft;										\
		mVUallocFMAC12a<vuIndex>(Fd, ACC, Fs, Ft);					\
		if (_XYZW_SS && _X) {										\
			SSE_MULSS_XMM_to_XMM(Fs, Ft);							\
			SSE_##operation##SS_XMM_to_XMM(Fs, ACC);				\
		}															\
		else {														\
			SSE_MULPS_XMM_to_XMM(Fs, Ft);							\
			SSE_##operation##PS_XMM_to_XMM(Fs, ACC);				\
		}															\
		mVUupdateFlags<vuIndex>(Fd, xmmT1, Ft, _X_Y_Z_W, 0);		\
		mVUallocFMAC12b<vuIndex>(Fd);								\
	}																\
}
// FMAC13 - MSUB FMAC Opcode Storing Result to Fd (I Reg)
#define mVU_FMAC13(operation) {										\
	microVU* mVU = mVUx;											\
	if (recPass == 0) {}											\
	else {															\
		int Fd, ACC, Fs, Ft;										\
		mVUallocFMAC13a<vuIndex>(Fd, ACC, Fs, Ft);					\
		if (_XYZW_SS && _X) {										\
			SSE_MULSS_XMM_to_XMM(Fs, Ft);							\
			SSE_##operation##SS_XMM_to_XMM(ACC, Fs);				\
		}															\
		else {														\
			SSE_MULPS_XMM_to_XMM(Fs, Ft);							\
			SSE_##operation##PS_XMM_to_XMM(ACC, Fs);				\
		}															\
		mVUupdateFlags<vuIndex>(Fd, Fs, Ft, _X_Y_Z_W, 0);			\
		mVUallocFMAC13b<vuIndex>(Fd);								\
	}																\
}
// FMAC14 - MADDA FMAC Opcode
#define mVU_FMAC14(operation) {										\
	microVU* mVU = mVUx;											\
	if (recPass == 0) {}											\
	else {															\
		int ACCw, ACCr, Fs, Ft;										\
		mVUallocFMAC14a<vuIndex>(ACCw, ACCr, Fs, Ft);				\
		if (_XYZW_SS && _X) {										\
			SSE_MULSS_XMM_to_XMM(Fs, Ft);							\
			SSE_##operation##SS_XMM_to_XMM(Fs, ACCr);				\
		}															\
		else {														\
			SSE_MULPS_XMM_to_XMM(Fs, Ft);							\
			SSE_##operation##PS_XMM_to_XMM(Fs, ACCr);				\
		}															\
		mVUupdateFlags<vuIndex>(Fs, xmmT1, Ft, _X_Y_Z_W, 0);		\
		mVUallocFMAC14b<vuIndex>(ACCw, Fs);							\
	}																\
}
// FMAC15 - MSUBA FMAC Opcode
#define mVU_FMAC15(operation) {										\
	microVU* mVU = mVUx;											\
	if (recPass == 0) {}											\
	else {															\
		int ACCw, ACCr, Fs, Ft;										\
		mVUallocFMAC15a<vuIndex>(ACCw, ACCr, Fs, Ft);				\
		if (_XYZW_SS && _X) {										\
			SSE_MULSS_XMM_to_XMM(Fs, Ft);							\
			SSE_##operation##SS_XMM_to_XMM(ACCr, Fs);				\
		}															\
		else {														\
			SSE_MULPS_XMM_to_XMM(Fs, Ft);							\
			SSE_##operation##PS_XMM_to_XMM(ACCr, Fs);				\
		}															\
		mVUupdateFlags<vuIndex>(ACCr, Fs, Ft, _X_Y_Z_W, 0);			\
		mVUallocFMAC15b<vuIndex>(ACCw, ACCr);						\
	}																\
}
// FMAC16 - MADDA BC(xyzw) FMAC Opcode
#define mVU_FMAC16(operation) {										\
	microVU* mVU = mVUx;											\
	if (recPass == 0) {}											\
	else {															\
		int ACCw, ACCr, Fs, Ft;										\
		mVUallocFMAC16a<vuIndex>(ACCw, ACCr, Fs, Ft);				\
		if (_XYZW_SS && _X) {										\
			SSE_MULSS_XMM_to_XMM(Fs, Ft);							\
			SSE_##operation##SS_XMM_to_XMM(Fs, ACCr);				\
		}															\
		else {														\
			SSE_MULPS_XMM_to_XMM(Fs, Ft);							\
			SSE_##operation##PS_XMM_to_XMM(Fs, ACCr);				\
		}															\
		mVUupdateFlags<vuIndex>(Fs, xmmT1, Ft, _X_Y_Z_W, 0);		\
		mVUallocFMAC16b<vuIndex>(ACCw, Fs);							\
	}																\
}
// FMAC17 - MSUBA BC(xyzw) FMAC Opcode
#define mVU_FMAC17(operation) {										\
	microVU* mVU = mVUx;											\
	if (recPass == 0) {}											\
	else {															\
		int ACCw, ACCr, Fs, Ft;										\
		mVUallocFMAC17a<vuIndex>(ACCw, ACCr, Fs, Ft);				\
		if (_XYZW_SS && _X) {										\
			SSE_MULSS_XMM_to_XMM(Fs, Ft);							\
			SSE_##operation##SS_XMM_to_XMM(ACCr, Fs);				\
		}															\
		else {														\
			SSE_MULPS_XMM_to_XMM(Fs, Ft);							\
			SSE_##operation##PS_XMM_to_XMM(ACCr, Fs);				\
		}															\
		mVUupdateFlags<vuIndex>(ACCr, Fs, Ft, _X_Y_Z_W, 0);			\
		mVUallocFMAC17b<vuIndex>(ACCw, ACCr);						\
	}																\
}
// FMAC18 - OPMULA FMAC Opcode
#define mVU_FMAC18(operation) {										\
	microVU* mVU = mVUx;											\
	if (recPass == 0) {}											\
	else {															\
		int ACC, Fs, Ft;											\
		mVUallocFMAC18a<vuIndex>(ACC, Fs, Ft);						\
		SSE_##operation##PS_XMM_to_XMM(Fs, Ft);						\
		mVUupdateFlags<vuIndex>(Fs, xmmT1, Ft, _X_Y_Z_W, 0);		\
		mVUallocFMAC18b<vuIndex>(ACC, Fs);							\
	}																\
}
// FMAC19 - OPMULA FMAC Opcode
#define mVU_FMAC19(operation) {										\
	microVU* mVU = mVUx;											\
	if (recPass == 0) {}											\
	else {															\
		int Fd, ACC, Fs, Ft;										\
		mVUallocFMAC19a<vuIndex>(Fd, ACC, Fs, Ft);					\
		SSE_MULPS_XMM_to_XMM(Fs, Ft);								\
		SSE_##operation##PS_XMM_to_XMM(ACC, Fs);					\
		mVUupdateFlags<vuIndex>(Fd, Fs, Ft, _X_Y_Z_W, 0);			\
		mVUallocFMAC19b<vuIndex>(Fd);								\
	}																\
}
// FMAC20 - MADDA FMAC Opcode (I Reg)
#define mVU_FMAC20(operation) {										\
	microVU* mVU = mVUx;											\
	if (recPass == 0) {}											\
	else {															\
		int ACCw, ACCr, Fs, Ft;										\
		mVUallocFMAC20a<vuIndex>(ACCw, ACCr, Fs, Ft);				\
		if (_XYZW_SS && _X) {										\
			SSE_MULSS_XMM_to_XMM(Fs, Ft);							\
			SSE_##operation##SS_XMM_to_XMM(Fs, ACCr);				\
		}															\
		else {														\
			SSE_MULPS_XMM_to_XMM(Fs, Ft);							\
			SSE_##operation##PS_XMM_to_XMM(Fs, ACCr);				\
		}															\
		mVUupdateFlags<vuIndex>(Fs, xmmT1, Ft, _X_Y_Z_W, 0);		\
		mVUallocFMAC20b<vuIndex>(ACCw, Fs);							\
	}																\
}
// FMAC21 - MSUBA FMAC Opcode (I Reg)
#define mVU_FMAC21(operation) {										\
	microVU* mVU = mVUx;											\
	if (recPass == 0) {}											\
	else {															\
		int ACCw, ACCr, Fs, Ft;										\
		mVUallocFMAC21a<vuIndex>(ACCw, ACCr, Fs, Ft);				\
		if (_XYZW_SS && _X) {										\
			SSE_MULSS_XMM_to_XMM(Fs, Ft);							\
			SSE_##operation##SS_XMM_to_XMM(ACCr, Fs);				\
		}															\
		else {														\
			SSE_MULPS_XMM_to_XMM(Fs, Ft);							\
			SSE_##operation##PS_XMM_to_XMM(ACCr, Fs);				\
		}															\
		mVUupdateFlags<vuIndex>(ACCr, Fs, Ft, _X_Y_Z_W, 0);			\
		mVUallocFMAC21b<vuIndex>(ACCw, ACCr);						\
	}																\
}
// FMAC22 - Normal FMAC Opcodes (Q Reg)
#define mVU_FMAC22(operation) {										\
	microVU* mVU = mVUx;											\
	if (recPass == 0) {}											\
	else {															\
		int Fd, Fs, Ft;												\
		mVUallocFMAC22a<vuIndex>(Fd, Fs, Ft);						\
		if (_XYZW_SS) SSE_##operation##SS_XMM_to_XMM(Fs, Ft);		\
		else		  SSE_##operation##PS_XMM_to_XMM(Fs, Ft);		\
		mVUupdateFlags<vuIndex>(Fd, xmmT1, Ft, _X_Y_Z_W, 1);		\
		mVUallocFMAC22b<vuIndex>(Fd);								\
	}																\
}
// FMAC23 - FMAC Opcodes Storing Result to ACC (Q Reg)
#define mVU_FMAC23(operation) {										\
	microVU* mVU = mVUx;											\
	if (recPass == 0) {}											\
	else {															\
		int ACC, Fs, Ft;											\
		mVUallocFMAC23a<vuIndex>(ACC, Fs, Ft);						\
		if (_XYZW_SS && _X) SSE_##operation##SS_XMM_to_XMM(Fs, Ft);	\
		else				SSE_##operation##PS_XMM_to_XMM(Fs, Ft);	\
		mVUupdateFlags<vuIndex>(Fs, xmmT1, Ft, _X_Y_Z_W, 0);		\
		mVUallocFMAC23b<vuIndex>(ACC, Fs);							\
	}																\
}
// FMAC24 - MADD FMAC Opcode Storing Result to Fd (Q Reg)
#define mVU_FMAC24(operation) {										\
	microVU* mVU = mVUx;											\
	if (recPass == 0) {}											\
	else {															\
		int Fd, ACC, Fs, Ft;										\
		mVUallocFMAC24a<vuIndex>(Fd, ACC, Fs, Ft);					\
		if (_XYZW_SS && _X) {										\
			SSE_MULSS_XMM_to_XMM(Fs, Ft);							\
			SSE_##operation##SS_XMM_to_XMM(Fs, ACC);				\
		}															\
		else {														\
			SSE_MULPS_XMM_to_XMM(Fs, Ft);							\
			SSE_##operation##PS_XMM_to_XMM(Fs, ACC);				\
		}															\
		mVUupdateFlags<vuIndex>(Fd, xmmT1, Ft, _X_Y_Z_W, 0);		\
		mVUallocFMAC24b<vuIndex>(Fd);								\
	}																\
}
// FMAC25 - MSUB FMAC Opcode Storing Result to Fd (Q Reg)
#define mVU_FMAC25(operation) {										\
	microVU* mVU = mVUx;											\
	if (recPass == 0) {}											\
	else {															\
		int Fd, ACC, Fs, Ft;										\
		mVUallocFMAC25a<vuIndex>(Fd, ACC, Fs, Ft);					\
		if (_XYZW_SS && _X) {										\
			SSE_MULSS_XMM_to_XMM(Fs, Ft);							\
			SSE_##operation##SS_XMM_to_XMM(ACC, Fs);				\
		}															\
		else {														\
			SSE_MULPS_XMM_to_XMM(Fs, Ft);							\
			SSE_##operation##PS_XMM_to_XMM(ACC, Fs);				\
		}															\
		mVUupdateFlags<vuIndex>(Fd, Fs, Ft, _X_Y_Z_W, 0);			\
		mVUallocFMAC25b<vuIndex>(Fd);								\
	}																\
}
// FMAC26 - MADDA FMAC Opcode (Q Reg)
#define mVU_FMAC26(operation) {										\
	microVU* mVU = mVUx;											\
	if (recPass == 0) {}											\
	else {															\
		int ACCw, ACCr, Fs, Ft;										\
		mVUallocFMAC26a<vuIndex>(ACCw, ACCr, Fs, Ft);				\
		if (_XYZW_SS && _X) {										\
			SSE_MULSS_XMM_to_XMM(Fs, Ft);							\
			SSE_##operation##SS_XMM_to_XMM(Fs, ACCr);				\
		}															\
		else {														\
			SSE_MULPS_XMM_to_XMM(Fs, Ft);							\
			SSE_##operation##PS_XMM_to_XMM(Fs, ACCr);				\
		}															\
		mVUupdateFlags<vuIndex>(Fs, xmmT1, Ft, _X_Y_Z_W, 0);		\
		mVUallocFMAC26b<vuIndex>(ACCw, Fs);							\
	}																\
}
// FMAC27 - MSUBA FMAC Opcode (Q Reg)
#define mVU_FMAC27(operation) {										\
	microVU* mVU = mVUx;											\
	if (recPass == 0) {}											\
	else {															\
		int ACCw, ACCr, Fs, Ft;										\
		mVUallocFMAC27a<vuIndex>(ACCw, ACCr, Fs, Ft);				\
		if (_XYZW_SS && _X) {										\
			SSE_MULSS_XMM_to_XMM(Fs, Ft);							\
			SSE_##operation##SS_XMM_to_XMM(ACCr, Fs);				\
		}															\
		else {														\
			SSE_MULPS_XMM_to_XMM(Fs, Ft);							\
			SSE_##operation##PS_XMM_to_XMM(ACCr, Fs);				\
		}															\
		mVUupdateFlags<vuIndex>(ACCr, Fs, Ft, _X_Y_Z_W, 0);			\
		mVUallocFMAC27b<vuIndex>(ACCw, ACCr);						\
	}																\
}

//------------------------------------------------------------------
// Micro VU Micromode Upper instructions
//------------------------------------------------------------------

microVUf(void) mVU_ABS() {
	microVU* mVU = mVUx;
	if (recPass == 0) {}
	else { 
		int Fs, Ft;
		mVUallocFMAC2a<vuIndex>(Fs, Ft);
		SSE_ANDPS_M128_to_XMM(Fs, (uptr)mVU_absclip);
		mVUallocFMAC1b<vuIndex>(Ft);
	}
}
microVUf(void) mVU_ADD()	 { mVU_FMAC1(ADD); }
microVUf(void) mVU_ADDi()	 { mVU_FMAC6(ADD); }
microVUf(void) mVU_ADDq()	 { mVU_FMAC22(ADD); }
microVUf(void) mVU_ADDx()	 { mVU_FMAC3(ADD); }
microVUf(void) mVU_ADDy()	 { mVU_FMAC3(ADD); }
microVUf(void) mVU_ADDz()	 { mVU_FMAC3(ADD); }
microVUf(void) mVU_ADDw()	 { mVU_FMAC3(ADD); }
microVUf(void) mVU_ADDA()	 { mVU_FMAC4(ADD); }
microVUf(void) mVU_ADDAi()	 { mVU_FMAC7(ADD); }
microVUf(void) mVU_ADDAq()	 { mVU_FMAC23(ADD); }
microVUf(void) mVU_ADDAx()	 { mVU_FMAC5(ADD); }
microVUf(void) mVU_ADDAy()	 { mVU_FMAC5(ADD); }
microVUf(void) mVU_ADDAz()	 { mVU_FMAC5(ADD); }
microVUf(void) mVU_ADDAw()	 { mVU_FMAC5(ADD); }
microVUf(void) mVU_SUB()	 { mVU_FMAC1(SUB); }
microVUf(void) mVU_SUBi()	 { mVU_FMAC6(SUB); }
microVUf(void) mVU_SUBq()	 { mVU_FMAC22(SUB); }
microVUf(void) mVU_SUBx()	 { mVU_FMAC3(SUB); }
microVUf(void) mVU_SUBy()	 { mVU_FMAC3(SUB); }
microVUf(void) mVU_SUBz()	 { mVU_FMAC3(SUB); }
microVUf(void) mVU_SUBw()	 { mVU_FMAC3(SUB); }
microVUf(void) mVU_SUBA()	 { mVU_FMAC4(SUB); }
microVUf(void) mVU_SUBAi()	 { mVU_FMAC7(SUB); }
microVUf(void) mVU_SUBAq()	 { mVU_FMAC23(SUB); }
microVUf(void) mVU_SUBAx()	 { mVU_FMAC5(SUB); }
microVUf(void) mVU_SUBAy()	 { mVU_FMAC5(SUB); }
microVUf(void) mVU_SUBAz()	 { mVU_FMAC5(SUB); }
microVUf(void) mVU_SUBAw()	 { mVU_FMAC5(SUB); }
microVUf(void) mVU_MUL()	 { mVU_FMAC1(MUL); }
microVUf(void) mVU_MULi()	 { mVU_FMAC6(MUL); }
microVUf(void) mVU_MULq()	 { mVU_FMAC22(MUL); }
microVUf(void) mVU_MULx()	 { mVU_FMAC3(MUL); }
microVUf(void) mVU_MULy()	 { mVU_FMAC3(MUL); }
microVUf(void) mVU_MULz()	 { mVU_FMAC3(MUL); }
microVUf(void) mVU_MULw()	 { mVU_FMAC3(MUL); }
microVUf(void) mVU_MULA()	 { mVU_FMAC4(MUL); }
microVUf(void) mVU_MULAi()	 { mVU_FMAC7(MUL); }
microVUf(void) mVU_MULAq()	 { mVU_FMAC23(MUL); }
microVUf(void) mVU_MULAx()	 { mVU_FMAC5(MUL); }
microVUf(void) mVU_MULAy()	 { mVU_FMAC5(MUL); }
microVUf(void) mVU_MULAz()	 { mVU_FMAC5(MUL); }
microVUf(void) mVU_MULAw()	 { mVU_FMAC5(MUL); }
microVUf(void) mVU_MADD()	 { mVU_FMAC8(ADD); }
microVUf(void) mVU_MADDi()	 { mVU_FMAC12(ADD); }
microVUf(void) mVU_MADDq()	 { mVU_FMAC24(ADD); }
microVUf(void) mVU_MADDx()	 { mVU_FMAC10(ADD); }
microVUf(void) mVU_MADDy()	 { mVU_FMAC10(ADD); }
microVUf(void) mVU_MADDz()	 { mVU_FMAC10(ADD); }
microVUf(void) mVU_MADDw()	 { mVU_FMAC10(ADD); }
microVUf(void) mVU_MADDA()	 { mVU_FMAC14(ADD); }
microVUf(void) mVU_MADDAi()	 { mVU_FMAC20(ADD); }
microVUf(void) mVU_MADDAq()	 { mVU_FMAC26(ADD); }
microVUf(void) mVU_MADDAx()	 { mVU_FMAC16(ADD); }
microVUf(void) mVU_MADDAy()	 { mVU_FMAC16(ADD); }
microVUf(void) mVU_MADDAz()	 { mVU_FMAC16(ADD); }
microVUf(void) mVU_MADDAw()	 { mVU_FMAC16(ADD); }
microVUf(void) mVU_MSUB()	 { mVU_FMAC9(SUB); }
microVUf(void) mVU_MSUBi()	 { mVU_FMAC13(SUB); }
microVUf(void) mVU_MSUBq()	 { mVU_FMAC25(SUB); }
microVUf(void) mVU_MSUBx()	 { mVU_FMAC11(SUB); }
microVUf(void) mVU_MSUBy()	 { mVU_FMAC11(SUB); }
microVUf(void) mVU_MSUBz()	 { mVU_FMAC11(SUB); }
microVUf(void) mVU_MSUBw()	 { mVU_FMAC11(SUB); }
microVUf(void) mVU_MSUBA()	 { mVU_FMAC14(SUB); }
microVUf(void) mVU_MSUBAi()	 { mVU_FMAC21(SUB); }
microVUf(void) mVU_MSUBAq()	 { mVU_FMAC27(SUB); }
microVUf(void) mVU_MSUBAx()	 { mVU_FMAC17(SUB); }
microVUf(void) mVU_MSUBAy()	 { mVU_FMAC17(SUB); }
microVUf(void) mVU_MSUBAz()	 { mVU_FMAC17(SUB); }
microVUf(void) mVU_MSUBAw()	 { mVU_FMAC17(SUB); }
microVUf(void) mVU_MAX()	 { mVU_FMAC1(MAX); }
microVUf(void) mVU_MAXi()	 { mVU_FMAC6(MAX); }
microVUf(void) mVU_MAXx()	 { mVU_FMAC3(MAX); }
microVUf(void) mVU_MAXy()	 { mVU_FMAC3(MAX); }
microVUf(void) mVU_MAXz()	 { mVU_FMAC3(MAX); }
microVUf(void) mVU_MAXw()	 { mVU_FMAC3(MAX); }
microVUf(void) mVU_MINI()	 { mVU_FMAC1(MIN); }
microVUf(void) mVU_MINIi()	 { mVU_FMAC6(MIN); }
microVUf(void) mVU_MINIx()	 { mVU_FMAC3(MIN); }
microVUf(void) mVU_MINIy()	 { mVU_FMAC3(MIN); }
microVUf(void) mVU_MINIz()	 { mVU_FMAC3(MIN); }
microVUf(void) mVU_MINIw()	 { mVU_FMAC3(MIN); }
microVUf(void) mVU_OPMULA()	 { mVU_FMAC18(MUL); }
microVUf(void) mVU_OPMSUB()	 { mVU_FMAC19(SUB); }
microVUf(void) mVU_NOP() {
	microVU* mVU = mVUx;
	if (recPass == 0) {}
	else {}
}
microVUq(void) mVU_FTOIx(uptr addr) {
	microVU* mVU = mVUx;
	if (recPass == 0) {}
	else { 
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

		mVUallocFMAC1b<vuIndex>(Ft);
	}
}
microVUf(void) mVU_FTOI0()	 { mVU_FTOIx<vuIndex, recPass>(0); }
microVUf(void) mVU_FTOI4()	 { mVU_FTOIx<vuIndex, recPass>((uptr)mVU_FTOI_4); }
microVUf(void) mVU_FTOI12()	 { mVU_FTOIx<vuIndex, recPass>((uptr)mVU_FTOI_12); }
microVUf(void) mVU_FTOI15()	 { mVU_FTOIx<vuIndex, recPass>((uptr)mVU_FTOI_15); }
microVUq(void) mVU_ITOFx(uptr addr) {
	microVU* mVU = mVUx;
	if (recPass == 0) {}
	else { 
		int Fs, Ft;
		mVUallocFMAC2a<vuIndex>(Fs, Ft);

		SSE2_CVTDQ2PS_XMM_to_XMM(Ft, Fs);
		if (addr) { SSE_MULPS_M128_to_XMM(Ft, addr); }
		//mVUclamp2(Ft, xmmT1, 15); // Clamp infinities (not sure if this is needed)
		
		mVUallocFMAC1b<vuIndex>(Ft);
	}
}
microVUf(void) mVU_ITOF0()	 { mVU_ITOFx<vuIndex, recPass>(0); }
microVUf(void) mVU_ITOF4()	 { mVU_ITOFx<vuIndex, recPass>((uptr)mVU_ITOF_4); }
microVUf(void) mVU_ITOF12()	 { mVU_ITOFx<vuIndex, recPass>((uptr)mVU_ITOF_12); }
microVUf(void) mVU_ITOF15()	 { mVU_ITOFx<vuIndex, recPass>((uptr)mVU_ITOF_15); }
microVUf(void) mVU_CLIP(){}
#endif //PCSX2_MICROVU
