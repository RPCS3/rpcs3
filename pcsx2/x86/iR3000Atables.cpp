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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include "PrecompiledHeader.h"
#include <time.h>

#include "PsxCommon.h"
#include "ix86/ix86.h"
#include "iR3000A.h"
#include "IopMem.h"
#include "IopDma.h"

extern int g_psxWriteOk;
extern u32 g_psxMaxRecMem;

// R3000A instruction implementation
#define REC_FUNC(f) \
static void rpsx##f() { \
	MOV32ItoM((uptr)&psxRegs.code, (u32)psxRegs.code); \
	_psxFlushCall(FLUSH_EVERYTHING); \
	CALLFunc((uptr)psx##f); \
	PSX_DEL_CONST(_Rt_); \
/*	branch = 2; */\
}

extern void psxLWL();
extern void psxLWR();
extern void psxSWL();
extern void psxSWR();

//// 
void rpsxADDIU_const()
{
	g_psxConstRegs[_Rt_] = g_psxConstRegs[_Rs_] + _Imm_; 
}

// adds a constant to sreg and puts into dreg
void rpsxADDconst(int dreg, int sreg, u32 off, int info)
{
	if (sreg) {
		if (sreg == dreg) {
			ADD32ItoM((uptr)&psxRegs.GPR.r[dreg], off);
		} else {
			MOV32MtoR(EAX, (uptr)&psxRegs.GPR.r[sreg]);
			if (off) ADD32ItoR(EAX, off);
			MOV32RtoM((uptr)&psxRegs.GPR.r[dreg], EAX);
		}
	}
	else {
		MOV32ItoM((uptr)&psxRegs.GPR.r[dreg], off);
	}
}

void rpsxADDIU_(int info)
{
	// Rt = Rs + Im
	if (!_Rt_) return;
	rpsxADDconst(_Rt_, _Rs_, _Imm_, info);
}

PSXRECOMPILE_CONSTCODE1(ADDIU);

void rpsxADDI() { rpsxADDIU(); }

//// SLTI
void rpsxSLTI_const()
{
	g_psxConstRegs[_Rt_] = *(int*)&g_psxConstRegs[_Rs_] < _Imm_;
}

void rpsxSLTconst(int info, int dreg, int sreg, int imm)
{
	XOR32RtoR(EAX, EAX);
    CMP32ItoM((uptr)&psxRegs.GPR.r[sreg], imm);
    SETL8R(EAX);
	MOV32RtoM((uptr)&psxRegs.GPR.r[dreg], EAX);
}

void rpsxSLTI_(int info) { rpsxSLTconst(info, _Rt_, _Rs_, _Imm_); }

PSXRECOMPILE_CONSTCODE1(SLTI);

//// SLTIU
void rpsxSLTIU_const()
{
	g_psxConstRegs[_Rt_] = g_psxConstRegs[_Rs_] < _ImmU_;
}

void rpsxSLTUconst(int info, int dreg, int sreg, int imm)
{
	XOR32RtoR(EAX, EAX);
	CMP32ItoM((uptr)&psxRegs.GPR.r[sreg], imm);
    SETB8R(EAX);
	MOV32RtoM((uptr)&psxRegs.GPR.r[dreg], EAX);
}

void rpsxSLTIU_(int info) { rpsxSLTUconst(info, _Rt_, _Rs_, (s32)_Imm_); }

PSXRECOMPILE_CONSTCODE1(SLTIU);

//// ANDI
void rpsxANDI_const()
{
	g_psxConstRegs[_Rt_] = g_psxConstRegs[_Rs_] & _ImmU_;
}

void rpsxANDconst(int info, int dreg, int sreg, u32 imm)
{
	if (imm) {
		if (sreg == dreg) {
			AND32ItoM((uptr)&psxRegs.GPR.r[dreg], imm);
		} else {
			MOV32MtoR(EAX, (uptr)&psxRegs.GPR.r[sreg]);
			AND32ItoR(EAX, imm);
			MOV32RtoM((uptr)&psxRegs.GPR.r[dreg], EAX);
		}
	} else {
		MOV32ItoM((uptr)&psxRegs.GPR.r[dreg], 0);
	}
}

void rpsxANDI_(int info) { rpsxANDconst(info, _Rt_, _Rs_, _ImmU_); }

PSXRECOMPILE_CONSTCODE1(ANDI);

//// ORI
void rpsxORI_const()
{
	g_psxConstRegs[_Rt_] = g_psxConstRegs[_Rs_] | _ImmU_;
}

void rpsxORconst(int info, int dreg, int sreg, u32 imm)
{
	if (imm) {
		if (sreg == dreg) {
			OR32ItoM((uptr)&psxRegs.GPR.r[dreg], imm);
		}
		else {
			MOV32MtoR(EAX, (uptr)&psxRegs.GPR.r[sreg]);
			OR32ItoR (EAX, imm);
			MOV32RtoM((uptr)&psxRegs.GPR.r[dreg], EAX);
		}
	}
	else {
		if( dreg != sreg ) {
			MOV32MtoR(ECX, (uptr)&psxRegs.GPR.r[sreg]);
			MOV32RtoM((uptr)&psxRegs.GPR.r[dreg], ECX);
		}
	}
}

void rpsxORI_(int info) { rpsxORconst(info, _Rt_, _Rs_, _ImmU_); }

PSXRECOMPILE_CONSTCODE1(ORI);

void rpsxXORI_const()
{
	g_psxConstRegs[_Rt_] = g_psxConstRegs[_Rs_] ^ _ImmU_;
}

void rpsxXORconst(int info, int dreg, int sreg, u32 imm)
{
	if( imm == 0xffffffff ) {
		if( dreg == sreg ) {
			NOT32M((uptr)&psxRegs.GPR.r[dreg]);
		}
		else {
			MOV32MtoR(ECX, (uptr)&psxRegs.GPR.r[sreg]);
			NOT32R(ECX);
			MOV32RtoM((uptr)&psxRegs.GPR.r[dreg], ECX);
		}
	}
	else if (imm) {

		if (sreg == dreg) {
			XOR32ItoM((uptr)&psxRegs.GPR.r[dreg], imm);
		}
		else {
			MOV32MtoR(EAX, (uptr)&psxRegs.GPR.r[sreg]);
			XOR32ItoR(EAX, imm);
			MOV32RtoM((uptr)&psxRegs.GPR.r[dreg], EAX);
		}
	}
	else {
		if( dreg != sreg ) {
			MOV32MtoR(ECX, (uptr)&psxRegs.GPR.r[sreg]);
			MOV32RtoM((uptr)&psxRegs.GPR.r[dreg], ECX);
		}
	}
}

void rpsxXORI_(int info) { rpsxXORconst(info, _Rt_, _Rs_, _ImmU_); }

PSXRECOMPILE_CONSTCODE1(XORI);

void rpsxLUI()
{
	if(!_Rt_) return;
	_psxOnWriteReg(_Rt_);
	_psxDeleteReg(_Rt_, 0);
	PSX_SET_CONST(_Rt_);
	g_psxConstRegs[_Rt_] = psxRegs.code << 16;
}

void rpsxADDU_const()
{
	g_psxConstRegs[_Rd_] = g_psxConstRegs[_Rs_] + g_psxConstRegs[_Rt_];
}

void rpsxADDU_consts(int info) { rpsxADDconst(_Rd_, _Rt_, g_psxConstRegs[_Rs_], info); }
void rpsxADDU_constt(int info)
{
	info |= PROCESS_EE_SET_S(EEREC_T);
	rpsxADDconst(_Rd_, _Rs_, g_psxConstRegs[_Rt_], info);
}

void rpsxADDU_(int info)
{
	if (_Rs_ && _Rt_) {
		MOV32MtoR(EAX, (uptr)&psxRegs.GPR.r[_Rs_]);
		ADD32MtoR(EAX, (uptr)&psxRegs.GPR.r[_Rt_]);
	} else if (_Rs_) {
		MOV32MtoR(EAX, (uptr)&psxRegs.GPR.r[_Rs_]);
	} else if (_Rt_) {
		MOV32MtoR(EAX, (uptr)&psxRegs.GPR.r[_Rt_]);
	} else {
		XOR32RtoR(EAX, EAX);
	}
	MOV32RtoM((uptr)&psxRegs.GPR.r[_Rd_], EAX);
}

PSXRECOMPILE_CONSTCODE0(ADDU);

void rpsxADD() { rpsxADDU(); }


void rpsxSUBU_const()
{
	g_psxConstRegs[_Rd_] = g_psxConstRegs[_Rs_] - g_psxConstRegs[_Rt_];
}

void rpsxSUBU_consts(int info)
{
	MOV32ItoR(EAX, g_psxConstRegs[_Rs_]);
	SUB32MtoR(EAX, (uptr)&psxRegs.GPR.r[_Rt_]);
	MOV32RtoM((uptr)&psxRegs.GPR.r[_Rd_], EAX);
}

void rpsxSUBU_constt(int info) { rpsxADDconst(_Rd_, _Rs_, -(int)g_psxConstRegs[_Rt_], info); }

void rpsxSUBU_(int info)
{
	// Rd = Rs - Rt
	if (!_Rd_) return;

	if( _Rd_ == _Rs_ ) {
		MOV32MtoR(EAX, (uptr)&psxRegs.GPR.r[_Rt_]);
		SUB32RtoM((uptr)&psxRegs.GPR.r[_Rd_], EAX);
	}
	else {
		MOV32MtoR(EAX, (uptr)&psxRegs.GPR.r[_Rs_]);
		SUB32MtoR(EAX, (uptr)&psxRegs.GPR.r[_Rt_]);
		MOV32RtoM((uptr)&psxRegs.GPR.r[_Rd_], EAX);
	}
}

PSXRECOMPILE_CONSTCODE0(SUBU);

void rpsxSUB() { rpsxSUBU(); }

