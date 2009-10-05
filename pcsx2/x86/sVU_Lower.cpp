/*  PCSX2 - PS2 Emulator for PCs
 *  Copyright (C) 2002-2009  PCSX2 Dev Team
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
#include "sVU_Micro.h"
#include "sVU_Debug.h"
#include "sVU_zerorec.h"
//------------------------------------------------------------------

//------------------------------------------------------------------
// Helper Macros
//------------------------------------------------------------------
#define _Ft_ (( VU->code >> 16) & 0x1F)  // The rt part of the instruction register 
#define _Fs_ (( VU->code >> 11) & 0x1F)  // The rd part of the instruction register 
#define _Fd_ (( VU->code >>  6) & 0x1F)  // The sa part of the instruction register
#define _It_ (_Ft_ & 15)
#define _Is_ (_Fs_ & 15)
#define _Id_ (_Fd_ & 15)

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


static const __aligned16 u32 VU_ONE[4] = {0x3f800000, 0xffffffff, 0xffffffff, 0xffffffff};
//------------------------------------------------------------------


//------------------------------------------------------------------
// *VU Lower Instructions!*
//
// Note: * = Checked for errors by cottonvibes
//------------------------------------------------------------------


//------------------------------------------------------------------
// DIV*
//------------------------------------------------------------------
void recVUMI_DIV(VURegs *VU, int info)
{
	u8 *pjmp, *pjmp1;
	u32 *ajmp32, *bjmp32;

	//Console.WriteLn("recVUMI_DIV()");
	AND32ItoM(VU_VI_ADDR(REG_STATUS_FLAG, 2), 0xFCF); // Clear D/I flags

	// FT can be zero here! so we need to check if its zero and set the correct flag.
	SSE_XORPS_XMM_to_XMM(EEREC_TEMP, EEREC_TEMP); // Clear EEREC_TEMP
	SSE_CMPEQPS_XMM_to_XMM(EEREC_TEMP, EEREC_T); // Set all F's if each vector is zero

	SSE_MOVMSKPS_XMM_to_R32( EAX, EEREC_TEMP); // Move the sign bits of the previous calculation

	AND32ItoR( EAX, (1<<_Ftf_) );  // Grab "Is Zero" bits from the previous calculation
	ajmp32 = JZ32(0); // Skip if none are

		SSE_XORPS_XMM_to_XMM(EEREC_TEMP, EEREC_TEMP); // Clear EEREC_TEMP
		SSE_CMPEQPS_XMM_to_XMM(EEREC_TEMP, EEREC_S); // Set all F's if each vector is zero
		SSE_MOVMSKPS_XMM_to_R32(EAX, EEREC_TEMP); // Move the sign bits of the previous calculation

		AND32ItoR( EAX, (1<<_Fsf_) );  // Grab "Is Zero" bits from the previous calculation
		pjmp = JZ8(0);
			OR32ItoM( VU_VI_ADDR(REG_STATUS_FLAG, 2), 0x410 ); // Set invalid flag (0/0)
			pjmp1 = JMP8(0);
		x86SetJ8(pjmp);
			OR32ItoM( VU_VI_ADDR(REG_STATUS_FLAG, 2), 0x820 ); // Zero divide (only when not 0/0)
		x86SetJ8(pjmp1);

		_unpackVFSS_xyzw(EEREC_TEMP, EEREC_S, _Fsf_);

		_vuFlipRegSS_xyzw(EEREC_T, _Ftf_);
		SSE_XORPS_XMM_to_XMM(EEREC_TEMP, EEREC_T);
		_vuFlipRegSS_xyzw(EEREC_T, _Ftf_);

		SSE_ANDPS_M128_to_XMM(EEREC_TEMP, (uptr)&const_clip[4]);
		SSE_ORPS_M128_to_XMM(EEREC_TEMP, (uptr)&g_maxvals[0]); // If division by zero, then EEREC_TEMP = +/- fmax

		bjmp32 = JMP32(0);

	x86SetJ32(ajmp32);

	if (CHECK_VU_EXTRA_OVERFLOW) {
		vuFloat5_useEAX(EEREC_S, EEREC_TEMP, (1 << (3-_Fsf_)));
		vuFloat5_useEAX(EEREC_T, EEREC_TEMP, (1 << (3-_Ftf_)));
	}

	_unpackVFSS_xyzw(EEREC_TEMP, EEREC_S, _Fsf_);

	_vuFlipRegSS_xyzw(EEREC_T, _Ftf_);
	SSE_DIVSS_XMM_to_XMM(EEREC_TEMP, EEREC_T);
	_vuFlipRegSS_xyzw(EEREC_T, _Ftf_);

	vuFloat_useEAX(info, EEREC_TEMP, 0x8);
	
	x86SetJ32(bjmp32);

	SSE_MOVSS_XMM_to_M32(VU_VI_ADDR(REG_Q, 0), EEREC_TEMP);
}
//------------------------------------------------------------------


//------------------------------------------------------------------
// SQRT*
//------------------------------------------------------------------
void recVUMI_SQRT( VURegs *VU, int info )
{
	u8* pjmp;
	//Console.WriteLn("recVUMI_SQRT()");

	_unpackVFSS_xyzw(EEREC_TEMP, EEREC_T, _Ftf_);
	AND32ItoM(VU_VI_ADDR(REG_STATUS_FLAG, 2), 0xFCF); // Clear D/I flags

	/* Check for negative sqrt */
	SSE_MOVMSKPS_XMM_to_R32(EAX, EEREC_TEMP);
	AND32ItoR(EAX, 1);  //Check sign
	pjmp = JZ8(0); //Skip if none are
		OR32ItoM(VU_VI_ADDR(REG_STATUS_FLAG, 2), 0x410); // Invalid Flag - Negative number sqrt
	x86SetJ8(pjmp);

	SSE_ANDPS_M128_to_XMM(EEREC_TEMP, (uptr)const_clip); // Do a cardinal sqrt
	if (CHECK_VU_OVERFLOW) SSE_MINSS_M32_to_XMM(EEREC_TEMP, (uptr)g_maxvals); // Clamp infinities (only need to do positive clamp since EEREC_TEMP is positive)
	SSE_SQRTSS_XMM_to_XMM(EEREC_TEMP, EEREC_TEMP);
	SSE_MOVSS_XMM_to_M32(VU_VI_ADDR(REG_Q, 0), EEREC_TEMP);
}
//------------------------------------------------------------------


//------------------------------------------------------------------
// RSQRT*
//------------------------------------------------------------------
__aligned16 u64 RSQRT_TEMP_XMM[2];
void recVUMI_RSQRT(VURegs *VU, int info)
{
	u8 *ajmp8, *bjmp8;
	u8 *qjmp1, *qjmp2;
	int t1reg, t1boolean;
	//Console.WriteLn("recVUMI_RSQRT()");

	_unpackVFSS_xyzw(EEREC_TEMP, EEREC_T, _Ftf_);
	AND32ItoM(VU_VI_ADDR(REG_STATUS_FLAG, 2), 0xFCF); // Clear D/I flags

	/* Check for negative divide */
	SSE_MOVMSKPS_XMM_to_R32(EAX, EEREC_TEMP);
	AND32ItoR(EAX, 1);  //Check sign
	ajmp8 = JZ8(0); //Skip if none are
		OR32ItoM(VU_VI_ADDR(REG_STATUS_FLAG, 2), 0x410); // Invalid Flag - Negative number sqrt
	x86SetJ8(ajmp8);

	SSE_ANDPS_M128_to_XMM(EEREC_TEMP, (uptr)const_clip); // Do a cardinal sqrt
	if (CHECK_VU_OVERFLOW) SSE_MINSS_M32_to_XMM(EEREC_TEMP, (uptr)g_maxvals); // Clamp Infinities to Fmax
	SSE_SQRTSS_XMM_to_XMM(EEREC_TEMP, EEREC_TEMP);

	t1reg = _vuGetTempXMMreg(info);
	if( t1reg < 0 ) {
		for (t1reg = 0; ( (t1reg == EEREC_TEMP) || (t1reg == EEREC_S) ); t1reg++)
			; // Makes t1reg not be EEREC_TEMP or EEREC_S.
		SSE_MOVAPS_XMM_to_M128( (uptr)&RSQRT_TEMP_XMM[0], t1reg ); // backup data in t1reg to a temp address
		t1boolean = 1;
	}
	else t1boolean = 0;

	// Ft can still be zero here! so we need to check if its zero and set the correct flag.
	SSE_XORPS_XMM_to_XMM(t1reg, t1reg); // Clear t1reg
	SSE_CMPEQSS_XMM_to_XMM(t1reg, EEREC_TEMP); // Set all F's if each vector is zero

	SSE_MOVMSKPS_XMM_to_R32(EAX, t1reg); // Move the sign bits of the previous calculation

	AND32ItoR( EAX, 0x01 );  // Grab "Is Zero" bits from the previous calculation
	ajmp8 = JZ8(0); // Skip if none are

		//check for 0/0
		_unpackVFSS_xyzw(EEREC_TEMP, EEREC_S, _Fsf_);

		SSE_XORPS_XMM_to_XMM(t1reg, t1reg); // Clear EEREC_TEMP
		SSE_CMPEQPS_XMM_to_XMM(t1reg, EEREC_TEMP); // Set all F's if each vector is zero
		SSE_MOVMSKPS_XMM_to_R32(EAX, t1reg); // Move the sign bits of the previous calculation

		AND32ItoR( EAX, 0x01 );  // Grab "Is Zero" bits from the previous calculation
		qjmp1 = JZ8(0);
			OR32ItoM( VU_VI_ADDR(REG_STATUS_FLAG, 2), 0x410 ); // Set invalid flag (0/0)
			qjmp2 = JMP8(0);
		x86SetJ8(qjmp1);
			OR32ItoM( VU_VI_ADDR(REG_STATUS_FLAG, 2), 0x820 ); // Zero divide (only when not 0/0)
		x86SetJ8(qjmp2);

		SSE_ANDPS_M128_to_XMM(EEREC_TEMP, (uptr)&const_clip[4]);
		SSE_ORPS_M128_to_XMM(EEREC_TEMP, (uptr)&g_maxvals[0]); // If division by zero, then EEREC_TEMP = +/- fmax
		SSE_MOVSS_XMM_to_M32(VU_VI_ADDR(REG_Q, 0), EEREC_TEMP);
		bjmp8 = JMP8(0);
	x86SetJ8(ajmp8);

	_unpackVFSS_xyzw(t1reg, EEREC_S, _Fsf_);
	if (CHECK_VU_EXTRA_OVERFLOW) vuFloat_useEAX(info, t1reg, 0x8); // Clamp Infinities
	SSE_DIVSS_XMM_to_XMM(t1reg, EEREC_TEMP);
	vuFloat_useEAX(info, t1reg, 0x8);
	SSE_MOVSS_XMM_to_M32(VU_VI_ADDR(REG_Q, 0), t1reg);

	x86SetJ8(bjmp8);
			
	if (t1boolean) SSE_MOVAPS_M128_to_XMM( t1reg, (uptr)&RSQRT_TEMP_XMM[0] ); // restore t1reg data
	else _freeXMMreg(t1reg); // free t1reg
}
//------------------------------------------------------------------


