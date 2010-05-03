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

#include "PrecompiledHeader.h"

#include "Common.h"
#include "GS.h"
#include "R5900OpcodeTables.h"
#include "iR5900.h"
#include "iMMI.h"
#include "iFPU.h"
#include "iCOP0.h"
#include "VUmicro.h"
#include "VUflags.h"
#include "sVU_Micro.h"
#include "sVU_Debug.h"
#include "sVU_zerorec.h"
//------------------------------------------------------------------
#define MINMAXFIX 1
//------------------------------------------------------------------
// Helper Macros
//------------------------------------------------------------------
#define _Ft_ (( VU->code >> 16) & 0x1F)  // The rt part of the instruction register
#define _Fs_ (( VU->code >> 11) & 0x1F)  // The rd part of the instruction register
#define _Fd_ (( VU->code >>  6) & 0x1F)  // The sa part of the instruction register

#define _X (( VU->code>>24) & 0x1)
#define _Y (( VU->code>>23) & 0x1)
#define _Z (( VU->code>>22) & 0x1)
#define _W (( VU->code>>21) & 0x1)

#define _XYZW_SS (_X+_Y+_Z+_W==1)

#define _Fsf_ (( VU->code >> 21) & 0x03)
#define _Ftf_ (( VU->code >> 23) & 0x03)

#define _Imm11_ 	(s32)(VU->code & 0x400 ? 0xfffffc00 | (VU->code & 0x3ff) : VU->code & 0x3ff)
#define _UImm11_	(s32)(VU->code & 0x7ff)

#define VU_VFx_ADDR(x)  (uptr)&VU->VF[x].UL[0]
#define VU_VFy_ADDR(x)  (uptr)&VU->VF[x].UL[1]
#define VU_VFz_ADDR(x)  (uptr)&VU->VF[x].UL[2]
#define VU_VFw_ADDR(x)  (uptr)&VU->VF[x].UL[3]

#define VU_REGR_ADDR    (uptr)&VU->VI[REG_R]
#define VU_REGQ_ADDR    (uptr)&VU->VI[REG_Q]
#define VU_REGMAC_ADDR  (uptr)&VU->VI[REG_MAC_FLAG]

#define VU_VI_ADDR(x, read) GetVIAddr(VU, x, read, info)

#define VU_ACCx_ADDR    (uptr)&VU->ACC.UL[0]
#define VU_ACCy_ADDR    (uptr)&VU->ACC.UL[1]
#define VU_ACCz_ADDR    (uptr)&VU->ACC.UL[2]
#define VU_ACCw_ADDR    (uptr)&VU->ACC.UL[3]

#define _X_Y_Z_W  ((( VU->code >> 21 ) & 0xF ) )
//------------------------------------------------------------------


//------------------------------------------------------------------
// Global Variables
//------------------------------------------------------------------
static const __aligned16 int SSEmovMask[ 16 ][ 4 ] =
{
	{ 0x00000000, 0x00000000, 0x00000000, 0x00000000 },
	{ 0x00000000, 0x00000000, 0x00000000, 0xFFFFFFFF },
	{ 0x00000000, 0x00000000, 0xFFFFFFFF, 0x00000000 },
	{ 0x00000000, 0x00000000, 0xFFFFFFFF, 0xFFFFFFFF },
	{ 0x00000000, 0xFFFFFFFF, 0x00000000, 0x00000000 },
	{ 0x00000000, 0xFFFFFFFF, 0x00000000, 0xFFFFFFFF },
	{ 0x00000000, 0xFFFFFFFF, 0xFFFFFFFF, 0x00000000 },
	{ 0x00000000, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF },
	{ 0xFFFFFFFF, 0x00000000, 0x00000000, 0x00000000 },
	{ 0xFFFFFFFF, 0x00000000, 0x00000000, 0xFFFFFFFF },
	{ 0xFFFFFFFF, 0x00000000, 0xFFFFFFFF, 0x00000000 },
	{ 0xFFFFFFFF, 0x00000000, 0xFFFFFFFF, 0xFFFFFFFF },
	{ 0xFFFFFFFF, 0xFFFFFFFF, 0x00000000, 0x00000000 },
	{ 0xFFFFFFFF, 0xFFFFFFFF, 0x00000000, 0xFFFFFFFF },
	{ 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0x00000000 },
	{ 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF }
};

static const __aligned16 u32 const_abs_table[16][4] =
{
   { 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff }, //0000
   { 0xffffffff, 0xffffffff, 0xffffffff, 0x7fffffff }, //0001
   { 0xffffffff, 0xffffffff, 0x7fffffff, 0xffffffff }, //0010
   { 0xffffffff, 0xffffffff, 0x7fffffff, 0x7fffffff }, //0011
   { 0xffffffff, 0x7fffffff, 0xffffffff, 0xffffffff }, //0100
   { 0xffffffff, 0x7fffffff, 0xffffffff, 0x7fffffff }, //0101
   { 0xffffffff, 0x7fffffff, 0x7fffffff, 0xffffffff }, //0110
   { 0xffffffff, 0x7fffffff, 0x7fffffff, 0x7fffffff }, //0111
   { 0x7fffffff, 0xffffffff, 0xffffffff, 0xffffffff }, //1000
   { 0x7fffffff, 0xffffffff, 0xffffffff, 0x7fffffff }, //1001
   { 0x7fffffff, 0xffffffff, 0x7fffffff, 0xffffffff }, //1010
   { 0x7fffffff, 0xffffffff, 0x7fffffff, 0x7fffffff }, //1011
   { 0x7fffffff, 0x7fffffff, 0xffffffff, 0xffffffff }, //1100
   { 0x7fffffff, 0x7fffffff, 0xffffffff, 0x7fffffff }, //1101
   { 0x7fffffff, 0x7fffffff, 0x7fffffff, 0xffffffff }, //1110
   { 0x7fffffff, 0x7fffffff, 0x7fffffff, 0x7fffffff }, //1111
};

static const __aligned16 float recMult_float_to_int4[4]		= { 16.0, 16.0, 16.0, 16.0 };
static const __aligned16 float recMult_float_to_int12[4]	= { 4096.0, 4096.0, 4096.0, 4096.0 };
static const __aligned16 float recMult_float_to_int15[4]	= { 32768.0, 32768.0, 32768.0, 32768.0 };

static const __aligned16 float recMult_int_to_float4[4]		= { 0.0625f, 0.0625f, 0.0625f, 0.0625f };
static const __aligned16 float recMult_int_to_float12[4]	= { 0.000244140625, 0.000244140625, 0.000244140625, 0.000244140625 };
static const __aligned16 float recMult_int_to_float15[4]	= { 0.000030517578125, 0.000030517578125, 0.000030517578125, 0.000030517578125 };

static const __aligned16 u32 VU_Underflow_Mask1[4]			= {0x7f800000, 0x7f800000, 0x7f800000, 0x7f800000};
static const __aligned16 u32 VU_Underflow_Mask2[4]			= {0x007fffff, 0x007fffff, 0x007fffff, 0x007fffff};
static const __aligned16 u32 VU_Zero_Mask[4]				= {0x00000000, 0x00000000, 0x00000000, 0x00000000};
static const __aligned16 u32 VU_Zero_Helper_Mask[4]			= {0x7fffffff, 0x7fffffff, 0x7fffffff, 0x7fffffff};
static const __aligned16 u32 VU_Signed_Zero_Mask[4]			= {0x80000000, 0x80000000, 0x80000000, 0x80000000};
static const __aligned16 u32 VU_Pos_Infinity[4]				= {0x7f800000, 0x7f800000, 0x7f800000, 0x7f800000};
static const __aligned16 u32 VU_Neg_Infinity[4]				= {0xff800000, 0xff800000, 0xff800000, 0xff800000};
//------------------------------------------------------------------


//------------------------------------------------------------------
// recUpdateFlags() - Computes the flags for the Upper Opcodes
//
// Note: Computes under/overflow flags if CHECK_VU_EXTRA_FLAGS is 1
//------------------------------------------------------------------
static __aligned16 u64 TEMPXMMData[2];
void recUpdateFlags(VURegs * VU, int reg, int info)
{
	static u8 *pjmp, *pjmp2;
	static u32 *pjmp32;
	static u32 macaddr, stataddr, prevstataddr;
	static int x86macflag, x86statflag, x86temp;
	static int t1reg, t1regBoolean;
	static const int flipMask[16] = {0, 8, 4, 12, 2, 10, 6, 14, 1, 9, 5, 13, 3, 11, 7, 15};

	if( !(info & PROCESS_VU_UPDATEFLAGS) ) {
		if (CHECK_VU_EXTRA_OVERFLOW) {
			if (reg != EEREC_TEMP) vuFloat2(reg, EEREC_TEMP, _X_Y_Z_W);
			else vuFloat_useEAX(info, reg, _X_Y_Z_W);
		}
		return;
	}

	//Console.WriteLn ("recUpdateFlags");

	macaddr = VU_VI_ADDR(REG_MAC_FLAG, 0);
	stataddr = VU_VI_ADDR(REG_STATUS_FLAG, 0); // write address
	prevstataddr = VU_VI_ADDR(REG_STATUS_FLAG, 2); // previous address

	if( stataddr == 0 ) stataddr = prevstataddr;
	if( macaddr == 0 ) {
		Console.WriteLn( "VU ALLOCATION WARNING: Using Mac Flag Previous Address!" );
		macaddr = VU_VI_ADDR(REG_MAC_FLAG, 2);
	}

	x86macflag	= ALLOCTEMPX86(0);
	x86statflag = ALLOCTEMPX86(0);

	if (reg == EEREC_TEMP) {
		t1reg = _vuGetTempXMMreg(info);
		if (t1reg < 0) {
			//Console.WriteLn( "VU ALLOCATION ERROR: Temp reg can't be allocated!!!!" );
			t1reg = (reg == 0) ? 1 : 0; // Make t1reg != reg
			SSE_MOVAPS_XMM_to_M128( (uptr)TEMPXMMData, t1reg ); // Backup data to temp address
			t1regBoolean = 1;
		}
		else t1regBoolean = 0;
	}
	else {
		t1reg = EEREC_TEMP;
		t1regBoolean = 2;
	}

	SSE_SHUFPS_XMM_to_XMM(reg, reg, 0x1B); // Flip wzyx to xyzw
	MOV32MtoR(x86statflag, prevstataddr); // Load the previous status in to x86statflag
	AND16ItoR(x86statflag, 0xff0); // Keep Sticky and D/I flags


	if (CHECK_VU_EXTRA_FLAGS) { // Checks all flags

		x86temp = ALLOCTEMPX86(0);

		//-------------------------Check for Overflow flags------------------------------

		//SSE_XORPS_XMM_to_XMM(t1reg, t1reg); // Clear t1reg
		//SSE_CMPUNORDPS_XMM_to_XMM(t1reg, reg); // If reg == NaN then set Vector to 0xFFFFFFFF

		//SSE_MOVAPS_XMM_to_XMM(t1reg, reg);
		//SSE_MINPS_M128_to_XMM(t1reg, (uptr)g_maxvals);
		//SSE_MAXPS_M128_to_XMM(t1reg, (uptr)g_minvals);
		//SSE_CMPNEPS_XMM_to_XMM(t1reg, reg); // If they're not equal, then overflow has occured

		SSE_MOVAPS_XMM_to_XMM(t1reg, reg);
		SSE_ANDPS_M128_to_XMM(t1reg, (uptr)VU_Zero_Helper_Mask);
		SSE_CMPEQPS_M128_to_XMM(t1reg, (uptr)VU_Pos_Infinity); // If infinity, then overflow has occured (NaN's don't report as overflow)

		SSE_MOVMSKPS_XMM_to_R32(x86macflag, t1reg); // Move the sign bits of the previous calculation

		AND16ItoR(x86macflag, _X_Y_Z_W );  // Grab "Has Overflowed" bits from the previous calculation (also make sure we're only grabbing from the XYZW being modified)
		pjmp = JZ8(0); // Skip if none are
			OR16ItoR(x86statflag, 0x208); // OS, O flags
			SHL16ItoR(x86macflag, 12);
			if (_XYZW_SS) pjmp32 = JMP32(0); // Skip Underflow Check
		x86SetJ8(pjmp);

		//-------------------------Check for Underflow flags------------------------------

		SSE_MOVAPS_XMM_to_XMM(t1reg, reg); // t1reg <- reg

		SSE_ANDPS_M128_to_XMM(t1reg, (uptr)&VU_Underflow_Mask1[ 0 ]);
		SSE_CMPEQPS_M128_to_XMM(t1reg, (uptr)&VU_Zero_Mask[ 0 ]); // If (t1reg == zero exponent) then set Vector to 0xFFFFFFFF

		SSE_ANDPS_XMM_to_XMM(t1reg, reg);
		SSE_ANDPS_M128_to_XMM(t1reg, (uptr)&VU_Underflow_Mask2[ 0 ]);
		SSE_CMPNEPS_M128_to_XMM(t1reg, (uptr)&VU_Zero_Mask[ 0 ]); // If (t1reg != zero mantisa) then set Vector to 0xFFFFFFFF

		SSE_MOVMSKPS_XMM_to_R32(EAX, t1reg); // Move the sign bits of the previous calculation

		AND16ItoR(EAX, _X_Y_Z_W );  // Grab "Has Underflowed" bits from the previous calculation
		pjmp = JZ8(0); // Skip if none are
			OR16ItoR(x86statflag, 0x104); // US, U flags
			SHL16ItoR(EAX, 8);
			OR32RtoR(x86macflag, EAX);
		x86SetJ8(pjmp);

		//-------------------------Optional Code: Denormals Are Zero------------------------------
		if (CHECK_VU_UNDERFLOW) {  // Sets underflow/denormals to zero
			SSE_ANDNPS_XMM_to_XMM(t1reg, reg); // t1reg = !t1reg & reg (t1reg = denormals are positive zero)
			VU_MERGE_REGS_SAFE(t1reg, reg, (15 - flipMask[_X_Y_Z_W])); // Send t1reg the vectors that shouldn't be modified (since reg was flipped, we need a mask to get the unmodified vectors)
			// Now we have Denormals are Positive Zero in t1reg; the next two lines take Signed Zero into account
			SSE_ANDPS_M128_to_XMM(reg, (uptr)&VU_Signed_Zero_Mask[ 0 ]); // Only keep the sign bit for each vector
			SSE_ORPS_XMM_to_XMM(reg, t1reg); // Denormals are Signed Zero, and unmodified vectors stay the same!
		}

		if (_XYZW_SS) x86SetJ32(pjmp32); // If we skipped the Underflow Flag Checking (when we had an Overflow), return here

		vuFloat2(reg, t1reg, flipMask[_X_Y_Z_W]); // Clamp overflowed vectors that were modified (remember reg's vectors have been flipped, so have to use a flipmask)

		//-------------------------Check for Signed flags------------------------------

		SSE_XORPS_XMM_to_XMM(t1reg, t1reg); // Clear t1reg
		SSE_CMPEQPS_XMM_to_XMM(t1reg, reg); // Set all F's if each vector is zero
		SSE_MOVMSKPS_XMM_to_R32(x86temp, t1reg); // Used for Zero Flag Calculation

		SSE_MOVMSKPS_XMM_to_R32(EAX, reg); // Move the sign bits of the t1reg

		AND16ItoR(EAX, _X_Y_Z_W );  // Grab "Is Signed" bits from the previous calculation
		pjmp = JZ8(0); // Skip if none are
			OR16ItoR(x86statflag, 0x82); // SS, S flags
			SHL16ItoR(EAX, 4);
			OR32RtoR(x86macflag, EAX);
			if (_XYZW_SS) pjmp2 = JMP8(0); // If negative and not Zero, we can skip the Zero Flag checking
		x86SetJ8(pjmp);

		//-------------------------Check for Zero flags------------------------------

		AND16ItoR(x86temp, _X_Y_Z_W );  // Grab "Is Zero" bits from the previous calculation
		pjmp = JZ8(0); // Skip if none are
			OR16ItoR(x86statflag, 0x41); // ZS, Z flags
			OR32RtoR(x86macflag, x86temp);
		x86SetJ8(pjmp);

		_freeX86reg(x86temp);
	}
	else { // Only Checks for Sign and Zero Flags

		vuFloat2(reg, t1reg, flipMask[_X_Y_Z_W]); // Clamp overflowed vectors that were modified (remember reg's vectors have been flipped, so have to use a flipmask)

		//-------------------------Check for Signed flags------------------------------

		// The following code makes sure the Signed Bit isn't set with Negative Zero
		SSE_XORPS_XMM_to_XMM(t1reg, t1reg); // Clear t1reg
		SSE_CMPEQPS_XMM_to_XMM(t1reg, reg); // Set all F's if each vector is zero
		SSE_MOVMSKPS_XMM_to_R32(EAX, t1reg); // Used for Zero Flag Calculation
		SSE_ANDNPS_XMM_to_XMM(t1reg, reg);

		SSE_MOVMSKPS_XMM_to_R32(x86macflag, t1reg); // Move the sign bits of the t1reg

		AND16ItoR(x86macflag, _X_Y_Z_W );  // Grab "Is Signed" bits from the previous calculation
		pjmp = JZ8(0); // Skip if none are
			OR16ItoR(x86statflag, 0x82); // SS, S flags
			SHL16ItoR(x86macflag, 4);
			if (_XYZW_SS) pjmp2 = JMP8(0); // If negative and not Zero, we can skip the Zero Flag checking
		x86SetJ8(pjmp);

		//-------------------------Check for Zero flags------------------------------

		AND16ItoR(EAX, _X_Y_Z_W );  // Grab "Is Zero" bits from the previous calculation
		pjmp = JZ8(0); // Skip if none are
			OR16ItoR(x86statflag, 0x41); // ZS, Z flags
			OR32RtoR(x86macflag, EAX);
		x86SetJ8(pjmp);
	}
	//-------------------------Finally: Send the Flags to the Mac Flag Address------------------------------

	if (_XYZW_SS) x86SetJ8(pjmp2); // If we skipped the Zero Flag Checking, return here

	if		(t1regBoolean == 2) SSE_SHUFPS_XMM_to_XMM(reg, reg, 0x1B); // Flip back reg to wzyx (have to do this because reg != EEREC_TEMP)
	else if (t1regBoolean == 1) SSE_MOVAPS_M128_to_XMM( t1reg, (uptr)TEMPXMMData ); // Restore data from temo address
	else	_freeXMMreg(t1reg); // Free temp reg

	MOV16RtoM(macaddr, x86macflag);
	MOV16RtoM(stataddr, x86statflag);

	_freeX86reg(x86macflag);
	_freeX86reg(x86statflag);
}
//------------------------------------------------------------------


//------------------------------------------------------------------
// Custom VU ADD/SUB routines by Nneeve
//
// Note: See FPU_ADD_SUB() for more info on what this is doing.
//------------------------------------------------------------------
static __aligned16 u32 VU_addsuband[2][4];
static __aligned16 u32 VU_addsub_reg[2][4];

static u32 tempECX;

