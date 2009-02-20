/*  Pcsx2 - Pc Ps2 Emulator
 *  Copyright (C) 2002-2009  Pcsx2 Team
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
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "PrecompiledHeader.h"

#include "Common.h"
#include "GS.h"
#include "R5900OpcodeTables.h"
#include "ix86/ix86.h"
#include "iR5900.h"
#include "iMMI.h"
#include "iFPU.h"
#include "iCOP0.h"
#include "VUmicro.h"
#include "VUflags.h"
#include "iVUmicro.h"
#include "iVUops.h"
#include "iVUzerorec.h"
//------------------------------------------------------------------

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


static const PCSX2_ALIGNED16(u32 VU_ONE[4]) = {0x3f800000, 0xffffffff, 0xffffffff, 0xffffffff};
//------------------------------------------------------------------


//------------------------------------------------------------------
// *VU Lower Instructions!*
//
// Note: * = Checked for errors by cottonvibes
//------------------------------------------------------------------


//------------------------------------------------------------------
// DIV*
//------------------------------------------------------------------
PCSX2_ALIGNED16(u64 DIV_TEMP_XMM[2]);
void recVUMI_DIV(VURegs *VU, int info)
{
	u8 *pjmp, *pjmp1;
	u32 *ajmp32, *bjmp32;

	//SysPrintf("recVUMI_DIV()\n");
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
	//SysPrintf("recVUMI_SQRT()\n");

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
PCSX2_ALIGNED16(u64 RSQRT_TEMP_XMM[2]);
void recVUMI_RSQRT(VURegs *VU, int info)
{
	u8 *ajmp8, *bjmp8;
	int t1reg, t1boolean;
	//SysPrintf("recVUMI_RSQRT()\n");

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
		OR32ItoM(VU_VI_ADDR(REG_STATUS_FLAG, 2), 0x820); // Zero divide flag
		
		_unpackVFSS_xyzw(EEREC_TEMP, EEREC_S, _Fsf_);
		SSE_ANDPS_M128_to_XMM(EEREC_TEMP, (uptr)&const_clip[4]);
		SSE_ORPS_M128_to_XMM(EEREC_TEMP, (uptr)&g_maxvals[0]); // EEREC_TEMP = +/-Max
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
	int fsreg = -1, ftreg;
	if (_Ft_ == 0) return;

	if( _Fs_ == 0 ) {
		ftreg = ALLOCVI(_Ft_, MODE_WRITE);
		MOV32ItoR(ftreg, imm&0xffff);
		return;
	}

	ADD_VI_NEEDED(_Ft_);
	fsreg = ALLOCVI(_Fs_, MODE_READ);
	ftreg = ALLOCVI(_Ft_, MODE_WRITE);

	if ( _Ft_ == _Fs_ ) {
		if (imm != 0 ) ADD16ItoR(ftreg, imm);
	} 
	else {
		if( imm ) {
			LEA32RtoR(ftreg, fsreg, imm);
			MOVZX32R16toR(ftreg, ftreg);
		}
		else MOV32RtoR(ftreg, fsreg);
	}
}
//------------------------------------------------------------------


//------------------------------------------------------------------
// IADDI
//------------------------------------------------------------------
void recVUMI_IADDI(VURegs *VU, int info)
{
	s16 imm;

	if ( _Ft_ == 0 ) return;
	//SysPrintf("recVUMI_IADDI  \n");
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

	if ( _Ft_ == 0 ) return;
	//SysPrintf("recVUMI_IADDIU  \n");
	imm = ( ( VU->code >> 10 ) & 0x7800 ) | ( VU->code & 0x7ff );
	_addISIMMtoIT(VU, imm, info);
}
//------------------------------------------------------------------


//------------------------------------------------------------------
// IADD
//------------------------------------------------------------------
void recVUMI_IADD( VURegs *VU, int info )
{
	int fdreg, fsreg = -1, ftreg = -1;
	if ( _Fd_ == 0 ) return;
	//SysPrintf("recVUMI_IADD  \n");
	if ( ( _Ft_ == 0 ) && ( _Fs_ == 0 ) ) {
		fdreg = ALLOCVI(_Fd_, MODE_WRITE);
		XOR32RtoR(fdreg, fdreg);
		return;
	}

	ADD_VI_NEEDED(_Fs_);
	ADD_VI_NEEDED(_Ft_);
	fdreg = ALLOCVI(_Fd_, MODE_WRITE);

	if ( _Fs_ == 0 )
	{
		if( (ftreg = _checkX86reg(X86TYPE_VI|((VU==&VU1)?X86TYPE_VU1:0), _Ft_, MODE_READ)) >= 0 ) {
			if( fdreg != ftreg ) MOV32RtoR(fdreg, ftreg);
		}
		else MOVZX32M16toR(fdreg, VU_VI_ADDR(_Ft_, 1));
	}
	else if ( _Ft_ == 0 )
	{
		if( (fsreg = _checkX86reg(X86TYPE_VI|((VU==&VU1)?X86TYPE_VU1:0), _Fs_, MODE_READ)) >= 0 ) {
			if( fdreg != fsreg ) MOV32RtoR(fdreg, fsreg);
		}
		else MOVZX32M16toR(fdreg, VU_VI_ADDR(_Fs_, 1));
	}
	else {
		//ADD_VI_NEEDED(_Ft_);
		fsreg = ALLOCVI(_Fs_, MODE_READ);
		ftreg = ALLOCVI(_Ft_, MODE_READ);

		if( fdreg == fsreg ) ADD32RtoR(fdreg, ftreg);
		else if( fdreg == ftreg ) ADD32RtoR(fdreg, fsreg);
		else LEA16RRtoR(fdreg, fsreg, ftreg);
		MOVZX32R16toR(fdreg, fdreg); // neeed since don't know if fdreg's upper bits are 0
	}
}
//------------------------------------------------------------------


//------------------------------------------------------------------
// IAND
//------------------------------------------------------------------
void recVUMI_IAND( VURegs *VU, int info )
{
	int fdreg, fsreg = -1, ftreg = -1;
	if ( _Fd_ == 0 ) return;
	//SysPrintf("recVUMI_IAND  \n");
	if ( ( _Fs_ == 0 ) || ( _Ft_ == 0 ) ) {
		fdreg = ALLOCVI(_Fd_, MODE_WRITE);
		XOR32RtoR(fdreg, fdreg);
		return;
	}

	ADD_VI_NEEDED(_Fs_);
	ADD_VI_NEEDED(_Ft_);
	fdreg = ALLOCVI(_Fd_, MODE_WRITE);

	fsreg = ALLOCVI(_Fs_, MODE_READ);
	ftreg = ALLOCVI(_Ft_, MODE_READ);

	if( fdreg == fsreg ) AND16RtoR(fdreg, ftreg);
	else if( fdreg == ftreg ) AND16RtoR(fdreg, fsreg);
	else {
		MOV32RtoR(fdreg, ftreg);
		AND32RtoR(fdreg, fsreg);
	}
}
//------------------------------------------------------------------


//------------------------------------------------------------------
// IOR
//------------------------------------------------------------------
void recVUMI_IOR( VURegs *VU, int info )
{
	int fdreg, fsreg = -1, ftreg = -1;
	if ( _Fd_ == 0 ) return;
	//SysPrintf("recVUMI_IOR  \n");
	if ( ( _Ft_ == 0 ) && ( _Fs_ == 0 ) ) {
		fdreg = ALLOCVI(_Fd_, MODE_WRITE);
		XOR32RtoR(fdreg, fdreg);
		return;
	} 

	ADD_VI_NEEDED(_Fs_);
	ADD_VI_NEEDED(_Ft_);
	fdreg = ALLOCVI(_Fd_, MODE_WRITE);

	if ( _Fs_ == 0 )
	{
		if( (ftreg = _checkX86reg(X86TYPE_VI|((VU==&VU1)?X86TYPE_VU1:0), _Ft_, MODE_READ)) >= 0 ) {
			if( fdreg != ftreg ) MOV32RtoR(fdreg, ftreg);
		}
		else MOVZX32M16toR(fdreg, VU_VI_ADDR(_Ft_, 1));
	}
	else if ( _Ft_ == 0 )
	{
		if( (fsreg = _checkX86reg(X86TYPE_VI|((VU==&VU1)?X86TYPE_VU1:0), _Fs_, MODE_READ)) >= 0 ) {
			if( fdreg != fsreg ) MOV32RtoR(fdreg, fsreg);
		}
		else MOVZX32M16toR(fdreg, VU_VI_ADDR(_Fs_, 1));
	}
	else
	{
		fsreg = ALLOCVI(_Fs_, MODE_READ);
		ftreg = ALLOCVI(_Ft_, MODE_READ);

		if( fdreg == fsreg ) OR16RtoR(fdreg, ftreg);
		else if( fdreg == ftreg ) OR16RtoR(fdreg, fsreg);
		else {
			MOV32RtoR(fdreg, fsreg);
			OR32RtoR(fdreg, ftreg);
		}
	}
}
//------------------------------------------------------------------


//------------------------------------------------------------------
// ISUB
//------------------------------------------------------------------
void recVUMI_ISUB( VURegs *VU, int info )
{
	int fdreg, fsreg = -1, ftreg = -1;
	if ( _Fd_ == 0 ) return;
	//SysPrintf("recVUMI_ISUB  \n");
	if ( ( _Ft_ == 0 ) && ( _Fs_ == 0 ) ) {
		fdreg = ALLOCVI(_Fd_, MODE_WRITE);
		XOR32RtoR(fdreg, fdreg);
		return;
	} 
	
	ADD_VI_NEEDED(_Fs_);
	ADD_VI_NEEDED(_Ft_);
	fdreg = ALLOCVI(_Fd_, MODE_WRITE);

	if ( _Fs_ == 0 )
	{
		if( (ftreg = _checkX86reg(X86TYPE_VI|((VU==&VU1)?X86TYPE_VU1:0), _Ft_, MODE_READ)) >= 0 ) {
			if( fdreg != ftreg ) MOV32RtoR(fdreg, ftreg);
		}
		else MOVZX32M16toR(fdreg, VU_VI_ADDR(_Ft_, 1));
		NEG16R(fdreg);
	}
	else if ( _Ft_ == 0 )
	{
		if( (fsreg = _checkX86reg(X86TYPE_VI|((VU==&VU1)?X86TYPE_VU1:0), _Fs_, MODE_READ)) >= 0 ) {
			if( fdreg != fsreg ) MOV32RtoR(fdreg, fsreg);
		}
		else MOVZX32M16toR(fdreg, VU_VI_ADDR(_Fs_, 1));
	}
	else
	{
		fsreg = ALLOCVI(_Fs_, MODE_READ);
		ftreg = ALLOCVI(_Ft_, MODE_READ);

		if( fdreg == fsreg ) SUB16RtoR(fdreg, ftreg);
		else if( fdreg == ftreg ) {
			SUB16RtoR(fdreg, fsreg);
			NEG16R(fdreg);
		}
		else {
			MOV32RtoR(fdreg, fsreg);
			SUB16RtoR(fdreg, ftreg);
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

	if ( _Ft_ == 0 ) return;
	//SysPrintf("recVUMI_ISUBIU  \n");
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
	//SysPrintf("recVUMI_MOVE  \n");
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
	//SysPrintf("recVUMI_MFIR  \n");
	_deleteX86reg(X86TYPE_VI|((VU==&VU1)?X86TYPE_VU1:0), _Fs_, 1);

	if( _XYZW_SS ) {
		SSE2_MOVD_M32_to_XMM(EEREC_TEMP, VU_VI_ADDR(_Fs_, 1)-2);
		_vuFlipRegSS(VU, EEREC_T);
		SSE2_PSRAD_I8_to_XMM(EEREC_TEMP, 16);
		SSE_MOVSS_XMM_to_XMM(EEREC_T, EEREC_TEMP);
		_vuFlipRegSS(VU, EEREC_T);
	}
	else if (_X_Y_Z_W != 0xf) {
		SSE2_MOVD_M32_to_XMM(EEREC_TEMP, VU_VI_ADDR(_Fs_, 1)-2);
		SSE2_PSRAD_I8_to_XMM(EEREC_TEMP, 16);
		SSE_SHUFPS_XMM_to_XMM(EEREC_TEMP, EEREC_TEMP, 0);
		VU_MERGE_REGS(EEREC_T, EEREC_TEMP);
	} 
	else {
		SSE2_MOVD_M32_to_XMM(EEREC_T, VU_VI_ADDR(_Fs_, 1)-2);
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
	if ( _Ft_ == 0 ) return;
	//SysPrintf("recVUMI_MTIR  \n");
	_deleteX86reg(X86TYPE_VI|((VU==&VU1)?X86TYPE_VU1:0), _Ft_, 2);

	if( _Fsf_ == 0 ) {
		SSE_MOVSS_XMM_to_M32(VU_VI_ADDR(_Ft_, 0), EEREC_S);
	}
	else {
		_unpackVFSS_xyzw(EEREC_TEMP, EEREC_S, _Fsf_);
		SSE_MOVSS_XMM_to_M32(VU_VI_ADDR(_Ft_, 0), EEREC_TEMP);
	}

	AND32ItoM(VU_VI_ADDR(_Ft_, 0), 0xffff);
} 
//------------------------------------------------------------------


//------------------------------------------------------------------
// MR32*
//------------------------------------------------------------------
void recVUMI_MR32( VURegs *VU, int info )
{	
	if ( (_Ft_ == 0) || (_X_Y_Z_W == 0) ) return;
	//SysPrintf("recVUMI_MR32  \n");
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
				SSE_MOVHPS_RmOffset_to_XMM(EEREC_T, x86reg, offset+8);
				break;
			case 6: // YZ
				SSE_SHUFPS_RmOffset_to_XMM(EEREC_T, x86reg, offset, 0x9c);
				SSE_SHUFPS_XMM_to_XMM(EEREC_T, EEREC_T, 0x78);
				break;

			case 8: // X
				SSE_MOVSS_RmOffset_to_XMM(EEREC_TEMP, x86reg, offset);
				SSE_MOVSS_XMM_to_XMM(EEREC_T, EEREC_TEMP);
				break;
			case 9: // XW
				SSE_SHUFPS_RmOffset_to_XMM(EEREC_T, x86reg, offset, 0xc9);
				SSE_SHUFPS_XMM_to_XMM(EEREC_T, EEREC_T, 0xd2);
				break;
			case 12: // XY
				SSE_MOVLPS_RmOffset_to_XMM(EEREC_T, x86reg, offset);
				break;
			case 15:
				if( VU == &VU1 ) SSE_MOVAPSRmtoROffset(EEREC_T, x86reg, offset);
				else SSE_MOVUPSRmtoROffset(EEREC_T, x86reg, offset);
				break;
			default:
				if( VU == &VU1 ) SSE_MOVAPSRmtoROffset(EEREC_TEMP, x86reg, offset);
				else SSE_MOVUPSRmtoROffset(EEREC_TEMP, x86reg, offset);

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
	//SysPrintf("recVUMI_LQ  \n");
	imm = (VU->code & 0x400) ? (VU->code & 0x3ff) | 0xfc00 : (VU->code & 0x3ff); 
	if (_Fs_ == 0) {
		_loadEAX(VU, -1, (uptr)GET_VU_MEM(VU, (u32)imm*16), info);
	} 
	else {
		int fsreg = ALLOCVI(_Fs_, MODE_READ);
		_loadEAX(VU, recVUTransformAddr(fsreg, VU, _Fs_, imm), (uptr)VU->Mem, info);
	}
}
//------------------------------------------------------------------


//------------------------------------------------------------------
// LQD
//------------------------------------------------------------------
void recVUMI_LQD( VURegs *VU, int info )
{
	int fsreg;
	//SysPrintf("recVUMI_LQD  \n");
	if ( _Fs_ != 0 ) {
		fsreg = ALLOCVI(_Fs_, MODE_READ|MODE_WRITE);
		SUB16ItoR( fsreg, 1 );
	}

	if ( _Ft_ == 0 ) return;

	if ( _Fs_ == 0 ) _loadEAX(VU, -1, (uptr)VU->Mem, info);
	else _loadEAX(VU, recVUTransformAddr(fsreg, VU, _Fs_, 0), (uptr)VU->Mem, info);
}
//------------------------------------------------------------------


//------------------------------------------------------------------
// LQI
//------------------------------------------------------------------
void recVUMI_LQI(VURegs *VU, int info)
{
	int fsreg;
	//SysPrintf("recVUMI_LQI  \n");
	if ( _Ft_ == 0 ) {
		if( _Fs_ != 0 ) {
			if( (fsreg = _checkX86reg(X86TYPE_VI|(VU==&VU1?X86TYPE_VU1:0), _Fs_, MODE_WRITE|MODE_READ)) >= 0 ) {
				ADD16ItoR(fsreg, 1);
			}
			else {
				ADD16ItoM( VU_VI_ADDR( _Fs_, 0 ), 1 );
			}
		}
		return;
	}

    if (_Fs_ == 0) {
		_loadEAX(VU, -1, (uptr)VU->Mem, info);
    } 
	else {
		fsreg = ALLOCVI(_Fs_, MODE_READ|MODE_WRITE);
		_loadEAX(VU, recVUTransformAddr(fsreg, VU, _Fs_, 0), (uptr)VU->Mem, info);
		ADD16ItoR( fsreg, 1 );
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
			if ( x86reg >= 0 ) MOV32ItoRmOffset(x86reg, c, offset+(_W?12:(_Z?8:(_Y?4:0))));
			else MOV32ItoM(offset+(_W?12:(_Z?8:(_Y?4:0))), c);
		}
		else {
			if ( x86reg >= 0 ) {
				if ( _X ) MOV32ItoRmOffset(x86reg, 0x00000000, offset);
				if ( _Y ) MOV32ItoRmOffset(x86reg, 0x00000000, offset+4);
				if ( _Z ) MOV32ItoRmOffset(x86reg, 0x00000000, offset+8);
				if ( _W ) MOV32ItoRmOffset(x86reg, 0x3f800000, offset+12);
			}
			else {
				if ( _X ) MOV32ItoM(offset,		0x00000000);
				if ( _Y ) MOV32ItoM(offset+4,	0x00000000);
				if ( _Z ) MOV32ItoM(offset+8,	0x00000000);
				if ( _W ) MOV32ItoM(offset+12,	0x3f800000);
			}
		}
		return;
	}

	switch ( _X_Y_Z_W ) {
		case 1: // W
			SSE2_PSHUFD_XMM_to_XMM(EEREC_TEMP, EEREC_S, 0x27);
			if ( x86reg >= 0 ) SSE_MOVSS_XMM_to_RmOffset(x86reg, EEREC_TEMP, offset+12);
			else SSE_MOVSS_XMM_to_M32(offset+12, EEREC_TEMP);
			break;
		case 2: // Z
			SSE_MOVHLPS_XMM_to_XMM(EEREC_TEMP, EEREC_S);
			if ( x86reg >= 0 ) SSE_MOVSS_XMM_to_RmOffset(x86reg, EEREC_TEMP, offset+8);
			else SSE_MOVSS_XMM_to_M32(offset+8, EEREC_TEMP);
			break;
		case 3: // ZW
			if ( x86reg >= 0 ) SSE_MOVHPS_XMM_to_RmOffset(x86reg, EEREC_S, offset+8);
			else SSE_MOVHPS_XMM_to_M64(offset+8, EEREC_S);
			break;
		case 4: // Y
			SSE2_PSHUFLW_XMM_to_XMM(EEREC_TEMP, EEREC_S, 0x4e);
			if ( x86reg >= 0 ) SSE_MOVSS_XMM_to_RmOffset(x86reg, EEREC_TEMP, offset+4);
			else SSE_MOVSS_XMM_to_M32(offset+4, EEREC_TEMP);
			break;
		case 5: // YW
			SSE_SHUFPS_XMM_to_XMM(EEREC_S, EEREC_S, 0xB1);
			SSE_MOVHLPS_XMM_to_XMM(EEREC_TEMP, EEREC_S);
			if ( x86reg >= 0 ) {
				SSE_MOVSS_XMM_to_RmOffset(x86reg, EEREC_S, offset+4);
				SSE_MOVSS_XMM_to_RmOffset(x86reg, EEREC_TEMP, offset+12);
			}
			else {
				SSE_MOVSS_XMM_to_M32(offset+4, EEREC_S);
				SSE_MOVSS_XMM_to_M32(offset+12, EEREC_TEMP);
			}
			SSE_SHUFPS_XMM_to_XMM(EEREC_S, EEREC_S, 0xB1);
			break;
		case 6: // YZ
			SSE2_PSHUFD_XMM_to_XMM(EEREC_TEMP, EEREC_S, 0xc9);
			if ( x86reg >= 0 ) SSE_MOVLPS_XMM_to_RmOffset(x86reg, EEREC_TEMP, offset+4);
			else SSE_MOVLPS_XMM_to_M64(offset+4, EEREC_TEMP);
			break;
		case 7: // YZW
			SSE2_PSHUFD_XMM_to_XMM(EEREC_TEMP, EEREC_S, 0x93); //ZYXW
			if ( x86reg >= 0 ) {
				SSE_MOVHPS_XMM_to_RmOffset(x86reg, EEREC_TEMP, offset+4);
				SSE_MOVSS_XMM_to_RmOffset(x86reg, EEREC_TEMP, offset+12);
			}
			else {
				SSE_MOVHPS_XMM_to_M64(offset+4, EEREC_TEMP);
				SSE_MOVSS_XMM_to_M32(offset+12, EEREC_TEMP);
			}
			break;
		case 8: // X
			if ( x86reg >= 0 ) SSE_MOVSS_XMM_to_RmOffset(x86reg, EEREC_S, offset);
			else SSE_MOVSS_XMM_to_M32(offset, EEREC_S);
			break;
		case 9: // XW
			SSE_MOVHLPS_XMM_to_XMM(EEREC_TEMP, EEREC_S);
			if ( x86reg >= 0 ) SSE_MOVSS_XMM_to_RmOffset(x86reg, EEREC_S, offset);
			else SSE_MOVSS_XMM_to_M32(offset, EEREC_S);
			
			if ( cpucaps.hasStreamingSIMD3Extensions ) SSE3_MOVSLDUP_XMM_to_XMM(EEREC_TEMP, EEREC_TEMP);
			else SSE_SHUFPS_XMM_to_XMM(EEREC_TEMP, EEREC_TEMP, 0x55);

			if ( x86reg >= 0 ) SSE_MOVSS_XMM_to_RmOffset(x86reg, EEREC_TEMP, offset+12);
			else SSE_MOVSS_XMM_to_M32(offset+12, EEREC_TEMP);

			break;
		case 10: //XZ
			SSE_MOVHLPS_XMM_to_XMM(EEREC_TEMP, EEREC_S);
			if ( x86reg >= 0 ) {
				SSE_MOVSS_XMM_to_RmOffset(x86reg, EEREC_S, offset);
				SSE_MOVSS_XMM_to_RmOffset(x86reg, EEREC_TEMP, offset+8);
			}
			else {
				SSE_MOVSS_XMM_to_M32(offset, EEREC_S);
				SSE_MOVSS_XMM_to_M32(offset+8, EEREC_TEMP);
			}
			break;
		case 11: //XZW
			if ( x86reg >= 0 ) {
				SSE_MOVSS_XMM_to_RmOffset(x86reg, EEREC_S, offset);
				SSE_MOVHPS_XMM_to_RmOffset(x86reg, EEREC_S, offset+8);
			}
			else {
				SSE_MOVSS_XMM_to_M32(offset, EEREC_S);
				SSE_MOVHPS_XMM_to_M64(offset+8, EEREC_S);
			}
			break;
		case 12: // XY
			if ( x86reg >= 0 ) SSE_MOVLPS_XMM_to_RmOffset(x86reg, EEREC_S, offset+0);
			else SSE_MOVLPS_XMM_to_M64(offset, EEREC_S);
			break;
		case 13: // XYW
			SSE2_PSHUFD_XMM_to_XMM(EEREC_TEMP, EEREC_S, 0x4b); //YXZW
			if ( x86reg >= 0 ) {
				SSE_MOVHPS_XMM_to_RmOffset(x86reg, EEREC_TEMP, offset+0);
				SSE_MOVSS_XMM_to_RmOffset(x86reg, EEREC_TEMP, offset+12);
			}
			else {
				SSE_MOVHPS_XMM_to_M64(offset, EEREC_TEMP);
				SSE_MOVSS_XMM_to_M32(offset+12, EEREC_TEMP);
			}
			break;
		case 14: // XYZ
			SSE_MOVHLPS_XMM_to_XMM(EEREC_TEMP, EEREC_S);
			if ( x86reg >= 0 ) {
				SSE_MOVLPS_XMM_to_RmOffset(x86reg, EEREC_S, offset+0);
				SSE_MOVSS_XMM_to_RmOffset(x86reg, EEREC_TEMP, offset+8);
			}
			else {
				SSE_MOVLPS_XMM_to_M64(offset, EEREC_S);
				SSE_MOVSS_XMM_to_M32(offset+8, EEREC_TEMP);
			}
			break;
		case 15: // XYZW
			if ( VU == &VU1 ) {
				if( x86reg >= 0 ) SSE_MOVAPSRtoRmOffset(x86reg, EEREC_S, offset+0);
				else SSE_MOVAPS_XMM_to_M128(offset, EEREC_S);
			}
			else {
				if( x86reg >= 0 ) SSE_MOVUPSRtoRmOffset(x86reg, EEREC_S, offset+0);
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
	//SysPrintf("recVUMI_SQ  \n");
	imm = ( VU->code & 0x400) ? ( VU->code & 0x3ff) | 0xfc00 : ( VU->code & 0x3ff); 
	if ( _Ft_ == 0 ) _saveEAX(VU, -1, (uptr)GET_VU_MEM(VU, (int)imm * 16), info);
	else {
		int ftreg = ALLOCVI(_Ft_, MODE_READ);
		_saveEAX(VU, recVUTransformAddr(ftreg, VU, _Ft_, imm), (uptr)VU->Mem, info);
	}
}
//------------------------------------------------------------------


//------------------------------------------------------------------
// SQD
//------------------------------------------------------------------
void recVUMI_SQD(VURegs *VU, int info)
{
	//SysPrintf("recVUMI_SQD  \n");
	if (_Ft_ == 0) _saveEAX(VU, -1, (uptr)VU->Mem, info);
	else {
		int ftreg = ALLOCVI(_Ft_, MODE_READ|MODE_WRITE);
		SUB16ItoR( ftreg, 1 );
		_saveEAX(VU, recVUTransformAddr(ftreg, VU, _Ft_, 0), (uptr)VU->Mem, info);
	}
}
//------------------------------------------------------------------


//------------------------------------------------------------------
// SQI
//------------------------------------------------------------------
void recVUMI_SQI(VURegs *VU, int info)
{
	//SysPrintf("recVUMI_SQI  \n");
	if (_Ft_ == 0) _saveEAX(VU, -1, (uptr)VU->Mem, info);
	else {
		int ftreg = ALLOCVI(_Ft_, MODE_READ|MODE_WRITE);
		_saveEAX(VU, recVUTransformAddr(ftreg, VU, _Ft_, 0), (uptr)VU->Mem, info);
		ADD16ItoR( ftreg, 1 );
	}
}
//------------------------------------------------------------------


//------------------------------------------------------------------
// ILW
//------------------------------------------------------------------
void recVUMI_ILW(VURegs *VU, int info)
{
	int ftreg;
	s16 imm, off;
 
	if ( ( _Ft_ == 0 ) || ( _X_Y_Z_W == 0 ) ) return;
	//SysPrintf("recVUMI_ILW  \n");
	imm = ( VU->code & 0x400) ? ( VU->code & 0x3ff) | 0xfc00 : ( VU->code & 0x3ff);
	if (_X) off = 0;
	else if (_Y) off = 4;
	else if (_Z) off = 8;
	else if (_W) off = 12;

	ADD_VI_NEEDED(_Fs_);
	ftreg = ALLOCVI(_Ft_, MODE_WRITE);

	if ( _Fs_ == 0 ) {
		MOVZX32M16toR( ftreg, (uptr)GET_VU_MEM(VU, (int)imm * 16 + off) );
	}
	else {
		int fsreg = ALLOCVI(_Fs_, MODE_READ);
		MOV32RmtoROffset(ftreg, recVUTransformAddr(fsreg, VU, _Fs_, imm), (uptr)VU->Mem + off);
	}
}
//------------------------------------------------------------------


//------------------------------------------------------------------
// ISW
//------------------------------------------------------------------
void recVUMI_ISW( VURegs *VU, int info )
{
	s16 imm;
	//SysPrintf("recVUMI_ISW  \n");
	imm = ( VU->code & 0x400) ? ( VU->code & 0x3ff) | 0xfc00 : ( VU->code & 0x3ff); 

	if (_Fs_ == 0) {
		uptr off = (uptr)GET_VU_MEM(VU, (int)imm * 16);
		int ftreg = ALLOCVI(_Ft_, MODE_READ);

		if (_X) MOV32RtoM(off, ftreg);
		if (_Y) MOV32RtoM(off+4, ftreg);
		if (_Z) MOV32RtoM(off+8, ftreg);
		if (_W) MOV32RtoM(off+12, ftreg);
	}
	else {
		int x86reg, fsreg, ftreg;

		ADD_VI_NEEDED(_Ft_);
		fsreg = ALLOCVI(_Fs_, MODE_READ);
		ftreg = ALLOCVI(_Ft_, MODE_READ);

		x86reg = recVUTransformAddr(fsreg, VU, _Fs_, imm);

		if (_X) MOV32RtoRmOffset(x86reg, ftreg, (uptr)VU->Mem);
		if (_Y) MOV32RtoRmOffset(x86reg, ftreg, (uptr)VU->Mem+4);
		if (_Z) MOV32RtoRmOffset(x86reg, ftreg, (uptr)VU->Mem+8);
		if (_W) MOV32RtoRmOffset(x86reg, ftreg, (uptr)VU->Mem+12);
	}
}
//------------------------------------------------------------------


//------------------------------------------------------------------
// ILWR
//------------------------------------------------------------------
void recVUMI_ILWR( VURegs *VU, int info )
{
	int off, ftreg;

	if ( ( _Ft_ == 0 ) || ( _X_Y_Z_W == 0 ) ) return;
	//SysPrintf("recVUMI_ILWR  \n");
	if (_X) off = 0;
	else if (_Y) off = 4;
	else if (_Z) off = 8;
	else if (_W) off = 12;

	ADD_VI_NEEDED(_Fs_);
	ftreg = ALLOCVI(_Ft_, MODE_WRITE);

	if ( _Fs_ == 0 ) {
		MOVZX32M16toR( ftreg, (uptr)VU->Mem + off );
	}
	else {
		int fsreg = ALLOCVI(_Fs_, MODE_READ);
		MOVZX32Rm16toROffset(ftreg, recVUTransformAddr(fsreg, VU, _Fs_, 0), (uptr)VU->Mem + off);
	}
}
//------------------------------------------------------------------


//------------------------------------------------------------------
// ISWR
//------------------------------------------------------------------
void recVUMI_ISWR( VURegs *VU, int info )
{
	int ftreg;
	//SysPrintf("recVUMI_ISWR  \n");
	ADD_VI_NEEDED(_Fs_);
	ftreg = ALLOCVI(_Ft_, MODE_READ);

	if (_Fs_ == 0) {
		if (_X) MOV32RtoM((uptr)VU->Mem, ftreg);
		if (_Y) MOV32RtoM((uptr)VU->Mem+4, ftreg);
		if (_Z) MOV32RtoM((uptr)VU->Mem+8, ftreg);
		if (_W) MOV32RtoM((uptr)VU->Mem+12, ftreg);
	}
	else {
		int x86reg;
		int fsreg = ALLOCVI(_Fs_, MODE_READ);
		x86reg = recVUTransformAddr(fsreg, VU, _Fs_, 0);

		if (_X) MOV32RtoRmOffset(x86reg, ftreg, (uptr)VU->Mem);
		if (_Y) MOV32RtoRmOffset(x86reg, ftreg, (uptr)VU->Mem+4);
		if (_Z) MOV32RtoRmOffset(x86reg, ftreg, (uptr)VU->Mem+8);
		if (_W) MOV32RtoRmOffset(x86reg, ftreg, (uptr)VU->Mem+12);
	}
}
//------------------------------------------------------------------


//------------------------------------------------------------------
// RINIT*
//------------------------------------------------------------------
void recVUMI_RINIT(VURegs *VU, int info)
{
	//SysPrintf("recVUMI_RINIT()\n");
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
	//SysPrintf("recVUMI_RGET()\n");
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
	//SysPrintf("recVUMI_RNEXT()\n");

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
	//SysPrintf("recVUMI_RXOR()\n");
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
	//SysPrintf("recVUMI_WAITQ  \n");
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
	int ftreg;
	u16 imm;
	//SysPrintf("recVUMI_FSAND  \n");
	imm = (((VU->code >> 21 ) & 0x1) << 11) | (VU->code & 0x7ff);
	if(_Ft_ == 0) return; 

	ftreg = ALLOCVI(_Ft_, MODE_WRITE);
	MOV32MtoR( ftreg, VU_VI_ADDR(REG_STATUS_FLAG, 1) );
	AND32ItoR( ftreg, imm );
}
//------------------------------------------------------------------


//------------------------------------------------------------------
// FSEQ
//------------------------------------------------------------------
void recVUMI_FSEQ( VURegs *VU, int info )
{
	int ftreg;
	u32 imm;
	if ( _Ft_ == 0 ) return;
	//SysPrintf("recVUMI_FSEQ  \n");
	imm = (((VU->code >> 21 ) & 0x1) << 11) | (VU->code & 0x7ff);

	ftreg = ALLOCVI(_Ft_, MODE_WRITE);

	MOVZX32M16toR( EAX, VU_VI_ADDR(REG_STATUS_FLAG, 1) );
	XOR32RtoR(ftreg, ftreg);

	CMP16ItoR(EAX, imm);
	SETE8R(ftreg);
}
//------------------------------------------------------------------


//------------------------------------------------------------------
// FSOR
//------------------------------------------------------------------
void recVUMI_FSOR( VURegs *VU, int info )
{
	int ftreg;
	u32 imm;
	if(_Ft_ == 0) return; 
	//SysPrintf("recVUMI_FSOR  \n");
	imm = (((VU->code >> 21 ) & 0x1) << 11) | (VU->code & 0x7ff);

	ftreg = ALLOCVI(_Ft_, MODE_WRITE);

	MOVZX32M16toR( ftreg, VU_VI_ADDR(REG_STATUS_FLAG, 1) );
	OR32ItoR( ftreg, imm );
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
	//SysPrintf("recVUMI_FSSET  \n");
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
	int fsreg, ftreg;
	if ( _Ft_ == 0 ) return;
	//SysPrintf("recVUMI_FMAND  \n");
	fsreg = _checkX86reg(X86TYPE_VI|(VU==&VU1?X86TYPE_VU1:0), _Fs_, MODE_READ);
	ftreg = ALLOCVI(_Ft_, MODE_WRITE);//|MODE_8BITREG);

	if( fsreg >= 0 ) {
		if( ftreg != fsreg ) MOV32RtoR(ftreg, fsreg);
	}
	else MOV16MtoR(ftreg, VU_VI_ADDR(_Fs_, 1));

	AND16MtoR( ftreg, VU_VI_ADDR(REG_MAC_FLAG, 1));
	//MOVZX32R16toR(ftreg, ftreg);
}
//------------------------------------------------------------------


//------------------------------------------------------------------
// FMEQ
//------------------------------------------------------------------
void recVUMI_FMEQ( VURegs *VU, int info )
{
	int ftreg, fsreg;
	if ( _Ft_ == 0 ) return;
	//SysPrintf("recVUMI_FMEQ  \n");
	if( _Ft_ == _Fs_ ) {
		ftreg = ALLOCVI(_Ft_, MODE_WRITE|MODE_READ);//|MODE_8BITREG);

		CMP16MtoR(ftreg, VU_VI_ADDR(REG_MAC_FLAG, 1));
		SETE8R(EAX);
		MOVZX32R8toR(ftreg, EAX);
	}
	else {
		ADD_VI_NEEDED(_Fs_);
		fsreg = ALLOCVI(_Fs_, MODE_READ);
		ftreg = ALLOCVI(_Ft_, MODE_WRITE|MODE_8BITREG);

		XOR32RtoR(ftreg, ftreg);
		
		CMP16MtoR(fsreg, VU_VI_ADDR(REG_MAC_FLAG, 1));
		SETE8R(ftreg);
	}
}
//------------------------------------------------------------------


//------------------------------------------------------------------
// FMOR
//------------------------------------------------------------------
void recVUMI_FMOR( VURegs *VU, int info )
{
	int fsreg, ftreg;
	if ( _Ft_ == 0 ) return;
	//SysPrintf("recVUMI_FMOR  \n");
	if( _Fs_ == 0 ) {
		ftreg = ALLOCVI(_Ft_, MODE_WRITE);//|MODE_8BITREG);
		MOVZX32M16toR( ftreg, VU_VI_ADDR(REG_MAC_FLAG, 1) );
	}
	else if( _Ft_ == _Fs_ ) {
		ftreg = ALLOCVI(_Ft_, MODE_WRITE);//|MODE_READ|MODE_8BITREG);
		OR16MtoR( ftreg, VU_VI_ADDR(REG_MAC_FLAG, 1) );
	}
	else {
		fsreg = _checkX86reg(X86TYPE_VI|(VU==&VU1?X86TYPE_VU1:0), _Fs_, MODE_READ);
		ftreg = ALLOCVI(_Ft_, MODE_WRITE);

		MOVZX32M16toR( ftreg, VU_VI_ADDR(REG_MAC_FLAG, 1) );

		if( fsreg >= 0 )
			OR16RtoR( ftreg, fsreg );
		else
			OR16MtoR( ftreg, VU_VI_ADDR(_Fs_, 1) );
	}
}
//------------------------------------------------------------------


//------------------------------------------------------------------
// FCAND
//------------------------------------------------------------------
void recVUMI_FCAND( VURegs *VU, int info )
{
	int ftreg = ALLOCVI(1, MODE_WRITE|MODE_8BITREG);
	//SysPrintf("recVUMI_FCAND  \n");
	MOV32MtoR( EAX, VU_VI_ADDR(REG_CLIP_FLAG, 1) );
	XOR32RtoR( ftreg, ftreg );
	AND32ItoR( EAX, VU->code & 0xFFFFFF );

	SETNZ8R(ftreg);
}
//------------------------------------------------------------------


//------------------------------------------------------------------
// FCEQ
//------------------------------------------------------------------
void recVUMI_FCEQ( VURegs *VU, int info )
{
	int ftreg = ALLOCVI(1, MODE_WRITE|MODE_8BITREG);
	//SysPrintf("recVUMI_FCEQ  \n");
	MOV32MtoR( EAX, VU_VI_ADDR(REG_CLIP_FLAG, 1) );
	AND32ItoR( EAX, 0xffffff );
	XOR32RtoR( ftreg, ftreg );
	CMP32ItoR( EAX, VU->code&0xffffff );

	SETE8R(ftreg);
}
//------------------------------------------------------------------


//------------------------------------------------------------------
// FCOR
//------------------------------------------------------------------
void recVUMI_FCOR( VURegs *VU, int info )
{
	int ftreg = ALLOCVI(1, MODE_WRITE|MODE_8BITREG);
	//SysPrintf("recVUMI_FCOR  \n");
	MOV32MtoR( EAX, VU_VI_ADDR(REG_CLIP_FLAG, 1) );
	XOR32RtoR( ftreg, ftreg );
	OR32ItoR( EAX, VU->code );
	AND32ItoR( EAX, 0xffffff );
	CMP32ItoR( EAX, 0xffffff );

	if(CHECK_FCORHACK) //ICO Misscalculated CLIP flag (bits missing id guess)
		SETNZ8R(ftreg);
	else
		SETZ8R(ftreg);
}
//------------------------------------------------------------------


//------------------------------------------------------------------
// FCSET
//------------------------------------------------------------------
void recVUMI_FCSET( VURegs *VU, int info )
{
	u32 addr = VU_VI_ADDR(REG_CLIP_FLAG, 0);
	//SysPrintf("recVUMI_FCSET  \n");
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
	int ftreg;
	if(_Ft_ == 0) return;
	//SysPrintf("recVUMI_FCGET  \n");
	ftreg = ALLOCVI(_Ft_, MODE_WRITE);

	MOV32MtoR(ftreg, VU_VI_ADDR(REG_CLIP_FLAG, 1));
	AND32ItoR(ftreg, 0x0fff);
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
	//SysPrintf("recVUMI_MFP  \n");
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
static PCSX2_ALIGNED16(float s_tempmem[4]);
void recVUMI_WAITP(VURegs *VU, int info)
{
	//SysPrintf("recVUMI_WAITP \n");
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
	//SysPrintf("VU: SUMXYZ\n");
	if( cpucaps.hasStreamingSIMD4Extensions )
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

		if( cpucaps.hasStreamingSIMD3Extensions ) {
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
	//SysPrintf("VU: ESADD\n");
	assert( VU == &VU1 );
	if( EEREC_TEMP == EEREC_D ) { // special code to reset P ( FixMe: don't know if this is still needed! (cottonvibes) )
		Console::Notice("ESADD: Resetting P reg!!!\n");
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
	//SysPrintf("VU: ERSADD\n");
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
	//SysPrintf("VU: ELENG\n");
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
	//SysPrintf("VU: ERLENG\n");
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
	//SysPrintf("recVUMI_EATANxy  \n");
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
	//SysPrintf("recVUMI_EATANxz  \n");
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
	//SysPrintf("VU: ESUM\n");
	assert( VU == &VU1 );

	if( cpucaps.hasStreamingSIMD3Extensions ) {
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
	//SysPrintf("VU1: ERCPR\n");

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

	//SysPrintf("VU1: ESQRT\n");
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
	//SysPrintf("VU1: ERSQRT\n");

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

	//SysPrintf("recVUMI_ESIN  \n");
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

	//SysPrintf("recVUMI_EATAN  \n");
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
	//SysPrintf("recVUMI_EEXP  \n");
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
	int ftreg;
	if (_Ft_ == 0) return;
	//SysPrintf("recVUMI_XITOP  \n");
	ftreg = ALLOCVI(_Ft_, MODE_WRITE);
	MOVZX32M16toR( ftreg, (uptr)&VU->vifRegs->itop );
}
//------------------------------------------------------------------


//------------------------------------------------------------------
// XTOP
//------------------------------------------------------------------
void recVUMI_XTOP( VURegs *VU, int info )
{
	int ftreg;
	if ( _Ft_ == 0 ) return;
	//SysPrintf("recVUMI_XTOP  \n");
	ftreg = ALLOCVI(_Ft_, MODE_WRITE);
	MOVZX32M16toR( ftreg, (uptr)&VU->vifRegs->top );
}
//------------------------------------------------------------------


//------------------------------------------------------------------
// VU1XGKICK_MTGSTransfer() - Called by ivuZerorec.cpp
//------------------------------------------------------------------
void VU1XGKICK_MTGSTransfer(u32 *pMem, u32 addr)
{
	u32 size;
    u32* data = (u32*)((u8*)pMem + (addr&0x3fff));

	// fixme: The gifTagDummy function in the MTGS (called by PrepDataPacket) has a
	// hack that aborts the packet if it goes past the end of VU1 memory.
	// Chances are this should be a "loops around memory" situation, and the packet
	// should be continued starting at addr zero (0).

	size = mtgsThread->PrepDataPacket( GIF_PATH_1, data, (0x4000-(addr&0x3fff)) >> 4);
    jASSUME( size > 0 );
	
    //if( size > 0 )
	{
		u8* pmem = mtgsThread->GetDataPacketPtr();
		memcpy_aligned(pmem, (u8*)pMem+addr, size<<4);
		mtgsThread->SendDataPacket();
	}
}
//------------------------------------------------------------------