//------------------------------------------------------------------
// _addISIMMtoIT() - Used in IADDI, IADDIU, and ISUBIU instructions
//------------------------------------------------------------------
void _addISIMMtoIT(VURegs *VU, s16 imm, int info)
{
	int isreg = -1, itreg;
	if (_It_ == 0) return;

	if( _Is_ == 0 ) {
		itreg = ALLOCVI(_It_, MODE_WRITE);
		MOV32ItoR(itreg, imm&0xffff);
		return;
	}

	ADD_VI_NEEDED(_It_);
	isreg = ALLOCVI(_Is_, MODE_READ);
	itreg = ALLOCVI(_It_, MODE_WRITE);

	if ( _It_ == _Is_ ) {
		if (imm != 0 ) ADD16ItoR(itreg, imm);
	} 
	else {
		if( imm ) {
			LEA32RtoR(itreg, isreg, imm);
			MOVZX32R16toR(itreg, itreg);
		}
		else MOV32RtoR(itreg, isreg);
	}
}
//------------------------------------------------------------------


//------------------------------------------------------------------
// IADDI
//------------------------------------------------------------------
void recVUMI_IADDI(VURegs *VU, int info)
{
	s16 imm;

	if ( _It_ == 0 ) return;
	//Console.WriteLn("recVUMI_IADDI");
	imm = ( VU->code >> 6 ) & 0x1f;
	imm = ( imm & 0x10 ? 0xfff0 : 0) | ( imm & 0xf );
	_addISIMMtoIT(VU, imm, info);
}
//------------------------------------------------------------------


//------------------------------------------------------------------
// IADDIU
//------------------------------------------------------------------
void recVUMI_IADDIU(VURegs *VU, int info)
{
	s16 imm;

	if ( _It_ == 0 ) return;
	//Console.WriteLn("recVUMI_IADDIU");
	imm = ( ( VU->code >> 10 ) & 0x7800 ) | ( VU->code & 0x7ff );
	_addISIMMtoIT(VU, imm, info);
}
//------------------------------------------------------------------


//------------------------------------------------------------------
// IADD
//------------------------------------------------------------------
void recVUMI_IADD( VURegs *VU, int info )
{
	int idreg, isreg = -1, itreg = -1;
	if ( _Id_ == 0 ) return;
	//Console.WriteLn("recVUMI_IADD");
	if ( ( _It_ == 0 ) && ( _Is_ == 0 ) ) {
		idreg = ALLOCVI(_Id_, MODE_WRITE);
		XOR32RtoR(idreg, idreg);
		return;
	}

	ADD_VI_NEEDED(_Is_);
	ADD_VI_NEEDED(_It_);
	idreg = ALLOCVI(_Id_, MODE_WRITE);

	if ( _Is_ == 0 )
	{
		if( (itreg = _checkX86reg(X86TYPE_VI|((VU==&VU1)?X86TYPE_VU1:0), _It_, MODE_READ)) >= 0 ) {
			if( idreg != itreg ) MOV32RtoR(idreg, itreg);
		}
		else MOVZX32M16toR(idreg, VU_VI_ADDR(_It_, 1));
	}
	else if ( _It_ == 0 )
	{
		if( (isreg = _checkX86reg(X86TYPE_VI|((VU==&VU1)?X86TYPE_VU1:0), _Is_, MODE_READ)) >= 0 ) {
			if( idreg != isreg ) MOV32RtoR(idreg, isreg);
		}
		else MOVZX32M16toR(idreg, VU_VI_ADDR(_Is_, 1));
	}
	else {
		//ADD_VI_NEEDED(_It_);
		isreg = ALLOCVI(_Is_, MODE_READ);
		itreg = ALLOCVI(_It_, MODE_READ);

		if( idreg == isreg ) ADD32RtoR(idreg, itreg);
		else if( idreg == itreg ) ADD32RtoR(idreg, isreg);
		else LEA32RRtoR(idreg, isreg, itreg);
		MOVZX32R16toR(idreg, idreg); // needed since don't know if idreg's upper bits are 0
	}
}
//------------------------------------------------------------------


//------------------------------------------------------------------
// IAND
//------------------------------------------------------------------
void recVUMI_IAND( VURegs *VU, int info )
{
	int idreg, isreg = -1, itreg = -1;
	if ( _Id_ == 0 ) return;
	//Console.WriteLn("recVUMI_IAND");
	if ( ( _Is_ == 0 ) || ( _It_ == 0 ) ) {
		idreg = ALLOCVI(_Id_, MODE_WRITE);
		XOR32RtoR(idreg, idreg);
		return;
	}

	ADD_VI_NEEDED(_Is_);
	ADD_VI_NEEDED(_It_);
	idreg = ALLOCVI(_Id_, MODE_WRITE);

	isreg = ALLOCVI(_Is_, MODE_READ);
	itreg = ALLOCVI(_It_, MODE_READ);

	if( idreg == isreg ) AND16RtoR(idreg, itreg);
	else if( idreg == itreg ) AND16RtoR(idreg, isreg);
	else {
		MOV32RtoR(idreg, itreg);
		AND32RtoR(idreg, isreg);
	}
}
//------------------------------------------------------------------


//------------------------------------------------------------------
// IOR
//------------------------------------------------------------------
void recVUMI_IOR( VURegs *VU, int info )
{
	int idreg, isreg = -1, itreg = -1;
	if ( _Id_ == 0 ) return;
	//Console.WriteLn("recVUMI_IOR");
	if ( ( _It_ == 0 ) && ( _Is_ == 0 ) ) {
		idreg = ALLOCVI(_Id_, MODE_WRITE);
		XOR32RtoR(idreg, idreg);
		return;
	} 

	ADD_VI_NEEDED(_Is_);
	ADD_VI_NEEDED(_It_);
	idreg = ALLOCVI(_Id_, MODE_WRITE);

	if ( _Is_ == 0 )
	{
		if( (itreg = _checkX86reg(X86TYPE_VI|((VU==&VU1)?X86TYPE_VU1:0), _It_, MODE_READ)) >= 0 ) {
			if( idreg != itreg ) MOV32RtoR(idreg, itreg);
		}
		else MOVZX32M16toR(idreg, VU_VI_ADDR(_It_, 1));
	}
	else if ( _It_ == 0 )
	{
		if( (isreg = _checkX86reg(X86TYPE_VI|((VU==&VU1)?X86TYPE_VU1:0), _Is_, MODE_READ)) >= 0 ) {
			if( idreg != isreg ) MOV32RtoR(idreg, isreg);
		}
		else MOVZX32M16toR(idreg, VU_VI_ADDR(_Is_, 1));
	}
	else
	{
		isreg = ALLOCVI(_Is_, MODE_READ);
		itreg = ALLOCVI(_It_, MODE_READ);

		if( idreg == isreg ) OR16RtoR(idreg, itreg);
		else if( idreg == itreg ) OR16RtoR(idreg, isreg);
		else {
			MOV32RtoR(idreg, isreg);
			OR32RtoR(idreg, itreg);
		}
	}
}
//------------------------------------------------------------------


//------------------------------------------------------------------
// ISUB
//------------------------------------------------------------------
void recVUMI_ISUB( VURegs *VU, int info )
{
	int idreg, isreg = -1, itreg = -1;
	if ( _Id_ == 0 ) return;
	//Console.WriteLn("recVUMI_ISUB");
	if ( ( _It_ == 0 ) && ( _Is_ == 0 ) ) {
		idreg = ALLOCVI(_Id_, MODE_WRITE);
		XOR32RtoR(idreg, idreg);
		return;
	} 
	
	ADD_VI_NEEDED(_Is_);
	ADD_VI_NEEDED(_It_);
	idreg = ALLOCVI(_Id_, MODE_WRITE);

	if ( _Is_ == 0 )
	{
		if( (itreg = _checkX86reg(X86TYPE_VI|((VU==&VU1)?X86TYPE_VU1:0), _It_, MODE_READ)) >= 0 ) {
			if( idreg != itreg ) MOV32RtoR(idreg, itreg);
		}
		else MOVZX32M16toR(idreg, VU_VI_ADDR(_It_, 1));
		NEG16R(idreg);
	}
	else if ( _It_ == 0 )
	{
		if( (isreg = _checkX86reg(X86TYPE_VI|((VU==&VU1)?X86TYPE_VU1:0), _Is_, MODE_READ)) >= 0 ) {
			if( idreg != isreg ) MOV32RtoR(idreg, isreg);
		}
		else MOVZX32M16toR(idreg, VU_VI_ADDR(_Is_, 1));
	}
	else
	{
		isreg = ALLOCVI(_Is_, MODE_READ);
		itreg = ALLOCVI(_It_, MODE_READ);

		if( idreg == isreg ) SUB16RtoR(idreg, itreg);
		else if( idreg == itreg ) {
			SUB16RtoR(idreg, isreg);
			NEG16R(idreg);
		}
		else {
			MOV32RtoR(idreg, isreg);
			SUB16RtoR(idreg, itreg);
		}
	}
}
//------------------------------------------------------------------

//------------------------------------------------------------------
// ISUBIU
//------------------------------------------------------------------
void recVUMI_ISUBIU( VURegs *VU, int info )
{
	s16 imm;

	if ( _It_ == 0 ) return;
	//Console.WriteLn("recVUMI_ISUBIU");
	imm = ( ( VU->code >> 10 ) & 0x7800 ) | ( VU->code & 0x7ff );
	imm = -imm;
	_addISIMMtoIT(VU, imm, info);
}
//------------------------------------------------------------------


//------------------------------------------------------------------
// MOVE*
//------------------------------------------------------------------
void recVUMI_MOVE( VURegs *VU, int info )
{	
	if ( (_Ft_ == 0) || (_X_Y_Z_W == 0) ) return;
	//Console.WriteLn("recVUMI_MOVE");
	if (_X_Y_Z_W == 0x8)  SSE_MOVSS_XMM_to_XMM(EEREC_T, EEREC_S);
	else if (_X_Y_Z_W == 0xf) SSE_MOVAPS_XMM_to_XMM(EEREC_T, EEREC_S);
	else {
		SSE_MOVAPS_XMM_to_XMM(EEREC_TEMP, EEREC_S);
		VU_MERGE_REGS(EEREC_T, EEREC_TEMP);
	}
}
//------------------------------------------------------------------