void VU_ADD_SUB(u32 regd, u32 regt, int is_sub, int info)
{
	u8 *localptr[4][8];

	MOV32RtoM((uptr)&tempECX, ECX);

	int temp1 = ECX; //receives regd
	int temp2 = ALLOCTEMPX86(0);

	if (temp2 == ECX)
	{
		temp2 = ALLOCTEMPX86(0);
		_freeX86reg(ECX);
	}

	SSE_MOVAPS_XMM_to_M128((uptr)&VU_addsub_reg[0][0], regd);
	SSE_MOVAPS_XMM_to_M128((uptr)&VU_addsub_reg[1][0], regt);

	SSE2_PCMPEQB_XMM_to_XMM(regd, regd);
	SSE_MOVAPS_XMM_to_M128((uptr)&VU_addsuband[0][0], regd);
	SSE_MOVAPS_XMM_to_M128((uptr)&VU_addsuband[1][0], regd);
	SSE_MOVAPS_M128_to_XMM(regd, (uptr)&VU_addsub_reg[0][0]);

	SSE2_PSLLD_I8_to_XMM(regd, 1);
	SSE2_PSLLD_I8_to_XMM(regt, 1);

	SSE2_PSRLD_I8_to_XMM(regd, 24);
	SSE2_PSRLD_I8_to_XMM(regt, 24);

	SSE2_PSUBD_XMM_to_XMM(regd, regt);

#define PERFORM(i) \
	\
	SSE_PEXTRW_XMM_to_R32(temp1, regd, i*2); \
	MOVSX32R16toR(temp1, temp1); \
	CMP32ItoR(temp1, 25);\
	localptr[i][0] = JGE8(0);\
	CMP32ItoR(temp1, 0);\
	localptr[i][1] = JG8(0);\
	localptr[i][2] = JE8(0);\
	CMP32ItoR(temp1, -25);\
	localptr[i][3] = JLE8(0);\
	\
	NEG32R(temp1); \
	DEC32R(temp1);\
	MOV32ItoR(temp2, 0xffffffff); \
	SHL32CLtoR(temp2); \
	MOV32RtoM((uptr)&VU_addsuband[0][i], temp2);\
	localptr[i][4] = JMP8(0);\
	\
	x86SetJ8(localptr[i][0]);\
	MOV32ItoM((uptr)&VU_addsuband[1][i], 0x80000000);\
	localptr[i][5] = JMP8(0);\
	\
	x86SetJ8(localptr[i][1]);\
	DEC32R(temp1);\
	MOV32ItoR(temp2, 0xffffffff);\
	SHL32CLtoR(temp2); \
	MOV32RtoM((uptr)&VU_addsuband[1][i], temp2);\
	localptr[i][6] = JMP8(0);\
	\
	x86SetJ8(localptr[i][3]);\
	MOV32ItoM((uptr)&VU_addsuband[0][i], 0x80000000);\
	localptr[i][7] = JMP8(0);\
	\
	x86SetJ8(localptr[i][2]);\
	\
	x86SetJ8(localptr[i][4]);\
	x86SetJ8(localptr[i][5]);\
	x86SetJ8(localptr[i][6]);\
	x86SetJ8(localptr[i][7]);

	PERFORM(0);
	PERFORM(1);
	PERFORM(2);
	PERFORM(3);
#undef PERFORM

	SSE_MOVAPS_M128_to_XMM(regd, (uptr)&VU_addsub_reg[0][0]);
	SSE_MOVAPS_M128_to_XMM(regt, (uptr)&VU_addsub_reg[1][0]);

	SSE_ANDPS_M128_to_XMM(regd, (uptr)&VU_addsuband[0][0]);
	SSE_ANDPS_M128_to_XMM(regt, (uptr)&VU_addsuband[1][0]);

	if (is_sub)	SSE_SUBPS_XMM_to_XMM(regd, regt);
	else		SSE_ADDPS_XMM_to_XMM(regd, regt);

	SSE_MOVAPS_M128_to_XMM(regt, (uptr)&VU_addsub_reg[1][0]);

	_freeX86reg(temp2);

	MOV32MtoR(ECX, (uptr)&tempECX);
}

void VU_ADD_SUB_SS(u32 regd, u32 regt, int is_sub, int is_mem, int info)
{
	u8 *localptr[8];
	u32 addrt = regt; //for case is_mem

	MOV32RtoM((uptr)&tempECX, ECX);

	int temp1 = ECX; //receives regd
	int temp2 = ALLOCTEMPX86(0);

	if (temp2 == ECX)
	{
		temp2 = ALLOCTEMPX86(0);
		_freeX86reg(ECX);
	}

	SSE_MOVAPS_XMM_to_M128((uptr)&VU_addsub_reg[0][0], regd);
	if (!is_mem) SSE_MOVAPS_XMM_to_M128((uptr)&VU_addsub_reg[1][0], regt);

	SSE2_MOVD_XMM_to_R(temp1, regd);
	SHR32ItoR(temp1, 23);

	if (is_mem) {
		MOV32MtoR(temp2, addrt);
		MOV32RtoM((uptr)&VU_addsub_reg[1][0], temp2);
		SHR32ItoR(temp2, 23);
	}
	else {
		SSE2_MOVD_XMM_to_R(temp2, regt);
		SHR32ItoR(temp2, 23);
	}

	AND32ItoR(temp1, 0xff);
	AND32ItoR(temp2, 0xff);

	SUB32RtoR(temp1, temp2); //temp1 = exponent difference

	CMP32ItoR(temp1, 25);
	localptr[0] = JGE8(0);
	CMP32ItoR(temp1, 0);
	localptr[1] = JG8(0);
	localptr[2] = JE8(0);
	CMP32ItoR(temp1, -25);
	localptr[3] = JLE8(0);

	NEG32R(temp1);
	DEC32R(temp1);
	MOV32ItoR(temp2, 0xffffffff);
	SHL32CLtoR(temp2);
	SSE2_PCMPEQB_XMM_to_XMM(regd, regd);
	if (is_mem) {
		SSE_PINSRW_R32_to_XMM(regd, temp2, 0);
		SHR32ItoR(temp2, 16);
		SSE_PINSRW_R32_to_XMM(regd, temp2, 1);
	}
	else {
		SSE2_MOVD_R_to_XMM(regt, temp2);
		SSE_MOVSS_XMM_to_XMM(regd, regt);
		SSE2_PCMPEQB_XMM_to_XMM(regt, regt);
	}
	localptr[4] = JMP8(0);

	x86SetJ8(localptr[0]);
	MOV32ItoR(temp2, 0x80000000);
	if (is_mem)
		AND32RtoM((uptr)&VU_addsub_reg[1][0], temp2);
	else {
		SSE2_PCMPEQB_XMM_to_XMM(regt, regt);
		SSE2_MOVD_R_to_XMM(regd, temp2);
		SSE_MOVSS_XMM_to_XMM(regt, regd);
	}
	SSE2_PCMPEQB_XMM_to_XMM(regd, regd);
	localptr[5] = JMP8(0);

	x86SetJ8(localptr[1]);
	DEC32R(temp1);
	MOV32ItoR(temp2, 0xffffffff);
	SHL32CLtoR(temp2);
	if (is_mem)
		AND32RtoM((uptr)&VU_addsub_reg[1][0], temp2);
	else {
		SSE2_PCMPEQB_XMM_to_XMM(regt, regt);
		SSE2_MOVD_R_to_XMM(regd, temp2);
		SSE_MOVSS_XMM_to_XMM(regt, regd);
	}
	SSE2_PCMPEQB_XMM_to_XMM(regd, regd);
	localptr[6] = JMP8(0);

	x86SetJ8(localptr[3]);
	MOV32ItoR(temp2, 0x80000000);
	SSE2_PCMPEQB_XMM_to_XMM(regd, regd);
	if (is_mem) {
		SSE_PINSRW_R32_to_XMM(regd, temp2, 0);
		SHR32ItoR(temp2, 16);
		SSE_PINSRW_R32_to_XMM(regd, temp2, 1);
	}
	else {
		SSE2_MOVD_R_to_XMM(regt, temp2);
		SSE_MOVSS_XMM_to_XMM(regd, regt);
		SSE2_PCMPEQB_XMM_to_XMM(regt, regt);
	}
	localptr[7] = JMP8(0);

	x86SetJ8(localptr[2]);
	x86SetJ8(localptr[4]);
	x86SetJ8(localptr[5]);
	x86SetJ8(localptr[6]);
	x86SetJ8(localptr[7]);

	if (is_mem)
	{
		SSE_ANDPS_M128_to_XMM(regd, (uptr)&VU_addsub_reg[0][0]); //regd contains mask

		if (is_sub)	SSE_SUBSS_M32_to_XMM(regd, (uptr)&VU_addsub_reg[1][0]);
		else		SSE_ADDSS_M32_to_XMM(regd, (uptr)&VU_addsub_reg[1][0]);
	}
	else
	{
		SSE_ANDPS_M128_to_XMM(regd, (uptr)&VU_addsub_reg[0][0]); //regd contains mask
		SSE_ANDPS_M128_to_XMM(regt, (uptr)&VU_addsub_reg[1][0]); //regt contains mask

		if (is_sub)	SSE_SUBSS_XMM_to_XMM(regd, regt);
		else		SSE_ADDSS_XMM_to_XMM(regd, regt);

		SSE_MOVAPS_M128_to_XMM(regt, (uptr)&VU_addsub_reg[1][0]);
	}

	_freeX86reg(temp2);

	MOV32MtoR(ECX, (uptr)&tempECX);
}

void SSE_ADDPS_XMM_to_XMM_custom(int info, int regd, int regt) {
	if (CHECK_VUADDSUBHACK) {
		VU_ADD_SUB(regd, regt, 0, info);
	}
	else SSE_ADDPS_XMM_to_XMM(regd, regt);
}
void SSE_SUBPS_XMM_to_XMM_custom(int info, int regd, int regt) {
	if (CHECK_VUADDSUBHACK) {
		VU_ADD_SUB(regd, regt, 1, info);
	}
	else SSE_SUBPS_XMM_to_XMM(regd, regt);
}
void SSE_ADDSS_XMM_to_XMM_custom(int info, int regd, int regt) {
	if (CHECK_VUADDSUBHACK) {
		VU_ADD_SUB_SS(regd, regt, 0, 0, info);
	}
	else SSE_ADDSS_XMM_to_XMM(regd, regt);
}
void SSE_SUBSS_XMM_to_XMM_custom(int info, int regd, int regt) {
	if (CHECK_VUADDSUBHACK) {
		VU_ADD_SUB_SS(regd, regt, 1, 0, info);
	}
	else SSE_SUBSS_XMM_to_XMM(regd, regt);
}
void SSE_ADDSS_M32_to_XMM_custom(int info, int regd, int regt) {
	if (CHECK_VUADDSUBHACK) {
		VU_ADD_SUB_SS(regd, regt, 0, 1, info);
	}
	else SSE_ADDSS_M32_to_XMM(regd, regt);
}
void SSE_SUBSS_M32_to_XMM_custom(int info, int regd, int regt) {
	if (CHECK_VUADDSUBHACK) {
		VU_ADD_SUB_SS(regd, regt, 1, 1, info);
	}
	else SSE_SUBSS_M32_to_XMM(regd, regt);
}
//------------------------------------------------------------------


//------------------------------------------------------------------
// *VU Upper Instructions!*
//
// Note: * = Checked for errors by cottonvibes
//------------------------------------------------------------------


//------------------------------------------------------------------
// ABS*
//------------------------------------------------------------------
void recVUMI_ABS(VURegs *VU, int info)
{
	//Console.WriteLn("recVUMI_ABS()");
	if ( (_Ft_ == 0) || (_X_Y_Z_W == 0) ) return;

	if ((_X_Y_Z_W == 0x8) || (_X_Y_Z_W == 0xf)) {
		VU_MERGE_REGS(EEREC_T, EEREC_S);
		SSE_ANDPS_M128_to_XMM(EEREC_T, (uptr)&const_abs_table[ _X_Y_Z_W ][ 0 ] );
	}
	else { // Use a temp reg because VU_MERGE_REGS() modifies source reg!
		SSE_MOVAPS_XMM_to_XMM(EEREC_TEMP, EEREC_S);
		SSE_ANDPS_M128_to_XMM(EEREC_TEMP, (uptr)&const_abs_table[ _X_Y_Z_W ][ 0 ] );
		VU_MERGE_REGS(EEREC_T, EEREC_TEMP);
	}
}
//------------------------------------------------------------------


//------------------------------------------------------------------
// ADD*, ADD_iq*, ADD_xyzw*
//------------------------------------------------------------------
static const __aligned16 float s_two[4] = {0,0,0,2};
void recVUMI_ADD(VURegs *VU, int info)
{
	//Console.WriteLn("recVUMI_ADD()");
	if ( _X_Y_Z_W == 0 ) goto flagUpdate; // Don't do anything and just clear flags
	if ( !_Fd_ ) info = (info & ~PROCESS_EE_SET_D(0xf)) | PROCESS_EE_SET_D(EEREC_TEMP);

	if ( _Fs_ == 0 && _Ft_ == 0 ) { // if adding VF00 with VF00, then the result is always 0,0,0,2
		if ( _X_Y_Z_W != 0xf ) {
			SSE_MOVAPS_M128_to_XMM(EEREC_TEMP, (uptr)s_two);
			VU_MERGE_REGS(EEREC_D, EEREC_TEMP);
		}
		else SSE_MOVAPS_M128_to_XMM(EEREC_D, (uptr)s_two);
	}
	else {
		if (CHECK_VU_EXTRA_OVERFLOW) {
			if (_Fs_) vuFloat5_useEAX( EEREC_S, EEREC_TEMP, _X_Y_Z_W );
			if (_Ft_) vuFloat5_useEAX( EEREC_T, EEREC_TEMP, _X_Y_Z_W );
		}
		if( _X_Y_Z_W == 8 ) { // If only adding x, then we can do a Scalar Add
			if (EEREC_D == EEREC_S) SSE_ADDSS_XMM_to_XMM(EEREC_D, EEREC_T);
			else if (EEREC_D == EEREC_T) SSE_ADDSS_XMM_to_XMM(EEREC_D, EEREC_S);
			else {
				SSE_MOVSS_XMM_to_XMM(EEREC_D, EEREC_S);
				SSE_ADDSS_XMM_to_XMM(EEREC_D, EEREC_T);
			}
		}
		else if (_X_Y_Z_W != 0xf) { // If xyzw != 1111, then we have to use a temp reg
			SSE_MOVAPS_XMM_to_XMM(EEREC_TEMP, EEREC_S);
			SSE_ADDPS_XMM_to_XMM(EEREC_TEMP, EEREC_T);
			VU_MERGE_REGS(EEREC_D, EEREC_TEMP);
		}
		else { // All xyzw being modified (xyzw == 1111)
			if (EEREC_D == EEREC_S) SSE_ADDPS_XMM_to_XMM(EEREC_D, EEREC_T);
			else if (EEREC_D == EEREC_T) SSE_ADDPS_XMM_to_XMM(EEREC_D, EEREC_S);
			else {
				SSE_MOVAPS_XMM_to_XMM(EEREC_D, EEREC_S);
				SSE_ADDPS_XMM_to_XMM(EEREC_D, EEREC_T);
			}
		}
	}
flagUpdate:
	recUpdateFlags(VU, EEREC_D, info);
}

void recVUMI_ADD_iq(VURegs *VU, uptr addr, int info)
{
	//Console.WriteLn("recVUMI_ADD_iq()");
	if ( _X_Y_Z_W == 0 ) goto flagUpdate;
	if ( !_Fd_ ) info = (info & ~PROCESS_EE_SET_D(0xf)) | PROCESS_EE_SET_D(EEREC_TEMP);
	if (CHECK_VU_EXTRA_OVERFLOW) {
		vuFloat3(addr);
		if (_Fs_) vuFloat5_useEAX( EEREC_S, EEREC_TEMP, _X_Y_Z_W );
	}

	if ( _XYZW_SS ) {
		if ( EEREC_D == EEREC_TEMP ) {
			_vuFlipRegSS(VU, EEREC_S);
			SSE_MOVSS_XMM_to_XMM(EEREC_D, EEREC_S);
			SSE_ADDSS_M32_to_XMM(EEREC_D, addr);
			_vuFlipRegSS(VU, EEREC_S);
			_vuFlipRegSS(VU, EEREC_D); // have to flip over EEREC_D for computing flags!
		}
		else if ( EEREC_D == EEREC_S ) {
			_vuFlipRegSS(VU, EEREC_D);
			SSE_ADDSS_M32_to_XMM(EEREC_D, addr);
			_vuFlipRegSS(VU, EEREC_D);
		}
		else {
			if ( _X ) {
				SSE_MOVSS_XMM_to_XMM(EEREC_D, EEREC_S);
				SSE_ADDSS_M32_to_XMM_custom(info, EEREC_D, addr);
			}
			else {
				SSE_MOVSS_M32_to_XMM(EEREC_TEMP, addr);
				SSE_SHUFPS_XMM_to_XMM(EEREC_TEMP, EEREC_TEMP, 0x00);
				SSE_ADDPS_XMM_to_XMM_custom(info, EEREC_TEMP, EEREC_S);
				VU_MERGE_REGS(EEREC_D, EEREC_TEMP);
			}
		}
	}
	else {
		if ( (_X_Y_Z_W != 0xf) || (EEREC_D == EEREC_S) || (EEREC_D == EEREC_TEMP) ) {
			SSE_MOVSS_M32_to_XMM(EEREC_TEMP, addr);
			SSE_SHUFPS_XMM_to_XMM(EEREC_TEMP, EEREC_TEMP, 0x00);
		}

		if (_X_Y_Z_W != 0xf) {
			SSE_ADDPS_XMM_to_XMM(EEREC_TEMP, EEREC_S);
			VU_MERGE_REGS(EEREC_D, EEREC_TEMP);
		}
		else {
			if ( EEREC_D == EEREC_TEMP ) SSE_ADDPS_XMM_to_XMM(EEREC_D, EEREC_S);
			else if ( EEREC_D == EEREC_S ) SSE_ADDPS_XMM_to_XMM(EEREC_D, EEREC_TEMP);
			else {
				SSE_MOVSS_M32_to_XMM(EEREC_D, addr);
				SSE_SHUFPS_XMM_to_XMM(EEREC_D, EEREC_D, 0x00);
				SSE_ADDPS_XMM_to_XMM(EEREC_D, EEREC_S);
			}
		}
	}
flagUpdate:
	recUpdateFlags(VU, EEREC_D, info);
}

void recVUMI_ADD_xyzw(VURegs *VU, int xyzw, int info)
{
	//Console.WriteLn("recVUMI_ADD_xyzw()");
	if ( _X_Y_Z_W == 0 ) goto flagUpdate;
	if ( !_Fd_ ) info = (info & ~PROCESS_EE_SET_D(0xf)) | PROCESS_EE_SET_D(EEREC_TEMP);
	if (CHECK_VU_EXTRA_OVERFLOW) {
		if (_Fs_) vuFloat5_useEAX( EEREC_S, EEREC_TEMP, _X_Y_Z_W );
		if (_Ft_) vuFloat5_useEAX( EEREC_T, EEREC_TEMP, ( 1 << (3 - xyzw) ) );
	}

	if ( _Ft_ == 0 && xyzw < 3 ) { // just move since adding zero
		if ( _X_Y_Z_W == 0x8 ) { VU_MERGE_REGS(EEREC_D, EEREC_S); }
		else if ( _X_Y_Z_W != 0xf ) {
			SSE_MOVAPS_XMM_to_XMM(EEREC_TEMP, EEREC_S);
			VU_MERGE_REGS(EEREC_D, EEREC_TEMP);
		}
		else SSE_MOVAPS_XMM_to_XMM(EEREC_D, EEREC_S);
	}
	else if ( _X_Y_Z_W == 8 && (EEREC_D != EEREC_TEMP) ) {
		if ( xyzw == 0 ) {
			if ( EEREC_D == EEREC_T ) SSE_ADDSS_XMM_to_XMM(EEREC_D, EEREC_S);
			else {
				SSE_MOVSS_XMM_to_XMM(EEREC_D, EEREC_S);
				SSE_ADDSS_XMM_to_XMM(EEREC_D, EEREC_T);
			}
		}
		else {
			_unpackVFSS_xyzw(EEREC_TEMP, EEREC_T, xyzw);
			SSE_MOVSS_XMM_to_XMM(EEREC_D, EEREC_S);
			SSE_ADDSS_XMM_to_XMM(EEREC_D, EEREC_TEMP);
		}
	}
	else if( _Fs_ == 0 && !_W ) { // just move
		_unpackVF_xyzw(EEREC_TEMP, EEREC_T, xyzw);
		VU_MERGE_REGS(EEREC_D, EEREC_TEMP);
	}
	else {
		if ( _X_Y_Z_W != 0xf ) {
			_unpackVF_xyzw(EEREC_TEMP, EEREC_T, xyzw);
			SSE_ADDPS_XMM_to_XMM(EEREC_TEMP, EEREC_S);
			VU_MERGE_REGS(EEREC_D, EEREC_TEMP);
		}
		else {
			if( EEREC_D == EEREC_TEMP )	  { _unpackVF_xyzw(EEREC_TEMP, EEREC_T, xyzw); SSE_ADDPS_XMM_to_XMM(EEREC_D, EEREC_S); }
			else if( EEREC_D == EEREC_S ) { _unpackVF_xyzw(EEREC_TEMP, EEREC_T, xyzw); SSE_ADDPS_XMM_to_XMM(EEREC_D, EEREC_TEMP); }
			else { _unpackVF_xyzw(EEREC_D, EEREC_T, xyzw); SSE_ADDPS_XMM_to_XMM(EEREC_D, EEREC_S); }
		}
	}
flagUpdate:
	recUpdateFlags(VU, EEREC_D, info);
}