void rpsxLogicalOp(int info, int op)
{
	if( _Rd_ == _Rs_ || _Rd_ == _Rt_ ) {
		int vreg = _Rd_ == _Rs_ ? _Rt_ : _Rs_;
		MOV32MtoR(ECX, (uptr)&psxRegs.GPR.r[vreg]);
		LogicalOp32RtoM((uptr)&psxRegs.GPR.r[_Rd_], ECX, op);
		if( op == 3 )
			NOT32M((uptr)&psxRegs.GPR.r[_Rd_]);
	}
	else {
		MOV32MtoR(ECX, (uptr)&psxRegs.GPR.r[_Rs_]);
		LogicalOp32MtoR(ECX, (uptr)&psxRegs.GPR.r[_Rt_], op);
		if( op == 3 )
			NOT32R(ECX);
		MOV32RtoM((uptr)&psxRegs.GPR.r[_Rd_], ECX);
	}
}

void rpsxAND_const()
{
	g_psxConstRegs[_Rd_] = g_psxConstRegs[_Rs_] & g_psxConstRegs[_Rt_];
}

void rpsxAND_consts(int info) { rpsxANDconst(info, _Rd_, _Rt_, g_psxConstRegs[_Rs_]); }
void rpsxAND_constt(int info) { rpsxANDconst(info, _Rd_, _Rs_, g_psxConstRegs[_Rt_]); }
void rpsxAND_(int info) { rpsxLogicalOp(info, 0); }

PSXRECOMPILE_CONSTCODE0(AND);

void rpsxOR_const()
{
	g_psxConstRegs[_Rd_] = g_psxConstRegs[_Rs_] | g_psxConstRegs[_Rt_];
}

void rpsxOR_consts(int info) { rpsxORconst(info, _Rd_, _Rt_, g_psxConstRegs[_Rs_]); }
void rpsxOR_constt(int info) { rpsxORconst(info, _Rd_, _Rs_, g_psxConstRegs[_Rt_]); }
void rpsxOR_(int info) { rpsxLogicalOp(info, 1); }

PSXRECOMPILE_CONSTCODE0(OR);

//// XOR
void rpsxXOR_const()
{
	g_psxConstRegs[_Rd_] = g_psxConstRegs[_Rs_] ^ g_psxConstRegs[_Rt_];
}

void rpsxXOR_consts(int info) { rpsxXORconst(info, _Rd_, _Rt_, g_psxConstRegs[_Rs_]); }
void rpsxXOR_constt(int info) { rpsxXORconst(info, _Rd_, _Rs_, g_psxConstRegs[_Rt_]); }
void rpsxXOR_(int info) { rpsxLogicalOp(info, 2); }

PSXRECOMPILE_CONSTCODE0(XOR);

//// NOR
void rpsxNOR_const()
{
	g_psxConstRegs[_Rd_] = ~(g_psxConstRegs[_Rs_] | g_psxConstRegs[_Rt_]);
}

void rpsxNORconst(int info, int dreg, int sreg, u32 imm)
{
	if( imm ) {
		if( dreg == sreg ) {
			OR32ItoM((uptr)&psxRegs.GPR.r[dreg], imm);
			NOT32M((uptr)&psxRegs.GPR.r[dreg]);
		}
		else {
			MOV32MtoR(ECX, (uptr)&psxRegs.GPR.r[sreg]);
			OR32ItoR(ECX, imm);
			NOT32R(ECX);
			MOV32RtoM((uptr)&psxRegs.GPR.r[dreg], ECX);
		}
	}
	else {
		if( dreg == sreg ) {
			NOT32M((uptr)&psxRegs.GPR.r[dreg]);
		}
		else {
			MOV32MtoR(ECX, (uptr)&psxRegs.GPR.r[sreg]);
			NOT32R(ECX);
			MOV32RtoM((uptr)&psxRegs.GPR.r[dreg], ECX);
		}
	}
}

void rpsxNOR_consts(int info) { rpsxNORconst(info, _Rd_, _Rt_, g_psxConstRegs[_Rs_]); }
void rpsxNOR_constt(int info) { rpsxNORconst(info, _Rd_, _Rs_, g_psxConstRegs[_Rt_]); }
void rpsxNOR_(int info) { rpsxLogicalOp(info, 3); }

PSXRECOMPILE_CONSTCODE0(NOR);

//// SLT
void rpsxSLT_const()
{
	g_psxConstRegs[_Rd_] = *(int*)&g_psxConstRegs[_Rs_] < *(int*)&g_psxConstRegs[_Rt_];
}

void rpsxSLT_consts(int info)
{
	XOR32RtoR(EAX, EAX);
    CMP32ItoM((uptr)&psxRegs.GPR.r[_Rt_], g_psxConstRegs[_Rs_]);
    SETG8R(EAX);
	MOV32RtoM((uptr)&psxRegs.GPR.r[_Rd_], EAX);
}

void rpsxSLT_constt(int info) { rpsxSLTconst(info, _Rd_, _Rs_, g_psxConstRegs[_Rt_]); }
void rpsxSLT_(int info)
{
	MOV32MtoR(EAX, (uptr)&psxRegs.GPR.r[_Rs_]);
    CMP32MtoR(EAX, (uptr)&psxRegs.GPR.r[_Rt_]);
    SETL8R   (EAX);
    AND32ItoR(EAX, 0xff);
	MOV32RtoM((uptr)&psxRegs.GPR.r[_Rd_], EAX);
}  

PSXRECOMPILE_CONSTCODE0(SLT);

//// SLTU
void rpsxSLTU_const()
{
	g_psxConstRegs[_Rd_] = g_psxConstRegs[_Rs_] < g_psxConstRegs[_Rt_];
}

void rpsxSLTU_consts(int info)
{
	XOR32RtoR(EAX, EAX);
    CMP32ItoM((uptr)&psxRegs.GPR.r[_Rt_], g_psxConstRegs[_Rs_]);
    SETA8R(EAX);
	MOV32RtoM((uptr)&psxRegs.GPR.r[_Rd_], EAX);
}

void rpsxSLTU_constt(int info) { rpsxSLTUconst(info, _Rd_, _Rs_, g_psxConstRegs[_Rt_]); }
void rpsxSLTU_(int info) 
{
	// Rd = Rs < Rt (unsigned)
	if (!_Rd_) return;

	MOV32MtoR(EAX, (uptr)&psxRegs.GPR.r[_Rs_]);
    CMP32MtoR(EAX, (uptr)&psxRegs.GPR.r[_Rt_]);
	SBB32RtoR(EAX, EAX);
	NEG32R   (EAX);
	MOV32RtoM((uptr)&psxRegs.GPR.r[_Rd_], EAX);
}

PSXRECOMPILE_CONSTCODE0(SLTU);

//// MULT
void rpsxMULT_const()
{
	u64 res = (s64)((s64)*(int*)&g_psxConstRegs[_Rs_] * (s64)*(int*)&g_psxConstRegs[_Rt_]);

	MOV32ItoM((uptr)&psxRegs.GPR.n.hi, (u32)((res >> 32) & 0xffffffff));
	MOV32ItoM((uptr)&psxRegs.GPR.n.lo, (u32)(res & 0xffffffff));
}

void rpsxMULTsuperconst(int info, int sreg, int imm, int sign)
{
	// Lo/Hi = Rs * Rt (signed)
	MOV32ItoR(EAX, imm);
	if( sign ) IMUL32M  ((uptr)&psxRegs.GPR.r[sreg]);
	else MUL32M  ((uptr)&psxRegs.GPR.r[sreg]);
	MOV32RtoM((uptr)&psxRegs.GPR.n.lo, EAX);
	MOV32RtoM((uptr)&psxRegs.GPR.n.hi, EDX);
}

void rpsxMULTsuper(int info, int sign)
{
	// Lo/Hi = Rs * Rt (signed)
	MOV32MtoR(EAX, (uptr)&psxRegs.GPR.r[_Rs_]);
	if( sign ) IMUL32M  ((uptr)&psxRegs.GPR.r[_Rt_]);
	else MUL32M  ((uptr)&psxRegs.GPR.r[_Rt_]);
	MOV32RtoM((uptr)&psxRegs.GPR.n.lo, EAX);
	MOV32RtoM((uptr)&psxRegs.GPR.n.hi, EDX);
}

void rpsxMULT_consts(int info) { rpsxMULTsuperconst(info, _Rt_, g_psxConstRegs[_Rs_], 1); }
void rpsxMULT_constt(int info) { rpsxMULTsuperconst(info, _Rs_, g_psxConstRegs[_Rt_], 1); }
void rpsxMULT_(int info) { rpsxMULTsuper(info, 1); }

PSXRECOMPILE_CONSTCODE3_PENALTY(MULT, 1, psxInstCycles_Mult);

//// MULTU
void rpsxMULTU_const()
{
	u64 res = (u64)((u64)g_psxConstRegs[_Rs_] * (u64)g_psxConstRegs[_Rt_]);

	MOV32ItoM((uptr)&psxRegs.GPR.n.hi, (u32)((res >> 32) & 0xffffffff));
	MOV32ItoM((uptr)&psxRegs.GPR.n.lo, (u32)(res & 0xffffffff));
}

void rpsxMULTU_consts(int info) { rpsxMULTsuperconst(info, _Rt_, g_psxConstRegs[_Rs_], 0); }
void rpsxMULTU_constt(int info) { rpsxMULTsuperconst(info, _Rs_, g_psxConstRegs[_Rt_], 0); }
void rpsxMULTU_(int info) { rpsxMULTsuper(info, 0); }

PSXRECOMPILE_CONSTCODE3_PENALTY(MULTU, 1, psxInstCycles_Mult);

//// DIV
void rpsxDIV_const()
{
	u32 lo, hi;

	if (g_psxConstRegs[_Rt_] != 0) {
		lo = *(int*)&g_psxConstRegs[_Rs_] / *(int*)&g_psxConstRegs[_Rt_];
		hi = *(int*)&g_psxConstRegs[_Rs_] % *(int*)&g_psxConstRegs[_Rt_];
		MOV32ItoM((uptr)&psxRegs.GPR.n.hi, hi);
		MOV32ItoM((uptr)&psxRegs.GPR.n.lo, lo);
	}
}