//------------------------------------------------------------------
// MFIR*
//------------------------------------------------------------------
void recVUMI_MFIR( VURegs *VU, int info )
{
	if ( (_Ft_ == 0)  || (_X_Y_Z_W == 0) ) return;
	//Console.WriteLn("recVUMI_MFIR");
	_deleteX86reg(X86TYPE_VI|((VU==&VU1)?X86TYPE_VU1:0), _Is_, 1);

	if( _XYZW_SS ) {
		SSE2_MOVD_M32_to_XMM(EEREC_TEMP, VU_VI_ADDR(_Is_, 1)-2);
		_vuFlipRegSS(VU, EEREC_T);
		SSE2_PSRAD_I8_to_XMM(EEREC_TEMP, 16);
		SSE_MOVSS_XMM_to_XMM(EEREC_T, EEREC_TEMP);
		_vuFlipRegSS(VU, EEREC_T);
	}
	else if (_X_Y_Z_W != 0xf) {
		SSE2_MOVD_M32_to_XMM(EEREC_TEMP, VU_VI_ADDR(_Is_, 1)-2);
		SSE2_PSRAD_I8_to_XMM(EEREC_TEMP, 16);
		SSE_SHUFPS_XMM_to_XMM(EEREC_TEMP, EEREC_TEMP, 0);
		VU_MERGE_REGS(EEREC_T, EEREC_TEMP);
	} 
	else {
		SSE2_MOVD_M32_to_XMM(EEREC_T, VU_VI_ADDR(_Is_, 1)-2);
		SSE2_PSRAD_I8_to_XMM(EEREC_T, 16);
		SSE_SHUFPS_XMM_to_XMM(EEREC_T, EEREC_T, 0);
	}
}
//------------------------------------------------------------------


//------------------------------------------------------------------
// MTIR*
//------------------------------------------------------------------
void recVUMI_MTIR( VURegs *VU, int info )
{
	if ( _It_ == 0 ) return;
	//Console.WriteLn("recVUMI_MTIR");
	_deleteX86reg(X86TYPE_VI|((VU==&VU1)?X86TYPE_VU1:0), _It_, 2);

	if( _Fsf_ == 0 ) {
		SSE_MOVSS_XMM_to_M32(VU_VI_ADDR(_It_, 0), EEREC_S);
	}
	else {
		_unpackVFSS_xyzw(EEREC_TEMP, EEREC_S, _Fsf_);
		SSE_MOVSS_XMM_to_M32(VU_VI_ADDR(_It_, 0), EEREC_TEMP);
	}

	AND32ItoM(VU_VI_ADDR(_It_, 0), 0xffff);
} 
//------------------------------------------------------------------


//------------------------------------------------------------------
// MR32*
//------------------------------------------------------------------
void recVUMI_MR32( VURegs *VU, int info )
{	
	if ( (_Ft_ == 0) || (_X_Y_Z_W == 0) ) return;
	//Console.WriteLn("recVUMI_MR32");
	if (_X_Y_Z_W != 0xf) {
		SSE_MOVAPS_XMM_to_XMM(EEREC_TEMP, EEREC_S);
		SSE_SHUFPS_XMM_to_XMM(EEREC_TEMP, EEREC_TEMP, 0x39);
		VU_MERGE_REGS(EEREC_T, EEREC_TEMP);
	}
	else {
		SSE_MOVAPS_XMM_to_XMM(EEREC_T, EEREC_S);
		SSE_SHUFPS_XMM_to_XMM(EEREC_T, EEREC_T, 0x39);
	}
}
//------------------------------------------------------------------


//------------------------------------------------------------------
// _loadEAX()
//
// NOTE: If x86reg < 0, reads directly from offset
//------------------------------------------------------------------
void _loadEAX(VURegs *VU, int x86reg, uptr offset, int info)
{
    assert( offset < 0x80000000 );

	if( x86reg >= 0 ) {
		switch(_X_Y_Z_W) {
			case 3: // ZW
				SSE_MOVHPS_Rm_to_XMM(EEREC_T, x86reg, offset+8);
				break;
			case 6: // YZ
				SSE_SHUFPS_Rm_to_XMM(EEREC_T, x86reg, offset, 0x9c);
				SSE_SHUFPS_XMM_to_XMM(EEREC_T, EEREC_T, 0x78);
				break;

			case 8: // X
				SSE_MOVSS_Rm_to_XMM(EEREC_TEMP, x86reg, offset);
				SSE_MOVSS_XMM_to_XMM(EEREC_T, EEREC_TEMP);
				break;
			case 9: // XW
				SSE_SHUFPS_Rm_to_XMM(EEREC_T, x86reg, offset, 0xc9);
				SSE_SHUFPS_XMM_to_XMM(EEREC_T, EEREC_T, 0xd2);
				break;
			case 12: // XY
				SSE_MOVLPS_Rm_to_XMM(EEREC_T, x86reg, offset);
				break;
			case 15:
				if( VU == &VU1 ) SSE_MOVAPSRmtoR(EEREC_T, x86reg, offset);
				else SSE_MOVUPSRmtoR(EEREC_T, x86reg, offset);
				break;
			default:
				if( VU == &VU1 ) SSE_MOVAPSRmtoR(EEREC_TEMP, x86reg, offset);
				else SSE_MOVUPSRmtoR(EEREC_TEMP, x86reg, offset);

				VU_MERGE_REGS(EEREC_T, EEREC_TEMP);
				break;
		}
	}
	else {
		switch(_X_Y_Z_W) {
			case 3: // ZW
				SSE_MOVHPS_M64_to_XMM(EEREC_T, offset+8);
				break;
			case 6: // YZ
				SSE_SHUFPS_M128_to_XMM(EEREC_T, offset, 0x9c);
				SSE_SHUFPS_XMM_to_XMM(EEREC_T, EEREC_T, 0x78);
				break;
			case 8: // X
				SSE_MOVSS_M32_to_XMM(EEREC_TEMP, offset);
				SSE_MOVSS_XMM_to_XMM(EEREC_T, EEREC_TEMP);
				break;
			case 9: // XW
				SSE_SHUFPS_M128_to_XMM(EEREC_T, offset, 0xc9);
				SSE_SHUFPS_XMM_to_XMM(EEREC_T, EEREC_T, 0xd2);
				break;
			case 12: // XY
				SSE_MOVLPS_M64_to_XMM(EEREC_T, offset);
				break;
			case 15:
				if( VU == &VU1 ) SSE_MOVAPS_M128_to_XMM(EEREC_T, offset);
				else SSE_MOVUPS_M128_to_XMM(EEREC_T, offset);
				break;
			default:
				if( VU == &VU1 ) SSE_MOVAPS_M128_to_XMM(EEREC_TEMP, offset);
				else SSE_MOVUPS_M128_to_XMM(EEREC_TEMP, offset);
				VU_MERGE_REGS(EEREC_T, EEREC_TEMP);
				break;
		}
	}
}
//------------------------------------------------------------------


//------------------------------------------------------------------
// recVUTransformAddr()
//------------------------------------------------------------------
int recVUTransformAddr(int x86reg, VURegs* VU, int vireg, int imm)
{
	u8* pjmp[2];
	if( x86reg == EAX ) {
		if (imm) ADD32ItoR(x86reg, imm);
	}
	else {
		if( imm ) LEA32RtoR(EAX, x86reg, imm);
		else MOV32RtoR(EAX, x86reg);
	}
	
	if( VU == &VU1 ) {
		AND32ItoR(EAX, 0x3ff); // wrap around
		SHL32ItoR(EAX, 4);
	}
	else {
	
		// VU0 has a somewhat interesting memory mapping:
		// if addr >= 0x4000, reads VU1's VF regs and VI regs
		// if addr < 0x4000, wrap around at 0x1000

		CMP32ItoR(EAX, 0x400);
		pjmp[0] = JL8(0); // if addr >= 0x4000, reads VU1's VF regs and VI regs
			AND32ItoR(EAX, 0x43f);
			pjmp[1] = JMP8(0);
		x86SetJ8(pjmp[0]);
			AND32ItoR(EAX, 0xff); // if addr < 0x4000, wrap around
		x86SetJ8(pjmp[1]);

		SHL32ItoR(EAX, 4); // multiply by 16 (shift left by 4)
	}

	return EAX;
}
//------------------------------------------------------------------


//------------------------------------------------------------------
// LQ
//------------------------------------------------------------------
void recVUMI_LQ(VURegs *VU, int info)
{
	s16 imm;
	if ( _Ft_ == 0 ) return;
	//Console.WriteLn("recVUMI_LQ");
	imm = (VU->code & 0x400) ? (VU->code & 0x3ff) | 0xfc00 : (VU->code & 0x3ff); 
	if (_Is_ == 0) {
		_loadEAX(VU, -1, (uptr)GET_VU_MEM(VU, (u32)imm*16), info);
	} 
	else {
		int isreg = ALLOCVI(_Is_, MODE_READ);
		_loadEAX(VU, recVUTransformAddr(isreg, VU, _Is_, imm), (uptr)VU->Mem, info);
	}
}
//------------------------------------------------------------------


//------------------------------------------------------------------
// LQD
//------------------------------------------------------------------
void recVUMI_LQD( VURegs *VU, int info )
{
	int isreg;
	//Console.WriteLn("recVUMI_LQD");
	if ( _Is_ != 0 ) {
		isreg = ALLOCVI(_Is_, MODE_READ|MODE_WRITE);
		SUB16ItoR( isreg, 1 );
	}

	if ( _Ft_ == 0 ) return;

	if ( _Is_ == 0 ) _loadEAX(VU, -1, (uptr)VU->Mem, info);
	else _loadEAX(VU, recVUTransformAddr(isreg, VU, _Is_, 0), (uptr)VU->Mem, info);
}
//------------------------------------------------------------------


//------------------------------------------------------------------
// LQI
//------------------------------------------------------------------
void recVUMI_LQI(VURegs *VU, int info)
{
	int isreg;
	//Console.WriteLn("recVUMI_LQI");
	if ( _Ft_ == 0 ) {
		if( _Is_ != 0 ) {
			if( (isreg = _checkX86reg(X86TYPE_VI|(VU==&VU1?X86TYPE_VU1:0), _Is_, MODE_WRITE|MODE_READ)) >= 0 ) {
				ADD16ItoR(isreg, 1);
			}
			else {
				ADD16ItoM( VU_VI_ADDR( _Is_, 0 ), 1 );
			}
		}
		return;
	}

    if (_Is_ == 0) {
		_loadEAX(VU, -1, (uptr)VU->Mem, info);
    } 
	else {
		isreg = ALLOCVI(_Is_, MODE_READ|MODE_WRITE);
		_loadEAX(VU, recVUTransformAddr(isreg, VU, _Is_, 0), (uptr)VU->Mem, info);
		ADD16ItoR( isreg, 1 );
    }
}
//------------------------------------------------------------------