void recVUMI_ADDi(VURegs *VU, int info) { recVUMI_ADD_iq(VU, VU_VI_ADDR(REG_I, 1), info); }
void recVUMI_ADDq(VURegs *VU, int info) { recVUMI_ADD_iq(VU, VU_REGQ_ADDR, info); }
void recVUMI_ADDx(VURegs *VU, int info) { recVUMI_ADD_xyzw(VU, 0, info); }
void recVUMI_ADDy(VURegs *VU, int info) { recVUMI_ADD_xyzw(VU, 1, info); }
void recVUMI_ADDz(VURegs *VU, int info) { recVUMI_ADD_xyzw(VU, 2, info); }
void recVUMI_ADDw(VURegs *VU, int info) { recVUMI_ADD_xyzw(VU, 3, info); }
//------------------------------------------------------------------


//------------------------------------------------------------------
// ADDA*, ADDA_iq*, ADDA_xyzw*
//------------------------------------------------------------------
void recVUMI_ADDA(VURegs *VU, int info)
{
	//Console.WriteLn("recVUMI_ADDA()");
	if ( _X_Y_Z_W == 0 ) goto flagUpdate;
	if (CHECK_VU_EXTRA_OVERFLOW) {
		if (_Fs_) vuFloat5_useEAX( EEREC_S, EEREC_TEMP, _X_Y_Z_W );
		if (_Ft_) vuFloat5_useEAX( EEREC_T, EEREC_TEMP, _X_Y_Z_W );
	}

	if( _X_Y_Z_W == 8 ) {
		if (EEREC_ACC == EEREC_S) SSE_ADDSS_XMM_to_XMM(EEREC_ACC, EEREC_T);	// Can this case happen? (cottonvibes)
		else if (EEREC_ACC == EEREC_T) SSE_ADDSS_XMM_to_XMM(EEREC_ACC, EEREC_S); // Can this case happen?
		else {
			SSE_MOVSS_XMM_to_XMM(EEREC_ACC, EEREC_S);
			SSE_ADDSS_XMM_to_XMM(EEREC_ACC, EEREC_T);
		}
	}
	else if (_X_Y_Z_W != 0xf) {
		SSE_MOVAPS_XMM_to_XMM(EEREC_TEMP, EEREC_S);
		SSE_ADDPS_XMM_to_XMM(EEREC_TEMP, EEREC_T);

		VU_MERGE_REGS(EEREC_ACC, EEREC_TEMP);
	}
	else {
		if( EEREC_ACC == EEREC_S ) SSE_ADDPS_XMM_to_XMM(EEREC_ACC, EEREC_T); // Can this case happen?
		else if( EEREC_ACC == EEREC_T ) SSE_ADDPS_XMM_to_XMM(EEREC_ACC, EEREC_S); // Can this case happen?
		else {
			SSE_MOVAPS_XMM_to_XMM(EEREC_ACC, EEREC_S);
			SSE_ADDPS_XMM_to_XMM(EEREC_ACC, EEREC_T);
		}
	}
flagUpdate:
	recUpdateFlags(VU, EEREC_ACC, info);
}

void recVUMI_ADDA_iq(VURegs *VU, uptr addr, int info)
{
	//Console.WriteLn("recVUMI_ADDA_iq()");
	if ( _X_Y_Z_W == 0 ) goto flagUpdate;
	if (CHECK_VU_EXTRA_OVERFLOW) {
		vuFloat3(addr);
		if (_Fs_) vuFloat5_useEAX( EEREC_S, EEREC_TEMP, _X_Y_Z_W );
	}

	if( _XYZW_SS ) {
		assert( EEREC_ACC != EEREC_TEMP );
		if( EEREC_ACC == EEREC_S ) {
			_vuFlipRegSS(VU, EEREC_ACC);
			SSE_ADDSS_M32_to_XMM(EEREC_ACC, addr);
			_vuFlipRegSS(VU, EEREC_ACC);
		}
		else {
			if( _X ) {
				SSE_MOVSS_XMM_to_XMM(EEREC_ACC, EEREC_S);
				SSE_ADDSS_M32_to_XMM(EEREC_ACC, addr);
			}
			else {
				SSE_MOVSS_M32_to_XMM(EEREC_TEMP, addr);
				SSE_SHUFPS_XMM_to_XMM(EEREC_TEMP, EEREC_TEMP, 0x00);
				SSE_ADDPS_XMM_to_XMM(EEREC_TEMP, EEREC_S);
				VU_MERGE_REGS(EEREC_ACC, EEREC_TEMP);
			}
		}
	}
	else {
		if( _X_Y_Z_W != 0xf || EEREC_ACC == EEREC_S ) {
			SSE_MOVSS_M32_to_XMM(EEREC_TEMP, addr);
			SSE_SHUFPS_XMM_to_XMM(EEREC_TEMP, EEREC_TEMP, 0x00);
		}

		if (_X_Y_Z_W != 0xf) {
			SSE_ADDPS_XMM_to_XMM(EEREC_TEMP, EEREC_S);
			VU_MERGE_REGS(EEREC_ACC, EEREC_TEMP);
		}
		else {
			if( EEREC_ACC == EEREC_S ) SSE_ADDPS_XMM_to_XMM(EEREC_ACC, EEREC_TEMP);
			else {
				SSE_MOVSS_M32_to_XMM(EEREC_ACC, addr);
				SSE_SHUFPS_XMM_to_XMM(EEREC_ACC, EEREC_ACC, 0x00);
				SSE_ADDPS_XMM_to_XMM(EEREC_ACC, EEREC_S);
			}
		}
	}
flagUpdate:
	recUpdateFlags(VU, EEREC_ACC, info);
}

void recVUMI_ADDA_xyzw(VURegs *VU, int xyzw, int info)
{
	//Console.WriteLn("recVUMI_ADDA_xyzw()");
	if ( _X_Y_Z_W == 0 ) goto flagUpdate;
	if (CHECK_VU_EXTRA_OVERFLOW) {
		if (_Fs_) vuFloat5_useEAX( EEREC_S, EEREC_TEMP, _X_Y_Z_W );
		if (_Ft_) vuFloat5_useEAX( EEREC_T, EEREC_TEMP, ( 1 << (3 - xyzw) ) );
	}

	if( _X_Y_Z_W == 8 ) {
		assert( EEREC_ACC != EEREC_T );
		if( xyzw == 0 ) {
			SSE_MOVSS_XMM_to_XMM(EEREC_ACC, EEREC_S);
			SSE_ADDSS_XMM_to_XMM(EEREC_ACC, EEREC_T);
		}
		else {
			_unpackVFSS_xyzw(EEREC_TEMP, EEREC_T, xyzw);
			if( _Fs_ == 0 ) {
				SSE_MOVSS_XMM_to_XMM(EEREC_ACC, EEREC_TEMP);
			}
			else {
				SSE_MOVSS_XMM_to_XMM(EEREC_ACC, EEREC_S);
				SSE_ADDSS_XMM_to_XMM(EEREC_ACC, EEREC_TEMP);
			}
		}
	}
	else {
		if( _X_Y_Z_W != 0xf || EEREC_ACC == EEREC_S )
			_unpackVF_xyzw(EEREC_TEMP, EEREC_T, xyzw);

		if (_X_Y_Z_W != 0xf) {
			SSE_ADDPS_XMM_to_XMM(EEREC_TEMP, EEREC_S);
			VU_MERGE_REGS(EEREC_ACC, EEREC_TEMP);
		}
		else {
			if( EEREC_ACC == EEREC_S ) SSE_ADDPS_XMM_to_XMM(EEREC_ACC, EEREC_TEMP);
			else {
				_unpackVF_xyzw(EEREC_ACC, EEREC_T, xyzw);
				SSE_ADDPS_XMM_to_XMM(EEREC_ACC, EEREC_S);
			}
		}
	}
flagUpdate:
	recUpdateFlags(VU, EEREC_ACC, info);
}

void recVUMI_ADDAi(VURegs *VU, int info) { recVUMI_ADDA_iq(VU, VU_VI_ADDR(REG_I, 1), info); }
void recVUMI_ADDAq(VURegs *VU, int info) { recVUMI_ADDA_iq(VU, VU_REGQ_ADDR, info); }
void recVUMI_ADDAx(VURegs *VU, int info) { recVUMI_ADDA_xyzw(VU, 0, info); }
void recVUMI_ADDAy(VURegs *VU, int info) { recVUMI_ADDA_xyzw(VU, 1, info); }
void recVUMI_ADDAz(VURegs *VU, int info) { recVUMI_ADDA_xyzw(VU, 2, info); }
void recVUMI_ADDAw(VURegs *VU, int info) { recVUMI_ADDA_xyzw(VU, 3, info); }
//------------------------------------------------------------------


//------------------------------------------------------------------
// SUB*, SUB_iq*, SUB_xyzw*
//------------------------------------------------------------------
void recVUMI_SUB(VURegs *VU, int info)
{
	//Console.WriteLn("recVUMI_SUB()");
	if ( _X_Y_Z_W == 0 ) goto flagUpdate;
	if ( !_Fd_ ) info = (info & ~PROCESS_EE_SET_D(0xf)) | PROCESS_EE_SET_D(EEREC_TEMP);

	if( EEREC_S == EEREC_T ) {
		if (_X_Y_Z_W != 0xf) SSE_ANDPS_M128_to_XMM(EEREC_D, (uptr)&SSEmovMask[15-_X_Y_Z_W][0]);
		else SSE_XORPS_XMM_to_XMM(EEREC_D, EEREC_D);
	}
	else if( _X_Y_Z_W == 8 ) {
		if (CHECK_VU_EXTRA_OVERFLOW) {
			if (_Fs_) vuFloat5_useEAX( EEREC_S, EEREC_TEMP, _X_Y_Z_W );
			if (_Ft_) vuFloat5_useEAX( EEREC_T, EEREC_TEMP, _X_Y_Z_W );
		}
		if (EEREC_D == EEREC_S) {
			if (_Ft_) SSE_SUBSS_XMM_to_XMM(EEREC_D, EEREC_T);
		}
		else if (EEREC_D == EEREC_T) {
			if (_Ft_) {
				SSE_MOVSS_XMM_to_XMM(EEREC_TEMP, EEREC_S);
				SSE_SUBSS_XMM_to_XMM(EEREC_TEMP, EEREC_T);
				SSE_MOVSS_XMM_to_XMM(EEREC_D, EEREC_TEMP);
			}
			else SSE_MOVSS_XMM_to_XMM(EEREC_D, EEREC_S);
		}
		else {
			SSE_MOVSS_XMM_to_XMM(EEREC_D, EEREC_S);
			if (_Ft_) SSE_SUBSS_XMM_to_XMM(EEREC_D, EEREC_T);
		}
	}
	else {
		if (CHECK_VU_EXTRA_OVERFLOW) {
			if (_Fs_) vuFloat5_useEAX( EEREC_S, EEREC_TEMP, _X_Y_Z_W );
			if (_Ft_) vuFloat5_useEAX( EEREC_T, EEREC_TEMP, _X_Y_Z_W );
		}
		if (_X_Y_Z_W != 0xf) {
			SSE_MOVAPS_XMM_to_XMM(EEREC_TEMP, EEREC_S);
			if( ( _Ft_ > 0 ) || _W ) SSE_SUBPS_XMM_to_XMM(EEREC_TEMP, EEREC_T);

			VU_MERGE_REGS(EEREC_D, EEREC_TEMP);
		}
		else {
			if (EEREC_D == EEREC_S) SSE_SUBPS_XMM_to_XMM(EEREC_D, EEREC_T);
			else if (EEREC_D == EEREC_T) {
				SSE_MOVAPS_XMM_to_XMM(EEREC_TEMP, EEREC_S);
				SSE_SUBPS_XMM_to_XMM(EEREC_TEMP, EEREC_T);
				SSE_MOVAPS_XMM_to_XMM(EEREC_D, EEREC_TEMP);
			}
			else {
				SSE_MOVAPS_XMM_to_XMM(EEREC_D, EEREC_S);
				SSE_SUBPS_XMM_to_XMM(EEREC_D, EEREC_T);
			}
		}
	}
flagUpdate:
	recUpdateFlags(VU, EEREC_D, info);
}

void recVUMI_SUB_iq(VURegs *VU, uptr addr, int info)
{
	//Console.WriteLn("recVUMI_SUB_iq()");
	if ( _X_Y_Z_W == 0 ) goto flagUpdate;
	if (CHECK_VU_EXTRA_OVERFLOW) {
		vuFloat3(addr);
		if (_Fs_) vuFloat5_useEAX( EEREC_S, EEREC_TEMP, _X_Y_Z_W );
	}
	if ( !_Fd_ ) info = (info & ~PROCESS_EE_SET_D(0xf)) | PROCESS_EE_SET_D(EEREC_TEMP);

	if( _XYZW_SS ) {
		if( EEREC_D == EEREC_TEMP ) {
			_vuFlipRegSS(VU, EEREC_S);
			SSE_MOVSS_XMM_to_XMM(EEREC_D, EEREC_S);
			SSE_SUBSS_M32_to_XMM(EEREC_D, addr);
			_vuFlipRegSS(VU, EEREC_S);
			_vuFlipRegSS(VU, EEREC_D);
		}
		else if( EEREC_D == EEREC_S ) {
			_vuFlipRegSS(VU, EEREC_D);
			SSE_SUBSS_M32_to_XMM(EEREC_D, addr);
			_vuFlipRegSS(VU, EEREC_D);
		}
		else {
			if( _X ) {
				SSE_MOVSS_XMM_to_XMM(EEREC_D, EEREC_S);
				SSE_SUBSS_M32_to_XMM(EEREC_D, addr);
			}
			else {
				_vuMoveSS(VU, EEREC_TEMP, EEREC_S);
				_vuFlipRegSS(VU, EEREC_D);
				SSE_SUBSS_M32_to_XMM(EEREC_TEMP, addr);
				SSE_MOVSS_XMM_to_XMM(EEREC_D, EEREC_TEMP);
				_vuFlipRegSS(VU, EEREC_D);
			}
		}
	}
	else {
		SSE_MOVSS_M32_to_XMM(EEREC_TEMP, addr);
		SSE_SHUFPS_XMM_to_XMM(EEREC_TEMP, EEREC_TEMP, 0x00);

		if (_X_Y_Z_W != 0xf) {
			int t1reg = _vuGetTempXMMreg(info);

			if( t1reg >= 0 ) {
				SSE_MOVAPS_XMM_to_XMM(t1reg, EEREC_S);
				SSE_SUBPS_XMM_to_XMM(t1reg, EEREC_TEMP);

				VU_MERGE_REGS(EEREC_D, t1reg);
				_freeXMMreg(t1reg);
			}
			else {
				// negate
				SSE_XORPS_M128_to_XMM(EEREC_TEMP, (uptr)&const_clip[4]);
				SSE_ADDPS_XMM_to_XMM(EEREC_TEMP, EEREC_S);
				VU_MERGE_REGS(EEREC_D, EEREC_TEMP);
			}
		}
		else {
			if( EEREC_D == EEREC_TEMP ) {
				SSE_XORPS_M128_to_XMM(EEREC_D, (uptr)&const_clip[4]);
				SSE_ADDPS_XMM_to_XMM(EEREC_D, EEREC_S);
			}
			else {
				SSE_MOVAPS_XMM_to_XMM(EEREC_D, EEREC_S);
				SSE_SUBPS_XMM_to_XMM(EEREC_D, EEREC_TEMP);
			}
		}
	}
flagUpdate:
	recUpdateFlags(VU, EEREC_D, info);
}

void recVUMI_SUB_xyzw(VURegs *VU, int xyzw, int info)
{
	//Console.WriteLn("recVUMI_SUB_xyzw()");
	if ( _X_Y_Z_W == 0 ) goto flagUpdate;
	if ( !_Fd_ ) info = (info & ~PROCESS_EE_SET_D(0xf)) | PROCESS_EE_SET_D(EEREC_TEMP);
	if (CHECK_VU_EXTRA_OVERFLOW) {
		if (_Fs_) vuFloat5_useEAX( EEREC_S, EEREC_TEMP, _X_Y_Z_W );
		if (_Ft_) vuFloat5_useEAX( EEREC_T, EEREC_TEMP, ( 1 << (3 - xyzw) ) );
	}

	if ( _X_Y_Z_W == 8 ) {
		if ( (xyzw == 0) && (_Ft_ == _Fs_) ) {
			SSE_ANDPS_M128_to_XMM(EEREC_D, (uptr)&SSEmovMask[7][0]);
		}
		else if ( EEREC_D == EEREC_TEMP ) {
			SSE_MOVSS_XMM_to_XMM(EEREC_D, EEREC_S);
			if ( (_Ft_ > 0) || (xyzw == 3) ) {
				_vuFlipRegSS_xyzw(EEREC_T, xyzw);
				SSE_SUBSS_XMM_to_XMM(EEREC_D, EEREC_T);
				_vuFlipRegSS_xyzw(EEREC_T, xyzw);
			}
		}
		else {
			if ( (_Ft_ > 0) || (xyzw == 3) ) {
				_unpackVFSS_xyzw(EEREC_TEMP, EEREC_T, xyzw);
				SSE_MOVSS_XMM_to_XMM(EEREC_D, EEREC_S);
				SSE_SUBSS_XMM_to_XMM(EEREC_D, EEREC_TEMP);
			}
			else SSE_MOVSS_XMM_to_XMM(EEREC_D, EEREC_S);
		}
	}
	else {
		_unpackVF_xyzw(EEREC_TEMP, EEREC_T, xyzw);

		if (_X_Y_Z_W != 0xf) {
			int t1reg = _vuGetTempXMMreg(info);

			if( t1reg >= 0 ) {
				SSE_MOVAPS_XMM_to_XMM(t1reg, EEREC_S);
				SSE_SUBPS_XMM_to_XMM(t1reg, EEREC_TEMP);

				VU_MERGE_REGS(EEREC_D, t1reg);
				_freeXMMreg(t1reg);
			}
			else {
				// negate
				SSE_XORPS_M128_to_XMM(EEREC_TEMP, (uptr)&const_clip[4]);
				SSE_ADDPS_XMM_to_XMM(EEREC_TEMP, EEREC_S);
				VU_MERGE_REGS(EEREC_D, EEREC_TEMP);
			}
		}
		else {
			if( EEREC_D == EEREC_TEMP ) {
				SSE_XORPS_M128_to_XMM(EEREC_D, (uptr)&const_clip[4]);
				SSE_ADDPS_XMM_to_XMM(EEREC_D, EEREC_S);
			}
			else {
				SSE_MOVAPS_XMM_to_XMM(EEREC_D, EEREC_S);
				SSE_SUBPS_XMM_to_XMM(EEREC_D, EEREC_TEMP);
			}
		}
	}
flagUpdate:
	recUpdateFlags(VU, EEREC_D, info);
}

void recVUMI_SUBi(VURegs *VU, int info) { recVUMI_SUB_iq(VU, VU_VI_ADDR(REG_I, 1), info); }
void recVUMI_SUBq(VURegs *VU, int info) { recVUMI_SUB_iq(VU, VU_REGQ_ADDR, info); }
void recVUMI_SUBx(VURegs *VU, int info) { recVUMI_SUB_xyzw(VU, 0, info); }
void recVUMI_SUBy(VURegs *VU, int info) { recVUMI_SUB_xyzw(VU, 1, info); }
void recVUMI_SUBz(VURegs *VU, int info) { recVUMI_SUB_xyzw(VU, 2, info); }
void recVUMI_SUBw(VURegs *VU, int info) { recVUMI_SUB_xyzw(VU, 3, info); }
//------------------------------------------------------------------


