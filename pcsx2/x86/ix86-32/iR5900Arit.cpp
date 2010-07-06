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
#include "R5900OpcodeTables.h"
#include "iR5900.h"

using namespace x86Emitter;

namespace R5900 {
namespace Dynarec {
namespace OpcodeImpl
{

/*********************************************************
* Register arithmetic                                    *
* Format:  OP rd, rs, rt                                 *
*********************************************************/

// TODO: overflow checks

#ifndef ARITHMETIC_RECOMPILE

namespace Interp = R5900::Interpreter::OpcodeImpl;

REC_FUNC_DEL(ADD, _Rd_);
REC_FUNC_DEL(ADDU, _Rd_);
REC_FUNC_DEL(DADD, _Rd_);
REC_FUNC_DEL(DADDU, _Rd_);
REC_FUNC_DEL(SUB, _Rd_);
REC_FUNC_DEL(SUBU, _Rd_);
REC_FUNC_DEL(DSUB, _Rd_);
REC_FUNC_DEL(DSUBU, _Rd_);
REC_FUNC_DEL(AND, _Rd_);
REC_FUNC_DEL(OR, _Rd_);
REC_FUNC_DEL(XOR, _Rd_);
REC_FUNC_DEL(NOR, _Rd_);
REC_FUNC_DEL(SLT, _Rd_);
REC_FUNC_DEL(SLTU, _Rd_);

#else

//// ADD
void recADD_const()
{
	g_cpuConstRegs[_Rd_].SD[0] = g_cpuConstRegs[_Rs_].SL[0] + g_cpuConstRegs[_Rt_].SL[0];
}

void recADD_constv(int info, int creg, int vreg)
{
	pxAssert( !(info&PROCESS_EE_XMM) );

	s32 cval = g_cpuConstRegs[creg].SL[0];

	xMOV(eax, ptr32[&cpuRegs.GPR.r[vreg].SL[0]]);
	if (cval)
		xADD(eax, cval);
	eeSignExtendTo(_Rd_, _Rd_ == vreg && !cval);
}

// s is constant
void recADD_consts(int info)
{
	recADD_constv(info, _Rs_, _Rt_);
}

// t is constant
void recADD_constt(int info)
{
	recADD_constv(info, _Rt_, _Rs_);
}

// nothing is constant
void recADD_(int info)
{
	pxAssert( !(info&PROCESS_EE_XMM) );

	xMOV(eax, ptr32[&cpuRegs.GPR.r[_Rs_].SL[0]]);
	if (_Rs_ == _Rt_)
		xADD(eax, eax);
	else
		xADD(eax, ptr32[&cpuRegs.GPR.r[_Rt_].SL[0]]);
	eeSignExtendTo(_Rd_);
}

EERECOMPILE_CODE0(ADD, XMMINFO_WRITED|XMMINFO_READS|XMMINFO_READT);

//// ADDU
void recADDU(void)
{
	recADD();
}

//// DADD
void recDADD_const(void)
{
	g_cpuConstRegs[_Rd_].SD[0] = g_cpuConstRegs[_Rs_].SD[0] + g_cpuConstRegs[_Rt_].SD[0];
}

void recDADD_constv(int info, int creg, int vreg)
{
	pxAssert( !(info&PROCESS_EE_XMM) );

	GPR_reg64 cval = g_cpuConstRegs[creg];

	if (_Rd_ == vreg) {
		if (!cval.SD[0])
			return; // no-op
		xADD(ptr32[&cpuRegs.GPR.r[_Rd_].SL[0]], cval.SL[0]);
		xADC(ptr32[&cpuRegs.GPR.r[_Rd_].SL[1]], cval.SL[1]);
	} else {
		xMOV(eax, ptr32[&cpuRegs.GPR.r[vreg].SL[0]]);
		xMOV(edx, ptr32[&cpuRegs.GPR.r[vreg].SL[1]]);
		if (cval.SD[0]) {
			xADD(eax, cval.SL[0]);
			xADC(edx, cval.SL[1]);
		}
		xMOV(ptr32[&cpuRegs.GPR.r[_Rd_].SL[0]], eax);
		xMOV(ptr32[&cpuRegs.GPR.r[_Rd_].SL[1]], edx);
	}
}

void recDADD_consts(int info)
{
	recDADD_constv(info, _Rs_, _Rt_);
}

void recDADD_constt(int info)
{
	recDADD_constv(info, _Rt_, _Rs_);
}

void recDADD_(int info)
{
	pxAssert( !(info&PROCESS_EE_XMM) );

	int rs = _Rs_, rt = _Rt_;
	if (_Rd_ == _Rt_)
		rs = _Rt_, rt = _Rs_;

	xMOV(eax, ptr32[&cpuRegs.GPR.r[rt].SL[0]]);

	if (_Rd_ == _Rs_ && _Rs_ == _Rt_) {
		xSHLD(ptr32[&cpuRegs.GPR.r[_Rd_].SL[1]], eax, 1);
		xSHL(ptr32[&cpuRegs.GPR.r[_Rd_].SL[0]], 1);
		return;
	}

	xMOV(edx, ptr32[&cpuRegs.GPR.r[rt].SL[1]]);

	if (_Rd_ == rs) {
		xADD(ptr32[&cpuRegs.GPR.r[_Rd_].SL[0]], eax);
		xADC(ptr32[&cpuRegs.GPR.r[_Rd_].SL[1]], edx);
		return;
	} else if (rs == rt) {
		xADD(eax, eax);
		xADC(edx, edx);
	} else {
		xADD(eax, ptr32[&cpuRegs.GPR.r[rs].SL[0]]);
		xADC(edx, ptr32[&cpuRegs.GPR.r[rs].SL[1]]);
	}

	xMOV(ptr32[&cpuRegs.GPR.r[_Rd_].SL[0]], eax);
	xMOV(ptr32[&cpuRegs.GPR.r[_Rd_].SL[1]], edx);
}

EERECOMPILE_CODE0(DADD, XMMINFO_WRITED|XMMINFO_READS|XMMINFO_READT);

//// DADDU
void recDADDU(void)
{
	recDADD();
}

//// SUB

void recSUB_const()
{
	g_cpuConstRegs[_Rd_].SD[0] = g_cpuConstRegs[_Rs_].SL[0] - g_cpuConstRegs[_Rt_].SL[0];
}

void recSUB_consts(int info)
{
	pxAssert( !(info&PROCESS_EE_XMM) );

	s32 sval = g_cpuConstRegs[_Rs_].SL[0];

	xMOV(eax, sval);
	xSUB(eax, ptr32[&cpuRegs.GPR.r[_Rt_].SL[0]]);
	eeSignExtendTo(_Rd_);
}

void recSUB_constt(int info)
{
	pxAssert( !(info&PROCESS_EE_XMM) );

	s32 tval = g_cpuConstRegs[_Rt_].SL[0];

	xMOV(eax, ptr32[&cpuRegs.GPR.r[_Rs_].SL[0]]);
	if (tval)
		xSUB(eax, tval);
	eeSignExtendTo(_Rd_, _Rd_ == _Rs_ && !tval);
}

void recSUB_(int info)
{
	pxAssert( !(info&PROCESS_EE_XMM) );

	if (_Rs_ == _Rt_) {
		xMOV(ptr32[&cpuRegs.GPR.r[_Rd_].SL[0]], 0);
		xMOV(ptr32[&cpuRegs.GPR.r[_Rd_].SL[1]], 0);
		return;
	}

	xMOV(eax, ptr32[&cpuRegs.GPR.r[_Rs_].SL[0]]);
	xSUB(eax, ptr32[&cpuRegs.GPR.r[_Rt_].SL[0]]);
	eeSignExtendTo(_Rd_);
}

EERECOMPILE_CODE0(SUB, XMMINFO_READS|XMMINFO_READT|XMMINFO_WRITED);

//// SUBU
void recSUBU(void)
{
	recSUB();
}

//// DSUB
void recDSUB_const()
{
	g_cpuConstRegs[_Rd_].SD[0] = g_cpuConstRegs[_Rs_].SD[0] - g_cpuConstRegs[_Rt_].SD[0];
}

void recDSUB_consts(int info)
{
	pxAssert( !(info&PROCESS_EE_XMM) );

	GPR_reg64 sval = g_cpuConstRegs[_Rs_];

	if (!sval.SD[0] && _Rd_ == _Rt_) {
		/* To understand this 64-bit negate, consider that a negate in 2's complement
		 * is a NOT then an ADD 1.  The upper word should only have the NOT stage unless
		 * the ADD overflows.  The ADD only overflows if the lower word is 0.
		 * Incrementing before a NEG is the same as a NOT and the carry flag is set for
		 * a non-zero lower word.
		 */
		xNEG(ptr32[&cpuRegs.GPR.r[_Rd_].SL[0]]);
		xADC(ptr32[&cpuRegs.GPR.r[_Rd_].SL[1]], 0);
		xNEG(ptr32[&cpuRegs.GPR.r[_Rd_].SL[1]]);
		return;
	} else {
		xMOV(eax, sval.SL[0]);
		xMOV(edx, sval.SL[1]);
	}

	xSUB(eax, ptr32[&cpuRegs.GPR.r[_Rt_].SL[0]]);
	xSBB(edx, ptr32[&cpuRegs.GPR.r[_Rt_].SL[1]]);
	xMOV(ptr32[&cpuRegs.GPR.r[_Rd_].SL[0]], eax);
	xMOV(ptr32[&cpuRegs.GPR.r[_Rd_].SL[1]], edx);
}

void recDSUB_constt(int info)
{
	pxAssert( !(info&PROCESS_EE_XMM) );

	GPR_reg64 tval = g_cpuConstRegs[_Rt_];

	if (_Rd_ == _Rs_) {
		xSUB(ptr32[&cpuRegs.GPR.r[_Rd_].SL[0]], tval.SL[0]);
		xSBB(ptr32[&cpuRegs.GPR.r[_Rd_].SL[1]], tval.SL[1]);
	} else {
		xMOV(eax, ptr32[&cpuRegs.GPR.r[_Rs_].SL[0]]);
		xMOV(edx, ptr32[&cpuRegs.GPR.r[_Rs_].SL[1]]);
		if (tval.SD[0]) {
			xSUB(eax, tval.SL[0]);
			xSBB(edx, tval.SL[1]);
		}
		xMOV(ptr32[&cpuRegs.GPR.r[_Rd_].SL[0]], eax);
		xMOV(ptr32[&cpuRegs.GPR.r[_Rd_].SL[1]], edx);
	}
}

void recDSUB_(int info)
{
	pxAssert( !(info&PROCESS_EE_XMM) );

	if (_Rs_ == _Rt_) {
		xMOV(ptr32[&cpuRegs.GPR.r[_Rd_].SL[0]], 0);
		xMOV(ptr32[&cpuRegs.GPR.r[_Rd_].SL[1]], 0);
	} else if (_Rd_ == _Rs_) {
		xMOV(eax, ptr32[&cpuRegs.GPR.r[_Rt_].SL[0]]);
		xMOV(edx, ptr32[&cpuRegs.GPR.r[_Rt_].SL[1]]);
		xSUB(ptr32[&cpuRegs.GPR.r[_Rd_].SL[0]], eax);
		xSBB(ptr32[&cpuRegs.GPR.r[_Rd_].SL[1]], edx);
	} else {
		xMOV(eax, ptr32[&cpuRegs.GPR.r[_Rs_].SL[0]]);
		xMOV(edx, ptr32[&cpuRegs.GPR.r[_Rs_].SL[1]]);
		xSUB(eax, ptr32[&cpuRegs.GPR.r[_Rt_].SL[0]]);
		xSBB(edx, ptr32[&cpuRegs.GPR.r[_Rt_].SL[1]]);
		xMOV(ptr32[&cpuRegs.GPR.r[_Rd_].SL[0]], eax);
		xMOV(ptr32[&cpuRegs.GPR.r[_Rd_].SL[1]], edx);
	}
}

EERECOMPILE_CODE0(DSUB, XMMINFO_READS|XMMINFO_READT|XMMINFO_WRITED);

//// DSUBU
void recDSUBU(void)
{
	recDSUB();
}

//// AND
void recAND_const()
{
	g_cpuConstRegs[_Rd_].UD[0] = g_cpuConstRegs[_Rs_].UD[0] & g_cpuConstRegs[_Rt_].UD[0];
}

void recAND_constv(int info, int creg, int vreg)
{
	pxAssert( !(info & PROCESS_EE_XMM) );

	GPR_reg64 cval = g_cpuConstRegs[creg];

	for (int i = 0; i < 2; i++) {
		if (!cval.UL[i]) {
			xMOV(ptr32[&cpuRegs.GPR.r[_Rd_].UL[i]], 0);
		} else if (_Rd_ == vreg) {
			if (cval.UL[i] != -1)
				xAND(ptr32[&cpuRegs.GPR.r[_Rd_].UL[i]], cval.UL[i]);
		} else {
			xMOV(eax, ptr32[&cpuRegs.GPR.r[vreg].UL[i]]);
			if (cval.UL[i] != -1)
				xAND(eax, cval.UL[i]);
			xMOV(ptr32[&cpuRegs.GPR.r[_Rd_].UL[i]], eax);
		}
	}
}

void recAND_consts(int info)
{
	recAND_constv(info, _Rs_, _Rt_);
}

void recAND_constt(int info)
{
	recAND_constv(info, _Rt_, _Rs_);
}

void recAND_(int info)
{
	pxAssert( !(info & PROCESS_EE_XMM) );

	int rs = _Rs_, rt = _Rt_;
	if (_Rd_ == _Rt_)
		rs = _Rt_, rt = _Rs_;

	for (int i = 0; i < 2; i++) {
		if (_Rd_ == rs) {
			if (rs == rt)
				continue;
			xMOV(eax, ptr32[&cpuRegs.GPR.r[rt].UL[i]]);
			xAND(ptr32[&cpuRegs.GPR.r[_Rd_].UL[i]], eax);
		} else {
			xMOV(eax, ptr32[&cpuRegs.GPR.r[rs].UL[i]]);
			if (rs != rt)
				xAND(eax, ptr32[&cpuRegs.GPR.r[rt].UL[i]]);
			xMOV(ptr32[&cpuRegs.GPR.r[_Rd_].UL[i]], eax);
		}
	}
}

EERECOMPILE_CODE0(AND, XMMINFO_READS|XMMINFO_READT|XMMINFO_WRITED);

//// OR
void recOR_const()
{
	g_cpuConstRegs[_Rd_].UD[0] = g_cpuConstRegs[_Rs_].UD[0] | g_cpuConstRegs[_Rt_].UD[0];
}

void recOR_constv(int info, int creg, int vreg)
{
	pxAssert( !(info & PROCESS_EE_XMM) );

	GPR_reg64 cval = g_cpuConstRegs[creg];

	for (int i = 0; i < 2; i++) {
		if (cval.UL[i] == -1) {
			xMOV(ptr32[&cpuRegs.GPR.r[_Rd_].UL[i]], -1);
		} else if (_Rd_ == vreg) {
			if (cval.UL[i])
				xOR(ptr32[&cpuRegs.GPR.r[_Rd_].UL[i]], cval.UL[i]);
		} else {
			xMOV(eax, ptr32[&cpuRegs.GPR.r[vreg].UL[i]]);
			if (cval.UL[i])
				xOR(eax, cval.UL[i]);
			xMOV(ptr32[&cpuRegs.GPR.r[_Rd_].UL[i]], eax);
		}
	}
}

void recOR_consts(int info)
{
	recOR_constv(info, _Rs_, _Rt_);
}

void recOR_constt(int info)
{
	recOR_constv(info, _Rt_, _Rs_);
}

void recOR_(int info)
{
	pxAssert( !(info & PROCESS_EE_XMM) );

	int rs = _Rs_, rt = _Rt_;
	if (_Rd_ == _Rt_)
		rs = _Rt_, rt = _Rs_;

	for (int i = 0; i < 2; i++) {
		if (_Rd_ == rs) {
			if (rs == rt)
				continue;
			xMOV(eax, ptr32[&cpuRegs.GPR.r[rt].UL[i]]);
			xOR(ptr32[&cpuRegs.GPR.r[_Rd_].UL[i]], eax);
		} else {
			xMOV(eax, ptr32[&cpuRegs.GPR.r[rs].UL[i]]);
			if (rs != rt)
				xOR(eax, ptr32[&cpuRegs.GPR.r[rt].UL[i]]);
			xMOV(ptr32[&cpuRegs.GPR.r[_Rd_].UL[i]], eax);
		}
	}
}

EERECOMPILE_CODE0(OR, XMMINFO_READS|XMMINFO_READT|XMMINFO_WRITED);

//// XOR
void recXOR_const()
{
	g_cpuConstRegs[_Rd_].UD[0] = g_cpuConstRegs[_Rs_].UD[0] ^ g_cpuConstRegs[_Rt_].UD[0];
}

void recXOR_constv(int info, int creg, int vreg)
{
	pxAssert( !(info & PROCESS_EE_XMM) );

	GPR_reg64 cval = g_cpuConstRegs[creg];

	for (int i = 0; i < 2; i++) {
		if (_Rd_ == vreg) {
			if (cval.UL[i])
				xXOR(ptr32[&cpuRegs.GPR.r[_Rd_].UL[i]], cval.UL[i]);
		} else {
			xMOV(eax, ptr32[&cpuRegs.GPR.r[vreg].UL[i]]);
			if (cval.UL[i])
				xXOR(eax, cval.UL[i]);
			xMOV(ptr32[&cpuRegs.GPR.r[_Rd_].UL[i]], eax);
		}
	}
}

void recXOR_consts(int info)
{
	recXOR_constv(info, _Rs_, _Rt_);
}

void recXOR_constt(int info)
{
	recXOR_constv(info, _Rt_, _Rs_);
}

void recXOR_(int info)
{
	pxAssert( !(info & PROCESS_EE_XMM) );

	int rs = _Rs_, rt = _Rt_;
	if (_Rd_ == _Rt_)
		rs = _Rt_, rt = _Rs_;

	for (int i = 0; i < 2; i++) {
		if (rs == rt) {
			xMOV(ptr32[&cpuRegs.GPR.r[_Rd_]], 0);
		} else if (_Rd_ == rs) {
			xMOV(eax, ptr32[&cpuRegs.GPR.r[rt].UL[i]]);
			xXOR(ptr32[&cpuRegs.GPR.r[_Rd_].UL[i]], eax);
		} else {
			xMOV(eax, ptr32[&cpuRegs.GPR.r[rs].UL[i]]);
			xXOR(eax, ptr32[&cpuRegs.GPR.r[rt].UL[i]]);
			xMOV(ptr32[&cpuRegs.GPR.r[_Rd_].UL[i]], eax);
		}
	}
}

EERECOMPILE_CODE0(XOR, XMMINFO_READS|XMMINFO_READT|XMMINFO_WRITED);

//// NOR
void recNOR_const()
{
	g_cpuConstRegs[_Rd_].UD[0] =~(g_cpuConstRegs[_Rs_].UD[0] | g_cpuConstRegs[_Rt_].UD[0]);
}

void recNOR_constv(int info, int creg, int vreg)
{
	pxAssert( !(info & PROCESS_EE_XMM) );

	GPR_reg64 cval = g_cpuConstRegs[creg];

	for (int i = 0; i < 2; i++) {
		if (_Rd_ == vreg) {
			if (cval.UL[i])
				xOR(ptr32[&cpuRegs.GPR.r[_Rd_].UL[i]], cval.UL[i]);
			xNOT(ptr32[&cpuRegs.GPR.r[_Rd_].UL[i]]);
		} else {
			xMOV(eax, ptr32[&cpuRegs.GPR.r[vreg].UL[i]]);
			if (cval.UL[i])
				xOR(eax, cval.UL[i]);
			xNOT(eax);
			xMOV(ptr32[&cpuRegs.GPR.r[_Rd_].UL[i]], eax);
		}
	}
}

void recNOR_consts(int info)
{
	recNOR_constv(info, _Rs_, _Rt_);
}

void recNOR_constt(int info)
{
	recNOR_constv(info, _Rt_, _Rs_);
}

void recNOR_(int info)
{
	pxAssert( !(info & PROCESS_EE_XMM) );

	int rs = _Rs_, rt = _Rt_;
	if (_Rd_ == _Rt_)
		rs = _Rt_, rt = _Rs_;

	for (int i = 0; i < 2; i++) {
		if (_Rd_ == rs) {
			if (rs == rt) {
				xNOT(ptr32[&cpuRegs.GPR.r[_Rd_].UL[i]]);
				continue;
			}
			xMOV(eax, ptr32[&cpuRegs.GPR.r[rt].UL[i]]);
			xOR(ptr32[&cpuRegs.GPR.r[_Rd_].UL[i]], eax);
			xNOT(ptr32[&cpuRegs.GPR.r[_Rd_].UL[i]]);
		} else {
			xMOV(eax, ptr32[&cpuRegs.GPR.r[rs].UL[i]]);
			if (rs != rt)
				xOR(eax, ptr32[&cpuRegs.GPR.r[rt].UL[i]]);
			xNOT(eax);
			xMOV(ptr32[&cpuRegs.GPR.r[_Rd_].UL[i]], eax);
		}
	}
}

EERECOMPILE_CODE0(NOR, XMMINFO_READS|XMMINFO_READT|XMMINFO_WRITED);

//// SLT - test with silent hill, lemans
void recSLT_const()
{
	g_cpuConstRegs[_Rd_].UD[0] = g_cpuConstRegs[_Rs_].SD[0] < g_cpuConstRegs[_Rt_].SD[0];
}

void recSLTs_const(int info, int sign, int st)
{
	pxAssert( !(info & PROCESS_EE_XMM) );

	GPR_reg64 cval = g_cpuConstRegs[st ? _Rt_ : _Rs_];

	xMOV(eax, 1);

	xCMP(ptr32[&cpuRegs.GPR.r[st ? _Rs_ : _Rt_].UL[1]], cval.UL[1]);
	xForwardJump8 pass1(st ? (sign ? Jcc_Less : Jcc_Below) : (sign ? Jcc_Greater : Jcc_Above));
	xForwardJump8 fail(st ? (sign ? Jcc_Greater : Jcc_Above) : (sign ? Jcc_Less : Jcc_Below));
	{
		xCMP(ptr32[&cpuRegs.GPR.r[st ? _Rs_ : _Rt_].UL[0]], cval.UL[0]);
		xForwardJump8 pass2(st ? Jcc_Below : Jcc_Above);

		fail.SetTarget();
		xMOV(eax, 0);
		pass2.SetTarget();
	}
	pass1.SetTarget();

	xMOV(ptr32[&cpuRegs.GPR.r[_Rd_].UL[0]], eax);
	xMOV(ptr32[&cpuRegs.GPR.r[_Rd_].UL[1]], 0);
}

void recSLTs_(int info, int sign)
{
	pxAssert( !(info & PROCESS_EE_XMM) );

	xMOV(eax, 1);

	xMOV(edx, ptr32[&cpuRegs.GPR.r[_Rs_].UL[1]]);
	xCMP(edx, ptr32[&cpuRegs.GPR.r[_Rt_].UL[1]]);
	xForwardJump8 pass1(sign ? Jcc_Less : Jcc_Below);
	xForwardJump8 fail(sign ? Jcc_Greater : Jcc_Above);
	{
		xMOV(edx, ptr32[&cpuRegs.GPR.r[_Rs_].UL[0]]);
		xCMP(edx, ptr32[&cpuRegs.GPR.r[_Rt_].UL[0]]);
		xForwardJump8 pass2(Jcc_Below);

		fail.SetTarget();
		xMOV(eax, 0);
		pass2.SetTarget();
	}
	pass1.SetTarget();

	xMOV(ptr32[&cpuRegs.GPR.r[_Rd_].UL[0]], eax);
	xMOV(ptr32[&cpuRegs.GPR.r[_Rd_].UL[1]], 0);
}

void recSLT_consts(int info)
{
	recSLTs_const(info, 1, 0);
}

void recSLT_constt(int info)
{
	recSLTs_const(info, 1, 1);
}

void recSLT_(int info)
{
	recSLTs_(info, 1);
}

EERECOMPILE_CODE0(SLT, XMMINFO_READS|XMMINFO_READT|XMMINFO_WRITED);

// SLTU - test with silent hill, lemans
void recSLTU_const()
{
	g_cpuConstRegs[_Rd_].UD[0] = g_cpuConstRegs[_Rs_].UD[0] < g_cpuConstRegs[_Rt_].UD[0];
}

void recSLTU_consts(int info)
{
	recSLTs_const(info, 0, 0);
}

void recSLTU_constt(int info)
{
	recSLTs_const(info, 0, 1);
}

void recSLTU_(int info)
{
	recSLTs_(info, 0);
}

EERECOMPILE_CODE0(SLTU, XMMINFO_READS|XMMINFO_READT|XMMINFO_WRITED);

#endif

} } }
