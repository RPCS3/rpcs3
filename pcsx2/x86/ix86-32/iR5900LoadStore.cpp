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
#include "iR5900LoadStore.h"
#include "iR5900.h"

using namespace x86Emitter;

#define REC_STORES
#define REC_LOADS

// Implemented at the bottom of the module:
void SetFastMemory(int bSetFast);

namespace R5900 {
namespace Dynarec {
namespace OpcodeImpl {

/*********************************************************
* Load and store for GPR                                 *
* Format:  OP rt, offset(base)                           *
*********************************************************/
#ifndef LOADSTORE_RECOMPILE

namespace Interp = R5900::Interpreter::OpcodeImpl;

REC_FUNC_DEL(LB, _Rt_);
REC_FUNC_DEL(LBU, _Rt_);
REC_FUNC_DEL(LH, _Rt_);
REC_FUNC_DEL(LHU, _Rt_);
REC_FUNC_DEL(LW, _Rt_);
REC_FUNC_DEL(LWU, _Rt_);
REC_FUNC_DEL(LWL, _Rt_);
REC_FUNC_DEL(LWR, _Rt_);
REC_FUNC_DEL(LD, _Rt_);
REC_FUNC_DEL(LDR, _Rt_);
REC_FUNC_DEL(LDL, _Rt_);
REC_FUNC_DEL(LQ, _Rt_);
REC_FUNC(SB);
REC_FUNC(SH);
REC_FUNC(SW);
REC_FUNC(SWL);
REC_FUNC(SWR);
REC_FUNC(SD);
REC_FUNC(SDL);
REC_FUNC(SDR);
REC_FUNC(SQ);
REC_FUNC(LWC1);
REC_FUNC(SWC1);
REC_FUNC(LQC2);
REC_FUNC(SQC2);

#else

__aligned16 u64 retValues[2];

void _eeOnLoadWrite(int reg)
{
	int regt;

	if( !reg ) return;

	_eeOnWriteReg(reg, 1);
	regt = _checkXMMreg(XMMTYPE_GPRREG, reg, MODE_READ);

	if( regt >= 0 ) {
		if( xmmregs[regt].mode & MODE_WRITE ) {
			if( reg != _Rs_ ) {
				SSE2_PUNPCKHQDQ_XMM_to_XMM(regt, regt);
				SSE2_MOVQ_XMM_to_M64((u32)&cpuRegs.GPR.r[reg].UL[2], regt);
			}
			else SSE_MOVHPS_XMM_to_M64((u32)&cpuRegs.GPR.r[reg].UL[2], regt);
		}
		xmmregs[regt].inuse = 0;
	}
}

using namespace Interpreter::OpcodeImpl;

__aligned16 u32 dummyValue[4];

void SetFastMemory(int bSetFast)
{
	// nothing
}

//////////////////////////////////////////////////////////////////////////////////////////
//
void recLoad64( u32 bits, bool sign )
{
	jASSUME( bits == 64 || bits == 128 );

	// Load EDX with the destination.
	// 64/128 bit modes load the result directly into the cpuRegs.GPR struct.

	if (_Rt_)
		xMOV(edx, (uptr)&cpuRegs.GPR.r[_Rt_].UL[0]);
	else
		xMOV(edx, (uptr)&dummyValue[0]);

	if (GPR_IS_CONST1(_Rs_))
	{
		u32 srcadr = g_cpuConstRegs[_Rs_].UL[0] + _Imm_;
		if (bits == 128)
			srcadr &= ~0x0f;

		_eeOnLoadWrite(_Rt_);
		_deleteEEreg(_Rt_, 0);

		vtlb_DynGenRead64_Const(bits, srcadr);
	}
	else
	{
		// Load ECX with the source memory address that we're reading from.
		_eeMoveGPRtoR(ECX, _Rs_);
		if (_Imm_ != 0)
			xADD(ecx, _Imm_);
		if (bits == 128)		// force 16 byte alignment on 128 bit reads
			xAND(ecx, ~0x0F);

		_eeOnLoadWrite(_Rt_);
		_deleteEEreg(_Rt_, 0);
		iFlushCall(FLUSH_FULLVTLB);

		vtlb_DynGenRead64(bits);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////
//
void recLoad32( u32 bits, bool sign )
{
	jASSUME( bits <= 32 );

	// 8/16/32 bit modes return the loaded value in EAX.

	if (GPR_IS_CONST1(_Rs_))
	{
		u32 srcadr = g_cpuConstRegs[_Rs_].UL[0] + _Imm_;

		_eeOnLoadWrite(_Rt_);
		_deleteEEreg(_Rt_, 0);

		vtlb_DynGenRead32_Const(bits, sign, srcadr);
	}
	else
	{
		// Load ECX with the source memory address that we're reading from.
		_eeMoveGPRtoR(ECX, _Rs_);
		if (_Imm_ != 0)
			ADD32ItoR( ECX, _Imm_ );

		_eeOnLoadWrite(_Rt_);
		_deleteEEreg(_Rt_, 0);

		iFlushCall(FLUSH_FULLVTLB);
		vtlb_DynGenRead32(bits, sign);
	}

	if (_Rt_)
	{
		// EAX holds the loaded value, so sign extend as needed:
		if (sign)
			xCDQ();

		xMOV(ptr32[&cpuRegs.GPR.r[_Rt_].UL[0]], eax);
		if (sign)
			xMOV(ptr32[&cpuRegs.GPR.r[_Rt_].UL[1]], edx);
		else
			xMOV(ptr32[&cpuRegs.GPR.r[_Rt_].UL[1]], 0);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////
//

void recStore(u32 bits)
{
        // Performance note: Const prop for the store address is good, always.
        // Constprop for the value being stored is not really worthwhile (better to use register
        // allocation -- simpler code and just as fast)

        // Load EDX first with the value being written, or the address of the value
        // being written (64/128 bit modes).

        if (bits < 64)
        {
                _eeMoveGPRtoR(EDX, _Rt_);
        }
        else if (bits == 128 || bits == 64)
        {
                _flushEEreg(_Rt_);          // flush register to mem
                xMOV(edx, (uptr)&cpuRegs.GPR.r[_Rt_].UL[0]);
        }

        // Load ECX with the destination address, or issue a direct optimized write
        // if the address is a constant propagation.

        if (GPR_IS_CONST1(_Rs_))
        {
                u32 dstadr = g_cpuConstRegs[_Rs_].UL[0] + _Imm_;
                if (bits == 128)
					dstadr &= ~0x0f;

                vtlb_DynGenWrite_Const( bits, dstadr );
        }
        else
        {
                _eeMoveGPRtoR(ECX, _Rs_);
                if (_Imm_ != 0)
                        xADD(ecx, _Imm_);
                if (bits == 128)
                        xAND(ecx, ~0x0F);

                iFlushCall(FLUSH_FULLVTLB);

				vtlb_DynGenWrite(bits);
        }
}


//////////////////////////////////////////////////////////////////////////////////////////
//
void recLB( void )  { recLoad32(8,true); }
void recLBU( void ) { recLoad32(8,false); }
void recLH( void )  { recLoad32(16,true); }
void recLHU( void ) { recLoad32(16,false); }
void recLW( void )  { recLoad32(32,true); }
void recLWU( void ) { recLoad32(32,false); }
void recLD( void )  { recLoad64(64,false); }
void recLQ( void )  { recLoad64(128,false); }

void recSB( void )  { recStore(8); }
void recSH( void )  { recStore(16); }
void recSW( void )  { recStore(32); }
void recSQ( void )  { recStore(128); }
void recSD( void )  { recStore(64); }

////////////////////////////////////////////////////

void recLWL( void )
{
#ifdef REC_LOADS
	iFlushCall(FLUSH_FULLVTLB);
	_deleteEEreg(_Rt_, 1);

	_eeMoveGPRtoR(ECX, _Rs_);
	if (_Imm_ != 0)
		xADD(ecx, _Imm_);

	// edi = bit offset in word
	xMOV(edi, ecx);
	xAND(edi, 3);
	xSHL(edi, 3);

	xAND(ecx, ~3);
	vtlb_DynGenRead32(32, false);

	if (!_Rt_)
		return;

	// mask off bytes loaded
	xMOV(ecx, edi);
	xMOV(edx, 0xffffff);
	xSHR(edx, cl);
	xAND(ptr32[&cpuRegs.GPR.r[_Rt_].UL[0]], edx);

	// OR in bytes loaded
	xMOV(ecx, 24);
	xSUB(ecx, edi);
	xSHL(eax, cl);
	xOR(ptr32[&cpuRegs.GPR.r[_Rt_].UL[0]], eax);

	// eax will always have the sign bit
	xCDQ();
	xMOV(ptr32[&cpuRegs.GPR.r[_Rt_].UL[1]], edx);
#else
	iFlushCall(FLUSH_INTERPRETER);
	_deleteEEreg(_Rs_, 1);
	_deleteEEreg(_Rt_, 1);

	recCall(LWL);
#endif
}

////////////////////////////////////////////////////
void recLWR(void)
{
#ifdef REC_LOADS
	iFlushCall(FLUSH_FULLVTLB);
	_deleteEEreg(_Rt_, 1);

	_eeMoveGPRtoR(ECX, _Rs_);
	if (_Imm_ != 0)
		xADD(ecx, _Imm_);

	// edi = bit offset in word
	xMOV(edi, ecx);
	xAND(edi, 3);
	xSHL(edi, 3);

	xAND(ecx, ~3);
	vtlb_DynGenRead32(32, false);

	if (!_Rt_)
		return;

	// mask off bytes loaded
	xMOV(ecx, 24);
	xSUB(ecx, edi);
	xMOV(edx, 0xffffff00);
	xSHL(edx, cl);
	xAND(ptr32[&cpuRegs.GPR.r[_Rt_].UL[0]], edx);

	// OR in bytes loaded
	xMOV(ecx, edi);
	xSHR(eax, cl);
	xOR(ptr32[&cpuRegs.GPR.r[_Rt_].UL[0]], eax);

	xCMP(edi, 0);
	xForwardJump8 nosignextend(Jcc_NotEqual);
	// if ((addr & 3) == 0)
	xCDQ();
	xMOV(ptr32[&cpuRegs.GPR.r[_Rt_].UL[1]], edx);
	nosignextend.SetTarget();
#else
	iFlushCall(FLUSH_INTERPRETER);
	_deleteEEreg(_Rs_, 1);
	_deleteEEreg(_Rt_, 1);

	recCall(LWR);
#endif
}

////////////////////////////////////////////////////
void recSWL(void)
{
#ifdef REC_STORES
	iFlushCall(FLUSH_FULLVTLB);

	_eeMoveGPRtoR(ECX, _Rs_);
	if (_Imm_ != 0)
		xADD(ecx, _Imm_);

	// edi = bit offset in word
	xMOV(edi, ecx);
	xAND(edi, 3);
	xSHL(edi, 3);

	xAND(ecx, ~3);
	vtlb_DynGenRead32(32, false);

	// mask read -> edx
	xMOV(ecx, edi);
	xMOV(edx, 0xffffff00);
	xSHL(edx, cl);
	xAND(edx, eax);

	if (_Rt_)
	{
		// mask write and OR -> edx
		xMOV(ecx, 24);
		xSUB(ecx, edi);
		_eeMoveGPRtoR(EAX, _Rt_);
		xSHR(eax, cl);
		xOR(edx, eax);
	}

	_eeMoveGPRtoR(ECX, _Rs_);
	if (_Imm_ != 0)
		xADD(ecx, _Imm_);
	xAND(ecx, ~3);

	vtlb_DynGenWrite(32);
#else
	iFlushCall(FLUSH_INTERPRETER);
	_deleteEEreg(_Rs_, 1);
	_deleteEEreg(_Rt_, 1);
	recCall(SWL);
#endif
}

////////////////////////////////////////////////////
void recSWR(void)
{
#ifdef REC_STORES
	iFlushCall(FLUSH_FULLVTLB);

	_eeMoveGPRtoR(ECX, _Rs_);
	if (_Imm_ != 0)
		xADD(ecx, _Imm_);

	// edi = bit offset in word
	xMOV(edi, ecx);
	xAND(edi, 3);
	xSHL(edi, 3);

	xAND(ecx, ~3);
	vtlb_DynGenRead32(32, false);

	// mask read -> edx
	xMOV(ecx, 24);
	xSUB(ecx, edi);
	xMOV(edx, 0xffffff);
	xSHR(edx, cl);
	xAND(edx, eax);

	if (_Rt_)
	{
		// mask write and OR -> edx
		xMOV(ecx, edi);
		_eeMoveGPRtoR(EAX, _Rt_);
		xSHL(eax, cl);
		xOR(edx, eax);
	}

	_eeMoveGPRtoR(ECX, _Rs_);
	if (_Imm_ != 0)
		xADD(ecx, _Imm_);
	xAND(ecx, ~3);

	vtlb_DynGenWrite(32);
#else
	iFlushCall(FLUSH_INTERPRETER);
	_deleteEEreg(_Rs_, 1);
	_deleteEEreg(_Rt_, 1);
	recCall(SWR);
#endif
}

////////////////////////////////////////////////////
void recLDL( void )
{
	iFlushCall(FLUSH_INTERPRETER);
	_deleteEEreg(_Rs_, 1);
	_deleteEEreg(_Rt_, 1);
	recCall(LDL);
}

////////////////////////////////////////////////////
void recLDR( void )
{
	iFlushCall(FLUSH_INTERPRETER);
	_deleteEEreg(_Rs_, 1);
	_deleteEEreg(_Rt_, 1);
	recCall(LDR);
}

////////////////////////////////////////////////////

void recSDL( void )
{
	iFlushCall(FLUSH_INTERPRETER);
	_deleteEEreg(_Rs_, 1);
	_deleteEEreg(_Rt_, 1);
	recCall(SDL);
}

////////////////////////////////////////////////////
void recSDR( void )
{
	iFlushCall(FLUSH_INTERPRETER);
	_deleteEEreg(_Rs_, 1);
	_deleteEEreg(_Rt_, 1);
	recCall(SDR);
}

//////////////////////////////////////////////////////////////////////////////////////////
/*********************************************************
* Load and store for COP1                                *
* Format:  OP rt, offset(base)                           *
*********************************************************/

////////////////////////////////////////////////////

void recLWC1( void )
{
	_deleteFPtoXMMreg(_Rt_, 2);

	if (GPR_IS_CONST1(_Rs_))
	{
		int addr = g_cpuConstRegs[_Rs_].UL[0] + _Imm_;
		vtlb_DynGenRead32_Const(32, false, addr);
	}
	else
	{
		_eeMoveGPRtoR(ECX, _Rs_);
		if (_Imm_ != 0)
			xADD(ecx, _Imm_);

		iFlushCall(FLUSH_FULLVTLB);

		vtlb_DynGenRead32(32, false);
	}

	xMOV(ptr32[&fpuRegs.fpr[_Rt_].UL], eax);
}

////////////////////////////////////////////////////

void recSWC1( void )
{
	_deleteFPtoXMMreg(_Rt_, 1);

	xMOV(edx, ptr32[&fpuRegs.fpr[_Rt_].UL] );

	if( GPR_IS_CONST1( _Rs_ ) )
	{
		int addr = g_cpuConstRegs[_Rs_].UL[0] + _Imm_;
		vtlb_DynGenWrite_Const(32, addr);
	}
	else
	{
		_eeMoveGPRtoR(ECX, _Rs_);
		if (_Imm_ != 0)
			xADD(ecx, _Imm_);

		iFlushCall(FLUSH_FULLVTLB);

		vtlb_DynGenWrite(32);
	}
}

////////////////////////////////////////////////////

/*********************************************************
* Load and store for COP2 (VU0 unit)                     *
* Format:  OP rt, offset(base)                           *
*********************************************************/

#define _Ft_ _Rt_
#define _Fs_ _Rd_
#define _Fd_ _Sa_



void recLQC2( void )
{
	_deleteVFtoXMMreg(_Ft_, 0, 2);

	if (_Rt_)
		xMOV(edx, (uptr)&VU0.VF[_Ft_].UD[0]);
	else
		xMOV(edx, (uptr)&dummyValue[0]);

	if (GPR_IS_CONST1(_Rs_))
	{
		int addr = g_cpuConstRegs[_Rs_].UL[0] + _Imm_;

		vtlb_DynGenRead64_Const(128, addr);
	}
	else
	{
		_eeMoveGPRtoR(ECX, _Rs_);
		if (_Imm_ != 0)
			xADD(ecx, _Imm_);

		iFlushCall(FLUSH_FULLVTLB);

		vtlb_DynGenRead64(128);
	}
}

////////////////////////////////////////////////////

void recSQC2( void )
{
	_deleteVFtoXMMreg(_Ft_, 0, 1); //Want to flush it but not clear it

	xMOV(edx, (uptr)&VU0.VF[_Ft_].UD[0]);

	if (GPR_IS_CONST1(_Rs_))
	{
		int addr = g_cpuConstRegs[_Rs_].UL[0] + _Imm_;
		vtlb_DynGenWrite_Const(128, addr);
	}
	else
	{
		_eeMoveGPRtoR(ECX, _Rs_);
		if (_Imm_ != 0)
			xADD(ecx, _Imm_);

		iFlushCall(FLUSH_FULLVTLB);

		vtlb_DynGenWrite(128);
	}
}

#endif

} } }	// end namespace R5900::Dynarec::OpcodeImpl

using namespace R5900::Dynarec;
using namespace R5900::Dynarec::OpcodeImpl;

void SetFastMemory(int bSetFast) {}