//------------------------------------------------------------------
// SUBA*, SUBA_iq, SUBA_xyzw
//------------------------------------------------------------------
void recVUMI_SUBA(VURegs *VU, int info)
{
	//Console.WriteLn("recVUMI_SUBA()");
	if ( _X_Y_Z_W == 0 ) goto flagUpdate;
	if (CHECK_VU_EXTRA_OVERFLOW) {
		if (_Fs_) vuFloat5_useEAX( EEREC_S, EEREC_TEMP, _X_Y_Z_W );
		if (_Ft_) vuFloat5_useEAX( EEREC_T, EEREC_TEMP, _X_Y_Z_W );
	}

	if( EEREC_S == EEREC_T ) {
		if (_X_Y_Z_W != 0xf) SSE_ANDPS_M128_to_XMM(EEREC_ACC, (uptr)&SSEmovMask[15-_X_Y_Z_W][0]);
		else SSE_XORPS_XMM_to_XMM(EEREC_ACC, EEREC_ACC);
	}
	else if( _X_Y_Z_W == 8 ) {
		if (EEREC_ACC == EEREC_S) SSE_SUBSS_XMM_to_XMM(EEREC_ACC, EEREC_T);
		else if (EEREC_ACC == EEREC_T) {
			SSE_MOVSS_XMM_to_XMM(EEREC_TEMP, EEREC_S);
			SSE_SUBSS_XMM_to_XMM(EEREC_TEMP, EEREC_T);
			SSE_MOVSS_XMM_to_XMM(EEREC_ACC, EEREC_TEMP);
		}
		else {
			SSE_MOVSS_XMM_to_XMM(EEREC_ACC, EEREC_S);
			SSE_SUBSS_XMM_to_XMM(EEREC_ACC, EEREC_T);
		}
	}
	else if (_X_Y_Z_W != 0xf) {
		SSE_MOVAPS_XMM_to_XMM(EEREC_TEMP, EEREC_S);
		SSE_SUBPS_XMM_to_XMM(EEREC_TEMP, EEREC_T);

		VU_MERGE_REGS(EEREC_ACC, EEREC_TEMP);
	}
	else {
		if( EEREC_ACC == EEREC_S ) SSE_SUBPS_XMM_to_XMM(EEREC_ACC, EEREC_T);
		else if( EEREC_ACC == EEREC_T ) {
			SSE_MOVAPS_XMM_to_XMM(EEREC_TEMP, EEREC_S);
			SSE_SUBPS_XMM_to_XMM(EEREC_TEMP, EEREC_T);
			SSE_MOVAPS_XMM_to_XMM(EEREC_ACC, EEREC_TEMP);
		}
		else {
			SSE_MOVAPS_XMM_to_XMM(EEREC_ACC, EEREC_S);
			SSE_SUBPS_XMM_to_XMM(EEREC_ACC, EEREC_T);
		}
	}
flagUpdate:
	recUpdateFlags(VU, EEREC_ACC, info);
}

void recVUMI_SUBA_iq(VURegs *VU, uptr addr, int info)
{
	//Console.WriteLn ("recVUMI_SUBA_iq");
	if (CHECK_VU_EXTRA_OVERFLOW) {
		vuFloat3(addr);
		if (_Fs_) vuFloat5_useEAX( EEREC_S, EEREC_TEMP, _X_Y_Z_W );
	}

	if( _XYZW_SS ) {
		if( EEREC_ACC == EEREC_S ) {
			_vuFlipRegSS(VU, EEREC_ACC);
			SSE_SUBSS_M32_to_XMM(EEREC_ACC, addr);
			_vuFlipRegSS(VU, EEREC_ACC);
		}
		else {
			if( _X ) {
				SSE_MOVSS_XMM_to_XMM(EEREC_ACC, EEREC_S);
				SSE_SUBSS_M32_to_XMM(EEREC_ACC, addr);
			}
			else {
				_vuMoveSS(VU, EEREC_TEMP, EEREC_S);
				_vuFlipRegSS(VU, EEREC_ACC);
				SSE_SUBSS_M32_to_XMM(EEREC_TEMP, addr);
				SSE_MOVSS_XMM_to_XMM(EEREC_ACC, EEREC_TEMP);
				_vuFlipRegSS(VU, EEREC_ACC);
			}
		}
	}
	else {
		SSE_MOVSS_M32_to_XMM(EEREC_TEMP, addr);
		SSE_SHUFPS_XMM_to_XMM(EEREC_TEMP, EEREC_TEMP, 0x00);

		if (_X_Y_Z_W != 0xf) {
			int t1reg = _vuGetTempXMMreg(info);

			if( t1reg >= 0 ) {
				SSE_MOVAPS_XMM_to_XMM(t1reg, EEREC_S);
				SSE_SUBPS_XMM_to_XMM(t1reg, EEREC_TEMP);

				VU_MERGE_REGS(EEREC_ACC, t1reg);
				_freeXMMreg(t1reg);
			}
			else {
				// negate
				SSE_XORPS_M128_to_XMM(EEREC_TEMP, (uptr)&const_clip[4]);
				SSE_ADDPS_XMM_to_XMM(EEREC_TEMP, EEREC_S);
				VU_MERGE_REGS(EEREC_ACC, EEREC_TEMP);
			}
		}
		else {
			SSE_MOVAPS_XMM_to_XMM(EEREC_ACC, EEREC_S);
			SSE_SUBPS_XMM_to_XMM(EEREC_ACC, EEREC_TEMP);
		}
	}
	recUpdateFlags(VU, EEREC_ACC, info);
}

void recVUMI_SUBA_xyzw(VURegs *VU, int xyzw, int info)
{
	//Console.WriteLn ("recVUMI_SUBA_xyzw");
	if (CHECK_VU_EXTRA_OVERFLOW) {
		if (_Fs_) vuFloat5_useEAX( EEREC_S, EEREC_TEMP, _X_Y_Z_W );
		if (_Ft_) vuFloat5_useEAX( EEREC_T, EEREC_TEMP, ( 1 << (3 - xyzw) ) );
	}

	if( _X_Y_Z_W == 8 ) {
		if( xyzw == 0 ) {
			SSE_MOVSS_XMM_to_XMM(EEREC_ACC, EEREC_S);
			SSE_SUBSS_XMM_to_XMM(EEREC_ACC, EEREC_T);
		}
		else {
			_unpackVFSS_xyzw(EEREC_TEMP, EEREC_T, xyzw);
			SSE_MOVSS_XMM_to_XMM(EEREC_ACC, EEREC_S);
			SSE_SUBSS_XMM_to_XMM(EEREC_ACC, EEREC_TEMP);
		}
	}
	else {
		_unpackVF_xyzw(EEREC_TEMP, EEREC_T, xyzw);

		if (_X_Y_Z_W != 0xf) {
			int t1reg = _vuGetTempXMMreg(info);

			if( t1reg >= 0 ) {
				SSE_MOVAPS_XMM_to_XMM(t1reg, EEREC_S);
				SSE_SUBPS_XMM_to_XMM(t1reg, EEREC_TEMP);

				VU_MERGE_REGS(EEREC_ACC, t1reg);
				_freeXMMreg(t1reg);
			}
			else {
				// negate
				SSE_XORPS_M128_to_XMM(EEREC_TEMP, (uptr)&const_clip[4]);
				SSE_ADDPS_XMM_to_XMM(EEREC_TEMP, EEREC_S);
				VU_MERGE_REGS(EEREC_ACC, EEREC_TEMP);
			}
		}
		else {
			SSE_MOVAPS_XMM_to_XMM(EEREC_ACC, EEREC_S);
			SSE_SUBPS_XMM_to_XMM(EEREC_ACC, EEREC_TEMP);
		}
	}
	recUpdateFlags(VU, EEREC_ACC, info);
}

void recVUMI_SUBAi(VURegs *VU, int info) { recVUMI_SUBA_iq(VU, VU_VI_ADDR(REG_I, 1), info); }
void recVUMI_SUBAq(VURegs *VU, int info) { recVUMI_SUBA_iq(VU, VU_REGQ_ADDR, info); }
void recVUMI_SUBAx(VURegs *VU, int info) { recVUMI_SUBA_xyzw(VU, 0, info); }
void recVUMI_SUBAy(VURegs *VU, int info) { recVUMI_SUBA_xyzw(VU, 1, info); }
void recVUMI_SUBAz(VURegs *VU, int info) { recVUMI_SUBA_xyzw(VU, 2, info); }
void recVUMI_SUBAw(VURegs *VU, int info) { recVUMI_SUBA_xyzw(VU, 3, info); }
//------------------------------------------------------------------


//------------------------------------------------------------------
// MUL
//------------------------------------------------------------------
void recVUMI_MUL_toD(VURegs *VU, int regd, int info)
{
	//Console.WriteLn ("recVUMI_MUL_toD");
	if (CHECK_VU_EXTRA_OVERFLOW) {
		if (_Fs_) vuFloat5_useEAX( EEREC_S, EEREC_TEMP, _X_Y_Z_W );
		if (_Ft_) vuFloat5_useEAX( EEREC_T, EEREC_TEMP, _X_Y_Z_W );
	}

	if (_X_Y_Z_W == 1 && (_Ft_ == 0 || _Fs_==0) ) { // W
		SSE_MOVAPS_XMM_to_XMM(EEREC_TEMP, _Ft_ ? EEREC_T : EEREC_S);
		VU_MERGE_REGS(regd, EEREC_TEMP);
	}
	else if( _Fd_ == _Fs_ && _Fs_ == _Ft_ && _XYZW_SS ) {
		_vuFlipRegSS(VU, EEREC_D);
		SSE_MULSS_XMM_to_XMM(EEREC_D, EEREC_D);
		_vuFlipRegSS(VU, EEREC_D);
	}
	else if( _X_Y_Z_W == 8 ) {
		if (regd == EEREC_S) SSE_MULSS_XMM_to_XMM(regd, EEREC_T);
		else if (regd == EEREC_T) SSE_MULSS_XMM_to_XMM(regd, EEREC_S);
		else {
			SSE_MOVSS_XMM_to_XMM(regd, EEREC_S);
			SSE_MULSS_XMM_to_XMM(regd, EEREC_T);
		}
	}
	else if (_X_Y_Z_W != 0xf) {
		SSE_MOVAPS_XMM_to_XMM(EEREC_TEMP, EEREC_S);
		SSE_MULPS_XMM_to_XMM(EEREC_TEMP, EEREC_T);

		VU_MERGE_REGS(regd, EEREC_TEMP);
	}
	else {
		if (regd == EEREC_S) SSE_MULPS_XMM_to_XMM(regd, EEREC_T);
		else if (regd == EEREC_T) SSE_MULPS_XMM_to_XMM(regd, EEREC_S);
		else {
			SSE_MOVAPS_XMM_to_XMM(regd, EEREC_S);
			SSE_MULPS_XMM_to_XMM(regd, EEREC_T);
		}
	}
}

void recVUMI_MUL_iq_toD(VURegs *VU, uptr addr, int regd, int info)
{
	//Console.WriteLn ("recVUMI_MUL_iq_toD");
	if (CHECK_VU_EXTRA_OVERFLOW) {
		vuFloat3(addr);
		if (_Fs_) vuFloat5_useEAX( EEREC_S, EEREC_TEMP, _X_Y_Z_W );
	}

	if( _XYZW_SS ) {
		if( regd == EEREC_TEMP ) {
			_vuFlipRegSS(VU, EEREC_S);
			SSE_MOVSS_XMM_to_XMM(regd, EEREC_S);
			SSE_MULSS_M32_to_XMM(regd, addr);
			_vuFlipRegSS(VU, EEREC_S);
			_vuFlipRegSS(VU, regd);
		}
		else if( regd == EEREC_S ) {
			_vuFlipRegSS(VU, regd);
			SSE_MULSS_M32_to_XMM(regd, addr);
			_vuFlipRegSS(VU, regd);
		}
		else {
			if( _X ) {
				SSE_MOVSS_XMM_to_XMM(regd, EEREC_S);
				SSE_MULSS_M32_to_XMM(regd, addr);
			}
			else {
				SSE_MOVSS_M32_to_XMM(EEREC_TEMP, addr);
				SSE_SHUFPS_XMM_to_XMM(EEREC_TEMP, EEREC_TEMP, 0x00);
				SSE_MULPS_XMM_to_XMM(EEREC_TEMP, EEREC_S);
				VU_MERGE_REGS(regd, EEREC_TEMP);
			}
		}
	}
	else {
		if( _X_Y_Z_W != 0xf || regd == EEREC_TEMP || regd == EEREC_S ) {
			SSE_MOVSS_M32_to_XMM(EEREC_TEMP, addr);
			SSE_SHUFPS_XMM_to_XMM(EEREC_TEMP, EEREC_TEMP, 0x00);
		}

		if (_X_Y_Z_W != 0xf) {
			SSE_MULPS_XMM_to_XMM(EEREC_TEMP, EEREC_S);
			VU_MERGE_REGS(regd, EEREC_TEMP);
		}
		else {
			if( regd == EEREC_TEMP ) SSE_MULPS_XMM_to_XMM(regd, EEREC_S);
			else if (regd == EEREC_S) SSE_MULPS_XMM_to_XMM(regd, EEREC_TEMP);
			else {
				SSE_MOVSS_M32_to_XMM(regd, addr);
				SSE_SHUFPS_XMM_to_XMM(regd, regd, 0x00);
				SSE_MULPS_XMM_to_XMM(regd, EEREC_S);
			}
		}
	}
}

void recVUMI_MUL_xyzw_toD(VURegs *VU, int xyzw, int regd, int info)
{
	//Console.WriteLn ("recVUMI_MUL_xyzw_toD");
	if (CHECK_VU_EXTRA_OVERFLOW) {
		if (_Ft_) vuFloat5_useEAX( EEREC_T, EEREC_TEMP, ( 1 << (3 - xyzw) ) );
	}
	if (_Fs_) { // This is needed for alot of games; so always clamp this operand
		if (CHECK_VU_SIGN_OVERFLOW) vFloats4_useEAX[_X_Y_Z_W]( EEREC_S, EEREC_TEMP ); // Always clamp EEREC_S, regardless if CHECK_VU_OVERFLOW is set
		else vFloats2[_X_Y_Z_W]( EEREC_S, EEREC_TEMP ); // Always clamp EEREC_S, regardless if CHECK_VU_OVERFLOW is set
	}
	if( _Ft_ == 0 ) {
		if( xyzw < 3 ) {
			if (_X_Y_Z_W != 0xf) {
				SSE_XORPS_XMM_to_XMM(EEREC_TEMP, EEREC_TEMP);
				VU_MERGE_REGS(regd, EEREC_TEMP);
			}
			else SSE_XORPS_XMM_to_XMM(regd, regd);
		}
		else {
			assert(xyzw==3);
			if (_X_Y_Z_W != 0xf) {
				SSE_MOVAPS_XMM_to_XMM(EEREC_TEMP, EEREC_S);
				VU_MERGE_REGS(regd, EEREC_TEMP);
			}
			else SSE_MOVAPS_XMM_to_XMM(regd, EEREC_S);
		}
	}
	else if( _X_Y_Z_W == 8 ) {
		if( regd == EEREC_TEMP ) {
			_unpackVFSS_xyzw(EEREC_TEMP, EEREC_T, xyzw);
			SSE_MULSS_XMM_to_XMM(regd, EEREC_S);
		}
		else {
			if( xyzw == 0 ) {
				if( regd == EEREC_T ) {
					SSE_MULSS_XMM_to_XMM(regd, EEREC_S);
				}
				else {
					SSE_MOVSS_XMM_to_XMM(regd, EEREC_S);
					SSE_MULSS_XMM_to_XMM(regd, EEREC_T);
				}
			}
			else {
				_unpackVFSS_xyzw(EEREC_TEMP, EEREC_T, xyzw);
				SSE_MOVSS_XMM_to_XMM(regd, EEREC_S);
				SSE_MULSS_XMM_to_XMM(regd, EEREC_TEMP);
			}
		}
	}
	else {
		if( _X_Y_Z_W != 0xf || regd == EEREC_TEMP || regd == EEREC_S )
			_unpackVF_xyzw(EEREC_TEMP, EEREC_T, xyzw);

		if (_X_Y_Z_W != 0xf) {
			SSE_MULPS_XMM_to_XMM(EEREC_TEMP, EEREC_S);
			VU_MERGE_REGS(regd, EEREC_TEMP);
		}
		else {
			if( regd == EEREC_TEMP ) SSE_MULPS_XMM_to_XMM(regd, EEREC_S);
			else if (regd == EEREC_S) SSE_MULPS_XMM_to_XMM(regd, EEREC_TEMP);
			else {
				_unpackVF_xyzw(regd, EEREC_T, xyzw);
				SSE_MULPS_XMM_to_XMM(regd, EEREC_S);
			}
		}
	}
}

void recVUMI_MUL(VURegs *VU, int info)
{
	//Console.WriteLn ("recVUMI_MUL");
	if( !_Fd_ ) info |= PROCESS_EE_SET_D(EEREC_TEMP);
	recVUMI_MUL_toD(VU, EEREC_D, info);
	recUpdateFlags(VU, EEREC_D, info);
}

void recVUMI_MUL_iq(VURegs *VU, int addr, int info)
{
	//Console.WriteLn ("recVUMI_MUL_iq");
	if( !_Fd_ ) info |= PROCESS_EE_SET_D(EEREC_TEMP);
	recVUMI_MUL_iq_toD(VU, addr, EEREC_D, info);
	recUpdateFlags(VU, EEREC_D, info);
	// spacefisherman needs overflow checking on MULi.z
}

void recVUMI_MUL_xyzw(VURegs *VU, int xyzw, int info)
{
	//Console.WriteLn ("recVUMI_MUL_xyzw");
	if( !_Fd_ ) info |= PROCESS_EE_SET_D(EEREC_TEMP);
	recVUMI_MUL_xyzw_toD(VU, xyzw, EEREC_D, info);
	recUpdateFlags(VU, EEREC_D, info);
}

void recVUMI_MULi(VURegs *VU, int info) { recVUMI_MUL_iq(VU, VU_VI_ADDR(REG_I, 1), info); }
void recVUMI_MULq(VURegs *VU, int info) { recVUMI_MUL_iq(VU, VU_REGQ_ADDR, info); }
void recVUMI_MULx(VURegs *VU, int info) { recVUMI_MUL_xyzw(VU, 0, info); }
void recVUMI_MULy(VURegs *VU, int info) { recVUMI_MUL_xyzw(VU, 1, info); }
void recVUMI_MULz(VURegs *VU, int info) { recVUMI_MUL_xyzw(VU, 2, info); }
void recVUMI_MULw(VURegs *VU, int info) { recVUMI_MUL_xyzw(VU, 3, info); }
//------------------------------------------------------------------


//------------------------------------------------------------------
// MULA
//------------------------------------------------------------------
void recVUMI_MULA( VURegs *VU, int info )
{
	//Console.WriteLn ("recVUMI_MULA");
	recVUMI_MUL_toD(VU, EEREC_ACC, info);
	recUpdateFlags(VU, EEREC_ACC, info);
}

void recVUMI_MULA_iq(VURegs *VU, int addr, int info)
{
	//Console.WriteLn ("recVUMI_MULA_iq");
	recVUMI_MUL_iq_toD(VU, addr, EEREC_ACC, info);
	recUpdateFlags(VU, EEREC_ACC, info);
}

void recVUMI_MULA_xyzw(VURegs *VU, int xyzw, int info)
{
	//Console.WriteLn ("recVUMI_MULA_xyzw");
	recVUMI_MUL_xyzw_toD(VU, xyzw, EEREC_ACC, info);
	recUpdateFlags(VU, EEREC_ACC, info);
}

void recVUMI_MULAi(VURegs *VU, int info) { recVUMI_MULA_iq(VU, VU_VI_ADDR(REG_I, 1), info); }
void recVUMI_MULAq(VURegs *VU, int info) { recVUMI_MULA_iq(VU, VU_REGQ_ADDR, info); }
void recVUMI_MULAx(VURegs *VU, int info) { recVUMI_MULA_xyzw(VU, 0, info); }
void recVUMI_MULAy(VURegs *VU, int info) { recVUMI_MULA_xyzw(VU, 1, info); }
void recVUMI_MULAz(VURegs *VU, int info) { recVUMI_MULA_xyzw(VU, 2, info); }
void recVUMI_MULAw(VURegs *VU, int info) { recVUMI_MULA_xyzw(VU, 3, info); }
//------------------------------------------------------------------