//------------------------------------------------------------------
// _saveEAX()
//------------------------------------------------------------------
void _saveEAX(VURegs *VU, int x86reg, uptr offset, int info)
{
	assert( offset < 0x80000000 );

	if ( _Fs_ == 0 ) {
		if ( _XYZW_SS ) {
			u32 c = _W ? 0x3f800000 : 0;
			if ( x86reg >= 0 ) MOV32ItoRm(x86reg, c, offset+(_W?12:(_Z?8:(_Y?4:0))));
			else MOV32ItoM(offset+(_W?12:(_Z?8:(_Y?4:0))), c);
		}
		else {

			// (this is one of my test cases for the new emitter --air)
			using namespace x86Emitter;
			xAddressReg thisreg( x86reg );

			if ( _X ) xMOV(ptr32[thisreg+offset],    0x00000000);
			if ( _Y ) xMOV(ptr32[thisreg+offset+4],  0x00000000);
			if ( _Z ) xMOV(ptr32[thisreg+offset+8],  0x00000000);
			if ( _W ) xMOV(ptr32[thisreg+offset+12], 0x3f800000);
		}
		return;
	}

	switch ( _X_Y_Z_W ) {
		case 1: // W
			SSE2_PSHUFD_XMM_to_XMM(EEREC_TEMP, EEREC_S, 0x27);
			if ( x86reg >= 0 ) SSE_MOVSS_XMM_to_Rm(x86reg, EEREC_TEMP, offset+12);
			else SSE_MOVSS_XMM_to_M32(offset+12, EEREC_TEMP);
			break;
		case 2: // Z
			SSE_MOVHLPS_XMM_to_XMM(EEREC_TEMP, EEREC_S);
			if ( x86reg >= 0 ) SSE_MOVSS_XMM_to_Rm(x86reg, EEREC_TEMP, offset+8);
			else SSE_MOVSS_XMM_to_M32(offset+8, EEREC_TEMP);
			break;
		case 3: // ZW
			if ( x86reg >= 0 ) SSE_MOVHPS_XMM_to_Rm(x86reg, EEREC_S, offset+8);
			else SSE_MOVHPS_XMM_to_M64(offset+8, EEREC_S);
			break;
		case 4: // Y
			SSE2_PSHUFLW_XMM_to_XMM(EEREC_TEMP, EEREC_S, 0x4e);
			if ( x86reg >= 0 ) SSE_MOVSS_XMM_to_Rm(x86reg, EEREC_TEMP, offset+4);
			else SSE_MOVSS_XMM_to_M32(offset+4, EEREC_TEMP);
			break;
		case 5: // YW
			SSE_SHUFPS_XMM_to_XMM(EEREC_S, EEREC_S, 0xB1);
			SSE_MOVHLPS_XMM_to_XMM(EEREC_TEMP, EEREC_S);
			if ( x86reg >= 0 ) {
				SSE_MOVSS_XMM_to_Rm(x86reg, EEREC_S, offset+4);
				SSE_MOVSS_XMM_to_Rm(x86reg, EEREC_TEMP, offset+12);
			}
			else {
				SSE_MOVSS_XMM_to_M32(offset+4, EEREC_S);
				SSE_MOVSS_XMM_to_M32(offset+12, EEREC_TEMP);
			}
			SSE_SHUFPS_XMM_to_XMM(EEREC_S, EEREC_S, 0xB1);
			break;
		case 6: // YZ
			SSE2_PSHUFD_XMM_to_XMM(EEREC_TEMP, EEREC_S, 0xc9);
			if ( x86reg >= 0 ) SSE_MOVLPS_XMM_to_Rm(x86reg, EEREC_TEMP, offset+4);
			else SSE_MOVLPS_XMM_to_M64(offset+4, EEREC_TEMP);
			break;
		case 7: // YZW
			SSE2_PSHUFD_XMM_to_XMM(EEREC_TEMP, EEREC_S, 0x93); //ZYXW
			if ( x86reg >= 0 ) {
				SSE_MOVHPS_XMM_to_Rm(x86reg, EEREC_TEMP, offset+4);
				SSE_MOVSS_XMM_to_Rm(x86reg, EEREC_TEMP, offset+12);
			}
			else {
				SSE_MOVHPS_XMM_to_M64(offset+4, EEREC_TEMP);
				SSE_MOVSS_XMM_to_M32(offset+12, EEREC_TEMP);
			}
			break;
		case 8: // X
			if ( x86reg >= 0 ) SSE_MOVSS_XMM_to_Rm(x86reg, EEREC_S, offset);
			else SSE_MOVSS_XMM_to_M32(offset, EEREC_S);
			break;
		case 9: // XW
			if ( x86reg >= 0 ) SSE_MOVSS_XMM_to_Rm(x86reg, EEREC_S, offset);
			else SSE_MOVSS_XMM_to_M32(offset, EEREC_S);
			
			SSE2_PSHUFD_XMM_to_XMM(EEREC_TEMP, EEREC_S, 0xff); //WWWW

			if ( x86reg >= 0 ) SSE_MOVSS_XMM_to_Rm(x86reg, EEREC_TEMP, offset+12);
			else SSE_MOVSS_XMM_to_M32(offset+12, EEREC_TEMP);

			break;
		case 10: //XZ
			SSE_MOVHLPS_XMM_to_XMM(EEREC_TEMP, EEREC_S);
			if ( x86reg >= 0 ) {
				SSE_MOVSS_XMM_to_Rm(x86reg, EEREC_S, offset);
				SSE_MOVSS_XMM_to_Rm(x86reg, EEREC_TEMP, offset+8);
			}
			else {
				SSE_MOVSS_XMM_to_M32(offset, EEREC_S);
				SSE_MOVSS_XMM_to_M32(offset+8, EEREC_TEMP);
			}
			break;
		case 11: //XZW
			if ( x86reg >= 0 ) {
				SSE_MOVSS_XMM_to_Rm(x86reg, EEREC_S, offset);
				SSE_MOVHPS_XMM_to_Rm(x86reg, EEREC_S, offset+8);
			}
			else {
				SSE_MOVSS_XMM_to_M32(offset, EEREC_S);
				SSE_MOVHPS_XMM_to_M64(offset+8, EEREC_S);
			}
			break;
		case 12: // XY
			if ( x86reg >= 0 ) SSE_MOVLPS_XMM_to_Rm(x86reg, EEREC_S, offset+0);
			else SSE_MOVLPS_XMM_to_M64(offset, EEREC_S);
			break;
		case 13: // XYW
			SSE2_PSHUFD_XMM_to_XMM(EEREC_TEMP, EEREC_S, 0x4b); //YXZW
			if ( x86reg >= 0 ) {
				SSE_MOVHPS_XMM_to_Rm(x86reg, EEREC_TEMP, offset+0);
				SSE_MOVSS_XMM_to_Rm(x86reg, EEREC_TEMP, offset+12);
			}
			else {
				SSE_MOVHPS_XMM_to_M64(offset, EEREC_TEMP);
				SSE_MOVSS_XMM_to_M32(offset+12, EEREC_TEMP);
			}
			break;
		case 14: // XYZ
			SSE_MOVHLPS_XMM_to_XMM(EEREC_TEMP, EEREC_S);
			if ( x86reg >= 0 ) {
				SSE_MOVLPS_XMM_to_Rm(x86reg, EEREC_S, offset+0);
				SSE_MOVSS_XMM_to_Rm(x86reg, EEREC_TEMP, offset+8);
			}
			else {
				SSE_MOVLPS_XMM_to_M64(offset, EEREC_S);
				SSE_MOVSS_XMM_to_M32(offset+8, EEREC_TEMP);
			}
			break;
		case 15: // XYZW
			if ( VU == &VU1 ) {
				if( x86reg >= 0 ) SSE_MOVAPSRtoRm(x86reg, EEREC_S, offset+0);
				else SSE_MOVAPS_XMM_to_M128(offset, EEREC_S);
			}
			else {
				if( x86reg >= 0 ) SSE_MOVUPSRtoRm(x86reg, EEREC_S, offset+0);
				else {
					if( offset & 15 ) SSE_MOVUPS_XMM_to_M128(offset, EEREC_S);
					else SSE_MOVAPS_XMM_to_M128(offset, EEREC_S);
				}
			}
			break;
	}
}
//------------------------------------------------------------------


//------------------------------------------------------------------
// SQ
//------------------------------------------------------------------
void recVUMI_SQ(VURegs *VU, int info)
{
	s16 imm;
	//Console.WriteLn("recVUMI_SQ");
	imm = ( VU->code & 0x400) ? ( VU->code & 0x3ff) | 0xfc00 : ( VU->code & 0x3ff); 
	if ( _It_ == 0 ) _saveEAX(VU, -1, (uptr)GET_VU_MEM(VU, (int)imm * 16), info);
	else {
		int itreg = ALLOCVI(_It_, MODE_READ);
		_saveEAX(VU, recVUTransformAddr(itreg, VU, _It_, imm), (uptr)VU->Mem, info);
	}
}
//------------------------------------------------------------------


//------------------------------------------------------------------
// SQD
//------------------------------------------------------------------
void recVUMI_SQD(VURegs *VU, int info)
{
	//Console.WriteLn("recVUMI_SQD");
	if (_It_ == 0) _saveEAX(VU, -1, (uptr)VU->Mem, info);
	else {
		int itreg = ALLOCVI(_It_, MODE_READ|MODE_WRITE);
		SUB16ItoR( itreg, 1 );
		_saveEAX(VU, recVUTransformAddr(itreg, VU, _It_, 0), (uptr)VU->Mem, info);
	}
}
//------------------------------------------------------------------


//------------------------------------------------------------------
// SQI
//------------------------------------------------------------------
void recVUMI_SQI(VURegs *VU, int info)
{
	//Console.WriteLn("recVUMI_SQI");
	if (_It_ == 0) _saveEAX(VU, -1, (uptr)VU->Mem, info);
	else {
		int itreg = ALLOCVI(_It_, MODE_READ|MODE_WRITE);
		_saveEAX(VU, recVUTransformAddr(itreg, VU, _It_, 0), (uptr)VU->Mem, info);
		ADD16ItoR( itreg, 1 );
	}
}
//------------------------------------------------------------------


//------------------------------------------------------------------
// ILW
//------------------------------------------------------------------
void recVUMI_ILW(VURegs *VU, int info)
{
	int itreg;
	s16 imm, off;
 
	if ( ( _It_ == 0 ) || ( _X_Y_Z_W == 0 ) ) return;
	//Console.WriteLn("recVUMI_ILW");
	imm = ( VU->code & 0x400) ? ( VU->code & 0x3ff) | 0xfc00 : ( VU->code & 0x3ff);
	if (_X) off = 0;
	else if (_Y) off = 4;
	else if (_Z) off = 8;
	else if (_W) off = 12;

	ADD_VI_NEEDED(_Is_);
	itreg = ALLOCVI(_It_, MODE_WRITE);

	if ( _Is_ == 0 ) {
		MOVZX32M16toR( itreg, (uptr)GET_VU_MEM(VU, (int)imm * 16 + off) );
	}
	else {
		int isreg = ALLOCVI(_Is_, MODE_READ);
		MOV32RmtoR(itreg, recVUTransformAddr(isreg, VU, _Is_, imm), (uptr)VU->Mem + off);
	}
}
//------------------------------------------------------------------