void rpsxDIVsuperconsts(int info, int sign)
{
	u32 imm = g_psxConstRegs[_Rs_];

	if( imm ) {
		// Lo/Hi = Rs / Rt (signed)
		MOV32MtoR(ECX, (uptr)&psxRegs.GPR.r[_Rt_]);
		CMP32ItoR(ECX, 0);
		j8Ptr[0] = JE8(0);
		MOV32ItoR(EAX, imm);
		
		
		if( sign ) {
		CDQ();
		IDIV32R  (ECX);
		}
		else {
			XOR32RtoR( EDX, EDX );
			DIV32R(ECX);
		}


		MOV32RtoM((uptr)&psxRegs.GPR.n.lo, EAX);
		MOV32RtoM((uptr)&psxRegs.GPR.n.hi, EDX);
		x86SetJ8(j8Ptr[0]);
	}
	else {
		XOR32RtoR(EAX, EAX);
		MOV32RtoM((uptr)&psxRegs.GPR.n.hi, EAX);
		MOV32RtoM((uptr)&psxRegs.GPR.n.lo, EAX);
	}
}

void rpsxDIVsuperconstt(int info, int sign)
{
	u32 imm = g_psxConstRegs[_Rt_];

	if( imm ) {
		MOV32MtoR(EAX, (uptr)&psxRegs.GPR.r[_Rs_]);
		MOV32ItoR(ECX, imm);
		//CDQ();
		
		if( sign ) {
			CDQ();
			IDIV32R  (ECX);
		}
		else {
			XOR32RtoR( EDX, EDX );
			DIV32R(ECX);
		}


		MOV32RtoM((uptr)&psxRegs.GPR.n.lo, EAX);
		MOV32RtoM((uptr)&psxRegs.GPR.n.hi, EDX);
	}
}

void rpsxDIVsuper(int info, int sign)
{
	// Lo/Hi = Rs / Rt (signed)
	MOV32MtoR(ECX, (uptr)&psxRegs.GPR.r[_Rt_]);
	CMP32ItoR(ECX, 0);
	j8Ptr[0] = JE8(0);
	MOV32MtoR(EAX, (uptr)&psxRegs.GPR.r[_Rs_]);
	
	if( sign ) {
		CDQ();
		IDIV32R  (ECX);
	}
	else {
		XOR32RtoR( EDX, EDX );
		DIV32R(ECX);
	}

	MOV32RtoM((uptr)&psxRegs.GPR.n.lo, EAX);
	MOV32RtoM((uptr)&psxRegs.GPR.n.hi, EDX);
	x86SetJ8(j8Ptr[0]);
}

void rpsxDIV_consts(int info) { rpsxDIVsuperconsts(info, 1); }
void rpsxDIV_constt(int info) { rpsxDIVsuperconstt(info, 1); }
void rpsxDIV_(int info) { rpsxDIVsuper(info, 1); }

PSXRECOMPILE_CONSTCODE3_PENALTY(DIV, 1, psxInstCycles_Div);

//// DIVU
void rpsxDIVU_const()
{
	u32 lo, hi;

	if (g_psxConstRegs[_Rt_] != 0) {
		lo = g_psxConstRegs[_Rs_] / g_psxConstRegs[_Rt_];
		hi = g_psxConstRegs[_Rs_] % g_psxConstRegs[_Rt_];
		MOV32ItoM((uptr)&psxRegs.GPR.n.hi, hi);
		MOV32ItoM((uptr)&psxRegs.GPR.n.lo, lo);
	}
}

void rpsxDIVU_consts(int info) { rpsxDIVsuperconsts(info, 0); }
void rpsxDIVU_constt(int info) { rpsxDIVsuperconstt(info, 0); }
void rpsxDIVU_(int info) { rpsxDIVsuper(info, 0); }

PSXRECOMPILE_CONSTCODE3_PENALTY(DIVU, 1, psxInstCycles_Div);

//// LoadStores
#ifdef PCSX2_VIRTUAL_MEM

// VM load store functions (fastest)

//#define REC_SLOWREAD
//#define REC_SLOWWRITE

int _psxPrepareReg(int gprreg)
{
	return 0;
}

static u32 s_nAddMemOffset = 0;

static __forceinline void SET_HWLOC_R3000A() { 
	x86SetJ8(j8Ptr[0]); 
	SHR32ItoR(ECX, 3); 
	if( s_nAddMemOffset ) ADD32ItoR(ECX, s_nAddMemOffset); 
} 

int rpsxSetMemLocation(int regs, int mmreg)
{
	s_nAddMemOffset = 0;
	MOV32MtoR( ECX, (int)&psxRegs.GPR.r[ regs ] );

	if ( _Imm_ != 0 ) ADD32ItoR( ECX, _Imm_ );

	SHL32ItoR(ECX, 3);
	j8Ptr[0] = JS8(0);
	SHR32ItoR(ECX, 3);
	AND32ItoR(ECX, 0x1fffff); // 2Mb
	return 1;
}

void recLoad32(u32 bit, u32 sign)
{
	int mmreg = -1;

#ifdef REC_SLOWREAD
	_psxFlushConstReg(_Rs_);
#else
	if( PSX_IS_CONST1( _Rs_ ) ) {
		// do const processing
		int ineax = 0;

		_psxOnWriteReg(_Rt_);
		mmreg = EAX;

		switch(bit) {
			case 8: ineax = psxRecMemConstRead8(mmreg, g_psxConstRegs[_Rs_]+_Imm_, sign); break;
			case 16:
				assert( (g_psxConstRegs[_Rs_]+_Imm_) % 2 == 0 );
				ineax = psxRecMemConstRead16(mmreg, g_psxConstRegs[_Rs_]+_Imm_, sign);
				break;
			case 32:
				assert( (g_psxConstRegs[_Rs_]+_Imm_) % 4 == 0 );
				ineax = psxRecMemConstRead32(mmreg, g_psxConstRegs[_Rs_]+_Imm_);
				break;
		}

		if( _Rt_ ) MOV32RtoM( (int)&psxRegs.GPR.r[ _Rt_ ], EAX );
	}
	else
#endif
	{
		int dohw;
		int mmregs = _psxPrepareReg(_Rs_);

		_psxOnWriteReg(_Rt_);
		_psxDeleteReg(_Rt_, 0);
		
		dohw = rpsxSetMemLocation(_Rs_, mmregs);

		switch(bit) {
			case 8: 
				if( sign ) MOVSX32Rm8toROffset(EAX, ECX, PS2MEM_PSX_+s_nAddMemOffset);
				else MOVZX32Rm8toROffset(EAX, ECX, PS2MEM_PSX_+s_nAddMemOffset);
				break;
			case 16:
				if( sign ) MOVSX32Rm16toROffset(EAX, ECX, PS2MEM_PSX_+s_nAddMemOffset);
				else MOVZX32Rm16toROffset(EAX, ECX, PS2MEM_PSX_+s_nAddMemOffset);
				break;
			case 32:
				MOV32RmtoROffset(EAX, ECX, PS2MEM_PSX_+s_nAddMemOffset);
				break;
		}

		if( dohw ) {
			j8Ptr[1] = JMP8(0);

			SET_HWLOC_R3000A();

			switch(bit) {
				case 8:
					CALLFunc( (int)psxRecMemRead8 );
					if( sign ) MOVSX32R8toR(EAX, EAX);
					else MOVZX32R8toR(EAX, EAX);
					break;
				case 16:
					CALLFunc( (int)psxRecMemRead16 );
					if( sign ) MOVSX32R16toR(EAX, EAX);
					else MOVZX32R16toR(EAX, EAX);
					break;
				case 32:
					CALLFunc( (int)psxRecMemRead32 );
					break;
			}
			
			x86SetJ8(j8Ptr[1]);
		}

		if( _Rt_ )
			MOV32RtoM( (int)&psxRegs.GPR.r[ _Rt_ ], EAX );
	}
}

void rpsxLB() { recLoad32(8, 1); }
void rpsxLBU() { recLoad32(8, 0); }
void rpsxLH() { recLoad32(16, 1); }
void rpsxLHU() { recLoad32(16, 0); }
void rpsxLW() { recLoad32(32, 0); }

extern void rpsxMemConstClear(u32 mem);

// check if mem is executable, and clear it
__declspec(naked) void rpsxWriteMemClear()
{
	_asm {
		mov edx, ecx
		shr edx, 14
		and dl, 0xfc
		add edx, psxRecLUT
		test dword ptr [edx], 0xffffffff
		jnz Clear32
		ret
Clear32:
		// recLUT[mem>>16] + (mem&0xfffc)
		mov edx, dword ptr [edx]
		mov eax, ecx
		and eax, 0xfffc
		// edx += 2*eax
		shl eax, 1
		add edx, eax
		cmp dword ptr [edx], 0
		je ClearRet
		sub esp, 4
		mov dword ptr [esp], edx
		call psxRecClearMem
		add esp, 4
ClearRet:
		ret
	}
}