//------------------------------------------------------------------
// MADD
//------------------------------------------------------------------
void recVUMI_MADD_toD(VURegs *VU, int regd, int info)
{
	//Console.WriteLn ("recVUMI_MADD_toD");
	if (CHECK_VU_EXTRA_OVERFLOW) {
		if (_Fs_) vuFloat5_useEAX( EEREC_S, EEREC_TEMP, _X_Y_Z_W );
		if (_Ft_) vuFloat5_useEAX( EEREC_T, EEREC_TEMP, _X_Y_Z_W );
		vuFloat5_useEAX( EEREC_ACC, EEREC_TEMP, _X_Y_Z_W );
	}


	if( _X_Y_Z_W == 8 ) {
		if( regd == EEREC_ACC ) {
			SSE_MOVSS_XMM_to_XMM(EEREC_TEMP, EEREC_S);
			SSE_MULSS_XMM_to_XMM(EEREC_TEMP, EEREC_T);
			if (CHECK_VU_EXTRA_OVERFLOW) { vuFloat_useEAX( info, EEREC_TEMP, 8); }
			SSE_ADDSS_XMM_to_XMM(regd, EEREC_TEMP);
		}
		else if (regd == EEREC_T) {
			SSE_MULSS_XMM_to_XMM(regd, EEREC_S);
			if (CHECK_VU_EXTRA_OVERFLOW) { vuFloat_useEAX( info, regd, 8); }
			SSE_ADDSS_XMM_to_XMM(regd, EEREC_ACC);
		}
		else if (regd == EEREC_S) {
			SSE_MULSS_XMM_to_XMM(regd, EEREC_T);
			if (CHECK_VU_EXTRA_OVERFLOW) { vuFloat_useEAX( info, regd, 8); }
			SSE_ADDSS_XMM_to_XMM(regd, EEREC_ACC);
		}
		else {
			SSE_MOVSS_XMM_to_XMM(regd, EEREC_S);
			SSE_MULSS_XMM_to_XMM(regd, EEREC_T);
			if (CHECK_VU_EXTRA_OVERFLOW) { vuFloat_useEAX( info, regd, 8); }
			SSE_ADDSS_XMM_to_XMM(regd, EEREC_ACC);
		}
	}
	else if (_X_Y_Z_W != 0xf) {
		SSE_MOVAPS_XMM_to_XMM(EEREC_TEMP, EEREC_S);
		SSE_MULPS_XMM_to_XMM(EEREC_TEMP, EEREC_T);
		if (CHECK_VU_EXTRA_OVERFLOW) { vuFloat_useEAX( info, EEREC_TEMP, _X_Y_Z_W ); }
		SSE_ADDPS_XMM_to_XMM(EEREC_TEMP, EEREC_ACC);

		VU_MERGE_REGS(regd, EEREC_TEMP);
	}
	else {
		if( regd == EEREC_ACC ) {
			SSE_MOVAPS_XMM_to_XMM(EEREC_TEMP, EEREC_S);
			SSE_MULPS_XMM_to_XMM(EEREC_TEMP, EEREC_T);
			if (CHECK_VU_EXTRA_OVERFLOW) { vuFloat_useEAX( info, EEREC_TEMP, _X_Y_Z_W ); }
			SSE_ADDPS_XMM_to_XMM(regd, EEREC_TEMP);
		}
		else if (regd == EEREC_T) {
			SSE_MULPS_XMM_to_XMM(regd, EEREC_S);
			if (CHECK_VU_EXTRA_OVERFLOW) { vuFloat_useEAX( info, regd, _X_Y_Z_W ); }
			SSE_ADDPS_XMM_to_XMM(regd, EEREC_ACC);
		}
		else if (regd == EEREC_S) {
			SSE_MULPS_XMM_to_XMM(regd, EEREC_T);
			if (CHECK_VU_EXTRA_OVERFLOW) { vuFloat_useEAX( info, regd, _X_Y_Z_W ); }
			SSE_ADDPS_XMM_to_XMM(regd, EEREC_ACC);
		}
		else {
			SSE_MOVAPS_XMM_to_XMM(regd, EEREC_S);
			SSE_MULPS_XMM_to_XMM(regd, EEREC_T);
			if (CHECK_VU_EXTRA_OVERFLOW) { vuFloat_useEAX( info, regd, _X_Y_Z_W ); }
			SSE_ADDPS_XMM_to_XMM(regd, EEREC_ACC);
		}
	}
}

void recVUMI_MADD_iq_toD(VURegs *VU, uptr addr, int regd, int info)
{
	//Console.WriteLn ("recVUMI_MADD_iq_toD");
	if (CHECK_VU_EXTRA_OVERFLOW) {
		vuFloat3(addr);
		if (_Fs_) vuFloat5_useEAX( EEREC_S, EEREC_TEMP, _X_Y_Z_W );
		vuFloat5_useEAX( EEREC_ACC, EEREC_TEMP, _X_Y_Z_W );
	}

	if( _X_Y_Z_W == 8 ) {
		if( _Fs_ == 0 ) {
			// do nothing if regd == ACC (ACCx <= ACCx + 0.0 * *addr)
			if( regd != EEREC_ACC ) {
				SSE_MOVSS_XMM_to_XMM(regd, EEREC_ACC);
			}
			return;
		}

		if( regd == EEREC_ACC ) {
			assert( EEREC_TEMP < iREGCNT_XMM );
			SSE_MOVSS_M32_to_XMM(EEREC_TEMP, addr);
			SSE_MULSS_XMM_to_XMM(EEREC_TEMP, EEREC_S);
			if (CHECK_VU_EXTRA_OVERFLOW) { vuFloat_useEAX( info, EEREC_TEMP, 8); }
			SSE_ADDSS_XMM_to_XMM(regd, EEREC_TEMP);
		}
		else if( regd == EEREC_S ) {
			SSE_MULSS_M32_to_XMM(regd, addr);
			if (CHECK_VU_EXTRA_OVERFLOW) { vuFloat_useEAX( info, regd, _X_Y_Z_W ); }
			SSE_ADDSS_XMM_to_XMM(regd, EEREC_ACC);
		}
		else {
			SSE_MOVSS_XMM_to_XMM(regd, EEREC_S);
			SSE_MULSS_M32_to_XMM(regd, addr);
			if (CHECK_VU_EXTRA_OVERFLOW) { vuFloat_useEAX( info, regd, _X_Y_Z_W ); }
			SSE_ADDSS_XMM_to_XMM(regd, EEREC_ACC);
		}
	}
	else {
		if( _Fs_ == 0 ) {
			if( regd == EEREC_ACC ) { // ACCxyz is unchanged, ACCw <= ACCw + *addr
				if( _W ) { // if _W is zero, do nothing
					SSE_MOVSS_M32_to_XMM(EEREC_TEMP, addr); // { *addr, 0, 0, 0 }
					SSE_SHUFPS_XMM_to_XMM(EEREC_TEMP, EEREC_TEMP, 0x27); // { 0, 0, 0, *addr }
					SSE_ADDPS_XMM_to_XMM(regd, EEREC_TEMP); // { ACCx, ACCy, ACCz, ACCw + *addr }
				}
			}
			else { // DESTxyz <= ACCxyz, DESTw <= ACCw + *addr
				if( _W ) {
					SSE_MOVSS_M32_to_XMM(EEREC_TEMP, addr); // { *addr, 0, 0, 0 }
					SSE_SHUFPS_XMM_to_XMM(EEREC_TEMP, EEREC_TEMP, 0x27); // { 0, 0, 0, *addr }
					SSE_ADDPS_XMM_to_XMM(EEREC_TEMP, EEREC_ACC); // { ACCx, ACCy, ACCz, ACCw + *addr }
				}
				else SSE_MOVAPS_XMM_to_XMM(EEREC_TEMP, EEREC_ACC);
				VU_MERGE_REGS(regd, EEREC_TEMP);
			}

			return;
		}

		if( _X_Y_Z_W != 0xf || regd == EEREC_ACC || regd == EEREC_TEMP || regd == EEREC_S ) {
			SSE_MOVSS_M32_to_XMM(EEREC_TEMP, addr);
			SSE_SHUFPS_XMM_to_XMM(EEREC_TEMP, EEREC_TEMP, 0x00);
		}

		if (_X_Y_Z_W != 0xf) {
			SSE_MULPS_XMM_to_XMM(EEREC_TEMP, EEREC_S);
			if (CHECK_VU_EXTRA_OVERFLOW) { vuFloat_useEAX( info, EEREC_TEMP, _X_Y_Z_W ); }
			SSE_ADDPS_XMM_to_XMM(EEREC_TEMP, EEREC_ACC);

			VU_MERGE_REGS(regd, EEREC_TEMP);
		}
		else {
			if( regd == EEREC_ACC ) {
				SSE_MULPS_XMM_to_XMM(EEREC_TEMP, EEREC_S);
				if (CHECK_VU_EXTRA_OVERFLOW) { vuFloat_useEAX( info, EEREC_TEMP, _X_Y_Z_W ); }
				SSE_ADDPS_XMM_to_XMM(regd, EEREC_TEMP);
			}
			else if( regd == EEREC_S ) {
				SSE_MULPS_XMM_to_XMM(regd, EEREC_TEMP);
				if (CHECK_VU_EXTRA_OVERFLOW) { vuFloat_useEAX( info, regd, _X_Y_Z_W ); }
				SSE_ADDPS_XMM_to_XMM(regd, EEREC_ACC);
			}
			else if( regd == EEREC_TEMP ) {
				SSE_MULPS_XMM_to_XMM(regd, EEREC_S);
				if (CHECK_VU_EXTRA_OVERFLOW) { vuFloat_useEAX( info, regd, _X_Y_Z_W ); }
				SSE_ADDPS_XMM_to_XMM(regd, EEREC_ACC);
			}
			else {
				SSE_MOVSS_M32_to_XMM(regd, addr);
				SSE_SHUFPS_XMM_to_XMM(regd, regd, 0x00);
				SSE_MULPS_XMM_to_XMM(regd, EEREC_S);
				if (CHECK_VU_EXTRA_OVERFLOW) { vuFloat_useEAX( info, regd, _X_Y_Z_W ); }
				SSE_ADDPS_XMM_to_XMM(regd, EEREC_ACC);
			}
		}
	}
}

void recVUMI_MADD_xyzw_toD(VURegs *VU, int xyzw, int regd, int info)
{
	//Console.WriteLn ("recVUMI_MADD_xyzw_toD");
	if (CHECK_VU_EXTRA_OVERFLOW) {
		if (_Ft_) vuFloat5_useEAX( EEREC_T, EEREC_TEMP, ( 1 << (3 - xyzw) ) );
		vuFloat5_useEAX( EEREC_ACC, EEREC_TEMP, _X_Y_Z_W );
	}
	if (_Fs_) { // This is needed for alot of games; so always clamp this operand
		if (CHECK_VU_SIGN_OVERFLOW) vFloats4_useEAX[_X_Y_Z_W]( EEREC_S, EEREC_TEMP ); // Always clamp EEREC_S, regardless if CHECK_VU_OVERFLOW is set
		else vFloats2[_X_Y_Z_W]( EEREC_S, EEREC_TEMP ); // Always clamp EEREC_S, regardless if CHECK_VU_OVERFLOW is set
	}
	if( _Ft_ == 0 ) {

		if( xyzw == 3 ) {
			// just add
			if( _X_Y_Z_W == 8 ) {
				if( regd == EEREC_S ) SSE_ADDSS_XMM_to_XMM(regd, EEREC_ACC);
				else {
					SSE_MOVSS_XMM_to_XMM(regd, EEREC_ACC);
					SSE_ADDSS_XMM_to_XMM(regd, EEREC_S);
				}
			}
			else {
				if( _X_Y_Z_W != 0xf ) {
					SSE_MOVAPS_XMM_to_XMM(EEREC_TEMP, EEREC_S);
					SSE_ADDPS_XMM_to_XMM(EEREC_TEMP, EEREC_ACC);

					VU_MERGE_REGS(regd, EEREC_TEMP);
				}
				else {
					if( regd == EEREC_S ) SSE_ADDPS_XMM_to_XMM(regd, EEREC_ACC);
					else {
						SSE_MOVAPS_XMM_to_XMM(regd, EEREC_ACC);
						SSE_ADDPS_XMM_to_XMM(regd, EEREC_S);
					}
				}
			}
		}
		else {
			// just move acc to regd
			if( _X_Y_Z_W != 0xf ) {
				SSE_MOVAPS_XMM_to_XMM(EEREC_TEMP, EEREC_ACC);
				VU_MERGE_REGS(regd, EEREC_TEMP);
			}
			else SSE_MOVAPS_XMM_to_XMM(regd, EEREC_ACC);
		}

		return;
	}

	if( _X_Y_Z_W == 8 ) {
		_unpackVFSS_xyzw(EEREC_TEMP, EEREC_T, xyzw);

		if( regd == EEREC_ACC ) {
			SSE_MULSS_XMM_to_XMM(EEREC_TEMP, EEREC_S);
			if (CHECK_VU_EXTRA_OVERFLOW) { vuFloat_useEAX( info, EEREC_TEMP, 8); }
			SSE_ADDSS_XMM_to_XMM(regd, EEREC_TEMP);
		}
		else if( regd == EEREC_S ) {
			SSE_MULSS_XMM_to_XMM(regd, EEREC_TEMP);
			if (CHECK_VU_EXTRA_OVERFLOW) { vuFloat_useEAX( info, regd, 8); }
			SSE_ADDSS_XMM_to_XMM(regd, EEREC_ACC);
		}
		else if( regd == EEREC_TEMP ) {
			SSE_MULSS_XMM_to_XMM(regd, EEREC_S);
			if (CHECK_VU_EXTRA_OVERFLOW) { vuFloat_useEAX( info, regd, 8); }
			SSE_ADDSS_XMM_to_XMM(regd, EEREC_ACC);
		}
		else {
			SSE_MOVSS_XMM_to_XMM(regd, EEREC_ACC);
			SSE_MULSS_XMM_to_XMM(EEREC_TEMP, EEREC_S);
			if (CHECK_VU_EXTRA_OVERFLOW) { vuFloat_useEAX( info, EEREC_TEMP, 8); }
			SSE_ADDSS_XMM_to_XMM(regd, EEREC_TEMP);
		}
	}
	else {
		if( _X_Y_Z_W != 0xf || regd == EEREC_ACC || regd == EEREC_TEMP || regd == EEREC_S ) {
			_unpackVF_xyzw(EEREC_TEMP, EEREC_T, xyzw);
		}

		if (_X_Y_Z_W != 0xf) {
			SSE_MULPS_XMM_to_XMM(EEREC_TEMP, EEREC_S);
			if (CHECK_VU_EXTRA_OVERFLOW) { vuFloat_useEAX( info, EEREC_TEMP, _X_Y_Z_W ); }
			SSE_ADDPS_XMM_to_XMM(EEREC_TEMP, EEREC_ACC);

			VU_MERGE_REGS(regd, EEREC_TEMP);
		}
		else {
			if( regd == EEREC_ACC ) {
				SSE_MULPS_XMM_to_XMM(EEREC_TEMP, EEREC_S);
				if (CHECK_VU_EXTRA_OVERFLOW) { vuFloat_useEAX( info, EEREC_TEMP, _X_Y_Z_W ); }
				SSE_ADDPS_XMM_to_XMM(regd, EEREC_TEMP);
			}
			else if( regd == EEREC_S ) {
				SSE_MULPS_XMM_to_XMM(regd, EEREC_TEMP);
				if (CHECK_VU_EXTRA_OVERFLOW) { vuFloat_useEAX( info, regd, _X_Y_Z_W ); }
				SSE_ADDPS_XMM_to_XMM(regd, EEREC_ACC);
			}
			else if( regd == EEREC_TEMP ) {
				SSE_MULPS_XMM_to_XMM(regd, EEREC_S);
				if (CHECK_VU_EXTRA_OVERFLOW) { vuFloat_useEAX( info, regd, _X_Y_Z_W ); }
				SSE_ADDPS_XMM_to_XMM(regd, EEREC_ACC);
			}
			else {
				_unpackVF_xyzw(regd, EEREC_T, xyzw);
				SSE_MULPS_XMM_to_XMM(regd, EEREC_S);
				if (CHECK_VU_EXTRA_OVERFLOW) { vuFloat_useEAX( info, regd, _X_Y_Z_W ); }
				SSE_ADDPS_XMM_to_XMM(regd, EEREC_ACC);
			}
		}
	}
}

void recVUMI_MADD(VURegs *VU, int info)
{
	//Console.WriteLn ("recVUMI_MADD");
	if( !_Fd_ ) info |= PROCESS_EE_SET_D(EEREC_TEMP);
	recVUMI_MADD_toD(VU, EEREC_D, info);
	recUpdateFlags(VU, EEREC_D, info);
}

void recVUMI_MADD_iq(VURegs *VU, int addr, int info)
{
	//Console.WriteLn ("recVUMI_MADD_iq");
	if( !_Fd_ ) info |= PROCESS_EE_SET_D(EEREC_TEMP);
	recVUMI_MADD_iq_toD(VU, addr, EEREC_D, info);
	recUpdateFlags(VU, EEREC_D, info);
}

void recVUMI_MADD_xyzw(VURegs *VU, int xyzw, int info)
{
	//Console.WriteLn ("recVUMI_MADD_xyzw");
	if( !_Fd_ ) info |= PROCESS_EE_SET_D(EEREC_TEMP);
	recVUMI_MADD_xyzw_toD(VU, xyzw, EEREC_D, info);
	recUpdateFlags(VU, EEREC_D, info);
	// super bust-a-move arrows needs overflow clamping
}

void recVUMI_MADDi(VURegs *VU, int info) { recVUMI_MADD_iq(VU, VU_VI_ADDR(REG_I, 1), info); }
void recVUMI_MADDq(VURegs *VU, int info) { recVUMI_MADD_iq(VU, VU_REGQ_ADDR, info); }
void recVUMI_MADDx(VURegs *VU, int info) { recVUMI_MADD_xyzw(VU, 0, info); }
void recVUMI_MADDy(VURegs *VU, int info) { recVUMI_MADD_xyzw(VU, 1, info); }
void recVUMI_MADDz(VURegs *VU, int info) { recVUMI_MADD_xyzw(VU, 2, info); }
void recVUMI_MADDw(VURegs *VU, int info) { recVUMI_MADD_xyzw(VU, 3, info); }
//------------------------------------------------------------------


//------------------------------------------------------------------
// MADDA
//------------------------------------------------------------------
void recVUMI_MADDA( VURegs *VU, int info )
{
	//Console.WriteLn ("recVUMI_MADDA");
	recVUMI_MADD_toD(VU, EEREC_ACC, info);
	recUpdateFlags(VU, EEREC_ACC, info);
}

void recVUMI_MADDAi( VURegs *VU , int info)
{
	//Console.WriteLn ("recVUMI_MADDAi");
	recVUMI_MADD_iq_toD( VU, VU_VI_ADDR(REG_I, 1), EEREC_ACC, info);
	recUpdateFlags(VU, EEREC_ACC, info);
}

void recVUMI_MADDAq( VURegs *VU , int info)
{
	//Console.WriteLn ("recVUMI_MADDAq ");
	recVUMI_MADD_iq_toD( VU, VU_REGQ_ADDR, EEREC_ACC, info);
	recUpdateFlags(VU, EEREC_ACC, info);
}

void recVUMI_MADDAx( VURegs *VU , int info)
{
	//Console.WriteLn ("recVUMI_MADDAx");
	recVUMI_MADD_xyzw_toD(VU, 0, EEREC_ACC, info);
	recUpdateFlags(VU, EEREC_ACC, info);
}

void recVUMI_MADDAy( VURegs *VU , int info)
{
	//Console.WriteLn ("recVUMI_MADDAy");
	recVUMI_MADD_xyzw_toD(VU, 1, EEREC_ACC, info);
	recUpdateFlags(VU, EEREC_ACC, info);
}

void recVUMI_MADDAz( VURegs *VU , int info)
{
	//Console.WriteLn ("recVUMI_MADDAz");
	recVUMI_MADD_xyzw_toD(VU, 2, EEREC_ACC, info);
	recUpdateFlags(VU, EEREC_ACC, info);
}

void recVUMI_MADDAw( VURegs *VU , int info)
{
	//Console.WriteLn ("recVUMI_MADDAw");
	recVUMI_MADD_xyzw_toD(VU, 3, EEREC_ACC, info);
	recUpdateFlags(VU, EEREC_ACC, info);
}
//------------------------------------------------------------------