//------------------------------------------------------------------
// ISW
//------------------------------------------------------------------
void recVUMI_ISW( VURegs *VU, int info )
{
	s16 imm;
	//Console.WriteLn("recVUMI_ISW");
	imm = ( VU->code & 0x400) ? ( VU->code & 0x3ff) | 0xfc00 : ( VU->code & 0x3ff); 

	if (_Is_ == 0) {
		uptr off = (uptr)GET_VU_MEM(VU, (int)imm * 16);
		int itreg = ALLOCVI(_It_, MODE_READ);

		if (_X) MOV32RtoM(off, itreg);
		if (_Y) MOV32RtoM(off+4, itreg);
		if (_Z) MOV32RtoM(off+8, itreg);
		if (_W) MOV32RtoM(off+12, itreg);
	}
	else {
		int x86reg, isreg, itreg;

		ADD_VI_NEEDED(_It_);
		isreg = ALLOCVI(_Is_, MODE_READ);
		itreg = ALLOCVI(_It_, MODE_READ);

		x86reg = recVUTransformAddr(isreg, VU, _Is_, imm);

		if (_X) MOV32RtoRm(x86reg, itreg, (uptr)VU->Mem);
		if (_Y) MOV32RtoRm(x86reg, itreg, (uptr)VU->Mem+4);
		if (_Z) MOV32RtoRm(x86reg, itreg, (uptr)VU->Mem+8);
		if (_W) MOV32RtoRm(x86reg, itreg, (uptr)VU->Mem+12);
	}
}
//------------------------------------------------------------------


//------------------------------------------------------------------
// ILWR
//------------------------------------------------------------------
void recVUMI_ILWR( VURegs *VU, int info )
{
	int off, itreg;

	if ( ( _It_ == 0 ) || ( _X_Y_Z_W == 0 ) ) return;
	//Console.WriteLn("recVUMI_ILWR");
	if (_X) off = 0;
	else if (_Y) off = 4;
	else if (_Z) off = 8;
	else if (_W) off = 12;

	ADD_VI_NEEDED(_Is_);
	itreg = ALLOCVI(_It_, MODE_WRITE);

	if ( _Is_ == 0 ) {
		MOVZX32M16toR( itreg, (uptr)VU->Mem + off );
	}
	else {
		int isreg = ALLOCVI(_Is_, MODE_READ);
		MOVZX32Rm16toR(itreg, recVUTransformAddr(isreg, VU, _Is_, 0), (uptr)VU->Mem + off);
	}
}
//------------------------------------------------------------------


//------------------------------------------------------------------
// ISWR
//------------------------------------------------------------------
void recVUMI_ISWR( VURegs *VU, int info )
{
	int itreg;
	//Console.WriteLn("recVUMI_ISWR");
	ADD_VI_NEEDED(_Is_);
	itreg = ALLOCVI(_It_, MODE_READ);

	if (_Is_ == 0) {
		if (_X) MOV32RtoM((uptr)VU->Mem, itreg);
		if (_Y) MOV32RtoM((uptr)VU->Mem+4, itreg);
		if (_Z) MOV32RtoM((uptr)VU->Mem+8, itreg);
		if (_W) MOV32RtoM((uptr)VU->Mem+12, itreg);
	}
	else {
		int x86reg;
		int isreg = ALLOCVI(_Is_, MODE_READ);
		x86reg = recVUTransformAddr(isreg, VU, _Is_, 0);

		if (_X) MOV32RtoRm(x86reg, itreg, (uptr)VU->Mem);
		if (_Y) MOV32RtoRm(x86reg, itreg, (uptr)VU->Mem+4);
		if (_Z) MOV32RtoRm(x86reg, itreg, (uptr)VU->Mem+8);
		if (_W) MOV32RtoRm(x86reg, itreg, (uptr)VU->Mem+12);
	}
}
//------------------------------------------------------------------


//------------------------------------------------------------------
// RINIT*
//------------------------------------------------------------------
void recVUMI_RINIT(VURegs *VU, int info)
{
	//Console.WriteLn("recVUMI_RINIT()");
	if( (xmmregs[EEREC_S].mode & MODE_WRITE) && (xmmregs[EEREC_S].mode & MODE_NOFLUSH) ) {
		_deleteX86reg(X86TYPE_VI|(VU==&VU1?X86TYPE_VU1:0), REG_R, 2);
		_unpackVFSS_xyzw(EEREC_TEMP, EEREC_S, _Fsf_);

		SSE_ANDPS_M128_to_XMM(EEREC_TEMP, (uptr)s_mask);
		SSE_ORPS_M128_to_XMM(EEREC_TEMP, (uptr)VU_ONE);
		SSE_MOVSS_XMM_to_M32(VU_REGR_ADDR, EEREC_TEMP);
	}
	else {
		int rreg = ALLOCVI(REG_R, MODE_WRITE);

		if( xmmregs[EEREC_S].mode & MODE_WRITE ) {
			SSE_MOVAPS_XMM_to_M128((uptr)&VU->VF[_Fs_], EEREC_S);
			xmmregs[EEREC_S].mode &= ~MODE_WRITE;
		}

		MOV32MtoR( rreg, VU_VFx_ADDR( _Fs_ ) + 4 * _Fsf_ );
		AND32ItoR( rreg, 0x7fffff );
		OR32ItoR( rreg, 0x7f << 23 );

		_deleteX86reg(X86TYPE_VI|(VU==&VU1?X86TYPE_VU1:0), REG_R, 1); 
	}
}
//------------------------------------------------------------------


//------------------------------------------------------------------
// RGET*
//------------------------------------------------------------------
void recVUMI_RGET(VURegs *VU, int info)
{
	//Console.WriteLn("recVUMI_RGET()");
	if ( (_Ft_ == 0) || (_X_Y_Z_W == 0)  ) return;

	_deleteX86reg(X86TYPE_VI|(VU==&VU1?X86TYPE_VU1:0), REG_R, 1);

	if (_X_Y_Z_W != 0xf) {
		SSE_MOVSS_M32_to_XMM(EEREC_TEMP, VU_REGR_ADDR);
		SSE_SHUFPS_XMM_to_XMM(EEREC_TEMP, EEREC_TEMP, 0);
		VU_MERGE_REGS(EEREC_T, EEREC_TEMP);
	}
	else {
		SSE_MOVSS_M32_to_XMM(EEREC_T, VU_REGR_ADDR);
		SSE_SHUFPS_XMM_to_XMM(EEREC_T, EEREC_T, 0);
	}
}
//------------------------------------------------------------------


//------------------------------------------------------------------
// RNEXT*
//------------------------------------------------------------------
void recVUMI_RNEXT( VURegs *VU, int info )
{
	int rreg, x86temp0, x86temp1;
	//Console.WriteLn("recVUMI_RNEXT()");

	rreg = ALLOCVI(REG_R, MODE_WRITE|MODE_READ);

	x86temp0 = ALLOCTEMPX86(0);
	x86temp1 = ALLOCTEMPX86(0);

	// code from www.project-fao.org
	//MOV32MtoR(rreg, VU_REGR_ADDR);
	MOV32RtoR(x86temp0, rreg);
	SHR32ItoR(x86temp0, 4);
	AND32ItoR(x86temp0, 1);

	MOV32RtoR(x86temp1, rreg);
	SHR32ItoR(x86temp1, 22);
	AND32ItoR(x86temp1, 1);

	SHL32ItoR(rreg, 1);
	XOR32RtoR(x86temp0, x86temp1);
	XOR32RtoR(rreg, x86temp0);
	AND32ItoR(rreg, 0x7fffff);
	OR32ItoR(rreg, 0x3f800000);

	_freeX86reg(x86temp0);
	_freeX86reg(x86temp1);

	if ( (_Ft_ == 0) || (_X_Y_Z_W == 0)  ) {
		_deleteX86reg(X86TYPE_VI|(VU==&VU1?X86TYPE_VU1:0), REG_R, 1); 
		return;
	}

	recVUMI_RGET(VU, info);
}
//------------------------------------------------------------------


//------------------------------------------------------------------
// RXOR*
//------------------------------------------------------------------
void recVUMI_RXOR( VURegs *VU, int info )
{
	//Console.WriteLn("recVUMI_RXOR()");
	if( (xmmregs[EEREC_S].mode & MODE_WRITE) && (xmmregs[EEREC_S].mode & MODE_NOFLUSH) ) {
		_deleteX86reg(X86TYPE_VI|(VU==&VU1?X86TYPE_VU1:0), REG_R, 1); 
		_unpackVFSS_xyzw(EEREC_TEMP, EEREC_S, _Fsf_);

		SSE_XORPS_M128_to_XMM(EEREC_TEMP, VU_REGR_ADDR);
		SSE_ANDPS_M128_to_XMM(EEREC_TEMP, (uptr)s_mask);
		SSE_ORPS_M128_to_XMM(EEREC_TEMP, (uptr)s_fones);
		SSE_MOVSS_XMM_to_M32(VU_REGR_ADDR, EEREC_TEMP);
	}
	else {
		int rreg = ALLOCVI(REG_R, MODE_WRITE|MODE_READ);

		if( xmmregs[EEREC_S].mode & MODE_WRITE ) {
			SSE_MOVAPS_XMM_to_M128((uptr)&VU->VF[_Fs_], EEREC_S);
			xmmregs[EEREC_S].mode &= ~MODE_WRITE;
		}

		XOR32MtoR( rreg, VU_VFx_ADDR( _Fs_ ) + 4 * _Fsf_ );
		AND32ItoR( rreg, 0x7fffff );
		OR32ItoR ( rreg, 0x3f800000 );

		_deleteX86reg(X86TYPE_VI|(VU==&VU1?X86TYPE_VU1:0), REG_R, 1); 
	}
} 
//------------------------------------------------------------------


//------------------------------------------------------------------
// WAITQ
//------------------------------------------------------------------
void recVUMI_WAITQ( VURegs *VU, int info )
{
	//Console.WriteLn("recVUMI_WAITQ");
//	if( info & PROCESS_VU_SUPER ) {
//		//CALLFunc(waitqfn);
//		SuperVUFlush(0, 1);
//	}
}
//------------------------------------------------------------------


//------------------------------------------------------------------
// FSAND
//------------------------------------------------------------------
void recVUMI_FSAND( VURegs *VU, int info )
{
	int itreg;
	u16 imm;
	//Console.WriteLn("recVUMI_FSAND");
	imm = (((VU->code >> 21 ) & 0x1) << 11) | (VU->code & 0x7ff);
	if(_It_ == 0) return; 

	itreg = ALLOCVI(_It_, MODE_WRITE);
	MOV32MtoR( itreg, VU_VI_ADDR(REG_STATUS_FLAG, 1) );
	AND32ItoR( itreg, imm );
}
//------------------------------------------------------------------


