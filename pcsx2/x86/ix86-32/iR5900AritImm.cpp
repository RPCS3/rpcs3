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

namespace R5900 {
namespace Dynarec {
namespace OpcodeImpl
{

/*********************************************************
* Arithmetic with immediate operand                      *
* Format:  OP rt, rs, immediate                          *
*********************************************************/

#ifndef ARITHMETICIMM_RECOMPILE

namespace Interp = R5900::Interpreter::OpcodeImpl;

REC_FUNC_DEL(ADDI, _Rt_);
REC_FUNC_DEL(ADDIU, _Rt_);
REC_FUNC_DEL(DADDI, _Rt_);
REC_FUNC_DEL(DADDIU, _Rt_);
REC_FUNC_DEL(ANDI, _Rt_);
REC_FUNC_DEL(ORI, _Rt_);
REC_FUNC_DEL(XORI, _Rt_);

REC_FUNC_DEL(SLTI, _Rt_);
REC_FUNC_DEL(SLTIU, _Rt_);

#else

//// ADDI
void recADDI_const( void )
{
	g_cpuConstRegs[_Rt_].SD[0] = (s64)(g_cpuConstRegs[_Rs_].SL[0] + (s32)_Imm_);
}

void recADDI_(int info)
{
	pxAssert( !(info&PROCESS_EE_XMM) );

	if ( _Rt_ == _Rs_ ) {
		// must perform the ADD unconditionally, to maintain flags status:
		ADD32ItoM((int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 0 ], _Imm_);
		_signExtendSFtoM( (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 1 ]);
	}
	else {
		MOV32MtoR( EAX, (int)&cpuRegs.GPR.r[ _Rs_ ].UL[ 0 ] );

		if ( _Imm_ != 0 ) ADD32ItoR( EAX, _Imm_ );

		CDQ( );
		MOV32RtoM( (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 0 ], EAX );
		MOV32RtoM( (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 1 ], EDX );
	}
}

EERECOMPILE_CODEX(eeRecompileCode1, ADDI);

////////////////////////////////////////////////////
void recADDIU( void )
{
	recADDI( );
}

////////////////////////////////////////////////////
void recDADDI_const( void )
{
	g_cpuConstRegs[_Rt_].SD[0] = g_cpuConstRegs[_Rs_].SD[0] + (s64)_Imm_;
}

void recDADDI_(int info)
{
	pxAssert( !(info&PROCESS_EE_XMM) );

	if( _Rt_ == _Rs_ ) {
		ADD32ItoM((int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 0 ], _Imm_);
		ADC32ItoM((int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 1 ], _Imm_<0?0xffffffff:0);
	}
	else {
		MOV32MtoR( EAX, (int)&cpuRegs.GPR.r[ _Rs_ ].UL[ 0 ] );

		MOV32MtoR( EDX, (int)&cpuRegs.GPR.r[ _Rs_ ].UL[ 1 ] );

		if ( _Imm_ != 0 )
		{
			ADD32ItoR( EAX, _Imm_ );
			ADC32ItoR( EDX, _Imm_ < 0?0xffffffff:0);
		}

		MOV32RtoM( (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 0 ], EAX );

		MOV32RtoM( (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 1 ], EDX );
	}
}

EERECOMPILE_CODEX(eeRecompileCode1, DADDI);

//// DADDIU
void recDADDIU( void )
{
	recDADDI( );
}

//// SLTIU
void recSLTIU_const()
{
	g_cpuConstRegs[_Rt_].UD[0] = g_cpuConstRegs[_Rs_].UD[0] < (u64)(_Imm_);
}

extern void recSLTmemconstt(int regd, int regs, u32 mem, int sign);
extern u32 s_sltone;

void recSLTIU_(int info)
{
	MOV32ItoR(EAX, 1);

	CMP32ItoM( (int)&cpuRegs.GPR.r[ _Rs_ ].UL[ 1 ], _Imm_ >= 0 ? 0 : 0xffffffff);
	j8Ptr[0] = JB8( 0 );
	j8Ptr[2] = JA8( 0 );

	CMP32ItoM( (int)&cpuRegs.GPR.r[ _Rs_ ].UL[ 0 ], (s32)_Imm_ );
	j8Ptr[1] = JB8(0);

	x86SetJ8(j8Ptr[2]);
	XOR32RtoR(EAX, EAX);

	x86SetJ8(j8Ptr[0]);
	x86SetJ8(j8Ptr[1]);

	MOV32RtoM( (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 0 ], EAX );
	MOV32ItoM( (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 1 ], 0 );
}

EERECOMPILE_CODEX(eeRecompileCode1, SLTIU);

//// SLTI
void recSLTI_const()
{
	g_cpuConstRegs[_Rt_].UD[0] = g_cpuConstRegs[_Rs_].SD[0] < (s64)(_Imm_);
}

void recSLTI_(int info)
{
	// test silent hill if modding
	MOV32ItoR(EAX, 1);

	CMP32ItoM( (int)&cpuRegs.GPR.r[ _Rs_ ].UL[ 1 ], _Imm_ >= 0 ? 0 : 0xffffffff);
	j8Ptr[0] = JL8( 0 );
	j8Ptr[2] = JG8( 0 );

	CMP32ItoM( (int)&cpuRegs.GPR.r[ _Rs_ ].UL[ 0 ], (s32)_Imm_ );
	j8Ptr[1] = JB8(0);

	x86SetJ8(j8Ptr[2]);
	XOR32RtoR(EAX, EAX);

	x86SetJ8(j8Ptr[0]);
	x86SetJ8(j8Ptr[1]);

	MOV32RtoM( (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 0 ], EAX );
	MOV32ItoM( (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 1 ], 0 );
}

EERECOMPILE_CODEX(eeRecompileCode1, SLTI);

//// ANDI
void recANDI_const()
{
	g_cpuConstRegs[_Rt_].UD[0] = g_cpuConstRegs[_Rs_].UD[0] & (u64)_ImmU_; // Zero-extended Immediate
}

void recLogicalOpI(int info, int op)
{
	if ( _ImmU_ != 0 )
	{
		if( _Rt_ == _Rs_ ) {
			LogicalOp32ItoM((int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 0 ], _ImmU_, op);
		}
		else {
			MOV32MtoR( EAX, (int)&cpuRegs.GPR.r[ _Rs_ ].UL[ 0 ] );
			if( op != 0 )
				MOV32MtoR( EDX, (int)&cpuRegs.GPR.r[ _Rs_ ].UL[ 1 ] );
			LogicalOp32ItoR( EAX, _ImmU_, op);
			if( op != 0 )
				MOV32RtoM( (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 1 ], EDX );
			MOV32RtoM( (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 0 ], EAX );
		}

		if( op == 0 ) {
			MOV32ItoM( (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 1 ], 0 );
		}
	}
	else
	{
		if( op == 0 ) {
			MOV32ItoM( (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 0 ], 0 );
			MOV32ItoM( (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 1 ], 0 );
		}
		else {
			if( _Rt_ != _Rs_ ) {
				MOV32MtoR(EAX, (int)&cpuRegs.GPR.r[ _Rs_ ].UL[ 0 ] );
				MOV32MtoR(EDX, (int)&cpuRegs.GPR.r[ _Rs_ ].UL[ 1 ] );
				MOV32RtoM((int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 0 ], EAX );
				MOV32RtoM((int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 1 ], EDX );
			}
		}
	}
}

void recANDI_(int info)
{
	recLogicalOpI(info, 0);
}

EERECOMPILE_CODEX(eeRecompileCode1, ANDI);

////////////////////////////////////////////////////
void recORI_const()
{
	g_cpuConstRegs[_Rt_].UD[0] = g_cpuConstRegs[_Rs_].UD[0] | (u64)_ImmU_; // Zero-extended Immediate
}

void recORI_(int info)
{
	recLogicalOpI(info, 1);
}

EERECOMPILE_CODEX(eeRecompileCode1, ORI);

////////////////////////////////////////////////////
void recXORI_const()
{
	g_cpuConstRegs[_Rt_].UD[0] = g_cpuConstRegs[_Rs_].UD[0] ^ (u64)_ImmU_; // Zero-extended Immediate
}

void recXORI_(int info)
{
	recLogicalOpI(info, 2);
}

EERECOMPILE_CODEX(eeRecompileCode1, XORI);

#endif

} } }