extern u32 s_psxBlockCycles;
void recStore(int bit)
{
#ifdef REC_SLOWWRITE
	_psxFlushConstReg(_Rs_);
#else
	if( PSX_IS_CONST1( _Rs_ ) ) {
		u8* pjmpok;
		u32 addr = g_psxConstRegs[_Rs_]+_Imm_;
		int doclear = 0;

		if( !(addr & 0x10000000) ) {
			// check g_psxWriteOk
			CMP32ItoM((uptr)&g_psxWriteOk, 0);
			pjmpok = JE8(0);
		}

		switch(bit) {
			case 8:
				if( PSX_IS_CONST1(_Rt_) ) doclear = psxRecMemConstWrite8(addr, MEM_PSXCONSTTAG|(_Rt_<<16));
				else {
					_psxMoveGPRtoR(EAX, _Rt_);
					doclear = psxRecMemConstWrite8(addr, EAX);
				}
				
				break;

			case 16:
				assert( (addr)%2 == 0 );
				if( PSX_IS_CONST1(_Rt_) ) doclear = psxRecMemConstWrite16(addr, MEM_PSXCONSTTAG|(_Rt_<<16));
				else {
					_psxMoveGPRtoR(EAX, _Rt_);
					doclear = psxRecMemConstWrite16(addr, EAX);
				}

				break;

			case 32:
				assert( (addr)%4 == 0 );
				if( PSX_IS_CONST1(_Rt_) ) doclear = psxRecMemConstWrite32(addr, MEM_PSXCONSTTAG|(_Rt_<<16));
				else {
					_psxMoveGPRtoR(EAX, _Rt_);
					doclear = psxRecMemConstWrite32(addr, EAX);
				}

				break;
		}

		if( !(addr & 0x10000000) ) {
			if( doclear ) rpsxMemConstClear((addr)&~3);
			x86SetJ8(pjmpok);
		}
	}
	else
#endif
	{
		int dohw;
		int mmregs = _psxPrepareReg(_Rs_);
		dohw = rpsxSetMemLocation(_Rs_, mmregs);

		CMP32ItoM((uptr)&g_psxWriteOk, 0);
		u8* pjmpok = JE8(0);

		if( PSX_IS_CONST1( _Rt_ ) ) {
			switch(bit) {
				case 8: MOV8ItoRmOffset(ECX, g_psxConstRegs[_Rt_], PS2MEM_PSX_+s_nAddMemOffset); break;
				case 16: MOV16ItoRmOffset(ECX, g_psxConstRegs[_Rt_], PS2MEM_PSX_+s_nAddMemOffset); break;
				case 32: MOV32ItoRmOffset(ECX, g_psxConstRegs[_Rt_], PS2MEM_PSX_+s_nAddMemOffset); break;
			}
		}
		else {
			switch(bit) {
				case 8:
					MOV8MtoR(EAX, (int)&psxRegs.GPR.r[ _Rt_ ]);
					MOV8RtoRmOffset(ECX, EAX, PS2MEM_PSX_+s_nAddMemOffset);
					break;

				case 16:
					MOV16MtoR(EAX, (int)&psxRegs.GPR.r[ _Rt_ ]);
					MOV16RtoRmOffset(ECX, EAX, PS2MEM_PSX_+s_nAddMemOffset);
					break;

				case 32:
					MOV32MtoR(EAX, (int)&psxRegs.GPR.r[ _Rt_ ]);
					MOV32RtoRmOffset(ECX, EAX, PS2MEM_PSX_+s_nAddMemOffset);
					break;
			}
		}

		if( s_nAddMemOffset ) ADD32ItoR(ECX, s_nAddMemOffset);
		CMP32MtoR(ECX, (uptr)&g_psxMaxRecMem);
		
		j8Ptr[1] = JAE8(0);

		if( bit < 32 ) AND8ItoR(ECX, 0xfc);
		CALLFunc((u32)rpsxWriteMemClear);

		if( dohw ) {
			j8Ptr[2] = JMP8(0);

			SET_HWLOC_R3000A();

			if( PSX_IS_CONST1(_Rt_) ) {
				switch(bit) {
					case 8: MOV8ItoR(EAX, g_psxConstRegs[_Rt_]); break;
					case 16: MOV16ItoR(EAX, g_psxConstRegs[_Rt_]); break;
					case 32: MOV32ItoR(EAX, g_psxConstRegs[_Rt_]); break;
				}
			}
			else {
				switch(bit) {
					case 8: MOV8MtoR(EAX, (int)&psxRegs.GPR.r[ _Rt_ ]); break;
					case 16: MOV16MtoR(EAX, (int)&psxRegs.GPR.r[ _Rt_ ]); break;
					case 32: MOV32MtoR(EAX, (int)&psxRegs.GPR.r[ _Rt_ ]); break;
				}
			}

			if( s_nAddMemOffset != 0 ) ADD32ItoR(ECX, s_nAddMemOffset);

			// some type of hardware write
			switch(bit) {
				case 8: CALLFunc( (int)psxRecMemWrite8 ); break;
				case 16: CALLFunc( (int)psxRecMemWrite16 ); break;
				case 32: CALLFunc( (int)psxRecMemWrite32 ); break;
			}

			x86SetJ8(j8Ptr[2]);
		}

		x86SetJ8(j8Ptr[1]);
		x86SetJ8(pjmpok);
	}
}

void rpsxSB() { recStore(8); }
void rpsxSH() { recStore(16); }
void rpsxSW() { recStore(32); }

REC_FUNC(LWL);
REC_FUNC(LWR);
REC_FUNC(SWL);
REC_FUNC(SWR);

#else

// TLB loadstore functions (slower
REC_FUNC(LWL);
REC_FUNC(LWR);
REC_FUNC(SWL);
REC_FUNC(SWR);

static void rpsxLB()
{
	_psxDeleteReg(_Rs_, 1);
	_psxOnWriteReg(_Rt_);
	_psxDeleteReg(_Rt_, 0);

	MOV32MtoR(X86ARG1, (uptr)&psxRegs.GPR.r[_Rs_]);
	if (_Imm_) ADD32ItoR(X86ARG1, _Imm_);
    _callFunctionArg1((uptr)psxMemRead8, X86ARG1|MEM_X86TAG, 0);
	if (_Rt_) {
		MOVSX32R8toR(EAX, EAX);
		MOV32RtoM((uptr)&psxRegs.GPR.r[_Rt_], EAX);
	}
	PSX_DEL_CONST(_Rt_);
}

static void rpsxLBU()
{
	_psxDeleteReg(_Rs_, 1);
	_psxOnWriteReg(_Rt_);
	_psxDeleteReg(_Rt_, 0);

	MOV32MtoR(X86ARG1, (uptr)&psxRegs.GPR.r[_Rs_]);
	if (_Imm_) ADD32ItoR(X86ARG1, _Imm_);
	_callFunctionArg1((uptr)psxMemRead8, X86ARG1|MEM_X86TAG, 0);
	if (_Rt_) {
		MOVZX32R8toR(EAX, EAX);
		MOV32RtoM((uptr)&psxRegs.GPR.r[_Rt_], EAX);
	}
	PSX_DEL_CONST(_Rt_);
}

static void rpsxLH()
{
	_psxDeleteReg(_Rs_, 1);
	_psxOnWriteReg(_Rt_);
	_psxDeleteReg(_Rt_, 0);

	MOV32MtoR(X86ARG1, (uptr)&psxRegs.GPR.r[_Rs_]);
	if (_Imm_) ADD32ItoR(X86ARG1, _Imm_);
	_callFunctionArg1((uptr)psxMemRead16, X86ARG1|MEM_X86TAG, 0);
	if (_Rt_) {
		MOVSX32R16toR(EAX, EAX);
		MOV32RtoM((uptr)&psxRegs.GPR.r[_Rt_], EAX);
	}
	PSX_DEL_CONST(_Rt_);
}

static void rpsxLHU()
{
	_psxDeleteReg(_Rs_, 1);
	_psxOnWriteReg(_Rt_);
	_psxDeleteReg(_Rt_, 0);

	MOV32MtoR(X86ARG1, (uptr)&psxRegs.GPR.r[_Rs_]);
	if (_Imm_) ADD32ItoR(X86ARG1, _Imm_);
	_callFunctionArg1((uptr)psxMemRead16, X86ARG1|MEM_X86TAG, 0);
	if (_Rt_) {
		MOVZX32R16toR(EAX, EAX);
		MOV32RtoM((uptr)&psxRegs.GPR.r[_Rt_], EAX);
	}
	PSX_DEL_CONST(_Rt_);
}

static void rpsxLW()
{
	_psxDeleteReg(_Rs_, 1);
	_psxOnWriteReg(_Rt_);
	_psxDeleteReg(_Rt_, 0);

	_psxFlushCall(FLUSH_EVERYTHING);
	MOV32MtoR(X86ARG1, (uptr)&psxRegs.GPR.r[_Rs_]);
	if (_Imm_) ADD32ItoR(X86ARG1, _Imm_);

#ifndef TLB_DEBUG_MEM
	TEST32ItoR(X86ARG1, 0x10000000);
	j8Ptr[0] = JZ8(0);
#endif

	_callFunctionArg1((uptr)psxMemRead32, X86ARG1|MEM_X86TAG, 0);
	if (_Rt_) {
		MOV32RtoM((uptr)&psxRegs.GPR.r[_Rt_], EAX);
	}
#ifndef TLB_DEBUG_MEM
	j8Ptr[1] = JMP8(0);
	x86SetJ8(j8Ptr[0]);

	// read from psM directly
	AND32ItoR(X86ARG1, 0x1fffff);
	ADD32ItoR(X86ARG1, (uptr)psxM);

	MOV32RmtoR( X86ARG1, X86ARG1 );
	MOV32RtoM( (uptr)&psxRegs.GPR.r[_Rt_], X86ARG1);

	x86SetJ8(j8Ptr[1]);
#endif
	PSX_DEL_CONST(_Rt_);
}

static void rpsxSB()
{
	_psxDeleteReg(_Rs_, 1);
	_psxDeleteReg(_Rt_, 1);

	MOV32MtoR(X86ARG1, (uptr)&psxRegs.GPR.r[_Rs_]);
	if (_Imm_) ADD32ItoR(X86ARG1, _Imm_);
	_callFunctionArg2((uptr)psxMemWrite8, X86ARG1|MEM_X86TAG, MEM_MEMORYTAG, 0, (uptr)&psxRegs.GPR.r[_Rt_]);
}