//------------------------------------------------------------------
// FSEQ
//------------------------------------------------------------------
void recVUMI_FSEQ( VURegs *VU, int info )
{
	int itreg;
	u16 imm;
	if ( _It_ == 0 ) return;
	//Console.WriteLn("recVUMI_FSEQ");
	imm = (((VU->code >> 21 ) & 0x1) << 11) | (VU->code & 0x7ff);

	itreg = ALLOCVI(_It_, MODE_WRITE|MODE_8BITREG);

	MOVZX32M16toR( EAX, VU_VI_ADDR(REG_STATUS_FLAG, 1) );
	XOR32RtoR(itreg, itreg);
	CMP16ItoR(EAX, imm);
	SETE8R(itreg);
}
//------------------------------------------------------------------


//------------------------------------------------------------------
// FSOR
//------------------------------------------------------------------
void recVUMI_FSOR( VURegs *VU, int info )
{
	int itreg;
	u32 imm;
	if(_It_ == 0) return; 
	//Console.WriteLn("recVUMI_FSOR");
	imm = (((VU->code >> 21 ) & 0x1) << 11) | (VU->code & 0x7ff);

	itreg = ALLOCVI(_It_, MODE_WRITE);

	MOVZX32M16toR( itreg, VU_VI_ADDR(REG_STATUS_FLAG, 1) );
	OR32ItoR( itreg, imm );
}
//------------------------------------------------------------------


//------------------------------------------------------------------
// FSSET
//------------------------------------------------------------------
void recVUMI_FSSET(VURegs *VU, int info)
{
	u32 writeaddr = VU_VI_ADDR(REG_STATUS_FLAG, 0);
	u32 prevaddr = VU_VI_ADDR(REG_STATUS_FLAG, 2);

	u16 imm = 0;
	//Console.WriteLn("recVUMI_FSSET");
	imm = (((VU->code >> 21 ) & 0x1) << 11) | (VU->code & 0x7FF);

    // keep the low 6 bits ONLY if the upper instruction is an fmac instruction (otherwise rewrite) - metal gear solid 3
    //if( (info & PROCESS_VU_SUPER) && VUREC_FMAC ) {
        MOV32MtoR(EAX, prevaddr);
	    AND32ItoR(EAX, 0x3f);
	    if ((imm&0xfc0) != 0) OR32ItoR(EAX, imm & 0xFC0);
        MOV32RtoM(writeaddr ? writeaddr : prevaddr, EAX);
    //}
    //else {
    //    MOV32ItoM(writeaddr ? writeaddr : prevaddr, imm&0xfc0);
	//}
}
//------------------------------------------------------------------


//------------------------------------------------------------------
// FMAND
//------------------------------------------------------------------
void recVUMI_FMAND( VURegs *VU, int info )
{
	int isreg, itreg;
	if ( _It_ == 0 ) return;
	//Console.WriteLn("recVUMI_FMAND");
	isreg = _checkX86reg(X86TYPE_VI|(VU==&VU1?X86TYPE_VU1:0), _Is_, MODE_READ);
	itreg = ALLOCVI(_It_, MODE_WRITE);//|MODE_8BITREG);

	if( isreg >= 0 ) {
		if( itreg != isreg ) MOV32RtoR(itreg, isreg);
	}
	else MOVZX32M16toR(itreg, VU_VI_ADDR(_Is_, 1));

	AND16MtoR( itreg, VU_VI_ADDR(REG_MAC_FLAG, 1));
}
//------------------------------------------------------------------


//------------------------------------------------------------------
// FMEQ
//------------------------------------------------------------------
void recVUMI_FMEQ( VURegs *VU, int info )
{
	int itreg, isreg;
	if ( _It_ == 0 ) return;
	//Console.WriteLn("recVUMI_FMEQ");
	if( _It_ == _Is_ ) {
		itreg = ALLOCVI(_It_, MODE_WRITE|MODE_READ);//|MODE_8BITREG

		CMP16MtoR(itreg, VU_VI_ADDR(REG_MAC_FLAG, 1));
		SETE8R(EAX);
		MOVZX32R8toR(itreg, EAX);
	}
	else {
		ADD_VI_NEEDED(_Is_);
		itreg = ALLOCVI(_It_, MODE_WRITE|MODE_8BITREG);
		isreg = ALLOCVI(_Is_, MODE_READ);
		
		XOR32RtoR(itreg, itreg);
		
		CMP16MtoR(isreg, VU_VI_ADDR(REG_MAC_FLAG, 1));
		SETE8R(itreg);
	}
}
//------------------------------------------------------------------


//------------------------------------------------------------------
// FMOR
//------------------------------------------------------------------
void recVUMI_FMOR( VURegs *VU, int info )
{
	int isreg, itreg;
	if ( _It_ == 0 ) return;
	//Console.WriteLn("recVUMI_FMOR");
	if( _Is_ == 0 ) {
		itreg = ALLOCVI(_It_, MODE_WRITE);//|MODE_8BITREG);
		MOVZX32M16toR( itreg, VU_VI_ADDR(REG_MAC_FLAG, 1) );
	}
	else if( _It_ == _Is_ ) {
		itreg = ALLOCVI(_It_, MODE_WRITE|MODE_READ);//|MODE_8BITREG);
		OR16MtoR( itreg, VU_VI_ADDR(REG_MAC_FLAG, 1) );
	}
	else {
		isreg = _checkX86reg(X86TYPE_VI|(VU==&VU1?X86TYPE_VU1:0), _Is_, MODE_READ);
		itreg = ALLOCVI(_It_, MODE_WRITE);

		MOVZX32M16toR( itreg, VU_VI_ADDR(REG_MAC_FLAG, 1) );

		if( isreg >= 0 )
			OR16RtoR( itreg, isreg );
		else
			OR16MtoR( itreg, VU_VI_ADDR(_Is_, 1) );
	}
}
//------------------------------------------------------------------


//------------------------------------------------------------------
// FCAND
//------------------------------------------------------------------
void recVUMI_FCAND( VURegs *VU, int info )
{
	int itreg = ALLOCVI(1, MODE_WRITE|MODE_8BITREG);
	//Console.WriteLn("recVUMI_FCAND");
	MOV32MtoR( EAX, VU_VI_ADDR(REG_CLIP_FLAG, 1) );
	XOR32RtoR( itreg, itreg );
	AND32ItoR( EAX, VU->code & 0xFFFFFF );

	SETNZ8R(itreg);
}
//------------------------------------------------------------------


//------------------------------------------------------------------
// FCEQ
//------------------------------------------------------------------
void recVUMI_FCEQ( VURegs *VU, int info )
{
	int itreg = ALLOCVI(1, MODE_WRITE|MODE_8BITREG);
	//Console.WriteLn("recVUMI_FCEQ");
	MOV32MtoR( EAX, VU_VI_ADDR(REG_CLIP_FLAG, 1) );
	AND32ItoR( EAX, 0xffffff );
	XOR32RtoR( itreg, itreg );
	CMP32ItoR( EAX, VU->code&0xffffff );

	SETE8R(itreg);
}
//------------------------------------------------------------------


//------------------------------------------------------------------
// FCOR
//------------------------------------------------------------------
void recVUMI_FCOR( VURegs *VU, int info )
{
	int itreg;
	//Console.WriteLn("recVUMI_FCOR");
	itreg = ALLOCVI(1, MODE_WRITE);
	MOV32MtoR( itreg, VU_VI_ADDR(REG_CLIP_FLAG, 1) );
	OR32ItoR ( itreg, VU->code );
	AND32ItoR( itreg, 0xffffff );
	ADD32ItoR( itreg, 1 );	// If 24 1's will make 25th bit 1, else 0
	SHR32ItoR( itreg, 24 );	// Get the 25th bit (also clears the rest of the garbage in the reg)	
}
//------------------------------------------------------------------


//------------------------------------------------------------------
// FCSET
//------------------------------------------------------------------
void recVUMI_FCSET( VURegs *VU, int info )
{
	u32 addr = VU_VI_ADDR(REG_CLIP_FLAG, 0);
	//Console.WriteLn("recVUMI_FCSET");
	MOV32ItoM(addr ? addr : VU_VI_ADDR(REG_CLIP_FLAG, 2), VU->code&0xffffff );

	if( !(info & (PROCESS_VU_SUPER|PROCESS_VU_COP2)) )
		MOV32ItoM( VU_VI_ADDR(REG_CLIP_FLAG, 1), VU->code&0xffffff );
}
//------------------------------------------------------------------


//------------------------------------------------------------------
// FCGET
//------------------------------------------------------------------
void recVUMI_FCGET( VURegs *VU, int info )
{
	int itreg;
	if(_It_ == 0) return;
	//Console.WriteLn("recVUMI_FCGET");
	itreg = ALLOCVI(_It_, MODE_WRITE);

	MOV32MtoR(itreg, VU_VI_ADDR(REG_CLIP_FLAG, 1));
	AND32ItoR(itreg, 0x0fff);
}
//------------------------------------------------------------------


//------------------------------------------------------------------
// _recbranchAddr()
//
// NOTE: Due to static var dependencies, several SuperVU branch instructions
// are still located in iVUzerorec.cpp.
//------------------------------------------------------------------

//------------------------------------------------------------------
// MFP*
//------------------------------------------------------------------
void recVUMI_MFP(VURegs *VU, int info)
{
	if ( (_Ft_ == 0) || (_X_Y_Z_W == 0) ) return; 
	//Console.WriteLn("recVUMI_MFP");
	if( _XYZW_SS ) {
		_vuFlipRegSS(VU, EEREC_T);
		SSE_MOVSS_M32_to_XMM(EEREC_TEMP, VU_VI_ADDR(REG_P, 1));
		SSE_MOVSS_XMM_to_XMM(EEREC_T, EEREC_TEMP);
		_vuFlipRegSS(VU, EEREC_T);
	}
	else if (_X_Y_Z_W != 0xf) {
		SSE_MOVSS_M32_to_XMM(EEREC_TEMP, VU_VI_ADDR(REG_P, 1));
		SSE_SHUFPS_XMM_to_XMM(EEREC_TEMP, EEREC_TEMP, 0);
		VU_MERGE_REGS(EEREC_T, EEREC_TEMP);
	}
	else {
		SSE_MOVSS_M32_to_XMM(EEREC_T, VU_VI_ADDR(REG_P, 1));
		SSE_SHUFPS_XMM_to_XMM(EEREC_T, EEREC_T, 0);
	}
}
//------------------------------------------------------------------


//------------------------------------------------------------------
// WAITP
//------------------------------------------------------------------
static __aligned16 float s_tempmem[4];
void recVUMI_WAITP(VURegs *VU, int info)
{
	//Console.WriteLn("recVUMI_WAITP");
//	if( info & PROCESS_VU_SUPER )
//		SuperVUFlush(1, 1);
}
//------------------------------------------------------------------