//------------------------------------------------------------------
// MSUB
//------------------------------------------------------------------
void recVUMI_MSUB_toD(VURegs *VU, int regd, int info)
{
	//Console.WriteLn ("recVUMI_MSUB_toD");
	if (CHECK_VU_EXTRA_OVERFLOW) {
		if (_Fs_) vuFloat5_useEAX( EEREC_S, EEREC_TEMP, _X_Y_Z_W );
		if (_Ft_) vuFloat5_useEAX( EEREC_T, EEREC_TEMP, _X_Y_Z_W );
		vuFloat5_useEAX( EEREC_ACC, EEREC_TEMP, _X_Y_Z_W );
	}

	if (_X_Y_Z_W != 0xf) {
		int t1reg = _vuGetTempXMMreg(info);

		SSE_MOVAPS_XMM_to_XMM(EEREC_TEMP, EEREC_S);
		SSE_MULPS_XMM_to_XMM(EEREC_TEMP, EEREC_T);
		if (CHECK_VU_EXTRA_OVERFLOW) { vuFloat_useEAX( info, EEREC_TEMP, _X_Y_Z_W ); }

		if( t1reg >= 0 ) {
			SSE_MOVAPS_XMM_to_XMM(t1reg, EEREC_ACC);
			SSE_SUBPS_XMM_to_XMM(t1reg, EEREC_TEMP);

			VU_MERGE_REGS(regd, t1reg);
			_freeXMMreg(t1reg);
		}
		else {
			SSE_XORPS_M128_to_XMM(EEREC_TEMP, (uptr)&const_clip[4]);
			SSE_ADDPS_XMM_to_XMM(EEREC_TEMP, EEREC_ACC);
			VU_MERGE_REGS(regd, EEREC_TEMP);
		}
	}
	else {
		if( regd == EEREC_S ) {
			assert( regd != EEREC_ACC );
			SSE_MULPS_XMM_to_XMM(regd, EEREC_T);
			if (CHECK_VU_EXTRA_OVERFLOW) { vuFloat_useEAX( info, regd, _X_Y_Z_W ); }
			SSE_SUBPS_XMM_to_XMM(regd, EEREC_ACC);
			SSE_XORPS_M128_to_XMM(regd, (uptr)&const_clip[4]);
		}
		else if( regd == EEREC_T ) {
			assert( regd != EEREC_ACC );
			SSE_MULPS_XMM_to_XMM(regd, EEREC_S);
			if (CHECK_VU_EXTRA_OVERFLOW) { vuFloat_useEAX( info, regd, _X_Y_Z_W ); }
			SSE_SUBPS_XMM_to_XMM(regd, EEREC_ACC);
			SSE_XORPS_M128_to_XMM(regd, (uptr)&const_clip[4]);
		}
		else if( regd == EEREC_TEMP ) {
			SSE_MOVAPS_XMM_to_XMM(regd, EEREC_S);
			SSE_MULPS_XMM_to_XMM(regd, EEREC_T);
			if (CHECK_VU_EXTRA_OVERFLOW) { vuFloat_useEAX( info, regd, _X_Y_Z_W ); }
			SSE_SUBPS_XMM_to_XMM(regd, EEREC_ACC);
			SSE_XORPS_M128_to_XMM(regd, (uptr)&const_clip[4]);
		}
		else {
			SSE_MOVAPS_XMM_to_XMM(EEREC_TEMP, EEREC_S);
			SSE_MOVAPS_XMM_to_XMM(regd, EEREC_ACC);
			SSE_MULPS_XMM_to_XMM(EEREC_TEMP, EEREC_T);
			if (CHECK_VU_EXTRA_OVERFLOW) { vuFloat_useEAX( info, EEREC_TEMP, _X_Y_Z_W ); }
			SSE_SUBPS_XMM_to_XMM(regd, EEREC_TEMP);
		}
	}
}

void recVUMI_MSUB_temp_toD(VURegs *VU, int regd, int info)
{
	//Console.WriteLn ("recVUMI_MSUB_temp_toD");

	if (_X_Y_Z_W != 0xf) {
		int t1reg = _vuGetTempXMMreg(info);

		SSE_MULPS_XMM_to_XMM(EEREC_TEMP, EEREC_S);
		if (CHECK_VU_EXTRA_OVERFLOW) { vuFloat_useEAX( info, EEREC_TEMP, _X_Y_Z_W ); }

		if( t1reg >= 0 ) {
			SSE_MOVAPS_XMM_to_XMM(t1reg, EEREC_ACC);
			SSE_SUBPS_XMM_to_XMM(t1reg, EEREC_TEMP);

			if ( regd != EEREC_TEMP ) { VU_MERGE_REGS(regd, t1reg); }
			else SSE_MOVAPS_XMM_to_XMM(regd, t1reg);

			_freeXMMreg(t1reg);
		}
		else {
			SSE_XORPS_M128_to_XMM(EEREC_TEMP, (uptr)&const_clip[4]);
			SSE_ADDPS_XMM_to_XMM(EEREC_TEMP, EEREC_ACC);
			VU_MERGE_REGS(regd, EEREC_TEMP);
		}
	}
	else {
		if( regd == EEREC_ACC ) {
			SSE_MULPS_XMM_to_XMM(EEREC_TEMP, EEREC_S);
			if (CHECK_VU_EXTRA_OVERFLOW) { vuFloat_useEAX( info, EEREC_TEMP, _X_Y_Z_W ); }
			SSE_SUBPS_XMM_to_XMM(regd, EEREC_TEMP);
		}
		else if( regd == EEREC_S ) {
			SSE_MULPS_XMM_to_XMM(regd, EEREC_TEMP);
			if (CHECK_VU_EXTRA_OVERFLOW) { vuFloat_useEAX( info, regd, _X_Y_Z_W ); }
			SSE_SUBPS_XMM_to_XMM(regd, EEREC_ACC);
			SSE_XORPS_M128_to_XMM(regd, (uptr)&const_clip[4]);
		}
		else if( regd == EEREC_TEMP ) {
			SSE_MULPS_XMM_to_XMM(regd, EEREC_S);
			if (CHECK_VU_EXTRA_OVERFLOW) { vuFloat_useEAX( info, regd, _X_Y_Z_W ); }
			SSE_SUBPS_XMM_to_XMM(regd, EEREC_ACC);
			SSE_XORPS_M128_to_XMM(regd, (uptr)&const_clip[4]);
		}
		else {
			SSE_MOVAPS_XMM_to_XMM(regd, EEREC_ACC);
			SSE_MULPS_XMM_to_XMM(EEREC_TEMP, EEREC_S);
			if (CHECK_VU_EXTRA_OVERFLOW) { vuFloat_useEAX( info, EEREC_TEMP, _X_Y_Z_W ); }
			SSE_SUBPS_XMM_to_XMM(regd, EEREC_TEMP);
		}
	}
}

void recVUMI_MSUB_iq_toD(VURegs *VU, int regd, int addr, int info)
{
	//Console.WriteLn ("recVUMI_MSUB_iq_toD");
	if (CHECK_VU_EXTRA_OVERFLOW) {
		if (_Fs_) vuFloat5_useEAX( EEREC_S, EEREC_TEMP, _X_Y_Z_W );
		vuFloat5_useEAX( EEREC_ACC, EEREC_TEMP, _X_Y_Z_W );
		vuFloat3(addr);
	}
	SSE_MOVSS_M32_to_XMM(EEREC_TEMP, addr);
	SSE_SHUFPS_XMM_to_XMM(EEREC_TEMP, EEREC_TEMP, 0x00);
	recVUMI_MSUB_temp_toD(VU, regd, info);
}

void recVUMI_MSUB_xyzw_toD(VURegs *VU, int regd, int xyzw, int info)
{
	//Console.WriteLn ("recVUMI_MSUB_xyzw_toD");
	if (CHECK_VU_EXTRA_OVERFLOW) {
		if (_Fs_) vuFloat5_useEAX( EEREC_S, EEREC_TEMP, _X_Y_Z_W );
		if (_Ft_) vuFloat5_useEAX( EEREC_T, EEREC_TEMP, 1 << (3 - xyzw));
		vuFloat5_useEAX( EEREC_ACC, EEREC_TEMP, _X_Y_Z_W );
	}
	_unpackVF_xyzw(EEREC_TEMP, EEREC_T, xyzw);
	recVUMI_MSUB_temp_toD(VU, regd, info);
}

void recVUMI_MSUB(VURegs *VU, int info)
{
	//Console.WriteLn ("recVUMI_MSUB");
	if( !_Fd_ ) info |= PROCESS_EE_SET_D(EEREC_TEMP);
	recVUMI_MSUB_toD(VU, EEREC_D, info);
	recUpdateFlags(VU, EEREC_D, info);
}

void recVUMI_MSUB_iq(VURegs *VU, int addr, int info)
{
	//Console.WriteLn ("recVUMI_MSUB_iq");
	if( !_Fd_ ) info |= PROCESS_EE_SET_D(EEREC_TEMP);
	recVUMI_MSUB_iq_toD(VU, EEREC_D, addr, info);
	recUpdateFlags(VU, EEREC_D, info);
}

void recVUMI_MSUBi(VURegs *VU, int info) { recVUMI_MSUB_iq(VU, VU_VI_ADDR(REG_I, 1), info); }
void recVUMI_MSUBq(VURegs *VU, int info) { recVUMI_MSUB_iq(VU, VU_REGQ_ADDR, info); }
void recVUMI_MSUBx(VURegs *VU, int info)
{
	//Console.WriteLn ("recVUMI_MSUBx");
	if( !_Fd_ ) info |= PROCESS_EE_SET_D(EEREC_TEMP);
	recVUMI_MSUB_xyzw_toD(VU, EEREC_D, 0, info);
	recUpdateFlags(VU, EEREC_D, info);
}

void recVUMI_MSUBy(VURegs *VU, int info)
{
	//Console.WriteLn ("recVUMI_MSUBy");
	if( !_Fd_ ) info |= PROCESS_EE_SET_D(EEREC_TEMP);
	recVUMI_MSUB_xyzw_toD(VU, EEREC_D, 1, info);
	recUpdateFlags(VU, EEREC_D, info);
}

void recVUMI_MSUBz(VURegs *VU, int info)
{
	//Console.WriteLn ("recVUMI_MSUBz");
	if( !_Fd_ ) info |= PROCESS_EE_SET_D(EEREC_TEMP);
	recVUMI_MSUB_xyzw_toD(VU, EEREC_D, 2, info);
	recUpdateFlags(VU, EEREC_D, info);
}

void recVUMI_MSUBw(VURegs *VU, int info)
{
	//Console.WriteLn ("recVUMI_MSUBw");
	if( !_Fd_ ) info |= PROCESS_EE_SET_D(EEREC_TEMP);
	recVUMI_MSUB_xyzw_toD(VU, EEREC_D, 3, info);
	recUpdateFlags(VU, EEREC_D, info);
}
//------------------------------------------------------------------


//------------------------------------------------------------------
// MSUBA
//------------------------------------------------------------------
void recVUMI_MSUBA( VURegs *VU, int info )
{
	//Console.WriteLn ("recVUMI_MSUBA");
	recVUMI_MSUB_toD(VU, EEREC_ACC, info);
	recUpdateFlags(VU, EEREC_ACC, info);
}

void recVUMI_MSUBAi( VURegs *VU, int info )
{
	//Console.WriteLn ("recVUMI_MSUBAi ");
	recVUMI_MSUB_iq_toD( VU, EEREC_ACC, VU_VI_ADDR(REG_I, 1), info );
	recUpdateFlags(VU, EEREC_ACC, info);
}

void recVUMI_MSUBAq( VURegs *VU, int info )
{
	//Console.WriteLn ("recVUMI_MSUBAq");
	recVUMI_MSUB_iq_toD( VU, EEREC_ACC, VU_REGQ_ADDR, info );
	recUpdateFlags(VU, EEREC_ACC, info);
}

void recVUMI_MSUBAx( VURegs *VU, int info )
{
	//Console.WriteLn ("recVUMI_MSUBAx");
	recVUMI_MSUB_xyzw_toD(VU, EEREC_ACC, 0, info);
	recUpdateFlags(VU, EEREC_ACC, info);
}

void recVUMI_MSUBAy( VURegs *VU, int info )
{
	//Console.WriteLn ("recVUMI_MSUBAy");
	recVUMI_MSUB_xyzw_toD(VU, EEREC_ACC, 1, info);
	recUpdateFlags(VU, EEREC_ACC, info);
}

void recVUMI_MSUBAz( VURegs *VU, int info )
{
	//Console.WriteLn ("recVUMI_MSUBAz ");
	recVUMI_MSUB_xyzw_toD(VU, EEREC_ACC, 2, info);
	recUpdateFlags(VU, EEREC_ACC, info);
}

void recVUMI_MSUBAw( VURegs *VU, int info )
{
	//Console.WriteLn ("recVUMI_MSUBAw");
	recVUMI_MSUB_xyzw_toD(VU, EEREC_ACC, 3, info);
	recUpdateFlags(VU, EEREC_ACC, info);
}
//------------------------------------------------------------------


static const __aligned16 u32 special_mask[4] = {0xffffffff, 0x80000000, 0xffffffff, 0x80000000};
static const __aligned16 u32 special_mask2[4] = {0, 0x40000000, 0, 0x40000000};

__aligned16 u32 temp_loc[4];
__aligned16 u32 temp_loc2[4];

//MAX/MINI are non-arithmetic operations that implicitly support numbers with the EXP field being 0 ("denormals").
//
//As such, they are sometimes used for integer move and (positive!) integer max/min, knowing that integers that
//represent denormals will not be flushed to 0.
//
//As such, this implementation performs a non-arithmetic operation that supports "denormals" and "infs/nans".
//There might be an easier way to do it but here, MAX/MIN is performed with PMAXPD/PMINPD.
//Fake double-precision numbers are constructed by copying the sign of the original numbers, clearing the upper 32 bits,
//setting the 62nd bit to 1 (to ensure double-precision number is "normalized") and having the lower 32bits
//being the same as the original number.

void MINMAXlogical(VURegs *VU, int info, int min, int mode, uptr addr = 0, int xyzw = 0)
//mode1 = iq, mode2 = xyzw, mode0 = normal
{
	int t1regbool = 0;
	int t1reg = _vuGetTempXMMreg(info);
	if (t1reg < 0)
	{
		t1regbool = 1;
		for (t1reg = 0; ( (t1reg == EEREC_D) || (t1reg == EEREC_S) || (mode != 1 && t1reg == EEREC_T)
			|| (t1reg == EEREC_TEMP) ); t1reg++); // Find unused reg (For first temp reg)
		SSE_MOVAPS_XMM_to_M128((uptr)temp_loc, t1reg); // Backup t1reg XMM reg
	}
	int t2regbool = -1;
	int t2reg = EEREC_TEMP;
	if (EEREC_TEMP == EEREC_D || EEREC_TEMP == EEREC_S || (mode != 1 && EEREC_TEMP == EEREC_T))
	{
		t2regbool = 0;
		t2reg = _vuGetTempXMMreg(info);
		if (t2reg < 0)
		{
			t2regbool = 1;
			for (t2reg = 0; ( (t2reg == EEREC_D) || (t2reg == EEREC_S) || (mode != 1 && t2reg == EEREC_T) ||
					(t2reg == t1reg) || (t2reg == EEREC_TEMP) ); t2reg++); // Find unused reg (For second temp reg)
			SSE_MOVAPS_XMM_to_M128((uptr)temp_loc2, t2reg); // Backup t2reg XMM reg
		}
	}

	if (_X || _Y)
	{
		SSE2_PSHUFD_XMM_to_XMM(t1reg, EEREC_S, 0x50);
		SSE2_PAND_M128_to_XMM(t1reg, (uptr)special_mask);
		SSE2_POR_M128_to_XMM(t1reg, (uptr)special_mask2);
		if (mode == 0)
			SSE2_PSHUFD_XMM_to_XMM(t2reg, EEREC_T, 0x50);
		else if (mode == 1)
		{
			SSE2_MOVD_M32_to_XMM(t2reg, addr);
			SSE2_PSHUFD_XMM_to_XMM(t2reg, t2reg, 0x00);
		}
		else if (mode == 2)
			_unpackVF_xyzw(t2reg, EEREC_T, xyzw);
		SSE2_PAND_M128_to_XMM(t2reg, (uptr)special_mask);
		SSE2_POR_M128_to_XMM(t2reg, (uptr)special_mask2);
		if (min)
			SSE2_MINPD_XMM_to_XMM(t1reg, t2reg);
		else
			SSE2_MAXPD_XMM_to_XMM(t1reg, t2reg);
		SSE2_PSHUFD_XMM_to_XMM(t1reg, t1reg, 0x88);
		VU_MERGE_REGS_CUSTOM(EEREC_D, t1reg, 0xc & _X_Y_Z_W);
	}

	if (_Z || _W)
	{
		SSE2_PSHUFD_XMM_to_XMM(t1reg, EEREC_S, 0xfa);
		SSE2_PAND_M128_to_XMM(t1reg, (uptr)special_mask);
		SSE2_POR_M128_to_XMM(t1reg, (uptr)special_mask2);
		if (mode == 0)
			SSE2_PSHUFD_XMM_to_XMM(t2reg, EEREC_T, 0xfa);
		else if (mode == 1)
		{
			SSE2_MOVD_M32_to_XMM(t2reg, addr);
			SSE2_PSHUFD_XMM_to_XMM(t2reg, t2reg, 0x00);
		}
		else if (mode == 2)
			_unpackVF_xyzw(t2reg, EEREC_T, xyzw);
		SSE2_PAND_M128_to_XMM(t2reg, (uptr)special_mask);
		SSE2_POR_M128_to_XMM(t2reg, (uptr)special_mask2);
		if (min)
			SSE2_MINPD_XMM_to_XMM(t1reg, t2reg);
		else
			SSE2_MAXPD_XMM_to_XMM(t1reg, t2reg);
		SSE2_PSHUFD_XMM_to_XMM(t1reg, t1reg, 0x88);
		VU_MERGE_REGS_CUSTOM(EEREC_D, t1reg, 0x3 & _X_Y_Z_W);
	}

	if (t1regbool == 0)
		_freeXMMreg(t1reg);
	else if (t1regbool == 1)
		SSE_MOVAPS_M128_to_XMM(t1reg, (uptr)temp_loc); // Restore t1reg XMM reg
	if (t2regbool == 0)
		_freeXMMreg(t2reg);
	else if (t2regbool == 1)
		SSE_MOVAPS_M128_to_XMM(t2reg, (uptr)temp_loc2); // Restore t2reg XMM reg
}

//------------------------------------------------------------------
// MAX
//------------------------------------------------------------------

void recVUMI_MAX(VURegs *VU, int info)
{
	if ( _Fd_ == 0 ) return;
	//Console.WriteLn ("recVUMI_MAX");

	if (MINMAXFIX)
		MINMAXlogical(VU, info, 0, 0);
	else
	{

		if (_Fs_) vuFloat4_useEAX( EEREC_S, EEREC_TEMP, _X_Y_Z_W ); // Always do Preserved Sign Clamping
		if (_Ft_) vuFloat4_useEAX( EEREC_T, EEREC_TEMP, _X_Y_Z_W );

		if( _X_Y_Z_W == 8 ) {
			if (EEREC_D == EEREC_S) SSE_MAXSS_XMM_to_XMM(EEREC_D, EEREC_T);
			else if (EEREC_D == EEREC_T) SSE_MAXSS_XMM_to_XMM(EEREC_D, EEREC_S);
			else {
				SSE_MOVSS_XMM_to_XMM(EEREC_D, EEREC_S);
				SSE_MAXSS_XMM_to_XMM(EEREC_D, EEREC_T);
			}
		}
		else if (_X_Y_Z_W != 0xf) {
			SSE_MOVAPS_XMM_to_XMM(EEREC_TEMP, EEREC_S);
			SSE_MAXPS_XMM_to_XMM(EEREC_TEMP, EEREC_T);

			VU_MERGE_REGS(EEREC_D, EEREC_TEMP);
		}
		else {
			if( EEREC_D == EEREC_S ) SSE_MAXPS_XMM_to_XMM(EEREC_D, EEREC_T);
			else if( EEREC_D == EEREC_T ) SSE_MAXPS_XMM_to_XMM(EEREC_D, EEREC_S);
			else {
				SSE_MOVAPS_XMM_to_XMM(EEREC_D, EEREC_S);
				SSE_MAXPS_XMM_to_XMM(EEREC_D, EEREC_T);
			}
		}
	}
}