static void rpsxSH()
{
	_psxDeleteReg(_Rs_, 1);
	_psxDeleteReg(_Rt_, 1);

	MOV32MtoR(X86ARG1, (uptr)&psxRegs.GPR.r[_Rs_]);
	if (_Imm_) ADD32ItoR(X86ARG1, _Imm_);
	_callFunctionArg2((uptr)psxMemWrite16, X86ARG1|MEM_X86TAG, MEM_MEMORYTAG, 0, (uptr)&psxRegs.GPR.r[_Rt_]);
}

static void rpsxSW()
{
	_psxDeleteReg(_Rs_, 1);
	_psxDeleteReg(_Rt_, 1);

	MOV32MtoR(X86ARG1, (uptr)&psxRegs.GPR.r[_Rs_]);
	if (_Imm_) ADD32ItoR(X86ARG1, _Imm_);
	_callFunctionArg2((uptr)psxMemWrite32, X86ARG1|MEM_X86TAG, MEM_MEMORYTAG, 0, (uptr)&psxRegs.GPR.r[_Rt_]);
}

#endif // end load store

//// SLL
void rpsxSLL_const()
{
	g_psxConstRegs[_Rd_] = g_psxConstRegs[_Rt_] << _Sa_;
}

// shifttype: 0 - sll, 1 - srl, 2 - sra
void rpsxShiftConst(int info, int rdreg, int rtreg, int imm, int shifttype)
{
	imm &= 0x1f;
	if (imm) {
		if( rdreg == rtreg ) {
			switch(shifttype) {
				case 0: SHL32ItoM((uptr)&psxRegs.GPR.r[rdreg], imm); break;
				case 1: SHR32ItoM((uptr)&psxRegs.GPR.r[rdreg], imm); break;
				case 2: SAR32ItoM((uptr)&psxRegs.GPR.r[rdreg], imm); break;
			}
		}
		else {
			MOV32MtoR(EAX, (uptr)&psxRegs.GPR.r[rtreg]);
			switch(shifttype) {
				case 0: SHL32ItoR(EAX, imm); break;
				case 1: SHR32ItoR(EAX, imm); break;
				case 2: SAR32ItoR(EAX, imm); break;
			}
			MOV32RtoM((uptr)&psxRegs.GPR.r[rdreg], EAX);
		}
	}
	else {
		if( rdreg != rtreg ) {
			MOV32MtoR(EAX, (uptr)&psxRegs.GPR.r[rtreg]);
			MOV32RtoM((uptr)&psxRegs.GPR.r[rdreg], EAX);
		}
	}
}

void rpsxSLL_(int info) { rpsxShiftConst(info, _Rd_, _Rt_, _Sa_, 0); }
PSXRECOMPILE_CONSTCODE2(SLL);

//// SRL
void rpsxSRL_const()
{
	g_psxConstRegs[_Rd_] = g_psxConstRegs[_Rt_] >> _Sa_;
}

void rpsxSRL_(int info) { rpsxShiftConst(info, _Rd_, _Rt_, _Sa_, 1); }
PSXRECOMPILE_CONSTCODE2(SRL);

//// SRA
void rpsxSRA_const()
{
	g_psxConstRegs[_Rd_] = *(int*)&g_psxConstRegs[_Rt_] >> _Sa_;
}

void rpsxSRA_(int info) { rpsxShiftConst(info, _Rd_, _Rt_, _Sa_, 2); }
PSXRECOMPILE_CONSTCODE2(SRA);

//// SLLV
void rpsxSLLV_const()
{
	g_psxConstRegs[_Rd_] = g_psxConstRegs[_Rt_] << (g_psxConstRegs[_Rs_]&0x1f);
}

void rpsxShiftVconsts(int info, int shifttype)
{
	rpsxShiftConst(info, _Rd_, _Rt_, g_psxConstRegs[_Rs_], shifttype);
}

void rpsxShiftVconstt(int info, int shifttype)
{
	MOV32ItoR(EAX, g_psxConstRegs[_Rt_]);
	MOV32MtoR(ECX, (uptr)&psxRegs.GPR.r[_Rs_]);
	switch(shifttype) {
		case 0: SHL32CLtoR(EAX); break;
		case 1: SHR32CLtoR(EAX); break;
		case 2: SAR32CLtoR(EAX); break;
	}
	MOV32RtoM((uptr)&psxRegs.GPR.r[_Rd_], EAX);
}

void rpsxSLLV_consts(int info) { rpsxShiftVconsts(info, 0); }
void rpsxSLLV_constt(int info) { rpsxShiftVconstt(info, 0); }
void rpsxSLLV_(int info)
{
	MOV32MtoR(EAX, (uptr)&psxRegs.GPR.r[_Rt_]);
	MOV32MtoR(ECX, (uptr)&psxRegs.GPR.r[_Rs_]);
	SHL32CLtoR(EAX);
	MOV32RtoM((uptr)&psxRegs.GPR.r[_Rd_], EAX);
}

PSXRECOMPILE_CONSTCODE0(SLLV);

//// SRLV
void rpsxSRLV_const()
{
	g_psxConstRegs[_Rd_] = g_psxConstRegs[_Rt_] >> (g_psxConstRegs[_Rs_]&0x1f);
}

void rpsxSRLV_consts(int info) { rpsxShiftVconsts(info, 1); }
void rpsxSRLV_constt(int info) { rpsxShiftVconstt(info, 1); }
void rpsxSRLV_(int info)
{
	MOV32MtoR(EAX, (uptr)&psxRegs.GPR.r[_Rt_]);
	MOV32MtoR(ECX, (uptr)&psxRegs.GPR.r[_Rs_]);
	SHR32CLtoR(EAX);
	MOV32RtoM((uptr)&psxRegs.GPR.r[_Rd_], EAX);
}

PSXRECOMPILE_CONSTCODE0(SRLV);

//// SRAV
void rpsxSRAV_const()
{
	g_psxConstRegs[_Rd_] = *(int*)&g_psxConstRegs[_Rt_] >> (g_psxConstRegs[_Rs_]&0x1f);
}

void rpsxSRAV_consts(int info) { rpsxShiftVconsts(info, 2); }
void rpsxSRAV_constt(int info) { rpsxShiftVconstt(info, 2); }
void rpsxSRAV_(int info)
{
	MOV32MtoR(EAX, (uptr)&psxRegs.GPR.r[_Rt_]);
	MOV32MtoR(ECX, (uptr)&psxRegs.GPR.r[_Rs_]);
	SAR32CLtoR(EAX);
	MOV32RtoM((uptr)&psxRegs.GPR.r[_Rd_], EAX);
}

PSXRECOMPILE_CONSTCODE0(SRAV);

extern void rpsxSYSCALL();
extern void rpsxBREAK();

void rpsxMFHI()
{
	if (!_Rd_) return;

	_psxOnWriteReg(_Rd_);
	_psxDeleteReg(_Rd_, 0);
	MOV32MtoR(EAX, (uptr)&psxRegs.GPR.n.hi);
	MOV32RtoM((uptr)&psxRegs.GPR.r[_Rd_], EAX);
}

void rpsxMTHI()
{
	if( PSX_IS_CONST1(_Rs_) ) {
		MOV32ItoM((uptr)&psxRegs.GPR.n.hi, g_psxConstRegs[_Rs_]);
	}
	else {
		_psxDeleteReg(_Rs_, 1);
		MOV32MtoR(EAX, (uptr)&psxRegs.GPR.r[_Rs_]);
		MOV32RtoM((uptr)&psxRegs.GPR.n.hi, EAX);
	}
}

void rpsxMFLO()
{
	if (!_Rd_) return;

	_psxOnWriteReg(_Rd_);
	_psxDeleteReg(_Rd_, 0);
	MOV32MtoR(EAX, (uptr)&psxRegs.GPR.n.lo);
	MOV32RtoM((uptr)&psxRegs.GPR.r[_Rd_], EAX);
}

void rpsxMTLO()
{
	if( PSX_IS_CONST1(_Rs_) ) {
		MOV32ItoM((uptr)&psxRegs.GPR.n.hi, g_psxConstRegs[_Rs_]);
	}
	else {
		_psxDeleteReg(_Rs_, 1);
		MOV32MtoR(EAX, (uptr)&psxRegs.GPR.r[_Rs_]);
		MOV32RtoM((uptr)&psxRegs.GPR.n.lo, EAX);
	}
}

void rpsxJ()
{
	// j target
	u32 newpc = _Target_ * 4 + (psxpc & 0xf0000000);
	psxRecompileNextInstruction(1);
	psxSetBranchImm(newpc);
}

void rpsxJAL()
{
	u32 newpc = (_Target_ << 2) + ( psxpc & 0xf0000000 );
	_psxDeleteReg(31, 0);
	PSX_SET_CONST(31);
	g_psxConstRegs[31] = psxpc + 4;

	psxRecompileNextInstruction(1);
	psxSetBranchImm(newpc);
}

void rpsxJR()
{
	psxSetBranchReg(_Rs_);
}

void rpsxJALR()
{
	// jalr Rs
	_allocX86reg(ESI, X86TYPE_PCWRITEBACK, 0, MODE_WRITE);
	_psxMoveGPRtoR(ESI, _Rs_);

	if ( _Rd_ ) 
	{
		_psxDeleteReg(_Rd_, 0);
		PSX_SET_CONST(_Rd_);
		g_psxConstRegs[_Rd_] = psxpc + 4;
	}
	
	psxRecompileNextInstruction(1);
	
	if( x86regs[ESI].inuse ) {
		assert( x86regs[ESI].type == X86TYPE_PCWRITEBACK );
		MOV32RtoM((uptr)&psxRegs.pc, ESI);
		x86regs[ESI].inuse = 0;
	}
	else {
		MOV32MtoR(EAX, (uptr)&g_recWriteback);
		MOV32RtoM((uptr)&psxRegs.pc, EAX);
	}

	psxSetBranchReg(0xffffffff);
}

//// BEQ
static void* s_pbranchjmp;
static u32 s_do32 = 0;

#define JUMPVALID(pjmp) (( x86Ptr - (u8*)pjmp ) <= 0x80)