//------------------------------------------------------------------
// vuSqSumXYZ()*
//
// NOTE: In all EFU insts, EEREC_D is a temp reg
//------------------------------------------------------------------
void vuSqSumXYZ(int regd, int regs, int regtemp) // regd.x =  x ^ 2 + y ^ 2 + z ^ 2
{
	//Console.WriteLn("VU: SUMXYZ");
	if( x86caps.hasStreamingSIMD4Extensions )
	{
		SSE_MOVAPS_XMM_to_XMM(regd, regs);
		if (CHECK_VU_EXTRA_OVERFLOW) vuFloat2(regd, regtemp, 0xf);
		SSE4_DPPS_XMM_to_XMM(regd, regd, 0x71);
	}
	else 
	{
		SSE_MOVAPS_XMM_to_XMM(regtemp, regs);
		if (CHECK_VU_EXTRA_OVERFLOW) vuFloat2(regtemp, regd, 0xf);
		SSE_MULPS_XMM_to_XMM(regtemp, regtemp); // xyzw ^ 2

		if( x86caps.hasStreamingSIMD3Extensions ) {
			SSE3_HADDPS_XMM_to_XMM(regd, regtemp);
			SSE_ADDPS_XMM_to_XMM(regd, regtemp); // regd.z = x ^ 2 + y ^ 2 + z ^ 2
			SSE_MOVHLPS_XMM_to_XMM(regd, regd); // regd.x = regd.z
		}
		else {
			SSE_MOVSS_XMM_to_XMM(regd, regtemp);
			SSE2_PSHUFLW_XMM_to_XMM(regtemp, regtemp, 0x4e); // wzyx -> wzxy
			SSE_ADDSS_XMM_to_XMM(regd, regtemp); // x ^ 2 + y ^ 2
			SSE_SHUFPS_XMM_to_XMM(regtemp, regtemp, 0xD2); // wzxy -> wxyz 
			SSE_ADDSS_XMM_to_XMM(regd, regtemp); // x ^ 2 + y ^ 2 + z ^ 2
		}
	}
}
//------------------------------------------------------------------


//------------------------------------------------------------------
// ESADD*
//------------------------------------------------------------------
void recVUMI_ESADD( VURegs *VU, int info)
{
	//Console.WriteLn("VU: ESADD");
	assert( VU == &VU1 );
	if( EEREC_TEMP == EEREC_D ) { // special code to reset P ( FixMe: don't know if this is still needed! (cottonvibes) )
		Console.Notice("ESADD: Resetting P reg!!!\n");
		MOV32ItoM(VU_VI_ADDR(REG_P, 0), 0);
		return;
	}
	vuSqSumXYZ(EEREC_D, EEREC_S, EEREC_TEMP);
	if (CHECK_VU_OVERFLOW) SSE_MINSS_M32_to_XMM(EEREC_D, (uptr)g_maxvals); // Only need to do positive clamp since (x ^ 2 + y ^ 2 + z ^ 2) is positive
	SSE_MOVSS_XMM_to_M32(VU_VI_ADDR(REG_P, 0), EEREC_D);
}
//------------------------------------------------------------------


//------------------------------------------------------------------
// ERSADD*
//------------------------------------------------------------------
void recVUMI_ERSADD( VURegs *VU, int info )
{
	//Console.WriteLn("VU: ERSADD");
	assert( VU == &VU1 );
	vuSqSumXYZ(EEREC_D, EEREC_S, EEREC_TEMP);
	// don't use RCPSS (very bad precision)
	SSE_MOVSS_M32_to_XMM(EEREC_TEMP, (uptr)VU_ONE);
	SSE_DIVSS_XMM_to_XMM(EEREC_TEMP, EEREC_D);
	if (CHECK_VU_OVERFLOW) SSE_MINSS_M32_to_XMM(EEREC_TEMP, (uptr)g_maxvals); // Only need to do positive clamp since (x ^ 2 + y ^ 2 + z ^ 2) is positive
	SSE_MOVSS_XMM_to_M32(VU_VI_ADDR(REG_P, 0), EEREC_TEMP);
}
//------------------------------------------------------------------


//------------------------------------------------------------------
// ELENG*
//------------------------------------------------------------------
void recVUMI_ELENG( VURegs *VU, int info )
{
	//Console.WriteLn("VU: ELENG");
	assert( VU == &VU1 );
	vuSqSumXYZ(EEREC_D, EEREC_S, EEREC_TEMP);
	if (CHECK_VU_OVERFLOW) SSE_MINSS_M32_to_XMM(EEREC_D, (uptr)g_maxvals); // Only need to do positive clamp since (x ^ 2 + y ^ 2 + z ^ 2) is positive
	SSE_SQRTSS_XMM_to_XMM(EEREC_D, EEREC_D);
	SSE_MOVSS_XMM_to_M32(VU_VI_ADDR(REG_P, 0), EEREC_D);
}
//------------------------------------------------------------------


//------------------------------------------------------------------
// ERLENG*
//------------------------------------------------------------------
void recVUMI_ERLENG( VURegs *VU, int info )
{
	//Console.WriteLn("VU: ERLENG");
	assert( VU == &VU1 );
	vuSqSumXYZ(EEREC_D, EEREC_S, EEREC_TEMP);
	if (CHECK_VU_OVERFLOW) SSE_MINSS_M32_to_XMM(EEREC_D, (uptr)g_maxvals); // Only need to do positive clamp since (x ^ 2 + y ^ 2 + z ^ 2) is positive
	SSE_SQRTSS_XMM_to_XMM(EEREC_D, EEREC_D); // regd <- sqrt(x^2 + y^2 + z^2)
	SSE_MOVSS_M32_to_XMM(EEREC_TEMP, (uptr)VU_ONE); // temp <- 1
	SSE_DIVSS_XMM_to_XMM(EEREC_TEMP, EEREC_D); // temp = 1 / sqrt(x^2 + y^2 + z^2)
	if (CHECK_VU_OVERFLOW) SSE_MINSS_M32_to_XMM(EEREC_TEMP, (uptr)g_maxvals); // Only need to do positive clamp
	SSE_MOVSS_XMM_to_M32(VU_VI_ADDR(REG_P, 0), EEREC_TEMP);
}
//------------------------------------------------------------------


//------------------------------------------------------------------
// EATANxy
//------------------------------------------------------------------
void recVUMI_EATANxy( VURegs *VU, int info )
{
	assert( VU == &VU1 );
	//Console.WriteLn("recVUMI_EATANxy");
	if( (xmmregs[EEREC_S].mode & MODE_WRITE) && (xmmregs[EEREC_S].mode&MODE_NOFLUSH) ) {
		SSE_MOVLPS_XMM_to_M64((uptr)s_tempmem, EEREC_S);
		FLD32((uptr)&s_tempmem[0]);
		FLD32((uptr)&s_tempmem[1]);
	}
	else {
		if( xmmregs[EEREC_S].mode & MODE_WRITE ) {
			SSE_MOVAPS_XMM_to_M128((uptr)&VU->VF[_Fs_], EEREC_S);
			xmmregs[EEREC_S].mode &= ~MODE_WRITE;
		}

		FLD32((uptr)&VU->VF[_Fs_].UL[0]);
		FLD32((uptr)&VU->VF[_Fs_].UL[1]);
	}

	FPATAN();
	FSTP32(VU_VI_ADDR(REG_P, 0));
}
//------------------------------------------------------------------


//------------------------------------------------------------------
// EATANxz
//------------------------------------------------------------------
void recVUMI_EATANxz( VURegs *VU, int info )
{
	assert( VU == &VU1 );
	//Console.WriteLn("recVUMI_EATANxz");
	if( (xmmregs[EEREC_S].mode & MODE_WRITE) && (xmmregs[EEREC_S].mode&MODE_NOFLUSH) ) {
		SSE_MOVLPS_XMM_to_M64((uptr)s_tempmem, EEREC_S);
		FLD32((uptr)&s_tempmem[0]);
		FLD32((uptr)&s_tempmem[2]);
	}
	else {
		if( xmmregs[EEREC_S].mode & MODE_WRITE ) {
			SSE_MOVAPS_XMM_to_M128((uptr)&VU->VF[_Fs_], EEREC_S);
			xmmregs[EEREC_S].mode &= ~MODE_WRITE;
		}

		FLD32((uptr)&VU->VF[_Fs_].UL[0]);
		FLD32((uptr)&VU->VF[_Fs_].UL[2]);
	}
	FPATAN();
	FSTP32(VU_VI_ADDR(REG_P, 0));
}
//------------------------------------------------------------------


//------------------------------------------------------------------
// ESUM*
//------------------------------------------------------------------
void recVUMI_ESUM( VURegs *VU, int info )
{
	//Console.WriteLn("VU: ESUM");
	assert( VU == &VU1 );

	if( x86caps.hasStreamingSIMD3Extensions ) {
		SSE_MOVAPS_XMM_to_XMM(EEREC_TEMP, EEREC_S);
		if (CHECK_VU_EXTRA_OVERFLOW) vuFloat_useEAX(info, EEREC_TEMP, 0xf);
		SSE3_HADDPS_XMM_to_XMM(EEREC_TEMP, EEREC_TEMP);
		SSE3_HADDPS_XMM_to_XMM(EEREC_TEMP, EEREC_TEMP);
	}
	else {
		SSE_MOVHLPS_XMM_to_XMM(EEREC_TEMP, EEREC_S); // z, w, z, w
		SSE_ADDPS_XMM_to_XMM(EEREC_TEMP, EEREC_S); // z+x, w+y, z+z, w+w
		SSE_UNPCKLPS_XMM_to_XMM(EEREC_TEMP, EEREC_TEMP); // z+x, z+x, w+y, w+y
		SSE_MOVHLPS_XMM_to_XMM(EEREC_D, EEREC_TEMP); // w+y, w+y, w+y, w+y
		SSE_ADDSS_XMM_to_XMM(EEREC_TEMP, EEREC_D); // x+y+z+w, w+y, w+y, w+y 
	}

	vuFloat_useEAX(info, EEREC_TEMP, 8);
	SSE_MOVSS_XMM_to_M32(VU_VI_ADDR(REG_P, 0), EEREC_TEMP);
}
//------------------------------------------------------------------