void recVUMI_MAX_iq(VURegs *VU, uptr addr, int info)
{
	if ( _Fd_ == 0 ) return;
	//Console.WriteLn ("recVUMI_MAX_iq");

	if (MINMAXFIX)
		MINMAXlogical(VU, info, 0, 1, addr);
	else
	{
		if (_Fs_) vuFloat4_useEAX( EEREC_S, EEREC_TEMP, _X_Y_Z_W ); // Always do Preserved Sign Clamping
		vuFloat3(addr);

		if( _XYZW_SS ) {
			if( EEREC_D == EEREC_TEMP ) {
				_vuFlipRegSS(VU, EEREC_S);
				SSE_MOVSS_XMM_to_XMM(EEREC_D, EEREC_S);
				SSE_MAXSS_M32_to_XMM(EEREC_D, addr);
				_vuFlipRegSS(VU, EEREC_S);

				// have to flip over EEREC_D if computing flags!
				//if( (info & PROCESS_VU_UPDATEFLAGS) )
					_vuFlipRegSS(VU, EEREC_D);
			}
			else if( EEREC_D == EEREC_S ) {
				_vuFlipRegSS(VU, EEREC_D);
				SSE_MAXSS_M32_to_XMM(EEREC_D, addr);
				_vuFlipRegSS(VU, EEREC_D);
			}
			else {
				if( _X ) {
					SSE_MOVSS_XMM_to_XMM(EEREC_D, EEREC_S);
					SSE_MAXSS_M32_to_XMM(EEREC_D, addr);
				}
				else {
					_vuMoveSS(VU, EEREC_TEMP, EEREC_S);
					_vuFlipRegSS(VU, EEREC_D);
					SSE_MAXSS_M32_to_XMM(EEREC_TEMP, addr);
					SSE_MOVSS_XMM_to_XMM(EEREC_D, EEREC_TEMP);
					_vuFlipRegSS(VU, EEREC_D);
				}
			}
		}
		else if (_X_Y_Z_W != 0xf) {
			SSE_MOVSS_M32_to_XMM(EEREC_TEMP, addr);
			SSE_SHUFPS_XMM_to_XMM(EEREC_TEMP, EEREC_TEMP, 0x00);
			SSE_MAXPS_XMM_to_XMM(EEREC_TEMP, EEREC_S);
			VU_MERGE_REGS(EEREC_D, EEREC_TEMP);
		}
		else {
			if(EEREC_D == EEREC_S) {
				SSE_MOVSS_M32_to_XMM(EEREC_TEMP, addr);
				SSE_SHUFPS_XMM_to_XMM(EEREC_TEMP, EEREC_TEMP, 0x00);
				SSE_MAXPS_XMM_to_XMM(EEREC_D, EEREC_TEMP);
			}
			else {
				SSE_MOVSS_M32_to_XMM(EEREC_D, addr);
				SSE_SHUFPS_XMM_to_XMM(EEREC_D, EEREC_D, 0x00);
				SSE_MAXPS_XMM_to_XMM(EEREC_D, EEREC_S);
			}
		}
	}
}

void recVUMI_MAX_xyzw(VURegs *VU, int xyzw, int info)
{
	if ( _Fd_ == 0 ) return;
	//Console.WriteLn ("recVUMI_MAX_xyzw");

	if (_Fs_ == 0 && _Ft_ == 0)
	{
		if( _X_Y_Z_W == 8 && (EEREC_D != EEREC_TEMP)) {
			if( xyzw < 3 ) {
				SSE_XORPS_XMM_to_XMM(EEREC_TEMP, EEREC_TEMP);
				SSE_MOVSS_XMM_to_XMM(EEREC_D, EEREC_TEMP);
			}
			else {
				SSE_MOVSS_M32_to_XMM(EEREC_TEMP, (uptr)s_fones);
				SSE_MOVSS_XMM_to_XMM(EEREC_D, EEREC_TEMP);
			}
		}
		else if (_X_Y_Z_W != 0xf) {
			if( xyzw < 3 ) {
				if( _X_Y_Z_W & 1 ) SSE_MOVAPS_M128_to_XMM(EEREC_TEMP, (uptr)&VU->VF[0].UL[0]); // w included, so insert the whole reg
				else SSE_XORPS_XMM_to_XMM(EEREC_TEMP, EEREC_TEMP); // w not included, can zero out
			}
			else SSE_MOVAPS_M128_to_XMM(EEREC_TEMP, (uptr)s_fones);
			VU_MERGE_REGS(EEREC_D, EEREC_TEMP);
		}
		else {
			//If VF0.w isnt chosen as the constant, then its going to be MAX( 0, VF0 ), so the result is VF0
			if( xyzw < 3 ) { SSE_MOVAPS_M128_to_XMM(EEREC_D, (uptr)&VU->VF[0].UL[0]); }
			else SSE_MOVAPS_M128_to_XMM(EEREC_D, (uptr)s_fones);
		}
		return;
	}

	if (MINMAXFIX)
		MINMAXlogical(VU, info, 0, 2, 0, xyzw);
	else
	{
		if (_Fs_) vuFloat4_useEAX( EEREC_S, EEREC_TEMP, _X_Y_Z_W ); // Always do Preserved Sign Clamping
		if (_Ft_) vuFloat4_useEAX( EEREC_T, EEREC_TEMP, ( 1 << (3 - xyzw) ) );

		if( _X_Y_Z_W == 8 && (EEREC_D != EEREC_TEMP)) {
			if( xyzw == 0 ) {
				if( EEREC_D == EEREC_S ) SSE_MAXSS_XMM_to_XMM(EEREC_D, EEREC_T);
				else if( EEREC_D == EEREC_T ) SSE_MAXSS_XMM_to_XMM(EEREC_D, EEREC_S);
				else {
					SSE_MOVSS_XMM_to_XMM(EEREC_D, EEREC_S);
					SSE_MAXSS_XMM_to_XMM(EEREC_D, EEREC_T);
				}
			}
			else {
				_unpackVFSS_xyzw(EEREC_TEMP, EEREC_T, xyzw);
				SSE_MOVSS_XMM_to_XMM(EEREC_D, EEREC_S);
				SSE_MAXSS_XMM_to_XMM(EEREC_D, EEREC_TEMP);
			}
		}
		else if (_X_Y_Z_W != 0xf) {
			_unpackVF_xyzw(EEREC_TEMP, EEREC_T, xyzw);
			SSE_MAXPS_XMM_to_XMM(EEREC_TEMP, EEREC_S);
			VU_MERGE_REGS(EEREC_D, EEREC_TEMP);
		}
		else {
			if (EEREC_D == EEREC_S) {
				_unpackVF_xyzw(EEREC_TEMP, EEREC_T, xyzw);
				SSE_MAXPS_XMM_to_XMM(EEREC_D, EEREC_TEMP);
			}
			else {
				_unpackVF_xyzw(EEREC_D, EEREC_T, xyzw);
				SSE_MAXPS_XMM_to_XMM(EEREC_D, EEREC_S);
			}
		}
	}
}

void recVUMI_MAXi(VURegs *VU, int info) { recVUMI_MAX_iq(VU, VU_VI_ADDR(REG_I, 1), info); }
void recVUMI_MAXx(VURegs *VU, int info) { recVUMI_MAX_xyzw(VU, 0, info); }
void recVUMI_MAXy(VURegs *VU, int info) { recVUMI_MAX_xyzw(VU, 1, info); }
void recVUMI_MAXz(VURegs *VU, int info) { recVUMI_MAX_xyzw(VU, 2, info); }
void recVUMI_MAXw(VURegs *VU, int info) { recVUMI_MAX_xyzw(VU, 3, info); }
//------------------------------------------------------------------


//------------------------------------------------------------------
// MINI
//------------------------------------------------------------------
void recVUMI_MINI(VURegs *VU, int info)
{
	if ( _Fd_ == 0 ) return;
	//Console.WriteLn ("recVUMI_MINI");

	if (MINMAXFIX)
		MINMAXlogical(VU, info, 1, 0);
	else
	{

		if (_Fs_) vuFloat4_useEAX( EEREC_S, EEREC_TEMP, _X_Y_Z_W ); // Always do Preserved Sign Clamping
		if (_Ft_) vuFloat4_useEAX( EEREC_T, EEREC_TEMP, _X_Y_Z_W );

		if( _X_Y_Z_W == 8 ) {
			if (EEREC_D == EEREC_S) SSE_MINSS_XMM_to_XMM(EEREC_D, EEREC_T);
			else if (EEREC_D == EEREC_T) SSE_MINSS_XMM_to_XMM(EEREC_D, EEREC_S);
			else {
				SSE_MOVSS_XMM_to_XMM(EEREC_D, EEREC_S);
				SSE_MINSS_XMM_to_XMM(EEREC_D, EEREC_T);
			}
		}
		else if (_X_Y_Z_W != 0xf) {
			SSE_MOVAPS_XMM_to_XMM(EEREC_TEMP, EEREC_S);
			SSE_MINPS_XMM_to_XMM(EEREC_TEMP, EEREC_T);

			VU_MERGE_REGS(EEREC_D, EEREC_TEMP);
		}
		else {
			if( EEREC_D == EEREC_S ) {
				//ClampUnordered(EEREC_T, EEREC_TEMP, 0); // need for GT4 vu0rec
				SSE_MINPS_XMM_to_XMM(EEREC_D, EEREC_T);
			}
			else if( EEREC_D == EEREC_T ) {
				//ClampUnordered(EEREC_S, EEREC_TEMP, 0); // need for GT4 vu0rec
				SSE_MINPS_XMM_to_XMM(EEREC_D, EEREC_S);
			}
			else {
				SSE_MOVAPS_XMM_to_XMM(EEREC_D, EEREC_S);
				SSE_MINPS_XMM_to_XMM(EEREC_D, EEREC_T);
			}
		}
	}
}

void recVUMI_MINI_iq(VURegs *VU, uptr addr, int info)
{
	if ( _Fd_ == 0 ) return;
	//Console.WriteLn ("recVUMI_MINI_iq");

	if (MINMAXFIX)
		MINMAXlogical(VU, info, 1, 1, addr);
	else
	{

		if (_Fs_) vuFloat4_useEAX( EEREC_S, EEREC_TEMP, _X_Y_Z_W ); // Always do Preserved Sign Clamping
		vuFloat3(addr);

		if( _XYZW_SS ) {
			if( EEREC_D == EEREC_TEMP ) {
				_vuFlipRegSS(VU, EEREC_S);
				SSE_MOVSS_XMM_to_XMM(EEREC_D, EEREC_S);
				SSE_MINSS_M32_to_XMM(EEREC_D, addr);
				_vuFlipRegSS(VU, EEREC_S);

				// have to flip over EEREC_D if computing flags!
				//if( (info & PROCESS_VU_UPDATEFLAGS) )
					_vuFlipRegSS(VU, EEREC_D);
			}
			else if( EEREC_D == EEREC_S ) {
				_vuFlipRegSS(VU, EEREC_D);
				SSE_MINSS_M32_to_XMM(EEREC_D, addr);
				_vuFlipRegSS(VU, EEREC_D);
			}
			else {
				if( _X ) {
					SSE_MOVSS_XMM_to_XMM(EEREC_D, EEREC_S);
					SSE_MINSS_M32_to_XMM(EEREC_D, addr);
				}
				else {
					_vuMoveSS(VU, EEREC_TEMP, EEREC_S);
					_vuFlipRegSS(VU, EEREC_D);
					SSE_MINSS_M32_to_XMM(EEREC_TEMP, addr);
					SSE_MOVSS_XMM_to_XMM(EEREC_D, EEREC_TEMP);
					_vuFlipRegSS(VU, EEREC_D);
				}
			}
		}
		else if (_X_Y_Z_W != 0xf) {
			SSE_MOVSS_M32_to_XMM(EEREC_TEMP, addr);
			SSE_SHUFPS_XMM_to_XMM(EEREC_TEMP, EEREC_TEMP, 0x00);
			SSE_MINPS_XMM_to_XMM(EEREC_TEMP, EEREC_S);
			VU_MERGE_REGS(EEREC_D, EEREC_TEMP);
		}
		else {
			if(EEREC_D == EEREC_S) {
				SSE_MOVSS_M32_to_XMM(EEREC_TEMP, addr);
				SSE_SHUFPS_XMM_to_XMM(EEREC_TEMP, EEREC_TEMP, 0x00);
				SSE_MINPS_XMM_to_XMM(EEREC_D, EEREC_TEMP);
			}
			else {
				SSE_MOVSS_M32_to_XMM(EEREC_D, addr);
				SSE_SHUFPS_XMM_to_XMM(EEREC_D, EEREC_D, 0x00);
				SSE_MINPS_XMM_to_XMM(EEREC_D, EEREC_S);
			}
		}
	}
}

void recVUMI_MINI_xyzw(VURegs *VU, int xyzw, int info)
{
	if ( _Fd_ == 0 ) return;
	//Console.WriteLn ("recVUMI_MINI_xyzw");

	if (_Fs_ == 0 && _Ft_ == 0)
	{
		if( _X_Y_Z_W == 0xf )
		{
			//If VF0.w is the constant, the result will match VF0, else its all 0's
			if(xyzw == 3) SSE_MOVAPS_M128_to_XMM(EEREC_D, (uptr)&VU->VF[0].UL[0]);
			else SSE_XORPS_XMM_to_XMM(EEREC_D, EEREC_D);
		}
		else
		{
			//If VF0.w is the constant, the result will match VF0, else its all 0's
			if(xyzw == 3) SSE_MOVAPS_M128_to_XMM(EEREC_TEMP, (uptr)&VU->VF[0].UL[0]);
			else SSE_XORPS_XMM_to_XMM(EEREC_TEMP, EEREC_TEMP);
			VU_MERGE_REGS(EEREC_D, EEREC_TEMP);
		}
		return;
	}
	if (MINMAXFIX)
		MINMAXlogical(VU, info, 1, 2, 0, xyzw);
	else
	{
		if (_Fs_) vuFloat4_useEAX( EEREC_S, EEREC_TEMP, _X_Y_Z_W ); // Always do Preserved Sign Clamping
		if (_Ft_) vuFloat4_useEAX( EEREC_T, EEREC_TEMP, ( 1 << (3 - xyzw) ) );

		if( _X_Y_Z_W == 8 && (EEREC_D != EEREC_TEMP)) {
			if( xyzw == 0 ) {
				if( EEREC_D == EEREC_S ) SSE_MINSS_XMM_to_XMM(EEREC_D, EEREC_T);
				else if( EEREC_D == EEREC_T ) SSE_MINSS_XMM_to_XMM(EEREC_D, EEREC_S);
				else {
					SSE_MOVSS_XMM_to_XMM(EEREC_D, EEREC_S);
					SSE_MINSS_XMM_to_XMM(EEREC_D, EEREC_T);
				}
			}
			else {
				_unpackVFSS_xyzw(EEREC_TEMP, EEREC_T, xyzw);
				SSE_MOVSS_XMM_to_XMM(EEREC_D, EEREC_S);
				SSE_MINSS_XMM_to_XMM(EEREC_D, EEREC_TEMP);
			}
		}
		else if (_X_Y_Z_W != 0xf) {
			_unpackVF_xyzw(EEREC_TEMP, EEREC_T, xyzw);
			SSE_MINPS_XMM_to_XMM(EEREC_TEMP, EEREC_S);
			VU_MERGE_REGS(EEREC_D, EEREC_TEMP);
		}
		else {
			if (EEREC_D == EEREC_S) {
				_unpackVF_xyzw(EEREC_TEMP, EEREC_T, xyzw);
				SSE_MINPS_XMM_to_XMM(EEREC_D, EEREC_TEMP);
			}
			else {
				_unpackVF_xyzw(EEREC_D, EEREC_T, xyzw);
				SSE_MINPS_XMM_to_XMM(EEREC_D, EEREC_S);
			}
		}
	}
}

void recVUMI_MINIi(VURegs *VU, int info) { recVUMI_MINI_iq(VU, VU_VI_ADDR(REG_I, 1), info); }
void recVUMI_MINIx(VURegs *VU, int info) { recVUMI_MINI_xyzw(VU, 0, info); }
void recVUMI_MINIy(VURegs *VU, int info) { recVUMI_MINI_xyzw(VU, 1, info); }
void recVUMI_MINIz(VURegs *VU, int info) { recVUMI_MINI_xyzw(VU, 2, info); }
void recVUMI_MINIw(VURegs *VU, int info) { recVUMI_MINI_xyzw(VU, 3, info); }
//------------------------------------------------------------------


//------------------------------------------------------------------
// OPMULA
//------------------------------------------------------------------
void recVUMI_OPMULA( VURegs *VU, int info )
{
	//Console.WriteLn ("recVUMI_OPMULA");
	if (CHECK_VU_EXTRA_OVERFLOW) {
		if (_Fs_) vuFloat5_useEAX( EEREC_S, EEREC_TEMP, 0xE);
		if (_Ft_) vuFloat5_useEAX( EEREC_T, EEREC_TEMP, 0xE);
	}

	SSE_MOVAPS_XMM_to_XMM( EEREC_TEMP, EEREC_S );
	SSE_SHUFPS_XMM_to_XMM( EEREC_T, EEREC_T, 0xD2 );		// EEREC_T = WYXZ
	SSE_SHUFPS_XMM_to_XMM( EEREC_TEMP, EEREC_TEMP, 0xC9 );	// EEREC_TEMP = WXZY
	SSE_MULPS_XMM_to_XMM( EEREC_TEMP, EEREC_T );

	VU_MERGE_REGS_CUSTOM(EEREC_ACC, EEREC_TEMP, 14);

	// revert EEREC_T
	if( EEREC_T != EEREC_ACC )
		SSE_SHUFPS_XMM_to_XMM(EEREC_T, EEREC_T, 0xC9);

	recUpdateFlags(VU, EEREC_ACC, info);
}
//------------------------------------------------------------------


//------------------------------------------------------------------
// OPMSUB
//------------------------------------------------------------------
void recVUMI_OPMSUB( VURegs *VU, int info )
{
	//Console.WriteLn ("recVUMI_OPMSUB");
	if (CHECK_VU_EXTRA_OVERFLOW) {
		if (_Fs_) vuFloat5_useEAX( EEREC_S, EEREC_TEMP, 0xE);
		if (_Ft_) vuFloat5_useEAX( EEREC_T, EEREC_TEMP, 0xE);
	}

	if( !_Fd_ ) info |= PROCESS_EE_SET_D(EEREC_TEMP);
	SSE_MOVAPS_XMM_to_XMM(EEREC_TEMP, EEREC_S);
	SSE_SHUFPS_XMM_to_XMM(EEREC_T, EEREC_T, 0xD2);			// EEREC_T = WYXZ
	SSE_SHUFPS_XMM_to_XMM(EEREC_TEMP, EEREC_TEMP, 0xC9);	// EEREC_TEMP = WXZY
	SSE_MULPS_XMM_to_XMM(EEREC_TEMP, EEREC_T);

	// negate and add
	SSE_XORPS_M128_to_XMM(EEREC_TEMP, (uptr)&const_clip[4]);
	SSE_ADDPS_XMM_to_XMM(EEREC_TEMP, EEREC_ACC);
	VU_MERGE_REGS_CUSTOM(EEREC_D, EEREC_TEMP, 14);

	// revert EEREC_T
	if( EEREC_T != EEREC_D ) SSE_SHUFPS_XMM_to_XMM(EEREC_T, EEREC_T, 0xC9);

	recUpdateFlags(VU, EEREC_D, info);
}
//------------------------------------------------------------------


//------------------------------------------------------------------
// NOP
//------------------------------------------------------------------
void recVUMI_NOP( VURegs *VU, int info )
{
	//Console.WriteLn ("recVUMI_NOP");
}
//------------------------------------------------------------------


//------------------------------------------------------------------
// recVUMI_FTOI_Saturate() - Saturates result from FTOI Instructions
//------------------------------------------------------------------

// unused, but leaving here for possible reference..
//static const __aligned16 int rec_const_0x8000000[4] = { 0x80000000, 0x80000000, 0x80000000, 0x80000000 };

void recVUMI_FTOI_Saturate(int rec_s, int rec_t, int rec_tmp1, int rec_tmp2)
{
	//Console.WriteLn ("recVUMI_FTOI_Saturate");
	//Duplicate the xor'd sign bit to the whole value
	//FFFF FFFF for positive,  0 for negative
	SSE_MOVAPS_XMM_to_XMM(rec_tmp1, rec_s);
	SSE2_PXOR_M128_to_XMM(rec_tmp1, (uptr)&const_clip[4]);
	SSE2_PSRAD_I8_to_XMM(rec_tmp1, 31);

	//Create mask: 0 where !=8000 0000
	SSE_MOVAPS_XMM_to_XMM(rec_tmp2, rec_t);
	SSE2_PCMPEQD_M128_to_XMM(rec_tmp2, (uptr)&const_clip[4]);

	//AND the mask w/ the edit values
	SSE_ANDPS_XMM_to_XMM(rec_tmp1, rec_tmp2);

	//if v==8000 0000 && positive -> 8000 0000 + FFFF FFFF -> 7FFF FFFF
	//if v==8000 0000 && negative -> 8000 0000 + 0 -> 8000 0000
	//if v!=8000 0000 -> v+0 (masked from the and)

	//Add the values as needed
	SSE2_PADDD_XMM_to_XMM(rec_t, rec_tmp1);
}
//------------------------------------------------------------------


