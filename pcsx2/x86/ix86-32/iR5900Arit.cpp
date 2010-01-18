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
#include "R5900OpcodeTables.h"
#include "iR5900.h"


namespace R5900 {
namespace Dynarec {
namespace OpcodeImpl
{

/*********************************************************
* Register arithmetic                                    *
* Format:  OP rd, rs, rt                                 *
*********************************************************/

// NOTE: The reason ADD/SUB/etc are so large is because they are very commonly
// used and recompiler needs to cover ALL possible usages to minimize code size (zerofrog)

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

#elif defined(EE_CONST_PROP)

//// ADD
void recADD_const()
{
	g_cpuConstRegs[_Rd_].UD[0] = g_cpuConstRegs[_Rs_].SL[0]  + g_cpuConstRegs[_Rt_].SL[0];
}

void recADD_constv(int info, int creg, int vreg)
{
	pxAssert( !(info&PROCESS_EE_XMM) );

	if(!g_cpuConstRegs[ creg ].UL[0]) {
		int mmreg = _allocMMXreg(-1, MMX_GPR+_Rd_, MODE_WRITE);
		SetMMXstate();

		if( EEINST_HASLIVE1(vreg) && EEINST_ISLIVE1(_Rd_) ) {
			MOVQMtoR(mmreg, (int)&cpuRegs.GPR.r[ vreg ].UL[ 0 ]);
		}
		else {
			if( g_cpuConstRegs[creg].UL[0] ) {
				MOVDMtoMMX(mmreg, (u32)_eeGetConstReg(creg));
				PADDDMtoR(mmreg, (u32)&cpuRegs.GPR.r[ vreg ].UL[ 0 ]);
			}
			else {
				if( EEINST_ISLIVE1(_Rd_) ) {
					_signExtendMtoMMX(mmreg, (u32)&cpuRegs.GPR.r[ vreg ].UL[ 0 ]);
				}
				else {
					MOVDMtoMMX(mmreg, (u32)&cpuRegs.GPR.r[ vreg ].UL[ 0 ]);
					EEINST_RESETHASLIVE1(_Rd_);
				}
			}
		}
	}
	else {
		if( _Rd_ == vreg ) {
			if( EEINST_ISLIVE1(_Rd_) )
			{
				// must perform the ADD unconditionally, to maintain flags status:
				ADD32ItoM((int)&cpuRegs.GPR.r[_Rd_].UL[ 0 ], g_cpuConstRegs[creg].UL[0]);
				_signExtendSFtoM( (int)&cpuRegs.GPR.r[ _Rd_ ].UL[ 1 ]);
			}
			else
			{
				if( g_cpuConstRegs[creg].UL[0] )
					ADD32ItoM((int)&cpuRegs.GPR.r[_Rd_].UL[ 0 ], g_cpuConstRegs[creg].UL[0]);
				EEINST_RESETHASLIVE1(_Rd_);
			}
		}
		else {
			MOV32MtoR( EAX, (int)&cpuRegs.GPR.r[ vreg ].UL[ 0 ] );
			if( g_cpuConstRegs[ creg ].UL[0] )
				ADD32ItoR( EAX, g_cpuConstRegs[ creg ].UL[0] );

			if( EEINST_ISLIVE1(_Rd_) ) {
				CDQ( );
				MOV32RtoM( (int)&cpuRegs.GPR.r[ _Rd_ ].UL[ 0 ], EAX );
				MOV32RtoM( (int)&cpuRegs.GPR.r[ _Rd_ ].UL[ 1 ], EDX );
			}
			else {
				EEINST_RESETHASLIVE1(_Rd_);
				MOV32RtoM( (int)&cpuRegs.GPR.r[ _Rd_ ].UL[ 0 ], EAX );
			}
		}
	}
}

// s is constant
void recADD_consts(int info)
{
	recADD_constv(info, _Rs_, _Rt_);
	EEINST_SETSIGNEXT(_Rd_);
	EEINST_SETSIGNEXT(_Rt_);
}

// t is constant
void recADD_constt(int info)
{
	recADD_constv(info, _Rt_, _Rs_);
	EEINST_SETSIGNEXT(_Rd_);
	EEINST_SETSIGNEXT(_Rs_);
}

// nothing is constant
void recADD_(int info)
{
	pxAssert( !(info&PROCESS_EE_XMM) );
	EEINST_SETSIGNEXT(_Rd_);
	EEINST_SETSIGNEXT(_Rs_);
	EEINST_SETSIGNEXT(_Rt_);

	if( _Rd_ == _Rs_ ) {
		if( _Rd_ == _Rt_ ) SHL32ItoM((int)&cpuRegs.GPR.r[ _Rd_ ].UL[ 0 ], 1); // mult by 2
		else {
			MOV32MtoR(ECX, (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 0 ]);
			ADD32RtoM((int)&cpuRegs.GPR.r[ _Rd_ ].UL[ 0 ], ECX);
		}

		if( EEINST_ISLIVE1(_Rd_) ) _signExtendSFtoM( (int)&cpuRegs.GPR.r[ _Rd_ ].UL[ 1 ]);
		else EEINST_RESETHASLIVE1(_Rd_);

		return;
	}
	else if( _Rd_ == _Rt_ ) {
		EEINST_RESETHASLIVE1(_Rd_);
		MOV32MtoR(ECX, (int)&cpuRegs.GPR.r[ _Rs_ ].UL[ 0 ]);
		ADD32RtoM((int)&cpuRegs.GPR.r[ _Rd_ ].UL[ 0 ], ECX);

		if( EEINST_ISLIVE1(_Rd_) ) _signExtendSFtoM( (int)&cpuRegs.GPR.r[ _Rd_ ].UL[ 1 ]);
		else EEINST_RESETHASLIVE1(_Rd_);
	}
	else {
		MOV32MtoR( EAX, (int)&cpuRegs.GPR.r[ _Rs_ ].UL[ 0 ] );
		if( _Rs_ != _Rt_ ) ADD32MtoR( EAX, (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 0 ] );
		else SHL32ItoR(EAX, 1); // mult by 2

		if( EEINST_ISLIVE1(_Rd_) ) {
			CDQ( );
			MOV32RtoM( (int)&cpuRegs.GPR.r[ _Rd_ ].UL[ 0 ], EAX );
			MOV32RtoM( (int)&cpuRegs.GPR.r[ _Rd_ ].UL[ 1 ], EDX );
		}
		else {
			EEINST_RESETHASLIVE1(_Rd_);
			MOV32RtoM( (int)&cpuRegs.GPR.r[ _Rd_ ].UL[ 0 ], EAX );
		}
	}
}

EERECOMPILE_CODE0(ADD, XMMINFO_WRITED|XMMINFO_READS|XMMINFO_READT);

//// ADDU
void recADDU( void ) 
{
	recADD( );
}

//// DADD
void recDADD_const( void ) 
{
	g_cpuConstRegs[_Rd_].UD[0] = g_cpuConstRegs[_Rs_].SD[0]  + g_cpuConstRegs[_Rt_].SD[0];
}

void recDADD_constv(int info, int creg, int vreg)
{
	pxAssert( !(info&PROCESS_EE_XMM) );

	if( g_cpuConstRegs[ creg ].UL[0] == 0 && g_cpuConstRegs[ creg ].UL[1] == 0 && _hasFreeMMXreg() ) {
		// copy
		int mmreg = _allocMMXreg(-1, MMX_GPR+_Rd_, MODE_WRITE);
		if( EEINST_ISLIVE1(_Rd_) ) MOVQMtoR(mmreg, (int)&cpuRegs.GPR.r[ vreg ].UL[ 0 ]);
		else {
			MOVDMtoMMX(mmreg, (int)&cpuRegs.GPR.r[ vreg ].UL[ 0 ]);
			EEINST_RESETHASLIVE1(_Rd_);
		}
	}
	else {

		if( _Rd_ == vreg ) {

			if( EEINST_ISLIVE1(_Rd_) && (g_cpuConstRegs[ creg ].UL[ 0 ] || g_cpuConstRegs[ creg ].UL[ 1 ]) ) {
				ADD32ItoM( (u32)&cpuRegs.GPR.r[_Rd_].UL[0], g_cpuConstRegs[ creg ].UL[ 0 ] );
				ADC32ItoM( (u32)&cpuRegs.GPR.r[_Rd_].UL[1], g_cpuConstRegs[ creg ].UL[ 1 ] );
			}
			else if( g_cpuConstRegs[ creg ].UL[ 0 ] ) {
				EEINST_RESETHASLIVE1(_Rd_);
				ADD32ItoM( (u32)&cpuRegs.GPR.r[_Rd_].UL[0], g_cpuConstRegs[ creg ].UL[ 0 ] );
			}

			return;
		}

		MOV32MtoR( EAX, (int)&cpuRegs.GPR.r[ vreg ].UL[ 0 ] );

		if( EEINST_ISLIVE1(_Rd_) )
			MOV32MtoR( EDX, (int)&cpuRegs.GPR.r[ vreg ].UL[ 1 ] );

		if( g_cpuConstRegs[ creg ].UL[0] || g_cpuConstRegs[ creg ].UL[1] ) {
			ADD32ItoR( EAX, g_cpuConstRegs[ creg ].UL[ 0 ] );
			if( EEINST_ISLIVE1(_Rd_) )
				ADC32ItoR( EDX, g_cpuConstRegs[ creg ].UL[ 1 ] );
		}
		MOV32RtoM( (int)&cpuRegs.GPR.r[ _Rd_ ].UL[ 0 ], EAX );

		if( EEINST_ISLIVE1(_Rd_) )
			MOV32RtoM( (int)&cpuRegs.GPR.r[ _Rd_ ].UL[ 1 ], EDX );

		if( !EEINST_ISLIVE1(_Rd_) )
			EEINST_RESETHASLIVE1(_Rd_);
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
	
	if( _Rd_ == _Rs_ || _Rd_ == _Rt_ ) {
		int vreg = _Rd_ == _Rs_ ? _Rt_ : _Rs_;
		MOV32MtoR( EAX, (int)&cpuRegs.GPR.r[ vreg ].UL[ 0 ] );
		if( EEINST_ISLIVE1(_Rd_) )
			MOV32MtoR( EDX, (int)&cpuRegs.GPR.r[ vreg ].UL[ 1 ] );
		ADD32RtoM((int)&cpuRegs.GPR.r[ _Rd_ ].UL[ 0 ], EAX);
		if( EEINST_ISLIVE1(_Rd_) )
			ADC32RtoM((int)&cpuRegs.GPR.r[ _Rd_ ].UL[ 1 ], EDX);
		else
			EEINST_RESETHASLIVE1(_Rd_);
	}
	else {
		MOV32MtoR( EAX, (int)&cpuRegs.GPR.r[ _Rs_ ].UL[ 0 ] );
		if( EEINST_ISLIVE1(_Rd_) )
			MOV32MtoR( EDX, (int)&cpuRegs.GPR.r[ _Rs_ ].UL[ 1 ] );
		ADD32MtoR( EAX, (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 0 ] );
		if( EEINST_ISLIVE1(_Rd_) )
			ADC32MtoR( EDX, (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 1 ] );
		MOV32RtoM( (int)&cpuRegs.GPR.r[ _Rd_ ].UL[ 0 ], EAX );
		if( EEINST_ISLIVE1(_Rd_) )
			MOV32RtoM( (int)&cpuRegs.GPR.r[ _Rd_ ].UL[ 1 ], EDX );

		if( !EEINST_ISLIVE1(_Rd_) )
			EEINST_RESETHASLIVE1(_Rd_);
	}
}

EERECOMPILE_CODE0(DADD, XMMINFO_WRITED|XMMINFO_READS|XMMINFO_READT);

//// DADDU
void recDADDU( void )
{
	recDADD( );
}

//// SUB

void recSUB_const() 
{
	g_cpuConstRegs[_Rd_].UD[0] = g_cpuConstRegs[_Rs_].SL[0]  - g_cpuConstRegs[_Rt_].SL[0];
}

void recSUB_consts(int info) 
{
	pxAssert( !(info&PROCESS_EE_XMM) );
	EEINST_SETSIGNEXT(_Rt_);
	EEINST_SETSIGNEXT(_Rd_);

	if( _Rd_ == _Rt_ ) {
		if( g_cpuConstRegs[ _Rs_ ].UL[ 0 ] ) SUB32ItoM((int)&cpuRegs.GPR.r[ _Rd_ ].UL[ 0 ], g_cpuConstRegs[ _Rs_ ].UL[ 0 ]);
		NEG32M((int)&cpuRegs.GPR.r[ _Rd_ ].UL[ 0 ]);

		if( EEINST_ISLIVE1(_Rd_) ) _signExtendSFtoM( (int)&cpuRegs.GPR.r[ _Rd_ ].UL[ 1 ]);
		else EEINST_RESETHASLIVE1(_Rd_);
	}
	else {
		if( g_cpuConstRegs[ _Rs_ ].UL[ 0 ] ) {
			MOV32ItoR( EAX, g_cpuConstRegs[ _Rs_ ].UL[ 0 ] );
			SUB32MtoR( EAX, (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 0 ] );
		}
		else {
			MOV32MtoR( EAX, (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 0 ] );
			NEG32R(EAX);
		}

		if( EEINST_ISLIVE1(_Rd_) ) {
			CDQ( );
			MOV32RtoM( (int)&cpuRegs.GPR.r[ _Rd_ ].UL[ 0 ], EAX );
			MOV32RtoM( (int)&cpuRegs.GPR.r[ _Rd_ ].UL[ 1 ], EDX );
		}
		else {
			EEINST_RESETHASLIVE1(_Rd_);
			MOV32RtoM( (int)&cpuRegs.GPR.r[ _Rd_ ].UL[ 0 ], EAX );
		}
	}
}

void recSUB_constt(int info) 
{
	pxAssert( !(info&PROCESS_EE_XMM) );
	EEINST_SETSIGNEXT(_Rs_);
	EEINST_SETSIGNEXT(_Rd_);

	if(!g_cpuConstRegs[_Rt_].UL[0]) {
		int mmreg = _allocMMXreg(-1, MMX_GPR+_Rd_, MODE_WRITE);
		SetMMXstate();

		if( EEINST_HASLIVE1(_Rs_) && EEINST_ISLIVE1(_Rd_) ) {
			MOVQMtoR(mmreg, (int)&cpuRegs.GPR.r[ _Rs_ ].UL[ 0 ]);
		}
		else {
			if( g_cpuConstRegs[_Rt_].UL[0] ) {
				MOVDMtoMMX(mmreg, (u32)&cpuRegs.GPR.r[ _Rs_ ].UL[ 0 ]);
				PSUBDMtoR(mmreg, (u32)_eeGetConstReg(_Rt_));
			}
			else {
				if( EEINST_ISLIVE1(_Rd_) ) {
					_signExtendMtoMMX(mmreg, (u32)&cpuRegs.GPR.r[ _Rs_ ].UL[ 0 ]);
				}
				else {
					MOVDMtoMMX(mmreg, (u32)&cpuRegs.GPR.r[ _Rs_ ].UL[ 0 ]);
					EEINST_RESETHASLIVE1(_Rd_);
				}
			}
		}
	}
	else {
		if( _Rd_ == _Rs_ ) {
			if( g_cpuConstRegs[ _Rt_ ].UL[ 0 ] ) {
				SUB32ItoM( (int)&cpuRegs.GPR.r[ _Rd_ ].UL[ 0 ], g_cpuConstRegs[ _Rt_ ].UL[ 0 ]);

				if( EEINST_ISLIVE1(_Rd_) ) _signExtendSFtoM( (int)&cpuRegs.GPR.r[ _Rd_ ].UL[ 1 ]);
				else EEINST_RESETHASLIVE1(_Rd_);
			}
		}
		else {
			MOV32MtoR( EAX, (int)&cpuRegs.GPR.r[ _Rs_ ].UL[ 0 ] );
			if( g_cpuConstRegs[ _Rt_ ].UL[ 0 ] )
				SUB32ItoR( EAX, g_cpuConstRegs[ _Rt_ ].UL[ 0 ] );

			if( EEINST_ISLIVE1(_Rd_) ) {
				CDQ( );
				MOV32RtoM( (int)&cpuRegs.GPR.r[ _Rd_ ].UL[ 0 ], EAX );
				MOV32RtoM( (int)&cpuRegs.GPR.r[ _Rd_ ].UL[ 1 ], EDX );
			}
			else {
				EEINST_RESETHASLIVE1(_Rd_);
				MOV32RtoM( (int)&cpuRegs.GPR.r[ _Rd_ ].UL[ 0 ], EAX );
			}
		}
	}
}

void recSUB_(int info) 
{
	pxAssert( !(info&PROCESS_EE_XMM) );
	EEINST_SETSIGNEXT(_Rs_);
	EEINST_SETSIGNEXT(_Rt_);
	EEINST_SETSIGNEXT(_Rd_);

	if( !EEINST_ISLIVE1(_Rd_) ) {
		if( _Rd_ == _Rs_) {
			EEINST_RESETHASLIVE1(_Rd_);
			MOV32MtoR( EAX, (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 0 ] );
			SUB32RtoM( (int)&cpuRegs.GPR.r[ _Rd_ ].UL[ 0 ], EAX );
			return;
		}
	}

	MOV32MtoR( EAX, (int)&cpuRegs.GPR.r[ _Rs_ ].UL[ 0 ] );
	SUB32MtoR( EAX, (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 0 ] );

	if( EEINST_ISLIVE1(_Rd_) ) {
		CDQ( );
		MOV32RtoM( (int)&cpuRegs.GPR.r[ _Rd_ ].UL[ 0 ], EAX );
		MOV32RtoM( (int)&cpuRegs.GPR.r[ _Rd_ ].UL[ 1 ], EDX );
	}
	else {
		EEINST_RESETHASLIVE1(_Rd_);
		MOV32RtoM( (int)&cpuRegs.GPR.r[ _Rd_ ].UL[ 0 ], EAX );
	}
}

EERECOMPILE_CODE0(SUB, XMMINFO_READS|XMMINFO_READT|XMMINFO_WRITED);

//// SUBU
void recSUBU( void ) 
{
	recSUB( );
}

//// DSUB
void recDSUB_const()
{
	g_cpuConstRegs[_Rd_].UD[0] = g_cpuConstRegs[_Rs_].SD[0]  - g_cpuConstRegs[_Rt_].SD[0];
}

void recDSUB_consts(int info) 
{
	pxAssert( !(info&PROCESS_EE_XMM) );

	if( g_cpuConstRegs[ _Rs_ ].UL[ 0 ] || g_cpuConstRegs[ _Rs_ ].UL[ 1 ] ) {
		MOV32ItoR( EAX, g_cpuConstRegs[ _Rs_ ].UL[ 0 ] );
		if( EEINST_ISLIVE1(_Rd_) )
			MOV32ItoR( EDX, g_cpuConstRegs[ _Rs_ ].UL[ 1 ] );
		SUB32MtoR( EAX, (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 0 ] );
		if( EEINST_ISLIVE1(_Rd_) )
			SBB32MtoR( EDX, (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 1 ] );

		if( !EEINST_ISLIVE1(_Rd_) )
			EEINST_RESETHASLIVE1(_Rd_);
	}
	else {

		if( _Rd_ == _Rt_ ) {
			// negate _Rt_ all in memory
			if( EEINST_ISLIVE1(_Rd_) ) {
				MOV32MtoR( EDX, (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 1 ] );
				NEG32M((int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 0 ]);
				ADC32ItoR(EDX, 0);
				NEG32R(EDX);
				MOV32RtoM((int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 1 ], EDX);
			}
			else {
				EEINST_RESETHASLIVE1(_Rd_);
				NEG32M((int)&cpuRegs.GPR.r[ _Rd_ ].UL[ 0 ]);
			}
			return;
		}

		MOV32MtoR( EAX, (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 0 ] );
		if( EEINST_ISLIVE1(_Rd_) )
			MOV32MtoR( EDX, (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 1 ] );
		// take negative of 64bit number
		NEG32R(EAX);

		if( EEINST_ISLIVE1(_Rd_) ) {
			ADC32ItoR(EDX, 0);
			NEG32R(EDX);
		}
		else {
			EEINST_RESETHASLIVE1(_Rd_);
		}
	}

	MOV32RtoM( (int)&cpuRegs.GPR.r[ _Rd_ ].UL[ 0 ], EAX );
	if( EEINST_ISLIVE1(_Rd_) )
		MOV32RtoM( (int)&cpuRegs.GPR.r[ _Rd_ ].UL[ 1 ], EDX );
	else {
		EEINST_RESETHASLIVE1(_Rd_);
	}
}

void recDSUB_constt(int info) 
{
	pxAssert( !(info&PROCESS_EE_XMM) );

	if( _Rd_ == _Rs_ ) {
		if( EEINST_ISLIVE1(_Rd_) && (g_cpuConstRegs[ _Rt_ ].UL[ 0 ] || g_cpuConstRegs[ _Rt_ ].UL[ 1 ]) ) {
			SUB32ItoM( (u32)&cpuRegs.GPR.r[_Rd_].UL[0], g_cpuConstRegs[ _Rt_ ].UL[ 0 ] );
			SBB32ItoM( (u32)&cpuRegs.GPR.r[_Rd_].UL[1], g_cpuConstRegs[ _Rt_ ].UL[ 1 ] );
		}
		else if( g_cpuConstRegs[ _Rt_ ].UL[ 0 ] ) {
			EEINST_RESETHASLIVE1(_Rd_);
			SUB32ItoM( (u32)&cpuRegs.GPR.r[_Rd_].UL[0], g_cpuConstRegs[ _Rt_ ].UL[ 0 ] );
		}
	}
	else {
		MOV32MtoR( EAX, (int)&cpuRegs.GPR.r[ _Rs_ ].UL[ 0 ] );
		if( EEINST_ISLIVE1(_Rd_) )
			MOV32MtoR( EDX, (int)&cpuRegs.GPR.r[ _Rs_ ].UL[ 1 ] );	

		if( g_cpuConstRegs[ _Rt_ ].UL[ 0 ] || g_cpuConstRegs[ _Rt_ ].UL[ 1 ] ) {		
			SUB32ItoR( EAX, g_cpuConstRegs[ _Rt_ ].UL[ 0 ] );
			if( EEINST_ISLIVE1(_Rd_) )
				SBB32ItoR( EDX, g_cpuConstRegs[ _Rt_ ].UL[ 1 ] );
		}

		MOV32RtoM( (int)&cpuRegs.GPR.r[ _Rd_ ].UL[ 0 ], EAX );
		if( EEINST_ISLIVE1(_Rd_) )
			MOV32RtoM( (int)&cpuRegs.GPR.r[ _Rd_ ].UL[ 1 ], EDX );
		else
			EEINST_RESETHASLIVE1(_Rd_);
	}
}

void recDSUB_(int info) 
{
	pxAssert( !(info&PROCESS_EE_XMM) );

	if( _Rd_ == _Rs_ ) {
		MOV32MtoR( EAX, (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 0 ] );
		if( EEINST_ISLIVE1(_Rd_) )
			MOV32MtoR( EDX, (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 1 ] );
		SUB32RtoM( (int)&cpuRegs.GPR.r[ _Rd_ ].UL[ 0 ], EAX );
		if( EEINST_ISLIVE1(_Rd_) )
			SBB32RtoM( (int)&cpuRegs.GPR.r[ _Rd_ ].UL[ 1 ], EDX );
		else
			EEINST_RESETHASLIVE1(_Rd_);
	}
	else {
		MOV32MtoR( EAX, (int)&cpuRegs.GPR.r[ _Rs_ ].UL[ 0 ] );
		if( EEINST_ISLIVE1(_Rd_) )
			MOV32MtoR( EDX, (int)&cpuRegs.GPR.r[ _Rs_ ].UL[ 1 ] );
		SUB32MtoR( EAX, (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 0 ] );
		if( EEINST_ISLIVE1(_Rd_) )
			SBB32MtoR( EDX, (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 1 ] );
		MOV32RtoM( (int)&cpuRegs.GPR.r[ _Rd_ ].UL[ 0 ], EAX );
		if( EEINST_ISLIVE1(_Rd_) )
			MOV32RtoM( (int)&cpuRegs.GPR.r[ _Rd_ ].UL[ 1 ], EDX );
		else
			EEINST_RESETHASLIVE1(_Rd_);
	}

}

EERECOMPILE_CODE0(DSUB, XMMINFO_READS|XMMINFO_READT|XMMINFO_WRITED);

//// DSUBU
void recDSUBU( void ) 
{
	recDSUB( );
}

//// AND
void recAND_const()
{
	g_cpuConstRegs[_Rd_].UD[0] = g_cpuConstRegs[_Rs_].UD[0]  & g_cpuConstRegs[_Rt_].UD[0];
}

void recAND_constv(int info, int creg, int vreg)
{
	pxAssert( !(info & PROCESS_EE_XMM) );

	if( g_cpuConstRegs[ creg ].UL[0] || g_cpuConstRegs[ creg ].UL[1] ) {

		if( _Rd_ == vreg ) {
			AND32ItoM((int)&cpuRegs.GPR.r[ vreg ].UL[0], g_cpuConstRegs[creg].UL[0]);
			if( EEINST_ISLIVE1(_Rd_) ) {
				if( g_cpuConstRegs[creg].UL[1] != 0xffffffff ) AND32ItoM((int)&cpuRegs.GPR.r[ vreg ].UL[1], g_cpuConstRegs[creg].UL[1]);
			}
			else
				EEINST_RESETHASLIVE1(_Rd_);
		}
		else {
			MOV32MtoR(EAX, (int)&cpuRegs.GPR.r[ vreg ]);
			if( EEINST_ISLIVE1(_Rd_) )
				MOV32MtoR(ECX, (int)&cpuRegs.GPR.r[ vreg ] + 4 );
			AND32ItoR(EAX, g_cpuConstRegs[ creg ].UL[0]);
			if( EEINST_ISLIVE1(_Rd_) )
				AND32ItoR(ECX, g_cpuConstRegs[ creg ].UL[1]);
			MOV32RtoM((int)&cpuRegs.GPR.r[ _Rd_ ], EAX);
			if( EEINST_ISLIVE1(_Rd_) )
				MOV32RtoM((int)&cpuRegs.GPR.r[ _Rd_ ]+4, ECX);
			else
				EEINST_RESETHASLIVE1(_Rd_);
		}
	}
	else {
		MOV32ItoM((int)&cpuRegs.GPR.r[ _Rd_ ], 0);
		if( EEINST_ISLIVE1(_Rd_) )
			MOV32ItoM((int)&cpuRegs.GPR.r[ _Rd_ ]+4, 0);
		else
			EEINST_RESETHASLIVE1(_Rd_);
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

void recLogicalOp(int info, int op)
{
	pxAssert( !(info & PROCESS_EE_XMM) );

	if( _Rd_ == _Rs_ || _Rd_ == _Rt_ ) {
		int vreg = _Rd_ == _Rs_ ? _Rt_ : _Rs_;
		MOV32MtoR(EAX, (int)&cpuRegs.GPR.r[ vreg ]);
		if( EEINST_ISLIVE1(_Rd_) )
			MOV32MtoR(EDX, (int)&cpuRegs.GPR.r[ vreg ] + 4 );
		LogicalOp32RtoM((int)&cpuRegs.GPR.r[ _Rd_ ], EAX, op );
		if( EEINST_ISLIVE1(_Rd_) )
			LogicalOp32RtoM((int)&cpuRegs.GPR.r[ _Rd_ ] + 4, EDX, op );
		if( op == 3 ) {
			NOT32M((int)&cpuRegs.GPR.r[ _Rd_ ]);
			if( EEINST_ISLIVE1(_Rd_) )
				NOT32M((int)&cpuRegs.GPR.r[ _Rd_ ]+4);
		}

		if( !EEINST_ISLIVE1(_Rd_) ) EEINST_RESETHASLIVE1(_Rd_);
	}
	else {
		MOV32MtoR(EAX, (int)&cpuRegs.GPR.r[ _Rs_ ]);
		if( EEINST_ISLIVE1(_Rd_) )
			MOV32MtoR(ECX, (int)&cpuRegs.GPR.r[ _Rs_ ] + 4 );
		LogicalOp32MtoR(EAX, (int)&cpuRegs.GPR.r[ _Rt_ ], op );
		if( EEINST_ISLIVE1(_Rd_) )
			LogicalOp32MtoR(ECX, (int)&cpuRegs.GPR.r[ _Rt_ ] + 4, op );

		if( op == 3 ) {
			NOT32R(EAX);
			if( EEINST_ISLIVE1(_Rd_) )
				NOT32R(ECX);
		}

		MOV32RtoM((int)&cpuRegs.GPR.r[ _Rd_ ], EAX);
		if( EEINST_ISLIVE1(_Rd_) )
			MOV32RtoM((int)&cpuRegs.GPR.r[ _Rd_ ]+4, ECX);
		else
			EEINST_RESETHASLIVE1(_Rd_);
	}
}

void recAND_(int info)
{
	recLogicalOp(info, 0);
}

EERECOMPILE_CODE0(AND, XMMINFO_READS|XMMINFO_READT|XMMINFO_WRITED);

//// OR
void recOR_const()
{
	g_cpuConstRegs[_Rd_].UD[0] = g_cpuConstRegs[_Rs_].UD[0]  | g_cpuConstRegs[_Rt_].UD[0];
}

void recOR_constv(int info, int creg, int vreg)
{
	pxAssert( !(info & PROCESS_EE_XMM) );

	if(_Rd_ != vreg && _hasFreeMMXreg()) {
		int rdreg = _allocMMXreg(-1, MMX_GPR+_Rd_, MODE_WRITE);
		SetMMXstate();

		MOVQMtoR(rdreg, (u32)&cpuRegs.GPR.r[vreg].UL[0] );

		if( g_cpuConstRegs[ creg ].UL[0] || g_cpuConstRegs[ creg ].UL[1] ) {
			PORMtoR(rdreg, (u32)_eeGetConstReg(creg) );
		}
	}
	else {
		if( _Rd_ == vreg ) {
			OR32ItoM((int)&cpuRegs.GPR.r[ vreg ].UL[0], g_cpuConstRegs[creg].UL[0]);
			if( EEINST_ISLIVE1(_Rd_) ) {
				if( g_cpuConstRegs[creg].UL[1] ) OR32ItoM((int)&cpuRegs.GPR.r[ vreg ].UL[1], g_cpuConstRegs[creg].UL[1]);
			}
			else
				EEINST_RESETHASLIVE1(_Rd_);
		}
		else {
			MOV32MtoR(EAX, (int)&cpuRegs.GPR.r[ vreg ]);
			if( EEINST_ISLIVE1(_Rd_) )
				MOV32MtoR(ECX, (int)&cpuRegs.GPR.r[ vreg ] + 4 );

			if( g_cpuConstRegs[ creg ].UL[0] ) OR32ItoR(EAX, g_cpuConstRegs[ creg ].UL[0]);
			if( g_cpuConstRegs[ creg ].UL[1] && EEINST_ISLIVE1(_Rd_) ) OR32ItoR(ECX, g_cpuConstRegs[ creg ].UL[1]);

			MOV32RtoM((int)&cpuRegs.GPR.r[ _Rd_ ], EAX);
			if( EEINST_ISLIVE1(_Rd_) )
				MOV32RtoM((int)&cpuRegs.GPR.r[ _Rd_ ]+4, ECX);
			else
				EEINST_RESETHASLIVE1(_Rd_);
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
	recLogicalOp(info, 1);
}

EERECOMPILE_CODE0(OR, XMMINFO_READS|XMMINFO_READT|XMMINFO_WRITED);

//// XOR
void recXOR_const()
{
	g_cpuConstRegs[_Rd_].UD[0] = g_cpuConstRegs[_Rs_].UD[0]  ^ g_cpuConstRegs[_Rt_].UD[0];
}

void recXOR_constv(int info, int creg, int vreg)
{
	pxAssert( !(info & PROCESS_EE_XMM) );

	if(_Rd_ != vreg && _hasFreeMMXreg()) {
		int rdreg = _allocMMXreg(-1, MMX_GPR+_Rd_, MODE_WRITE);
		SetMMXstate();
		MOVQMtoR(rdreg, (u32)&cpuRegs.GPR.r[vreg].UL[0] );

		if( g_cpuConstRegs[ creg ].UL[0] || g_cpuConstRegs[ creg ].UL[1] ) {
			PXORMtoR(rdreg, (u32)_eeGetConstReg(creg) );
		}
	}
	else {
		if( _Rd_ == vreg ) {
			XOR32ItoM((int)&cpuRegs.GPR.r[ vreg ].UL[0], g_cpuConstRegs[creg].UL[0]);
			if( EEINST_ISLIVE1(_Rd_) ) {
				if( g_cpuConstRegs[creg].UL[1] ) XOR32ItoM((int)&cpuRegs.GPR.r[ vreg ].UL[1], g_cpuConstRegs[creg].UL[1]);
			}
			else
				EEINST_RESETHASLIVE1(_Rd_);
		}
		else {
			MOV32MtoR(EAX, (int)&cpuRegs.GPR.r[ vreg ]);
			if( EEINST_ISLIVE1(_Rd_) )
				MOV32MtoR(ECX, (int)&cpuRegs.GPR.r[ vreg ] + 4 );

			if( g_cpuConstRegs[ creg ].UL[0] ) XOR32ItoR(EAX, g_cpuConstRegs[ creg ].UL[0]);
			if( g_cpuConstRegs[ creg ].UL[1] && EEINST_ISLIVE1(_Rd_) ) XOR32ItoR(ECX, g_cpuConstRegs[ creg ].UL[1]);

			MOV32RtoM((int)&cpuRegs.GPR.r[ _Rd_ ], EAX);
			if( EEINST_ISLIVE1(_Rd_) )
				MOV32RtoM((int)&cpuRegs.GPR.r[ _Rd_ ]+4, ECX);
			else
				EEINST_RESETHASLIVE1(_Rd_);
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
	recLogicalOp(info, 2);
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

	if(_Rd_ != vreg && _hasFreeMMXreg()) {
		int rdreg = _allocMMXreg(-1, MMX_GPR+_Rd_, MODE_WRITE);
		int t0reg = _allocMMXreg(-1, MMX_TEMP, 0);

		SetMMXstate();

		MOVQMtoR(rdreg, (u32)&cpuRegs.GPR.r[vreg].UL[0] );
		PCMPEQDRtoR( t0reg,t0reg);
		if( g_cpuConstRegs[ creg ].UD[0] ) PORMtoR(rdreg, (u32)_eeGetConstReg(creg) );

		// take the NOT			
		PXORRtoR( rdreg,t0reg);

		_freeMMXreg(t0reg);
	}
	else {
		if( _Rd_ == vreg ) {
			NOT32M((int)&cpuRegs.GPR.r[ vreg ].UL[0]);
			if( EEINST_ISLIVE1(_Rd_) ) NOT32M((int)&cpuRegs.GPR.r[ vreg ].UL[1]);

			if( g_cpuConstRegs[creg].UL[0] ) AND32ItoM((int)&cpuRegs.GPR.r[ vreg ].UL[0], ~g_cpuConstRegs[creg].UL[0]);
			if( EEINST_ISLIVE1(_Rd_) ) {
				if( g_cpuConstRegs[creg].UL[1] ) AND32ItoM((int)&cpuRegs.GPR.r[ vreg ].UL[1], ~g_cpuConstRegs[creg].UL[1]);
			}
			else
				EEINST_RESETHASLIVE1(_Rd_);
		}
		else {
			MOV32MtoR(EAX, (int)&cpuRegs.GPR.r[ vreg ]);
			if( EEINST_ISLIVE1(_Rd_) )
				MOV32MtoR(ECX, (int)&cpuRegs.GPR.r[ vreg ] + 4 );
			if( g_cpuConstRegs[ creg ].UL[0] ) OR32ItoR(EAX, g_cpuConstRegs[ creg ].UL[0]);
			if( g_cpuConstRegs[ creg ].UL[1] && EEINST_ISLIVE1(_Rd_) ) OR32ItoR(ECX, g_cpuConstRegs[ creg ].UL[1]);
			NOT32R(EAX);
			if( EEINST_ISLIVE1(_Rd_) )
				NOT32R(ECX);
			MOV32RtoM((int)&cpuRegs.GPR.r[ _Rd_ ], EAX);
			if( EEINST_ISLIVE1(_Rd_) )
				MOV32RtoM((int)&cpuRegs.GPR.r[ _Rd_ ]+4, ECX);
			else
				EEINST_RESETHASLIVE1(_Rd_);
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
	recLogicalOp(info, 3);
}

EERECOMPILE_CODE0(NOR, XMMINFO_READS|XMMINFO_READT|XMMINFO_WRITED);

//// SLT - test with silent hill, lemans
void recSLT_const()
{
	g_cpuConstRegs[_Rd_].UD[0] = g_cpuConstRegs[_Rs_].SD[0] < g_cpuConstRegs[_Rt_].SD[0];
}

static u32 s_sltconst = 0x80000000;
//static u32 s_sltconst64[2] = {0, 0x80000000};
u32 s_sltone = 1;

void recSLTs_consts(int info, int sign)
{
	pxAssert( !(info & PROCESS_EE_XMM) );
	
	XOR32RtoR(EAX, EAX);

	CMP32ItoM( (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 1 ], g_cpuConstRegs[_Rs_].UL[1]);
	if( sign ) {
		j8Ptr[0] = JL8( 0 );
		j8Ptr[2] = JG8( 0 );
	}
	else {
		j8Ptr[0] = JB8( 0 );
		j8Ptr[2] = JA8( 0 );
	}

	CMP32ItoM((int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 0 ], g_cpuConstRegs[_Rs_].UL[0]);
	j8Ptr[1] = JBE8(0);
	
	x86SetJ8(j8Ptr[2]);
	MOV32ItoR(EAX, 1);

	x86SetJ8(j8Ptr[0]);
	x86SetJ8(j8Ptr[1]);

	MOV32RtoM( (int)&cpuRegs.GPR.r[ _Rd_ ].UL[ 0 ], EAX );
	if( EEINST_ISLIVE1(_Rd_) ) MOV32ItoM( (int)&cpuRegs.GPR.r[ _Rd_ ].UL[ 1 ], 0 );
	else EEINST_RESETHASLIVE1(_Rd_);
	
}

// SLT with one operand coming from mem (compares only low 32 bits)
void recSLTmemconstt(int regd, int regs, u32 mem, int sign)
{
	// just compare the lower values
	int t0reg;

	if( sign ) {
		if( regd == regs ) {
			t0reg = _allocMMXreg(-1, MMX_TEMP, 0);
			MOVQMtoR(t0reg, mem);
			PCMPGTDRtoR(t0reg, regs);

			PUNPCKLDQRtoR(t0reg, t0reg);
			PSRLQItoR(t0reg, 63);

			// swap regs
			mmxregs[t0reg] = mmxregs[regd];
			mmxregs[regd].inuse = 0;
		}
		else {
			MOVQMtoR(regd, mem);
			PCMPGTDRtoR(regd, regs);

			PUNPCKLDQRtoR(regd, regd);
			PSRLQItoR(regd, 63);
		}
	}
	else {
		t0reg = _allocMMXreg(-1, MMX_TEMP, 0);

		if( regd == regs ) {
			MOVQMtoR(t0reg, mem);
			PXORMtoR(regs, (u32)&s_sltconst);
			PCMPGTDRtoR(t0reg, regs);

			PUNPCKLDQRtoR(t0reg, t0reg);
			PSRLQItoR(t0reg, 63);

			// swap regs
			mmxregs[t0reg] = mmxregs[regd];
			mmxregs[regd].inuse = 0;
		}
		else {
			MOVQRtoR(t0reg, regs);
			MOVQMtoR(regd, mem);
			PXORMtoR(t0reg, (u32)&s_sltconst);
			PCMPGTDRtoR(regd, t0reg);

			PUNPCKLDQRtoR(regd, regd);
			PSRLQItoR(regd, 63);
			_freeMMXreg(t0reg);
		}
	}
}

void recSLTs_constt(int info, int sign)
{
	pxAssert( !(info & PROCESS_EE_XMM) );
	
	MOV32ItoR(EAX, 1);

	CMP32ItoM( (int)&cpuRegs.GPR.r[ _Rs_ ].UL[ 1 ], g_cpuConstRegs[_Rt_].UL[1]);
	if( sign ) {
		j8Ptr[0] = JL8( 0 );
		j8Ptr[2] = JG8( 0 );
	}
	else {
		j8Ptr[0] = JB8( 0 );
		j8Ptr[2] = JA8( 0 );
	}

	CMP32ItoM((int)&cpuRegs.GPR.r[ _Rs_ ].UL[ 0 ], g_cpuConstRegs[_Rt_].UL[0]);
	j8Ptr[1] = JB8(0);
	
	x86SetJ8(j8Ptr[2]);
	XOR32RtoR(EAX, EAX);

	x86SetJ8(j8Ptr[0]);
	x86SetJ8(j8Ptr[1]);

	MOV32RtoM( (int)&cpuRegs.GPR.r[ _Rd_ ].UL[ 0 ], EAX );
	if( EEINST_ISLIVE1(_Rd_) ) MOV32ItoM( (int)&cpuRegs.GPR.r[ _Rd_ ].UL[ 1 ], 0 );
	else EEINST_RESETHASLIVE1(_Rd_);
	
}

void recSLTs_(int info, int sign)
{
	MOV32ItoR(EAX, 1);

	MOV32MtoR(ECX, (int)&cpuRegs.GPR.r[ _Rs_ ].UL[ 1 ]);
	CMP32MtoR( ECX, (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 1 ]);

	if( sign ) {
		j8Ptr[0] = JL8( 0 );
		j8Ptr[2] = JG8( 0 );
	}
	else {
		j8Ptr[0] = JB8( 0 );
		j8Ptr[2] = JA8( 0 );
	}

	MOV32MtoR(ECX, (int)&cpuRegs.GPR.r[ _Rs_ ].UL[ 0 ]);
	CMP32MtoR( ECX, (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 0 ]);
	j8Ptr[1] = JB8(0);
	
	x86SetJ8(j8Ptr[2]);
	XOR32RtoR(EAX, EAX);

	x86SetJ8(j8Ptr[0]);
	x86SetJ8(j8Ptr[1]);

	MOV32RtoM( (int)&cpuRegs.GPR.r[ _Rd_ ].UL[ 0 ], EAX );
	if( EEINST_ISLIVE1(_Rd_) ) MOV32ItoM( (int)&cpuRegs.GPR.r[ _Rd_ ].UL[ 1 ], 0 );
	else EEINST_RESETHASLIVE1(_Rd_);
}

void recSLT_consts(int info)
{
	recSLTs_consts(info, 1);
}

void recSLT_constt(int info)
{
	recSLTs_constt(info, 1);
}

void recSLT_(int info)
{
	pxAssert( !(info & PROCESS_EE_XMM) );

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
	recSLTs_consts(info, 0);
	EEINST_SETSIGNEXT(_Rd_);
}

void recSLTU_constt(int info)
{
	recSLTs_constt(info, 0);
	EEINST_SETSIGNEXT(_Rd_);
}

void recSLTU_(int info)
{
	pxAssert( !(info & PROCESS_EE_XMM) );
	EEINST_SETSIGNEXT(_Rd_);

	recSLTs_(info, 0);
}

EERECOMPILE_CODE0(SLTU, XMMINFO_READS|XMMINFO_READT|XMMINFO_WRITED);

#else

////////////////////////////////////////////////////
void recADD( void ) 
{
	if ( ! _Rd_ )
	{
		return;
	}

	MOV32MtoR( EAX, (int)&cpuRegs.GPR.r[ _Rs_ ].UL[ 0 ] );
	ADD32MtoR( EAX, (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 0 ] );
	CDQ( );
	MOV32RtoM( (int)&cpuRegs.GPR.r[ _Rd_ ].UL[ 0 ], EAX );
	MOV32RtoM( (int)&cpuRegs.GPR.r[ _Rd_ ].UL[ 1 ], EDX );
}

////////////////////////////////////////////////////
void recADDU( void ) 
{
	recADD( );
}

////////////////////////////////////////////////////
void recDADD( void ) 
{
   if ( ! _Rd_ )
   {
      return;
   }
	   MOV32MtoR( EAX, (int)&cpuRegs.GPR.r[ _Rs_ ].UL[ 0 ] );
	   MOV32MtoR( EDX, (int)&cpuRegs.GPR.r[ _Rs_ ].UL[ 1 ] );
	   ADD32MtoR( EAX, (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 0 ] );
	   ADC32MtoR( EDX, (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 1 ] );
	   MOV32RtoM( (int)&cpuRegs.GPR.r[ _Rd_ ].UL[ 0 ], EAX );
	   MOV32RtoM( (int)&cpuRegs.GPR.r[ _Rd_ ].UL[ 1 ], EDX );  
}

////////////////////////////////////////////////////
void recDADDU( void )
{
	recDADD( );
}

////////////////////////////////////////////////////
void recSUB( void ) 
{
       if ( ! _Rd_ ) return;
	   MOV32MtoR( EAX, (int)&cpuRegs.GPR.r[ _Rs_ ].UL[ 0 ] );
	   SUB32MtoR( EAX, (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 0 ] );
	   CDQ( );
	   MOV32RtoM( (int)&cpuRegs.GPR.r[ _Rd_ ].UL[ 0 ], EAX );
	   MOV32RtoM( (int)&cpuRegs.GPR.r[ _Rd_ ].UL[ 1 ], EDX );
   
}

////////////////////////////////////////////////////
void recSUBU( void ) 
{
	recSUB( );
}

////////////////////////////////////////////////////
void recDSUB( void ) 
{
      if ( ! _Rd_ ) return;

	   MOV32MtoR( EAX, (int)&cpuRegs.GPR.r[ _Rs_ ].UL[ 0 ] );
	   MOV32MtoR( EDX, (int)&cpuRegs.GPR.r[ _Rs_ ].UL[ 1 ] );
	   SUB32MtoR( EAX, (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 0 ] );
	   SBB32MtoR( EDX, (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 1 ] );
	   MOV32RtoM( (int)&cpuRegs.GPR.r[ _Rd_ ].UL[ 0 ], EAX );
	   MOV32RtoM( (int)&cpuRegs.GPR.r[ _Rd_ ].UL[ 1 ], EDX );

}

////////////////////////////////////////////////////
void recDSUBU( void ) 
{
	recDSUB( );
}

////////////////////////////////////////////////////
void recAND( void )
{
   if ( ! _Rd_ )
   {
      return;
   }
	   if ( ( _Rt_ == 0 ) || ( _Rs_ == 0 ) )
	   {
		   MOV32ItoM( (int)&cpuRegs.GPR.r[ _Rd_ ].UL[ 0 ], 0 );	
		   MOV32ItoM( (int)&cpuRegs.GPR.r[ _Rd_ ].UL[ 1 ], 0 );	
	   }
      else
      {
	      MOVQMtoR( MM0, (int)&cpuRegs.GPR.r[ _Rs_ ] );
	      MOVQMtoR( MM1, (int)&cpuRegs.GPR.r[ _Rt_ ] );
	      PANDRtoR( MM0, MM1);
	      MOVQRtoM( (int)&cpuRegs.GPR.r[ _Rd_ ], MM0 );
	      SetMMXstate();
      }

}

////////////////////////////////////////////////////
void recOR( void )
{
   if ( ! _Rd_ )
   {
      return;
   }

	   if ( ( _Rs_ == 0 ) && ( _Rt_ == 0  ) )
	   {
		   MOV32ItoM( (int)&cpuRegs.GPR.r[ _Rd_ ].UL[ 0 ], 0x0 );
		   MOV32ItoM( (int)&cpuRegs.GPR.r[ _Rd_ ].UL[ 1 ], 0x0 );
	   }
      else if ( _Rs_ == 0 )
	   {
		   MOVQMtoR( MM0, (int)&cpuRegs.GPR.r[ _Rt_ ] );
		   MOVQRtoM( (int)&cpuRegs.GPR.r[ _Rd_ ], MM0 );
		   SetMMXstate();
	   } 
      else if ( _Rt_ == 0 )
	   {
		   MOVQMtoR( MM0, (int)&cpuRegs.GPR.r[ _Rs_ ] );
		   MOVQRtoM( (int)&cpuRegs.GPR.r[ _Rd_ ], MM0 );
		   SetMMXstate();
	   }
      else
      {
	      MOVQMtoR( MM0, (int)&cpuRegs.GPR.r[ _Rs_ ] );
	      MOVQMtoR( MM1, (int)&cpuRegs.GPR.r[ _Rt_ ] );
	      PORRtoR( MM0, MM1 );
	      MOVQRtoM( (int)&cpuRegs.GPR.r[ _Rd_ ], MM0 );
	      SetMMXstate();
      }
}

////////////////////////////////////////////////////
void recXOR( void ) 
{
   if ( ! _Rd_ )
   {
      return;
   }

	   if ( ( _Rs_ == 0 ) && ( _Rt_ == 0 ) )
	   {
		   MOV32ItoM( (int)&cpuRegs.GPR.r[ _Rd_ ].UL[ 0 ],0x0);
		   MOV32ItoM( (int)&cpuRegs.GPR.r[ _Rd_ ].UL[ 1 ],0x0);
		   return;
	   }

	   if ( _Rs_ == 0 )
	   {
		   MOVQMtoR( MM0, (int)&cpuRegs.GPR.r[ _Rt_ ] );
		   MOVQRtoM( (int)&cpuRegs.GPR.r[ _Rd_ ], MM0 );
		   SetMMXstate();
		   return;
	   }

	   if ( _Rt_ == 0 )
	   {
		   MOVQMtoR( MM0, (int)&cpuRegs.GPR.r[ _Rs_ ] );
		   MOVQRtoM( (int)&cpuRegs.GPR.r[ _Rd_ ], MM0 );
		   SetMMXstate();
		   return;
	   }

	   MOVQMtoR( MM0, (int)&cpuRegs.GPR.r[ _Rs_ ] );
	   MOVQMtoR( MM1, (int)&cpuRegs.GPR.r[ _Rt_ ] );
	   PXORRtoR( MM0, MM1);
	   MOVQRtoM( (int)&cpuRegs.GPR.r[ _Rd_ ], MM0 );
	   SetMMXstate();
}

////////////////////////////////////////////////////
void recNOR( void ) 
{
   if ( ! _Rd_ )
   {
      return;
   }

	   if ( ( _Rs_ == 0 ) && ( _Rt_ == 0 ) )
	   {
		   MOV32ItoM( (int)&cpuRegs.GPR.r[ _Rd_ ].UL[ 0 ],0xffffffff);
		   MOV32ItoM( (int)&cpuRegs.GPR.r[ _Rd_ ].UL[ 1 ],0xffffffff);
		   return;
	   }

	   if ( _Rs_ == 0 )
	   {
		   MOVQMtoR( MM0, (int)&cpuRegs.GPR.r[ _Rt_ ] );
		   PCMPEQDRtoR( MM1,MM1);
		   PXORRtoR( MM0,MM1);
		   MOVQRtoM( (int)&cpuRegs.GPR.r[ _Rd_ ],MM0);
		   SetMMXstate();
		   return;
	   }

	   if ( _Rt_ == 0 )
	   {
		   MOVQMtoR( MM0, (int)&cpuRegs.GPR.r[ _Rs_ ] );
		   PCMPEQDRtoR( MM1,MM1);
		   PXORRtoR( MM0,MM1);
		   MOVQRtoM( (int)&cpuRegs.GPR.r[ _Rd_ ],MM0);
		   SetMMXstate();
		   return;
	   }

	   MOVQMtoR( MM0, (int)&cpuRegs.GPR.r[ _Rs_ ] );
	   PCMPEQDRtoR( MM1,MM1);
	   PORMtoR( MM0,(int)&cpuRegs.GPR.r[ _Rt_ ] );
	   PXORRtoR( MM0,MM1);
	   MOVQRtoM( (int)&cpuRegs.GPR.r[ _Rd_ ],MM0);
	   SetMMXstate();
}

////////////////////////////////////////////////////
// test with silent hill, lemans
void recSLT( void )
{
	if ( ! _Rd_ )
		return;
	
	MOV32ItoR(EAX, 1);

	if( _Rs_ == 0 ) {
		CMP32ItoM( (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 1 ], 0);
		j8Ptr[0] = JG8( 0 );
		j8Ptr[2] = JL8( 0 );

		CMP32ItoM( (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 0 ], 0 );
		j8Ptr[1] = JA8(0);
		
		x86SetJ8(j8Ptr[2]);
		XOR32RtoR(EAX, EAX);
	}
	else if( _Rt_ == 0 ) {
		CMP32ItoM( (int)&cpuRegs.GPR.r[ _Rs_ ].UL[ 1 ], 0);
		j8Ptr[0] = JL8( 0 );
		j8Ptr[2] = JG8( 0 );

		CMP32ItoM( (int)&cpuRegs.GPR.r[ _Rs_ ].UL[ 0 ], 0);
		j8Ptr[1] = JB8(0);

		x86SetJ8(j8Ptr[2]);
		XOR32RtoR(EAX, EAX);
	}
	else {
		MOV32MtoR(ECX, (int)&cpuRegs.GPR.r[ _Rs_ ].UL[ 1 ]);
		CMP32MtoR( ECX, (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 1 ]);
		j8Ptr[0] = JL8( 0 );
		j8Ptr[2] = JG8( 0 );

		MOV32MtoR(ECX, (int)&cpuRegs.GPR.r[ _Rs_ ].UL[ 0 ]);
		CMP32MtoR( ECX, (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 0 ]);
		j8Ptr[1] = JB8(0);
		
		x86SetJ8(j8Ptr[2]);
		XOR32RtoR(EAX, EAX);
	}

	x86SetJ8(j8Ptr[0]);
	x86SetJ8(j8Ptr[1]);
	MOV32RtoM( (int)&cpuRegs.GPR.r[ _Rd_ ].UL[ 0 ], EAX );
	MOV32ItoM( (int)&cpuRegs.GPR.r[ _Rd_ ].UL[ 1 ], 0 );
}

////////////////////////////////////////////////////
void recSLTU( void )
{
	MOV32ItoR(EAX, 1);

	if( _Rs_ == 0 ) {
		CMP32ItoM( (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 1 ], 0);
		j8Ptr[0] = JA8( 0 );
		j8Ptr[2] = JB8( 0 );

		CMP32ItoM( (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 0 ], 0 );
		j8Ptr[1] = JA8(0);
		
		x86SetJ8(j8Ptr[2]);
		XOR32RtoR(EAX, EAX);
	}
	else if( _Rt_ == 0 ) {
		CMP32ItoM( (int)&cpuRegs.GPR.r[ _Rs_ ].UL[ 1 ], 0);
		j8Ptr[0] = JB8( 0 );
		j8Ptr[2] = JA8( 0 );

		CMP32ItoM( (int)&cpuRegs.GPR.r[ _Rs_ ].UL[ 0 ], 0);
		j8Ptr[1] = JB8(0);
		
		x86SetJ8(j8Ptr[2]);
		XOR32RtoR(EAX, EAX);
	}
	else {
		MOV32MtoR(ECX, (int)&cpuRegs.GPR.r[ _Rs_ ].UL[ 1 ]);
		CMP32MtoR( ECX, (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 1 ]);
		j8Ptr[0] = JB8( 0 );
		j8Ptr[2] = JA8( 0 );

		MOV32MtoR(ECX, (int)&cpuRegs.GPR.r[ _Rs_ ].UL[ 0 ]);
		CMP32MtoR( ECX, (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 0 ]);
		j8Ptr[1] = JB8(0);
		
		x86SetJ8(j8Ptr[2]);
		XOR32RtoR(EAX, EAX);
	}

	x86SetJ8(j8Ptr[0]);
	x86SetJ8(j8Ptr[1]);
	MOV32RtoM( (int)&cpuRegs.GPR.r[ _Rd_ ].UL[ 0 ], EAX );
	MOV32ItoM( (int)&cpuRegs.GPR.r[ _Rd_ ].UL[ 1 ], 0 );
}

#endif

} } }