//------------------------------------------------------------------
// ERCPR*
//------------------------------------------------------------------
void recVUMI_ERCPR( VURegs *VU, int info )
{
	assert( VU == &VU1 );
	//Console.WriteLn("VU1: ERCPR");

	// don't use RCPSS (very bad precision)
	switch ( _Fsf_ ) {
		case 0: //0001
			if (CHECK_VU_EXTRA_OVERFLOW) vuFloat5_useEAX(EEREC_S, EEREC_TEMP, 8);
			SSE_MOVSS_M32_to_XMM(EEREC_TEMP, (uptr)VU_ONE); // temp <- 1
			SSE_DIVSS_XMM_to_XMM(EEREC_TEMP, EEREC_S);
			break;
		case 1: //0010
			SSE2_PSHUFLW_XMM_to_XMM(EEREC_S, EEREC_S, 0x4e);
			if (CHECK_VU_EXTRA_OVERFLOW) vuFloat5_useEAX(EEREC_S, EEREC_TEMP, 8);
			SSE_MOVSS_M32_to_XMM(EEREC_TEMP, (uptr)VU_ONE); // temp <- 1
			SSE_DIVSS_XMM_to_XMM(EEREC_TEMP, EEREC_S);
			SSE2_PSHUFLW_XMM_to_XMM(EEREC_S, EEREC_S, 0x4e);
			break;
		case 2: //0100
			SSE_SHUFPS_XMM_to_XMM(EEREC_S, EEREC_S, 0xc6);
			if (CHECK_VU_EXTRA_OVERFLOW) vuFloat5_useEAX(EEREC_S, EEREC_TEMP, 8);
			SSE_MOVSS_M32_to_XMM(EEREC_TEMP, (uptr)VU_ONE); // temp <- 1
			SSE_DIVSS_XMM_to_XMM(EEREC_TEMP, EEREC_S);
			SSE_SHUFPS_XMM_to_XMM(EEREC_S, EEREC_S, 0xc6);
			break;
		case 3: //1000
			SSE_SHUFPS_XMM_to_XMM(EEREC_S, EEREC_S, 0x27);
			if (CHECK_VU_EXTRA_OVERFLOW) vuFloat5_useEAX(EEREC_S, EEREC_TEMP, 8);
			SSE_MOVSS_M32_to_XMM(EEREC_TEMP, (uptr)VU_ONE); // temp <- 1
			SSE_DIVSS_XMM_to_XMM(EEREC_TEMP, EEREC_S);
			SSE_SHUFPS_XMM_to_XMM(EEREC_S, EEREC_S, 0x27);
			break;
	}

	vuFloat_useEAX(info, EEREC_TEMP, 8);
	SSE_MOVSS_XMM_to_M32(VU_VI_ADDR(REG_P, 0), EEREC_TEMP);
}
//------------------------------------------------------------------


//------------------------------------------------------------------
// ESQRT*
//------------------------------------------------------------------
void recVUMI_ESQRT( VURegs *VU, int info )
{
	assert( VU == &VU1 );

	//Console.WriteLn("VU1: ESQRT");
	_unpackVFSS_xyzw(EEREC_TEMP, EEREC_S, _Fsf_);
	SSE_ANDPS_M128_to_XMM(EEREC_TEMP, (uptr)const_clip); // abs(x)
	if (CHECK_VU_OVERFLOW) SSE_MINSS_M32_to_XMM(EEREC_TEMP, (uptr)g_maxvals); // Only need to do positive clamp
	SSE_SQRTSS_XMM_to_XMM(EEREC_TEMP, EEREC_TEMP);

	SSE_MOVSS_XMM_to_M32(VU_VI_ADDR(REG_P, 0), EEREC_TEMP);
}
//------------------------------------------------------------------


//------------------------------------------------------------------
// ERSQRT*
//------------------------------------------------------------------
void recVUMI_ERSQRT( VURegs *VU, int info )
{
	int t1reg = _vuGetTempXMMreg(info);
 
	assert( VU == &VU1 );
	//Console.WriteLn("VU1: ERSQRT");

	_unpackVFSS_xyzw(EEREC_TEMP, EEREC_S, _Fsf_);
	SSE_ANDPS_M128_to_XMM(EEREC_TEMP, (uptr)const_clip); // abs(x)
	SSE_MINSS_M32_to_XMM(EEREC_TEMP, (uptr)g_maxvals); // Clamp Infinities to Fmax
	SSE_SQRTSS_XMM_to_XMM(EEREC_TEMP, EEREC_TEMP); // SQRT(abs(x))

	if( t1reg >= 0 ) 
	{
		SSE_MOVSS_M32_to_XMM(t1reg, (uptr)VU_ONE);
		SSE_DIVSS_XMM_to_XMM(t1reg, EEREC_TEMP);
		vuFloat_useEAX(info, t1reg, 8);
		SSE_MOVSS_XMM_to_M32(VU_VI_ADDR(REG_P, 0), t1reg);
		_freeXMMreg(t1reg);
	}
	else
	{
		SSE_MOVSS_XMM_to_M32(VU_VI_ADDR(REG_P, 0), EEREC_TEMP);
		SSE_MOVSS_M32_to_XMM(EEREC_TEMP, (uptr)VU_ONE);
		SSE_DIVSS_M32_to_XMM(EEREC_TEMP, VU_VI_ADDR(REG_P, 0));
		vuFloat_useEAX(info, EEREC_TEMP, 8);
		SSE_MOVSS_XMM_to_M32(VU_VI_ADDR(REG_P, 0), EEREC_TEMP);
	}
}
//------------------------------------------------------------------


//------------------------------------------------------------------
// ESIN
//------------------------------------------------------------------
void recVUMI_ESIN( VURegs *VU, int info )
{
	assert( VU == &VU1 );

	//Console.WriteLn("recVUMI_ESIN");
	if( (xmmregs[EEREC_S].mode & MODE_WRITE) && (xmmregs[EEREC_S].mode&MODE_NOFLUSH) ) {
		switch(_Fsf_) {
			case 0: SSE_MOVSS_XMM_to_M32((uptr)s_tempmem, EEREC_S);
			case 1: SSE_MOVLPS_XMM_to_M64((uptr)s_tempmem, EEREC_S);
			default: SSE_MOVHPS_XMM_to_M64((uptr)&s_tempmem[2], EEREC_S);
		}
		FLD32((uptr)&s_tempmem[_Fsf_]);
	}
	else {
		if( xmmregs[EEREC_S].mode & MODE_WRITE ) {
			SSE_MOVAPS_XMM_to_M128((uptr)&VU->VF[_Fs_], EEREC_S);
			xmmregs[EEREC_S].mode &= ~MODE_WRITE;
		}

		FLD32((uptr)&VU->VF[_Fs_].UL[_Fsf_]);
	}

	FSIN();
	FSTP32(VU_VI_ADDR(REG_P, 0));
}
//------------------------------------------------------------------


//------------------------------------------------------------------
// EATAN
//------------------------------------------------------------------
void recVUMI_EATAN( VURegs *VU, int info )
{
	assert( VU == &VU1 );

	//Console.WriteLn("recVUMI_EATAN");
	if( (xmmregs[EEREC_S].mode & MODE_WRITE) && (xmmregs[EEREC_S].mode&MODE_NOFLUSH) ) {
		switch(_Fsf_) {
			case 0: SSE_MOVSS_XMM_to_M32((uptr)s_tempmem, EEREC_S);
			case 1: SSE_MOVLPS_XMM_to_M64((uptr)s_tempmem, EEREC_S);
			default: SSE_MOVHPS_XMM_to_M64((uptr)&s_tempmem[2], EEREC_S);
		}
		FLD32((uptr)&s_tempmem[_Fsf_]);
	}
	else {
		if( xmmregs[EEREC_S].mode & MODE_WRITE ) {
			SSE_MOVAPS_XMM_to_M128((uptr)&VU->VF[_Fs_], EEREC_S);
			xmmregs[EEREC_S].mode &= ~MODE_WRITE;
		}
	}

	FLD1();
	FLD32((uptr)&VU->VF[_Fs_].UL[_Fsf_]);
	FPATAN();
	FSTP32(VU_VI_ADDR(REG_P, 0));
}
//------------------------------------------------------------------


//------------------------------------------------------------------
// EEXP
//------------------------------------------------------------------
void recVUMI_EEXP( VURegs *VU, int info )
{
	assert( VU == &VU1 );
	//Console.WriteLn("recVUMI_EEXP");
	FLDL2E();

	if( (xmmregs[EEREC_S].mode & MODE_WRITE) && (xmmregs[EEREC_S].mode&MODE_NOFLUSH) ) {
		switch(_Fsf_) {
			case 0: SSE_MOVSS_XMM_to_M32((uptr)s_tempmem, EEREC_S);
			case 1: SSE_MOVLPS_XMM_to_M64((uptr)s_tempmem, EEREC_S);
			default: SSE_MOVHPS_XMM_to_M64((uptr)&s_tempmem[2], EEREC_S);
		}
		FMUL32((uptr)&s_tempmem[_Fsf_]);
	}
	else {
		if( xmmregs[EEREC_S].mode & MODE_WRITE ) {
			SSE_MOVAPS_XMM_to_M128((uptr)&VU->VF[_Fs_], EEREC_S);
			xmmregs[EEREC_S].mode &= ~MODE_WRITE;
		}

		FMUL32((uptr)&VU->VF[_Fs_].UL[_Fsf_]);
	}

	// basically do 2^(log_2(e) * val)
	FLD(0);
	FRNDINT();
	FXCH(1);
	FSUB32Rto0(1);
	F2XM1();
	FLD1();
	FADD320toR(1);
	FSCALE();
	FSTP(1);
	
	FSTP32(VU_VI_ADDR(REG_P, 0));
}
//------------------------------------------------------------------


//------------------------------------------------------------------
// XITOP
//------------------------------------------------------------------
void recVUMI_XITOP( VURegs *VU, int info )
{
	int itreg;
	if (_It_ == 0) return;
	//Console.WriteLn("recVUMI_XITOP");
	itreg = ALLOCVI(_It_, MODE_WRITE);
	MOVZX32M16toR( itreg, (uptr)&VU->vifRegs->itop );
}
//------------------------------------------------------------------


//------------------------------------------------------------------
// XTOP
//------------------------------------------------------------------
void recVUMI_XTOP( VURegs *VU, int info )
{
	int itreg;
	if ( _It_ == 0 ) return;
	//Console.WriteLn("recVUMI_XTOP");
	itreg = ALLOCVI(_It_, MODE_WRITE);
	MOVZX32M16toR( itreg, (uptr)&VU->vifRegs->top );
}
//------------------------------------------------------------------


//------------------------------------------------------------------
// VU1XGKICK_MTGSTransfer() - Called by ivuZerorec.cpp
//------------------------------------------------------------------
void VU1XGKICK_MTGSTransfer(u32 *pMem, u32 addr)
{
	u32 size;
    u8* data = ((u8*)pMem + (addr&0x3fff));
	
	size = mtgsThread.PrepDataPacket(GIF_PATH_1, data, (0x4000-(addr&0x3fff)) / 16);
    jASSUME( size > 0 );
	
	u8* pmem = mtgsThread.GetDataPacketPtr();
	
	if((size << 4) > (0x4000-(addr&0x3fff))) 
	{
		//DevCon.Notice("addr + Size = 0x%x, transferring %x then doing %x", (addr&0x3fff) + (size << 4), (0x4000-(addr&0x3fff)) >> 4, size - ((0x4000-(addr&0x3fff)) >> 4));
		memcpy_aligned(pmem, (u8*)pMem+addr, 0x4000-(addr&0x3fff));
		size -= (0x4000-(addr&0x3fff)) >> 4;
		//DevCon.Notice("Size left %x", size);
		pmem += 0x4000-(addr&0x3fff);
		memcpy_aligned(pmem, (u8*)pMem, size<<4);
	}
	else {
		memcpy_aligned(pmem, (u8*)pMem+addr, size<<4);
	}

	mtgsThread.SendDataPacket();
}
//------------------------------------------------------------------