void rpsxSetBranchEQ(int info, int process)
{
	if( process & PROCESS_CONSTS ) {
		CMP32ItoM( (uptr)&psxRegs.GPR.r[ _Rt_ ], g_psxConstRegs[_Rs_] );
		if( s_do32 ) s_pbranchjmp = JNE32( 0 );
		else s_pbranchjmp = JNE8( 0 );
	}
	else if( process & PROCESS_CONSTT ) {
		CMP32ItoM( (uptr)&psxRegs.GPR.r[ _Rs_ ], g_psxConstRegs[_Rt_] );
		if( s_do32 ) s_pbranchjmp = JNE32( 0 );
		else s_pbranchjmp = JNE8( 0 );
	}
	else {
		MOV32MtoR( EAX, (uptr)&psxRegs.GPR.r[ _Rs_ ] );
		CMP32MtoR( EAX, (uptr)&psxRegs.GPR.r[ _Rt_ ] );
		if( s_do32 ) s_pbranchjmp = JNE32( 0 );
		else s_pbranchjmp = JNE8( 0 );
	}
}

void rpsxBEQ_const()
{
	u32 branchTo;
	
	if( g_psxConstRegs[_Rs_] == g_psxConstRegs[_Rt_] )
		branchTo = ((s32)_Imm_ * 4) + psxpc;
	else
		branchTo = psxpc+4;

	psxRecompileNextInstruction(1);
	psxSetBranchImm( branchTo );
}

void rpsxBEQ_process(int info, int process)
{
	u32 branchTo = ((s32)_Imm_ * 4) + psxpc;

	if ( _Rs_ == _Rt_ )
	{
		psxRecompileNextInstruction(1);
		psxSetBranchImm( branchTo );
	}
	else
	{
		_psxFlushAllUnused();
		u8* prevx86 = x86Ptr;
		s_do32 = 0;
		psxSaveBranchState();

		rpsxSetBranchEQ(info, process);
		
		psxRecompileNextInstruction(1);
		psxSetBranchImm(branchTo);

		if( JUMPVALID(s_pbranchjmp) ) {
			x86SetJ8A( (u8*)s_pbranchjmp ); 
		}
		else {
			x86Ptr = prevx86;
			s_do32 = 1;
			psxpc -= 4;
			psxRegs.code = *(u32*)PSXM( psxpc - 4 );
			psxLoadBranchState();
			rpsxSetBranchEQ(info, process);
			psxRecompileNextInstruction(1);
			psxSetBranchImm(branchTo);
			x86SetJ32A( (u32*)s_pbranchjmp ); 
		}

		// recopy the next inst
		psxpc -= 4;
		psxLoadBranchState();
		psxRecompileNextInstruction(1);

		psxSetBranchImm(psxpc);
	}
}

void rpsxBEQ_(int info) { rpsxBEQ_process(info, 0); }
void rpsxBEQ_consts(int info) { rpsxBEQ_process(info, PROCESS_CONSTS); }
void rpsxBEQ_constt(int info) { rpsxBEQ_process(info, PROCESS_CONSTT); }
PSXRECOMPILE_CONSTCODE3(BEQ, 0);

//// BNE
void rpsxBNE_const()
{
	u32 branchTo;
	
	if( g_psxConstRegs[_Rs_] != g_psxConstRegs[_Rt_] )
		branchTo = ((s32)_Imm_ * 4) + psxpc;
	else
		branchTo = psxpc+4;

	psxRecompileNextInstruction(1);
	psxSetBranchImm( branchTo );
}

void rpsxBNE_process(int info, int process)
{
	u32 branchTo = ((s32)_Imm_ * 4) + psxpc;

	if ( _Rs_ == _Rt_ )
	{
		psxRecompileNextInstruction(1);
		psxSetBranchImm(psxpc);
		return;
	}

	_psxFlushAllUnused();
	u8* prevx86 = x86Ptr;
	s_do32 = 0;
	rpsxSetBranchEQ(info, process);

	psxSaveBranchState();
	psxRecompileNextInstruction(1);
	psxSetBranchImm(psxpc);

	if( JUMPVALID(s_pbranchjmp) ) {
		x86SetJ8A( (u8*)s_pbranchjmp ); 
	}
	else {
		x86Ptr = prevx86;
		s_do32 = 1;
		psxpc -= 4;
		psxRegs.code = *(u32*)PSXM( psxpc - 4 );
		psxLoadBranchState();
		rpsxSetBranchEQ(info, process);
		psxRecompileNextInstruction(1);
		psxSetBranchImm(psxpc);
		x86SetJ32A( (u32*)s_pbranchjmp ); 
	}

	// recopy the next inst
	psxpc -= 4;
	psxLoadBranchState();
	psxRecompileNextInstruction(1);

	psxSetBranchImm(branchTo);
}

void rpsxBNE_(int info) { rpsxBNE_process(info, 0); }
void rpsxBNE_consts(int info) { rpsxBNE_process(info, PROCESS_CONSTS); }
void rpsxBNE_constt(int info) { rpsxBNE_process(info, PROCESS_CONSTT); }
PSXRECOMPILE_CONSTCODE3(BNE, 0);

//// BLTZ
void rpsxBLTZ()
{
	// Branch if Rs < 0
	u32 branchTo = (s32)_Imm_ * 4 + psxpc;
	
	_psxFlushAllUnused();

	if( PSX_IS_CONST1(_Rs_) ) {
		if( (int)g_psxConstRegs[_Rs_] >= 0 )
			branchTo = psxpc+4;

		psxRecompileNextInstruction(1);
		psxSetBranchImm( branchTo );
		return;
	}

	CMP32ItoM((uptr)&psxRegs.GPR.r[_Rs_], 0);
	u8* prevx86 = x86Ptr;
	u8* pjmp = JL8(0);

	psxSaveBranchState();
	psxRecompileNextInstruction(1);

	psxSetBranchImm(psxpc);

	if( JUMPVALID(pjmp) ) {
		x86SetJ8A( pjmp ); 
	}
	else {
		x86Ptr = prevx86;
		psxpc -= 4;
		psxRegs.code = *(u32*)PSXM( psxpc - 4 );
		psxLoadBranchState();
		u32* pjmp32 = JL32(0);
		psxRecompileNextInstruction(1);
		psxSetBranchImm(psxpc);
		x86SetJ32A( pjmp32 ); 
	}

	// recopy the next inst
	psxpc -= 4;
	psxLoadBranchState();
	psxRecompileNextInstruction(1);

	psxSetBranchImm(branchTo);
}

//// BGEZ
void rpsxBGEZ() 
{
	u32 branchTo = ((s32)_Imm_ * 4) + psxpc;

	_psxFlushAllUnused();

	if( PSX_IS_CONST1(_Rs_) ) {
		if ( (int)g_psxConstRegs[_Rs_] < 0 )
			branchTo = psxpc+4;

		psxRecompileNextInstruction(1);
		psxSetBranchImm( branchTo );
		return;
	}

	CMP32ItoM((uptr)&psxRegs.GPR.r[_Rs_], 0);
	u8* prevx86 = x86Ptr;
	u8* pjmp = JGE8(0);

	psxSaveBranchState();
	psxRecompileNextInstruction(1);

	psxSetBranchImm(psxpc);

	if( JUMPVALID(pjmp) ) {
		x86SetJ8A( pjmp ); 
	}
	else {
		x86Ptr = prevx86;
		psxpc -= 4;
		psxRegs.code = *(u32*)PSXM( psxpc - 4 );
		psxLoadBranchState();
		u32* pjmp32 = JGE32(0);
		psxRecompileNextInstruction(1);
		psxSetBranchImm(psxpc);
		x86SetJ32A( pjmp32 ); 
	}

	// recopy the next inst
	psxpc -= 4;
	psxLoadBranchState();
	psxRecompileNextInstruction(1);

	psxSetBranchImm(branchTo);
}

//// BLTZAL
void rpsxBLTZAL()
{
	// Branch if Rs < 0
	u32 branchTo = (s32)_Imm_ * 4 + psxpc;

	_psxFlushConstReg(31);
	_psxDeleteReg(31, 0);
	_psxFlushAllUnused();

	if( PSX_IS_CONST1(_Rs_) ) {
		if( (int)g_psxConstRegs[_Rs_] >= 0 )
			branchTo = psxpc+4;
		else {
			PSX_SET_CONST(_Rt_);
			g_psxConstRegs[31] = psxpc+4;
		}

		psxRecompileNextInstruction(1);
		psxSetBranchImm( branchTo );
		return;
	}

	CMP32ItoM((uptr)&psxRegs.GPR.r[_Rs_], 0);
	u8* prevx86 = x86Ptr;
	u8* pjmp = JL8(0);

	psxSaveBranchState();

	MOV32ItoM((uptr)&psxRegs.GPR.r[31], psxpc+4);
	psxRecompileNextInstruction(1);

	psxSetBranchImm(psxpc);

	if( JUMPVALID(pjmp) ) {
		x86SetJ8A( pjmp ); 
	}
	else {
		x86Ptr = prevx86;
		psxpc -= 4;
		psxRegs.code = *(u32*)PSXM( psxpc - 4 );
		psxLoadBranchState();
		u32* pjmp32 = JL32(0);
		MOV32ItoM((uptr)&psxRegs.GPR.r[31], psxpc+4);
		psxRecompileNextInstruction(1);
		psxSetBranchImm(psxpc);
		x86SetJ32A( pjmp32 ); 
	}

	// recopy the next inst
	psxpc -= 4;
	psxLoadBranchState();
	psxRecompileNextInstruction(1);

	psxSetBranchImm(branchTo);
}