//------------------------------------------------------------------
// FTOI 0/4/12/15
//------------------------------------------------------------------
static __aligned16 float FTIO_Temp1[4] = { 0x00000000, 0x00000000, 0x00000000, 0x00000000 };
static __aligned16 float FTIO_Temp2[4] = { 0x00000000, 0x00000000, 0x00000000, 0x00000000 };
void recVUMI_FTOI0(VURegs *VU, int info)
{
	int t1reg, t2reg; // Temp XMM regs

	if ( _Ft_ == 0 ) return;

	//Console.WriteLn ("recVUMI_FTOI0");

	if (_X_Y_Z_W != 0xf) {
		SSE_MOVAPS_XMM_to_XMM(EEREC_TEMP, EEREC_S);
		vuFloat_useEAX( info, EEREC_TEMP, 0xf ); // Clamp Infs and NaNs to pos/neg fmax (NaNs always to positive fmax)
		SSE2_CVTTPS2DQ_XMM_to_XMM(EEREC_TEMP, EEREC_TEMP);

		t1reg = _vuGetTempXMMreg(info);

		if( t1reg >= 0 ) { // If theres a temp XMM reg available
			for (t2reg = 0; ( (t2reg == EEREC_S) || (t2reg == EEREC_T) || (t2reg == EEREC_TEMP) || (t2reg == t1reg) ); t2reg++)
				; // Find unused reg (For second temp reg)
			SSE_MOVAPS_XMM_to_M128((uptr)FTIO_Temp1, t2reg); // Backup XMM reg

			recVUMI_FTOI_Saturate(EEREC_S, EEREC_TEMP, t1reg, t2reg); // Saturate if Float->Int conversion returned illegal result

			SSE_MOVAPS_M128_to_XMM(t2reg, (uptr)FTIO_Temp1); // Restore XMM reg
			_freeXMMreg(t1reg); // Free temp reg
		}
		else { // No temp reg available
			for (t1reg = 0; ( (t1reg == EEREC_S) || (t1reg == EEREC_T) || (t1reg == EEREC_TEMP) ); t1reg++)
				; // Find unused reg (For first temp reg)
			SSE_MOVAPS_XMM_to_M128((uptr)FTIO_Temp1, t1reg); // Backup t1reg XMM reg

			for (t2reg = 0; ( (t2reg == EEREC_S) || (t2reg == EEREC_T) || (t2reg == EEREC_TEMP) || (t2reg == t1reg) ); t2reg++)
				; // Find unused reg (For second temp reg)
			SSE_MOVAPS_XMM_to_M128((uptr)FTIO_Temp2, t2reg); // Backup t2reg XMM reg

			recVUMI_FTOI_Saturate(EEREC_S, EEREC_TEMP, t1reg, t2reg); // Saturate if Float->Int conversion returned illegal result

			SSE_MOVAPS_M128_to_XMM(t1reg, (uptr)FTIO_Temp1); // Restore t1reg XMM reg
			SSE_MOVAPS_M128_to_XMM(t2reg, (uptr)FTIO_Temp2); // Restore t2reg XMM reg
		}

		VU_MERGE_REGS(EEREC_T, EEREC_TEMP);
	}
	else {
		if (EEREC_T != EEREC_S) {
			SSE_MOVAPS_XMM_to_XMM(EEREC_T, EEREC_S);
			vuFloat_useEAX( info, EEREC_T, 0xf ); // Clamp Infs and NaNs to pos/neg fmax (NaNs always to positive fmax)
			SSE2_CVTTPS2DQ_XMM_to_XMM(EEREC_T, EEREC_T);

			t1reg = _vuGetTempXMMreg(info);

			if( t1reg >= 0 ) { // If theres a temp XMM reg available
				recVUMI_FTOI_Saturate(EEREC_S, EEREC_T, EEREC_TEMP, t1reg); // Saturate if Float->Int conversion returned illegal result
				_freeXMMreg(t1reg); // Free temp reg
			}
			else { // No temp reg available
				for (t1reg = 0; ( (t1reg == EEREC_S) || (t1reg == EEREC_T) || (t1reg == EEREC_TEMP) ); t1reg++)
					; // Find unused reg
				SSE_MOVAPS_XMM_to_M128((uptr)FTIO_Temp1, t1reg); // Backup t1reg XMM reg

				recVUMI_FTOI_Saturate(EEREC_S, EEREC_T, EEREC_TEMP, t1reg); // Saturate if Float->Int conversion returned illegal result

				SSE_MOVAPS_M128_to_XMM(t1reg, (uptr)FTIO_Temp1); // Restore t1reg XMM reg
			}
		}
		else {
			SSE_MOVAPS_XMM_to_XMM(EEREC_TEMP, EEREC_S);
			vuFloat_useEAX( info, EEREC_TEMP, 0xf ); // Clamp Infs and NaNs to pos/neg fmax (NaNs always to positive fmax)
			SSE2_CVTTPS2DQ_XMM_to_XMM(EEREC_TEMP, EEREC_TEMP);

			t1reg = _vuGetTempXMMreg(info);

			if( t1reg >= 0 ) { // If theres a temp XMM reg available
				for (t2reg = 0; ( (t2reg == EEREC_S) || (t2reg == EEREC_T) || (t2reg == EEREC_TEMP) || (t2reg == t1reg)); t2reg++)
					; // Find unused reg (For second temp reg)
				SSE_MOVAPS_XMM_to_M128((uptr)FTIO_Temp1, t2reg); // Backup XMM reg

				recVUMI_FTOI_Saturate(EEREC_S, EEREC_TEMP, t1reg, t2reg); // Saturate if Float->Int conversion returned illegal result

				SSE_MOVAPS_M128_to_XMM(t2reg, (uptr)FTIO_Temp1); // Restore XMM reg
				_freeXMMreg(t1reg); // Free temp reg
			}
			else { // No temp reg available
				for (t1reg = 0; ( (t1reg == EEREC_S) || (t1reg == EEREC_T) || (t1reg == EEREC_TEMP) ); t1reg++)
					; // Find unused reg (For first temp reg)
				SSE_MOVAPS_XMM_to_M128((uptr)FTIO_Temp1, t1reg); // Backup t1reg XMM reg

				for (t2reg = 0; ( (t2reg == EEREC_S) || (t2reg == EEREC_T) || (t2reg == EEREC_TEMP) || (t2reg == t1reg) ); t2reg++)
					; // Find unused reg (For second temp reg)
				SSE_MOVAPS_XMM_to_M128((uptr)FTIO_Temp2, t2reg); // Backup t2reg XMM reg

				recVUMI_FTOI_Saturate(EEREC_S, EEREC_TEMP, t1reg, t2reg); // Saturate if Float->Int conversion returned illegal result

				SSE_MOVAPS_M128_to_XMM(t1reg, (uptr)FTIO_Temp1); // Restore t1reg XMM reg
				SSE_MOVAPS_M128_to_XMM(t2reg, (uptr)FTIO_Temp2); // Restore t2reg XMM reg
			}

			SSE_MOVAPS_XMM_to_XMM(EEREC_T, EEREC_TEMP);
		}
	}
}

void recVUMI_FTOIX(VURegs *VU, int addr, int info)
{
	int t1reg, t2reg; // Temp XMM regs

	if ( _Ft_ == 0 ) return;

	//Console.WriteLn ("recVUMI_FTOIX");
	if (_X_Y_Z_W != 0xf) {
		SSE_MOVAPS_XMM_to_XMM(EEREC_TEMP, EEREC_S);
		SSE_MULPS_M128_to_XMM(EEREC_TEMP, addr);
		vuFloat_useEAX( info, EEREC_TEMP, 0xf ); // Clamp Infs and NaNs to pos/neg fmax (NaNs always to positive fmax)
		SSE2_CVTTPS2DQ_XMM_to_XMM(EEREC_TEMP, EEREC_TEMP);

		t1reg = _vuGetTempXMMreg(info);

		if( t1reg >= 0 ) { // If theres a temp XMM reg available
			for (t2reg = 0; ( (t2reg == EEREC_S) || (t2reg == EEREC_T) || (t2reg == EEREC_TEMP)  || (t2reg == t1reg)); t2reg++)
				; // Find unused reg (For second temp reg)
			SSE_MOVAPS_XMM_to_M128((uptr)FTIO_Temp1, t2reg); // Backup XMM reg

			recVUMI_FTOI_Saturate(EEREC_S, EEREC_TEMP, t1reg, t2reg); // Saturate if Float->Int conversion returned illegal result

			SSE_MOVAPS_M128_to_XMM(t2reg, (uptr)FTIO_Temp1); // Restore XMM reg
			_freeXMMreg(t1reg); // Free temp reg
		}
		else { // No temp reg available
			for (t1reg = 0; ( (t1reg == EEREC_S) || (t1reg == EEREC_T) || (t1reg == EEREC_TEMP) ); t1reg++)
				; // Find unused reg (For first temp reg)
			SSE_MOVAPS_XMM_to_M128((uptr)FTIO_Temp1, t1reg); // Backup t1reg XMM reg

			for (t2reg = 0; ( (t2reg == EEREC_S) || (t2reg == EEREC_T) || (t2reg == EEREC_TEMP) || (t2reg == t1reg) ); t2reg++)
				; // Find unused reg (For second temp reg)
			SSE_MOVAPS_XMM_to_M128((uptr)FTIO_Temp2, t2reg); // Backup t2reg XMM reg

			recVUMI_FTOI_Saturate(EEREC_S, EEREC_TEMP, t1reg, t2reg); // Saturate if Float->Int conversion returned illegal result

			SSE_MOVAPS_M128_to_XMM(t1reg, (uptr)FTIO_Temp1); // Restore t1reg XMM reg
			SSE_MOVAPS_M128_to_XMM(t2reg, (uptr)FTIO_Temp2); // Restore t2reg XMM reg
		}

		VU_MERGE_REGS(EEREC_T, EEREC_TEMP);
	}
	else {
		if (EEREC_T != EEREC_S) {
			SSE_MOVAPS_XMM_to_XMM(EEREC_T, EEREC_S);
			SSE_MULPS_M128_to_XMM(EEREC_T, addr);
			vuFloat_useEAX( info, EEREC_T, 0xf ); // Clamp Infs and NaNs to pos/neg fmax (NaNs always to positive fmax)
			SSE2_CVTTPS2DQ_XMM_to_XMM(EEREC_T, EEREC_T);

			t1reg = _vuGetTempXMMreg(info);

			if( t1reg >= 0 ) { // If theres a temp XMM reg available
				recVUMI_FTOI_Saturate(EEREC_S, EEREC_T, EEREC_TEMP, t1reg); // Saturate if Float->Int conversion returned illegal result
				_freeXMMreg(t1reg); // Free temp reg
			}
			else { // No temp reg available
				for (t1reg = 0; ( (t1reg == EEREC_S) || (t1reg == EEREC_T) || (t1reg == EEREC_TEMP) ); t1reg++)
					; // Find unused reg
				SSE_MOVAPS_XMM_to_M128((uptr)FTIO_Temp1, t1reg); // Backup t1reg XMM reg

				recVUMI_FTOI_Saturate(EEREC_S, EEREC_T, EEREC_TEMP, t1reg); // Saturate if Float->Int conversion returned illegal result

				SSE_MOVAPS_M128_to_XMM(t1reg, (uptr)FTIO_Temp1); // Restore t1reg XMM reg
			}
		}
		else {
			SSE_MOVAPS_XMM_to_XMM(EEREC_TEMP, EEREC_S);
			SSE_MULPS_M128_to_XMM(EEREC_TEMP, addr);
			vuFloat_useEAX( info, EEREC_TEMP, 0xf ); // Clamp Infs and NaNs to pos/neg fmax (NaNs always to positive fmax)
			SSE2_CVTTPS2DQ_XMM_to_XMM(EEREC_TEMP, EEREC_TEMP);

			t1reg = _vuGetTempXMMreg(info);

			if( t1reg >= 0 ) { // If theres a temp XMM reg available
				for (t2reg = 0; ( (t2reg == EEREC_S) || (t2reg == EEREC_T) || (t2reg == EEREC_TEMP) || (t2reg == t1reg)); t2reg++)
					; // Find unused reg (For second temp reg)
				SSE_MOVAPS_XMM_to_M128((uptr)FTIO_Temp1, t2reg); // Backup XMM reg

				recVUMI_FTOI_Saturate(EEREC_S, EEREC_TEMP, t1reg, t2reg); // Saturate if Float->Int conversion returned illegal result

				SSE_MOVAPS_M128_to_XMM(t2reg, (uptr)FTIO_Temp1); // Restore XMM reg
				_freeXMMreg(t1reg); // Free temp reg
			}
			else { // No temp reg available
				for (t1reg = 0; ( (t1reg == EEREC_S) || (t1reg == EEREC_T) || (t1reg == EEREC_TEMP) ); t1reg++)
					; // Find unused reg (For first temp reg)
				SSE_MOVAPS_XMM_to_M128((uptr)FTIO_Temp1, t1reg); // Backup t1reg XMM reg

				for (t2reg = 0; ( (t2reg == EEREC_S) || (t2reg == EEREC_T) || (t2reg == EEREC_TEMP) || (t2reg == t1reg) ); t2reg++)
					; // Find unused reg (For second temp reg)
				SSE_MOVAPS_XMM_to_M128((uptr)FTIO_Temp2, t2reg); // Backup t2reg XMM reg

				recVUMI_FTOI_Saturate(EEREC_S, EEREC_TEMP, t1reg, t2reg); // Saturate if Float->Int conversion returned illegal result

				SSE_MOVAPS_M128_to_XMM(t1reg, (uptr)FTIO_Temp1); // Restore t1reg XMM reg
				SSE_MOVAPS_M128_to_XMM(t2reg, (uptr)FTIO_Temp2); // Restore t2reg XMM reg
			}

			SSE_MOVAPS_XMM_to_XMM(EEREC_T, EEREC_TEMP);
		}
	}
}

void recVUMI_FTOI4( VURegs *VU, int info ) { recVUMI_FTOIX(VU, (uptr)&recMult_float_to_int4[0], info); }
void recVUMI_FTOI12( VURegs *VU, int info ) { recVUMI_FTOIX(VU, (uptr)&recMult_float_to_int12[0], info); }
void recVUMI_FTOI15( VURegs *VU, int info ) { recVUMI_FTOIX(VU, (uptr)&recMult_float_to_int15[0], info); }
//------------------------------------------------------------------


//------------------------------------------------------------------
// ITOF 0/4/12/15
//------------------------------------------------------------------
void recVUMI_ITOF0( VURegs *VU, int info )
{
	if ( _Ft_ == 0 ) return;

	//Console.WriteLn ("recVUMI_ITOF0");
	if (_X_Y_Z_W != 0xf) {
		SSE2_CVTDQ2PS_XMM_to_XMM(EEREC_TEMP, EEREC_S);
		vuFloat_useEAX( info, EEREC_TEMP, 15); // Clamp infinities
		VU_MERGE_REGS(EEREC_T, EEREC_TEMP);
		xmmregs[EEREC_T].mode |= MODE_WRITE;
	}
	else {
		SSE2_CVTDQ2PS_XMM_to_XMM(EEREC_T, EEREC_S);
		vuFloat2(EEREC_T, EEREC_TEMP, 15); // Clamp infinities
	}
}

void recVUMI_ITOFX(VURegs *VU, int addr, int info)
{
	if ( _Ft_ == 0 ) return;

	//Console.WriteLn ("recVUMI_ITOFX");
	if (_X_Y_Z_W != 0xf) {
		SSE2_CVTDQ2PS_XMM_to_XMM(EEREC_TEMP, EEREC_S);
		SSE_MULPS_M128_to_XMM(EEREC_TEMP, addr);
		vuFloat_useEAX( info, EEREC_TEMP, 15); // Clamp infinities
		VU_MERGE_REGS(EEREC_T, EEREC_TEMP);
		xmmregs[EEREC_T].mode |= MODE_WRITE;
	}
	else {
		SSE2_CVTDQ2PS_XMM_to_XMM(EEREC_T, EEREC_S);
		SSE_MULPS_M128_to_XMM(EEREC_T, addr);
		vuFloat2(EEREC_T, EEREC_TEMP, 15); // Clamp infinities
	}
}

void recVUMI_ITOF4( VURegs *VU, int info ) { recVUMI_ITOFX(VU, (uptr)&recMult_int_to_float4[0], info); }
void recVUMI_ITOF12( VURegs *VU, int info ) { recVUMI_ITOFX(VU, (uptr)&recMult_int_to_float12[0], info); }
void recVUMI_ITOF15( VURegs *VU, int info ) { recVUMI_ITOFX(VU, (uptr)&recMult_int_to_float15[0], info); }
//------------------------------------------------------------------


//------------------------------------------------------------------
// CLIP
//------------------------------------------------------------------
void recVUMI_CLIP(VURegs *VU, int info)
{
	int t1reg = EEREC_D;
	int t2reg = EEREC_ACC;
	int x86temp1, x86temp2;

	u32 clipaddr = VU_VI_ADDR(REG_CLIP_FLAG, 0);
	u32 prevclipaddr = VU_VI_ADDR(REG_CLIP_FLAG, 2);

	if( clipaddr == 0 ) { // battle star has a clip right before fcset
		Console.WriteLn("skipping vu clip");
		return;
	}

	//Flush the clip flag before processing, incase of double clip commands (GoW)

	if( prevclipaddr != (uptr)&VU->VI[REG_CLIP_FLAG] ) {
		MOV32MtoR(EAX, prevclipaddr);
		MOV32RtoM((uptr)&VU->VI[REG_CLIP_FLAG], EAX);
	}

	assert( clipaddr != 0 );
	assert( t1reg != t2reg && t1reg != EEREC_TEMP && t2reg != EEREC_TEMP );

	x86temp1 = ALLOCTEMPX86(MODE_8BITREG);
	x86temp2 = ALLOCTEMPX86(MODE_8BITREG);

	//if ( (x86temp1 == 0) || (x86temp2 == 0) ) Console.Error("VU CLIP Allocation Error: EAX being allocated!");

	_freeXMMreg(t1reg); // These should have been freed at allocation in eeVURecompileCode()
	_freeXMMreg(t2reg); // but if they've been used since then, then free them. (just doing this incase :p (cottonvibes))

	if( _Ft_ == 0 ) {
		SSE_MOVAPS_M128_to_XMM(EEREC_TEMP, (uptr)&s_fones[0]); // all 1s
		SSE_MOVAPS_M128_to_XMM(t1reg, (uptr)&s_fones[4]);
	}
	else {
		_unpackVF_xyzw(EEREC_TEMP, EEREC_T, 3);
		SSE_ANDPS_M128_to_XMM(EEREC_TEMP, (uptr)&const_clip[0]);
		SSE_MOVAPS_XMM_to_XMM(t1reg, EEREC_TEMP);
		SSE_ORPS_M128_to_XMM(t1reg, (uptr)&const_clip[4]);
	}

	MOV32MtoR(EAX, prevclipaddr);

	SSE_CMPNLEPS_XMM_to_XMM(t1reg, EEREC_S);  //-w, -z, -y, -x
	SSE_CMPLTPS_XMM_to_XMM(EEREC_TEMP, EEREC_S); //+w, +z, +y, +x

	SHL32ItoR(EAX, 6);

	SSE_MOVAPS_XMM_to_XMM(t2reg, EEREC_TEMP); //t2 = +w, +z, +y, +x
	SSE_UNPCKLPS_XMM_to_XMM(EEREC_TEMP, t1reg); //EEREC_TEMP = -y,+y,-x,+x
	SSE_UNPCKHPS_XMM_to_XMM(t2reg, t1reg); //t2reg = -w,+w,-z,+z
	SSE_MOVMSKPS_XMM_to_R32(x86temp2, EEREC_TEMP); // -y,+y,-x,+x
	SSE_MOVMSKPS_XMM_to_R32(x86temp1, t2reg); // -w,+w,-z,+z

	AND8ItoR(x86temp1, 0x3);
	SHL8ItoR(x86temp1, 4);
	OR8RtoR(EAX, x86temp1);
	AND8ItoR(x86temp2, 0xf);
	OR8RtoR(EAX, x86temp2);
	AND32ItoR(EAX, 0xffffff);

	MOV32RtoM(clipaddr, EAX);

	if (( !(info & (PROCESS_VU_SUPER|PROCESS_VU_COP2)) ) )  //Instantly update the flag if its called from elsewhere (unlikely, but ok)
		MOV32RtoM((uptr)&VU->VI[REG_CLIP_FLAG], EAX);

	_freeX86reg(x86temp1);
	_freeX86reg(x86temp2);
}