//// BGEZAL
void rpsxBGEZAL()
{
	u32 branchTo = ((s32)_Imm_ * 4) + psxpc;

	_psxFlushConstReg(31);
	_psxDeleteReg(31, 0);
	_psxFlushAllUnused();

	if( PSX_IS_CONST1(_Rs_) ) {
		if( g_psxConstRegs[_Rs_] < 0 )
			branchTo = psxpc+4;
		else MOV32ItoM((uptr)&psxRegs.GPR.r[31], psxpc+4);

		psxRecompileNextInstruction(1);
		psxSetBranchImm( branchTo );
		return;
	}

	CMP32ItoM((uptr)&psxRegs.GPR.r[_Rs_], 0);
	u8* prevx86 = x86Ptr;
	u8* pjmp = JGE8(0);

	MOV32ItoM((uptr)&psxRegs.GPR.r[31], psxpc+4);

	psxSaveBranchState();
	psxRecompileNextInstruction(1);

	psxSetBranchImm(psxpc);

	if( JUMPVALID(pjmp) ) {
		x86SetJ8A( pjmp ); 
	}
	else {
		x86Ptr = prevx86;
		psxpc -= 4;
		psxRegs.code = *(u32*)PSXM( psxpc - 4 );
		psxLoadBranchState();
		u32* pjmp32 = JGE32(0);
		MOV32ItoM((uptr)&psxRegs.GPR.r[31], psxpc+4);
		psxRecompileNextInstruction(1);
		psxSetBranchImm(psxpc);
		x86SetJ32A( pjmp32 ); 
	}

	// recopy the next inst
	psxpc -= 4;
	psxLoadBranchState();
	psxRecompileNextInstruction(1);

	psxSetBranchImm(branchTo);
}

//// BLEZ
void rpsxBLEZ()
{
	// Branch if Rs <= 0
	u32 branchTo = (s32)_Imm_ * 4 + psxpc;
	
	_psxFlushAllUnused();

	if( PSX_IS_CONST1(_Rs_) ) {
		if( (int)g_psxConstRegs[_Rs_] > 0 )
			branchTo = psxpc+4;

		psxRecompileNextInstruction(1);
		psxSetBranchImm( branchTo );
		return;
	}

	_psxDeleteReg(_Rs_, 1);
	_clearNeededX86regs();

	CMP32ItoM((uptr)&psxRegs.GPR.r[_Rs_], 0);
	u8* prevx86 = x86Ptr;
	u8* pjmp = JLE8(0);

	psxSaveBranchState();
	psxRecompileNextInstruction(1);
	psxSetBranchImm(psxpc);

	if( JUMPVALID(pjmp) ) {
		x86SetJ8A( pjmp ); 
	}
	else {
		x86Ptr = prevx86;
		psxpc -= 4;
		psxRegs.code = *(u32*)PSXM( psxpc - 4 );
		psxLoadBranchState();
		u32* pjmp32 = JLE32(0);
		psxRecompileNextInstruction(1);
		psxSetBranchImm(psxpc);
		x86SetJ32A( pjmp32 ); 
	}

	psxpc -= 4;
	psxLoadBranchState();
	psxRecompileNextInstruction(1);
	psxSetBranchImm(branchTo);
}

//// BGTZ
void rpsxBGTZ()
{
	// Branch if Rs > 0
	u32 branchTo = (s32)_Imm_ * 4 + psxpc;
	
	_psxFlushAllUnused();

	if( PSX_IS_CONST1(_Rs_) ) {
		if( (int)g_psxConstRegs[_Rs_] <= 0 )
			branchTo = psxpc+4;

		psxRecompileNextInstruction(1);
		psxSetBranchImm( branchTo );
		return;
	}

	_psxDeleteReg(_Rs_, 1);
	_clearNeededX86regs();

	CMP32ItoM((uptr)&psxRegs.GPR.r[_Rs_], 0);
	u8* prevx86 = x86Ptr;
	u8* pjmp = JG8(0);

	psxSaveBranchState();
	psxRecompileNextInstruction(1);
	psxSetBranchImm(psxpc);

	if( JUMPVALID(pjmp) ) {
		x86SetJ8A( pjmp ); 
	}
	else {
		x86Ptr = prevx86;
		psxpc -= 4;
		psxRegs.code = *(u32*)PSXM( psxpc - 4 );
		psxLoadBranchState();
		u32* pjmp32 = JG32(0);
		psxRecompileNextInstruction(1);
		psxSetBranchImm(psxpc);
		x86SetJ32A( pjmp32 ); 
	}

	psxpc -= 4;
	psxLoadBranchState();
	psxRecompileNextInstruction(1);
	psxSetBranchImm(branchTo);
}

void rpsxMFC0()
{
	// Rt = Cop0->Rd
	if (!_Rt_) return;

	_psxOnWriteReg(_Rt_);
	MOV32MtoR(EAX, (uptr)&psxRegs.CP0.r[_Rd_]);
	MOV32RtoM((uptr)&psxRegs.GPR.r[_Rt_], EAX);
}

void rpsxCFC0()
{
	// Rt = Cop0->Rd
	if (!_Rt_) return;

	_psxOnWriteReg(_Rt_);
	MOV32MtoR(EAX, (uptr)&psxRegs.CP0.r[_Rd_]);
	MOV32RtoM((uptr)&psxRegs.GPR.r[_Rt_], EAX);
}

void rpsxMTC0()
{
	// Cop0->Rd = Rt
	if( PSX_IS_CONST1(_Rt_) ) {
		MOV32ItoM((uptr)&psxRegs.CP0.r[_Rd_], g_psxConstRegs[_Rt_]);
	}
	else {
		_psxDeleteReg(_Rt_, 1);
		MOV32MtoR(EAX, (uptr)&psxRegs.GPR.r[_Rt_]);
		MOV32RtoM((uptr)&psxRegs.CP0.r[_Rd_], EAX);
	}
}

void rpsxCTC0()
{
	// Cop0->Rd = Rt
	rpsxMTC0();
}

void rpsxRFE()
{
	MOV32MtoR(EAX, (uptr)&psxRegs.CP0.n.Status);
	MOV32RtoR(ECX, EAX);
	AND32ItoR(EAX, 0xfffffff0);
	AND32ItoR(ECX, 0x3c);
	SHR32ItoR(ECX, 2);
	OR32RtoR (EAX, ECX);
	MOV32RtoM((uptr)&psxRegs.CP0.n.Status, EAX);

	// Test the IOP's INTC status, so that any pending ints get raised.

	_psxFlushCall(0);
	CALLFunc( (uptr)&iopTestIntc );
}

// R3000A tables
extern void (*rpsxBSC[64])();
extern void (*rpsxSPC[64])();
extern void (*rpsxREG[32])();
extern void (*rpsxCP0[32])();
extern void (*rpsxCP2[64])();
extern void (*rpsxCP2BSC[32])();

static void rpsxSPECIAL() { rpsxSPC[_Funct_](); }
static void rpsxREGIMM() { rpsxREG[_Rt_](); }
static void rpsxCOP0() { rpsxCP0[_Rs_](); }
//static void rpsxBASIC() { rpsxCP2BSC[_Rs_](); }

static void rpsxNULL() {
	SysPrintf("psxUNK: %8.8x\n", psxRegs.code);
}

void (*rpsxBSC[64])() = {
	rpsxSPECIAL, rpsxREGIMM, rpsxJ   , rpsxJAL  , rpsxBEQ , rpsxBNE , rpsxBLEZ, rpsxBGTZ,
	rpsxADDI   , rpsxADDIU , rpsxSLTI, rpsxSLTIU, rpsxANDI, rpsxORI , rpsxXORI, rpsxLUI ,
	rpsxCOP0   , rpsxNULL  , rpsxNULL, rpsxNULL , rpsxNULL, rpsxNULL, rpsxNULL, rpsxNULL,
	rpsxNULL   , rpsxNULL  , rpsxNULL, rpsxNULL , rpsxNULL, rpsxNULL, rpsxNULL, rpsxNULL,
	rpsxLB     , rpsxLH    , rpsxLWL , rpsxLW   , rpsxLBU , rpsxLHU , rpsxLWR , rpsxNULL,
	rpsxSB     , rpsxSH    , rpsxSWL , rpsxSW   , rpsxNULL, rpsxNULL, rpsxSWR , rpsxNULL,
	rpsxNULL   , rpsxNULL  , rpsxNULL, rpsxNULL , rpsxNULL, rpsxNULL, rpsxNULL, rpsxNULL,
	rpsxNULL   , rpsxNULL  , rpsxNULL, rpsxNULL , rpsxNULL, rpsxNULL, rpsxNULL, rpsxNULL
};

void (*rpsxSPC[64])() = {
	rpsxSLL , rpsxNULL, rpsxSRL , rpsxSRA , rpsxSLLV   , rpsxNULL , rpsxSRLV, rpsxSRAV,
	rpsxJR  , rpsxJALR, rpsxNULL, rpsxNULL, rpsxSYSCALL, rpsxBREAK, rpsxNULL, rpsxNULL,
	rpsxMFHI, rpsxMTHI, rpsxMFLO, rpsxMTLO, rpsxNULL   , rpsxNULL , rpsxNULL, rpsxNULL,
	rpsxMULT, rpsxMULTU, rpsxDIV, rpsxDIVU, rpsxNULL   , rpsxNULL , rpsxNULL, rpsxNULL,
	rpsxADD , rpsxADDU, rpsxSUB , rpsxSUBU, rpsxAND    , rpsxOR   , rpsxXOR , rpsxNOR ,
	rpsxNULL, rpsxNULL, rpsxSLT , rpsxSLTU, rpsxNULL   , rpsxNULL , rpsxNULL, rpsxNULL,
	rpsxNULL, rpsxNULL, rpsxNULL, rpsxNULL, rpsxNULL   , rpsxNULL , rpsxNULL, rpsxNULL,
	rpsxNULL, rpsxNULL, rpsxNULL, rpsxNULL, rpsxNULL   , rpsxNULL , rpsxNULL, rpsxNULL
};

void (*rpsxREG[32])() = {
	rpsxBLTZ  , rpsxBGEZ  , rpsxNULL, rpsxNULL, rpsxNULL, rpsxNULL, rpsxNULL, rpsxNULL,
	rpsxNULL  , rpsxNULL  , rpsxNULL, rpsxNULL, rpsxNULL, rpsxNULL, rpsxNULL, rpsxNULL,
	rpsxBLTZAL, rpsxBGEZAL, rpsxNULL, rpsxNULL, rpsxNULL, rpsxNULL, rpsxNULL, rpsxNULL,
	rpsxNULL  , rpsxNULL  , rpsxNULL, rpsxNULL, rpsxNULL, rpsxNULL, rpsxNULL, rpsxNULL
};

void (*rpsxCP0[32])() = {
	rpsxMFC0, rpsxNULL, rpsxCFC0, rpsxNULL, rpsxMTC0, rpsxNULL, rpsxCTC0, rpsxNULL,
	rpsxNULL, rpsxNULL, rpsxNULL, rpsxNULL, rpsxNULL, rpsxNULL, rpsxNULL, rpsxNULL,
	rpsxRFE , rpsxNULL, rpsxNULL, rpsxNULL, rpsxNULL, rpsxNULL, rpsxNULL, rpsxNULL,
	rpsxNULL, rpsxNULL, rpsxNULL, rpsxNULL, rpsxNULL, rpsxNULL, rpsxNULL, rpsxNULL
};

// coissued insts
void (*rpsxBSC_co[64] )() = {
    rpsxNULL,	rpsxNULL,   rpsxNULL, rpsxNULL,  rpsxNULL, rpsxNULL, rpsxNULL,  rpsxNULL,
    rpsxNULL,	rpsxNULL,   rpsxNULL, rpsxNULL,  rpsxNULL, rpsxNULL, rpsxNULL,  rpsxNULL,
    rpsxNULL,	rpsxNULL,   rpsxNULL, rpsxNULL,  rpsxNULL, rpsxNULL, rpsxNULL,  rpsxNULL,
    rpsxNULL,	rpsxNULL,   rpsxNULL, rpsxNULL,  rpsxNULL, rpsxNULL, rpsxNULL,  rpsxNULL,
    rpsxNULL,	rpsxNULL,   rpsxNULL, rpsxNULL,  rpsxNULL, rpsxNULL, rpsxNULL,  rpsxNULL,
    rpsxNULL,	rpsxNULL,   rpsxNULL, rpsxNULL,  rpsxNULL, rpsxNULL, rpsxNULL,  rpsxNULL,
    rpsxNULL,	rpsxNULL,   rpsxNULL, rpsxNULL,  rpsxNULL, rpsxNULL, rpsxNULL,  rpsxNULL,
    rpsxNULL,	rpsxNULL,   rpsxNULL, rpsxNULL,  rpsxNULL, rpsxNULL, rpsxNULL,  rpsxNULL,
};

////////////////////////////////////////////////
// Back-Prob Function Tables - Gathering Info //
////////////////////////////////////////////////
#define rpsxpropSetRead(reg) { \
	if( !(pinst->regs[reg] & EEINST_USED) ) \
		pinst->regs[reg] |= EEINST_LASTUSE; \
	prev->regs[reg] |= EEINST_LIVE0|EEINST_USED; \
	pinst->regs[reg] |= EEINST_USED; \
	_recFillRegister(*pinst, XMMTYPE_GPRREG, reg, 0); \
} \

#define rpsxpropSetWrite(reg) { \
	prev->regs[reg] &= ~EEINST_LIVE0; \
	if( !(pinst->regs[reg] & EEINST_USED) ) \
		pinst->regs[reg] |= EEINST_LASTUSE; \
	pinst->regs[reg] |= EEINST_USED; \
	prev->regs[reg] |= EEINST_USED; \
	_recFillRegister(*pinst, XMMTYPE_GPRREG, reg, 1); \
}

void rpsxpropBSC(EEINST* prev, EEINST* pinst);
void rpsxpropSPECIAL(EEINST* prev, EEINST* pinst);
void rpsxpropREGIMM(EEINST* prev, EEINST* pinst);
void rpsxpropCP0(EEINST* prev, EEINST* pinst);
void rpsxpropCP2(EEINST* prev, EEINST* pinst);

//SPECIAL, REGIMM, J   , JAL  , BEQ , BNE , BLEZ, BGTZ,
//ADDI   , ADDIU , SLTI, SLTIU, ANDI, ORI , XORI, LUI ,
//COP0   , NULL  , COP2, NULL , NULL, NULL, NULL, NULL,
//NULL   , NULL  , NULL, NULL , NULL, NULL, NULL, NULL,
//LB     , LH    , LWL , LW   , LBU , LHU , LWR , NULL,
//SB     , SH    , SWL , SW   , NULL, NULL, SWR , NULL,
//NULL   , NULL  , NULL, NULL , NULL, NULL, NULL, NULL,
//NULL   , NULL  , NULL, NULL , NULL, NULL, NULL, NULL
void rpsxpropBSC(EEINST* prev, EEINST* pinst)
{
	switch(psxRegs.code >> 26) {
		case 0: rpsxpropSPECIAL(prev, pinst); break;
		case 1: rpsxpropREGIMM(prev, pinst); break;
		case 2: // j
			break;
		case 3: // jal
			rpsxpropSetWrite(31);
			break;
		case 4: // beq
		case 5: // bne
			rpsxpropSetRead(_Rs_);
			rpsxpropSetRead(_Rt_);
			break;

		case 6: // blez
		case 7: // bgtz
			rpsxpropSetRead(_Rs_);
			break;

		case 15: // lui
			rpsxpropSetWrite(_Rt_);
			break;

		case 16: rpsxpropCP0(prev, pinst); break;
		case 18: assert(0); break;

		// stores
		case 40: case 41: case 42: case 43: case 46:
			rpsxpropSetRead(_Rt_);
			rpsxpropSetRead(_Rs_);
			break;

		default:
			rpsxpropSetWrite(_Rt_);
			rpsxpropSetRead(_Rs_);
			break;
	}
}

//SLL , NULL, SRL , SRA , SLLV   , NULL , SRLV, SRAV,
//JR  , JALR, NULL, NULL, SYSCALL, BREAK, NULL, NULL,
//MFHI, MTHI, MFLO, MTLO, NULL   , NULL , NULL, NULL,
//MULT, MULTU, DIV, DIVU, NULL   , NULL , NULL, NULL,
//ADD , ADDU, SUB , SUBU, AND    , OR   , XOR , NOR ,
//NULL, NULL, SLT , SLTU, NULL   , NULL , NULL, NULL,
//NULL, NULL, NULL, NULL, NULL   , NULL , NULL, NULL,
//NULL, NULL, NULL, NULL, NULL   , NULL , NULL, NULL
void rpsxpropSPECIAL(EEINST* prev, EEINST* pinst)
{
	switch(_Funct_) {
		case 0: // SLL
		case 2: // SRL
		case 3: // SRA
			rpsxpropSetWrite(_Rd_);
			rpsxpropSetRead(_Rt_);
			break;

		case 8: // JR
			rpsxpropSetRead(_Rs_);
			break;
		case 9: // JALR
			rpsxpropSetWrite(_Rd_);
			rpsxpropSetRead(_Rs_);
			break;

		case 12: // syscall
		case 13: // break
			_recClearInst(prev);
			prev->info = 0;
			break;
		case 15: // sync
			break;

		case 16: // mfhi
			rpsxpropSetWrite(_Rd_);
			rpsxpropSetRead(PSX_HI);
			break;
		case 17: // mthi
			rpsxpropSetWrite(PSX_HI);
			rpsxpropSetRead(_Rs_);
			break;
		case 18: // mflo
			rpsxpropSetWrite(_Rd_);
			rpsxpropSetRead(PSX_LO);
			break;
		case 19: // mtlo
			rpsxpropSetWrite(PSX_LO);
			rpsxpropSetRead(_Rs_);
			break;

		case 24: // mult
		case 25: // multu
		case 26: // div
		case 27: // divu
			rpsxpropSetWrite(PSX_LO);
			rpsxpropSetWrite(PSX_HI);
			rpsxpropSetRead(_Rs_);
			rpsxpropSetRead(_Rt_);
			break;

		case 32: // add
		case 33: // addu
		case 34: // sub
		case 35: // subu
			rpsxpropSetWrite(_Rd_);
			if( _Rs_ ) rpsxpropSetRead(_Rs_);
			if( _Rt_ ) rpsxpropSetRead(_Rt_);
			break;

		default:
			rpsxpropSetWrite(_Rd_);
			rpsxpropSetRead(_Rs_);
			rpsxpropSetRead(_Rt_);
			break;
	}
}

//BLTZ  , BGEZ  , NULL, NULL, NULL, NULL, NULL, NULL,
//NULL  , NULL  , NULL, NULL, NULL, NULL, NULL, NULL,
//BLTZAL, BGEZAL, NULL, NULL, NULL, NULL, NULL, NULL,
//NULL  , NULL  , NULL, NULL, NULL, NULL, NULL, NULL
void rpsxpropREGIMM(EEINST* prev, EEINST* pinst)
{
	switch(_Rt_) {
		case 0: // bltz
		case 1: // bgez
			rpsxpropSetRead(_Rs_);
			break;

		case 16: // bltzal
		case 17: // bgezal
			// do not write 31
			rpsxpropSetRead(_Rs_);
			break;

		jNO_DEFAULT
	}
}

//MFC0, NULL, CFC0, NULL, MTC0, NULL, CTC0, NULL,
//NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
//RFE , NULL, NULL, NULL, NULL, NULL, NULL, NULL,
//NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL
void rpsxpropCP0(EEINST* prev, EEINST* pinst)
{
	switch(_Rs_) {
		case 0: // mfc0
		case 2: // cfc0
			rpsxpropSetWrite(_Rt_);
			break;
		
		case 4: // mtc0
		case 6: // ctc0
			rpsxpropSetRead(_Rt_);
			break;
		case 16: // rfe
			break;

		jNO_DEFAULT
	}
}
