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

#include "Common.h"
#include "R5900OpcodeTables.h"
#include "iR5900.h"


#ifdef _WIN32
//#pragma warning(disable:4244)
//#pragma warning(disable:4761)
#endif

namespace R5900 { 
namespace Dynarec { 
namespace OpcodeImpl
{

/*********************************************************
* Shift arithmetic with constant shift                   *
* Format:  OP rd, rt, sa                                 *
*********************************************************/
#ifndef MOVE_RECOMPILE

namespace Interp = R5900::Interpreter::OpcodeImpl;

REC_FUNC_DEL(LUI,_Rt_);
REC_FUNC_DEL(MFLO, _Rd_);
REC_FUNC_DEL(MFHI, _Rd_);
REC_FUNC(MTLO);
REC_FUNC(MTHI);

REC_FUNC_DEL(MFLO1, _Rd_);
REC_FUNC_DEL(MFHI1, _Rd_);
REC_FUNC( MTHI1 );
REC_FUNC( MTLO1 );

REC_FUNC_DEL(MOVZ, _Rd_);
REC_FUNC_DEL(MOVN, _Rd_);

#elif defined(EE_CONST_PROP)

/*********************************************************
* Load higher 16 bits of the first word in GPR with imm  *
* Format:  OP rt, immediate                              *
*********************************************************/

//// LUI
void recLUI()
{
	int mmreg;
	if(!_Rt_) return;

	_eeOnWriteReg(_Rt_, 1);

	if( (mmreg = _checkXMMreg(XMMTYPE_GPRREG, _Rt_, MODE_WRITE)) >= 0 ) {
		if( xmmregs[mmreg].mode & MODE_WRITE ) {
			SSE_MOVHPS_XMM_to_M64((u32)&cpuRegs.GPR.r[_Rt_].UL[2], mmreg);
		}
		xmmregs[mmreg].inuse = 0;
	}

	_deleteEEreg(_Rt_, 0);
	GPR_SET_CONST(_Rt_);
	g_cpuConstRegs[_Rt_].UD[0] = (s32)(cpuRegs.code << 16);
}

////////////////////////////////////////////////////
void recMFHILO(int hi)
{
	int reghi, regd, xmmhilo;
	if ( ! _Rd_ )
		return;

	xmmhilo = hi ? XMMGPR_HI : XMMGPR_LO;
	reghi = _checkXMMreg(XMMTYPE_GPRREG, xmmhilo, MODE_READ);

	_eeOnWriteReg(_Rd_, 0);

	regd = _checkXMMreg(XMMTYPE_GPRREG, _Rd_, MODE_READ|MODE_WRITE);

	if( reghi >= 0 ) {
		if( regd >= 0 ) {
			assert( regd != reghi );

			xmmregs[regd].inuse = 0;

			SSE2_MOVQ_XMM_to_M64((u32)&cpuRegs.GPR.r[_Rd_].UL[0], reghi);

			if( xmmregs[regd].mode & MODE_WRITE ) {
				SSE_MOVHPS_XMM_to_M64((u32)&cpuRegs.GPR.r[_Rd_].UL[2], regd);
			}
		}
		else {
			regd = _allocCheckGPRtoMMX(g_pCurInstInfo, _Rd_, MODE_WRITE);

			if( regd >= 0 ) {
				SSE2_MOVDQ2Q_XMM_to_MM(regd, reghi);
			}
			else {
				_deleteEEreg(_Rd_, 0);
				SSE2_MOVQ_XMM_to_M64((int)&cpuRegs.GPR.r[ _Rd_ ].UD[ 0 ], reghi);
			}
		}
	}
	else {
		reghi = _checkMMXreg(MMX_GPR+xmmhilo, MODE_READ);

		if( reghi >= 0 ) {

			if( regd >= 0 ) {
				if( EEINST_ISLIVE2(_Rd_) ) {
					if( xmmregs[regd].mode & MODE_WRITE ) {
						SSE_MOVHPS_XMM_to_M64((int)&cpuRegs.GPR.r[ _Rd_ ].UL[ 2 ], regd);
					}
					xmmregs[regd].inuse = 0;
					MOVQRtoM((int)&cpuRegs.GPR.r[ _Rd_ ].UD[ 0 ], reghi);
				}
				else {
					SetMMXstate();
					SSE2_MOVQ2DQ_MM_to_XMM(regd, reghi);
					xmmregs[regd].mode |= MODE_WRITE;
				}
			}
			else {
				regd = _allocCheckGPRtoMMX(g_pCurInstInfo, _Rd_, MODE_WRITE);
				SetMMXstate();

				if( regd >= 0 ) {
					MOVQRtoR(regd, reghi);
				}
				else {
					_deleteEEreg(_Rd_, 0);
					MOVQRtoM((int)&cpuRegs.GPR.r[ _Rd_ ].UD[ 0 ], reghi);
				}
			}
		}
		else {
			if( regd >= 0 ) {
				if( EEINST_ISLIVE2(_Rd_) ) SSE_MOVLPS_M64_to_XMM(regd, hi ? (int)&cpuRegs.HI.UD[ 0 ] : (int)&cpuRegs.LO.UD[ 0 ]);
				else SSE2_MOVQ_M64_to_XMM(regd, hi ? (int)&cpuRegs.HI.UD[ 0 ] : (int)&cpuRegs.LO.UD[ 0 ]);
			}
			else {
				regd = _allocCheckGPRtoMMX(g_pCurInstInfo, _Rd_, MODE_WRITE);

				if( regd >= 0 ) {
					SetMMXstate();
					MOVQMtoR(regd, hi ? (int)&cpuRegs.HI.UD[ 0 ] : (int)&cpuRegs.LO.UD[ 0 ]);
				}
				else {
					_deleteEEreg(_Rd_, 0);
					MOV32MtoR( EAX, hi ? (int)&cpuRegs.HI.UL[ 0 ] : (int)&cpuRegs.LO.UL[ 0 ]);
					if( EEINST_ISLIVE1(_Rd_) )
						MOV32MtoR( EDX, hi ? (int)&cpuRegs.HI.UL[ 1 ] : (int)&cpuRegs.LO.UL[ 1 ]);
					MOV32RtoM( (int)&cpuRegs.GPR.r[ _Rd_ ].UL[ 0 ], EAX );
					if( EEINST_ISLIVE1(_Rd_) )
						MOV32RtoM( (int)&cpuRegs.GPR.r[ _Rd_ ].UL[ 1 ], EDX );
					else
						EEINST_RESETHASLIVE1(_Rd_);
				}
			}
		}
	}
}

void recMTHILO(int hi)
{
	int reghi, regs, xmmhilo;
	u32 addrhilo;

	xmmhilo = hi ? XMMGPR_HI : XMMGPR_LO;
	addrhilo = hi ? (int)&cpuRegs.HI.UD[0] : (int)&cpuRegs.LO.UD[0];

	regs = _checkXMMreg(XMMTYPE_GPRREG, _Rs_, MODE_READ);
	reghi = _checkXMMreg(XMMTYPE_GPRREG, xmmhilo, MODE_READ|MODE_WRITE);

	if( reghi >= 0 ) {
		if( regs >= 0 ) {
			assert( reghi != regs );

			_deleteGPRtoXMMreg(_Rs_, 0);
			SSE2_PUNPCKHQDQ_XMM_to_XMM(reghi, reghi);
			SSE2_PUNPCKLQDQ_XMM_to_XMM(regs, reghi);

			// swap regs
			xmmregs[regs] = xmmregs[reghi];
			xmmregs[reghi].inuse = 0;
			xmmregs[regs].mode |= MODE_WRITE;
			
		}
		else {
			regs = _checkMMXreg(MMX_GPR+_Rs_, MODE_READ);

			if( regs >= 0 ) {

				if( EEINST_ISLIVE2(xmmhilo) ) {
					if( xmmregs[reghi].mode & MODE_WRITE ) {
						SSE_MOVHPS_XMM_to_M64(addrhilo+8, reghi);
					}
					xmmregs[reghi].inuse = 0;
					MOVQRtoM(addrhilo, regs);
				}
				else {
					SetMMXstate();
					SSE2_MOVQ2DQ_MM_to_XMM(reghi, regs);
					xmmregs[reghi].mode |= MODE_WRITE;
				}
			}
			else {
				_flushConstReg(_Rs_);
				SSE_MOVLPS_M64_to_XMM(reghi, (int)&cpuRegs.GPR.r[ _Rs_ ].UD[ 0 ]);
				xmmregs[reghi].mode |= MODE_WRITE;
			}
		}
	}
	else {
		reghi = _allocCheckGPRtoMMX(g_pCurInstInfo, xmmhilo, MODE_WRITE);

		if( reghi >= 0 ) {
			if( regs >= 0 ) {
				//SetMMXstate();
				SSE2_MOVDQ2Q_XMM_to_MM(reghi, regs);
			}
			else {
				regs = _checkMMXreg(MMX_GPR+_Rs_, MODE_WRITE);

				if( regs >= 0 ) {
					SetMMXstate();
					MOVQRtoR(reghi, regs);
				}
				else {
					_flushConstReg(_Rs_);
					MOVQMtoR(reghi, (int)&cpuRegs.GPR.r[ _Rs_ ].UD[ 0 ]);
				}
			}
		}
		else {
			if( regs >= 0 ) {
				SSE2_MOVQ_XMM_to_M64(addrhilo, regs);
			}
			else {
				regs = _checkMMXreg(MMX_GPR+_Rs_, MODE_WRITE);

				if( regs >= 0 ) {
					SetMMXstate();
					MOVQRtoM(addrhilo, regs);
				}
				else {
					if( GPR_IS_CONST1(_Rs_) ) {
						MOV32ItoM(addrhilo, g_cpuConstRegs[_Rs_].UL[0] );
						MOV32ItoM(addrhilo+4, g_cpuConstRegs[_Rs_].UL[1] );
					}
					else {
						_deleteEEreg(_Rs_, 1);
						MOV32MtoR( EAX, (int)&cpuRegs.GPR.r[ _Rs_ ].UL[ 0 ]);
						MOV32MtoR( EDX, (int)&cpuRegs.GPR.r[ _Rs_ ].UL[ 1 ]);
						MOV32RtoM( addrhilo, EAX );
						MOV32RtoM( addrhilo+4, EDX );
					}
				}
			}
		}
	}
}

void recMFHI( void ) 
{
	recMFHILO(1);
}

void recMFLO( void ) 
{
	recMFHILO(0);
}

void recMTHI( void ) 
{
	recMTHILO(1);
}

void recMTLO( void )
{
	recMTHILO(0);
}

////////////////////////////////////////////////////
void recMFHILO1(int hi)
{
	int reghi, regd, xmmhilo;
	if ( ! _Rd_ )
		return;

	xmmhilo = hi ? XMMGPR_HI : XMMGPR_LO;
	reghi = _checkXMMreg(XMMTYPE_GPRREG, xmmhilo, MODE_READ);

	_eeOnWriteReg(_Rd_, 0);

	regd = _checkXMMreg(XMMTYPE_GPRREG, _Rd_, MODE_READ|MODE_WRITE);

	if( reghi >= 0 ) {
		if( regd >= 0 ) {
			SSE_MOVHLPS_XMM_to_XMM(regd, reghi);
			xmmregs[regd].mode |= MODE_WRITE;
		}
		else {
			_deleteEEreg(_Rd_, 0);
			SSE_MOVHPS_XMM_to_M64((int)&cpuRegs.GPR.r[ _Rd_ ].UD[ 0 ], reghi);
		}
	}
	else {
		if( regd >= 0 ) {
			if( EEINST_ISLIVE2(_Rd_) ) {
				SSE2_PUNPCKHQDQ_M128_to_XMM(regd, hi ? (int)&cpuRegs.HI.UD[ 0 ] : (int)&cpuRegs.LO.UD[ 0 ]);
				SSE2_PSHUFD_XMM_to_XMM(regd, regd, 0x4e);
			}
			else {
				SSE2_MOVQ_M64_to_XMM(regd, hi ? (int)&cpuRegs.HI.UD[ 1 ] : (int)&cpuRegs.LO.UD[ 1 ]);
			}

			xmmregs[regd].mode |= MODE_WRITE;
		}
		else {
			regd = _allocCheckGPRtoMMX(g_pCurInstInfo, _Rd_, MODE_WRITE);

			if( regd >= 0 ) {
				SetMMXstate();
				MOVQMtoR(regd, hi ? (int)&cpuRegs.HI.UD[ 1 ] : (int)&cpuRegs.LO.UD[ 1 ]);
			}
			else {
				_deleteEEreg(_Rd_, 0);
				MOV32MtoR( EAX, hi ? (int)&cpuRegs.HI.UL[ 2 ] : (int)&cpuRegs.LO.UL[ 2 ]);
				MOV32MtoR( EDX, hi ? (int)&cpuRegs.HI.UL[ 3 ] : (int)&cpuRegs.LO.UL[ 3 ]);
				MOV32RtoM( (int)&cpuRegs.GPR.r[ _Rd_ ].UL[ 0 ], EAX );
				MOV32RtoM( (int)&cpuRegs.GPR.r[ _Rd_ ].UL[ 1 ], EDX );
			}
		}
	}
}

void recMTHILO1(int hi)
{
	int reghi, regs, xmmhilo;
	u32 addrhilo;

	xmmhilo = hi ? XMMGPR_HI : XMMGPR_LO;
	addrhilo = hi ? (int)&cpuRegs.HI.UD[0] : (int)&cpuRegs.LO.UD[0];

	regs = _checkXMMreg(XMMTYPE_GPRREG, _Rs_, MODE_READ);
	reghi = _allocCheckGPRtoXMM(g_pCurInstInfo, xmmhilo, MODE_WRITE|MODE_READ);

	if( reghi >= 0 ) {
		if( regs >= 0 ) {
			SSE2_PUNPCKLQDQ_XMM_to_XMM(reghi, regs);	
		}
		else {
			_deleteEEreg(_Rs_, 1);
			SSE2_PUNPCKLQDQ_M128_to_XMM(reghi, (int)&cpuRegs.GPR.r[ _Rs_ ].UD[ 0 ]);
		}
	}
	else {
		if( regs >= 0 ) {
			SSE2_MOVQ_XMM_to_M64(addrhilo+8, regs);
		}
		else {
			regs = _checkMMXreg(MMX_GPR+_Rs_, MODE_WRITE);

			if( regs >= 0 ) {
				SetMMXstate();
				MOVQRtoM(addrhilo+8, regs);
			}
			else {
				if( GPR_IS_CONST1(_Rs_) ) {
					MOV32ItoM(addrhilo+8, g_cpuConstRegs[_Rs_].UL[0] );
					MOV32ItoM(addrhilo+12, g_cpuConstRegs[_Rs_].UL[1] );
				}
				else {
					_deleteEEreg(_Rs_, 1);
					MOV32MtoR( EAX, (int)&cpuRegs.GPR.r[ _Rs_ ].UL[ 0 ]);
					MOV32MtoR( EDX, (int)&cpuRegs.GPR.r[ _Rs_ ].UL[ 1 ]);
					MOV32RtoM( addrhilo+8, EAX );
					MOV32RtoM( addrhilo+12, EDX );
				}
			}
		}
	}
}

void recMFHI1( void )
{
	recMFHILO1(1);
}

void recMFLO1( void )
{
	recMFHILO1(0);
}

void recMTHI1( void )
{
	recMTHILO1(1);
}

void recMTLO1( void )
{
	recMTHILO1(0);
}

//// MOVZ
void recMOVZtemp_const()
{
	g_cpuConstRegs[_Rd_].UD[0] = g_cpuConstRegs[_Rs_].UD[0];
}

//static PCSX2_ALIGNED16(u32 s_zero[4]) = {0,0,0xffffffff, 0xffffffff};

void recMOVZtemp_consts(int info)
{
	if( info & PROCESS_EE_MMX ) {

		u32* mem;
		int t0reg = _allocMMXreg(-1, MMX_TEMP, 0);
		PXORRtoR(t0reg, t0reg);
		PCMPEQDRtoR(t0reg, EEREC_T);
		PMOVMSKBMMXtoR(EAX, t0reg);
		CMP8ItoR(EAX, 0xff);
		j8Ptr[ 0 ] = JNE8( 0 );

		if( g_cpuFlushedConstReg & (1<<_Rs_) )
			mem = &cpuRegs.GPR.r[_Rs_].UL[0];
		else
			mem = recGetImm64(g_cpuConstRegs[_Rs_].UL[1], g_cpuConstRegs[_Rs_].UL[0]);

		MOVQMtoR(EEREC_D, (uptr)mem);
		x86SetJ8( j8Ptr[ 0 ] ); 

		_freeMMXreg(t0reg);
		return;
	}

	MOV32MtoR( EAX, (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 0 ] );
	OR32MtoR( EAX, (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 1 ] );
	j8Ptr[ 0 ] = JNZ8( 0 );

	MOV32ItoM( (int)&cpuRegs.GPR.r[ _Rd_ ].UL[ 0 ], g_cpuConstRegs[_Rs_].UL[0] );
	MOV32ItoM( (int)&cpuRegs.GPR.r[ _Rd_ ].UL[ 1 ], g_cpuConstRegs[_Rs_].UL[1] );

	x86SetJ8( j8Ptr[ 0 ] ); 
}

void recMOVZtemp_constt(int info)
{
	if( info & PROCESS_EE_MMX ) {
		if( EEREC_D != EEREC_S ) MOVQRtoR(EEREC_D, EEREC_S);
		return;
	}
	
	if( _hasFreeXMMreg() ) {
		int t0reg = _allocMMXreg(-1, MMX_TEMP, 0);
		MOVQMtoR(t0reg, (int)&cpuRegs.GPR.r[ _Rs_ ].UL[ 0 ]);
		MOVQRtoM((int)&cpuRegs.GPR.r[ _Rd_ ].UL[ 0 ], t0reg);
		_freeMMXreg(t0reg);
	}
	else {
		MOV32MtoR(EAX, (int)&cpuRegs.GPR.r[ _Rs_ ].UL[ 0 ]);
		MOV32MtoR(EDX, (int)&cpuRegs.GPR.r[ _Rs_ ].UL[ 1 ]);
		MOV32RtoM((int)&cpuRegs.GPR.r[ _Rd_ ].UL[ 0 ], EAX);
		MOV32RtoM((int)&cpuRegs.GPR.r[ _Rd_ ].UL[ 1 ], EDX);
	}
}

void recMOVZtemp_(int info)
{
	int t0reg = -1;

	if( info & PROCESS_EE_MMX ) {

		t0reg = _allocMMXreg(-1, MMX_TEMP, 0);
		PXORRtoR(t0reg, t0reg);
		PCMPEQDRtoR(t0reg, EEREC_T);
		PMOVMSKBMMXtoR(EAX, t0reg);
		CMP8ItoR(EAX, 0xff);
		j8Ptr[ 0 ] = JNE8( 0 );

		MOVQRtoR(EEREC_D, EEREC_S);
		x86SetJ8( j8Ptr[ 0 ] );

		_freeMMXreg(t0reg);
		return;
	}

	if( _hasFreeXMMreg() )
		t0reg = _allocMMXreg(-1, MMX_TEMP, 0);

	MOV32MtoR( EAX, (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 0 ] );
	OR32MtoR( EAX, (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 1 ] );
	j8Ptr[ 0 ] = JNZ8( 0 );

	if( t0reg >= 0 ) {	
		MOVQMtoR(t0reg, (int)&cpuRegs.GPR.r[ _Rs_ ].UL[ 0 ]);
		MOVQRtoM((int)&cpuRegs.GPR.r[ _Rd_ ].UL[ 0 ], t0reg);
		_freeMMXreg(t0reg);
	}
	else {
		MOV32MtoR(EAX, (int)&cpuRegs.GPR.r[ _Rs_ ].UL[ 0 ]);
		MOV32MtoR(EDX, (int)&cpuRegs.GPR.r[ _Rs_ ].UL[ 1 ]);
		MOV32RtoM((int)&cpuRegs.GPR.r[ _Rd_ ].UL[ 0 ], EAX);
		MOV32RtoM((int)&cpuRegs.GPR.r[ _Rd_ ].UL[ 1 ], EDX);
	}

	x86SetJ8( j8Ptr[ 0 ] );
	SetMMXstate();
}

EERECOMPILE_CODE0(MOVZtemp, XMMINFO_READS|XMMINFO_READD|XMMINFO_READD|XMMINFO_WRITED);

void recMOVZ()
{
	if( _Rs_ == _Rd_ )
		return;

	if(GPR_IS_CONST1(_Rt_)) {
		if (g_cpuConstRegs[_Rt_].UD[0] != 0)
			return;
	} else if (GPR_IS_CONST1(_Rd_))
		_deleteEEreg(_Rd_, 1);

	recMOVZtemp();
}

//// MOVN
void recMOVNtemp_const()
{
	g_cpuConstRegs[_Rd_].UD[0] = g_cpuConstRegs[_Rs_].UD[0];
}

void recMOVNtemp_consts(int info)
{
	if( info & PROCESS_EE_MMX ) {

		u32* mem;
		int t0reg = _allocMMXreg(-1, MMX_TEMP, 0);
		PXORRtoR(t0reg, t0reg);
		PCMPEQDRtoR(t0reg, EEREC_T);

		PMOVMSKBMMXtoR(EAX, t0reg);
		CMP8ItoR(EAX, 0xff);
		j8Ptr[ 0 ] = JE8( 0 );

		if( g_cpuFlushedConstReg & (1<<_Rs_) )
			mem = &cpuRegs.GPR.r[_Rs_].UL[0];
		else
			mem = recGetImm64(g_cpuConstRegs[_Rs_].UL[1], g_cpuConstRegs[_Rs_].UL[0]);

		MOVQMtoR(EEREC_D, (uptr)mem);
		x86SetJ8( j8Ptr[ 0 ] ); 

		_freeMMXreg(t0reg);
		return;
	}

	MOV32MtoR( EAX, (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 0 ] );
	OR32MtoR( EAX, (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 1 ] );
	j8Ptr[ 0 ] = JZ8( 0 );

	MOV32ItoM( (int)&cpuRegs.GPR.r[ _Rd_ ].UL[ 0 ], g_cpuConstRegs[_Rs_].UL[0] );
	MOV32ItoM( (int)&cpuRegs.GPR.r[ _Rd_ ].UL[ 1 ], g_cpuConstRegs[_Rs_].UL[1] );

	x86SetJ8( j8Ptr[ 0 ] ); 
}

void recMOVNtemp_constt(int info)
{
	if( _hasFreeXMMreg() ) {
		int t0reg = _allocMMXreg(-1, MMX_TEMP, 0);
		MOVQMtoR(t0reg, (int)&cpuRegs.GPR.r[ _Rs_ ].UL[ 0 ]);
		MOVQRtoM((int)&cpuRegs.GPR.r[ _Rd_ ].UL[ 0 ], t0reg);
		_freeMMXreg(t0reg);
	}
	else {
		MOV32MtoR(EAX, (int)&cpuRegs.GPR.r[ _Rs_ ].UL[ 0 ]);
		MOV32MtoR(EDX, (int)&cpuRegs.GPR.r[ _Rs_ ].UL[ 1 ]);
		MOV32RtoM((int)&cpuRegs.GPR.r[ _Rd_ ].UL[ 0 ], EAX);
		MOV32RtoM((int)&cpuRegs.GPR.r[ _Rd_ ].UL[ 1 ], EDX);
	}
}

void recMOVNtemp_(int info)
{
	int t0reg=-1;

	if( info & PROCESS_EE_MMX ) {

		t0reg = _allocMMXreg(-1, MMX_TEMP, 0);
		PXORRtoR(t0reg, t0reg);
		PCMPEQDRtoR(t0reg, EEREC_T);
		PMOVMSKBMMXtoR(EAX, t0reg);
		CMP8ItoR(EAX, 0xff);
		j8Ptr[ 0 ] = JE8( 0 );

		MOVQRtoR(EEREC_D, EEREC_S);
		x86SetJ8( j8Ptr[ 0 ] );

		_freeMMXreg(t0reg);
		return;
	}

	if( _hasFreeXMMreg() )
		t0reg = _allocMMXreg(-1, MMX_TEMP, 0);

	MOV32MtoR( EAX, (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 0 ] );
	OR32MtoR( EAX, (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 1 ] );
	j8Ptr[ 0 ] = JZ8( 0 );

	if( t0reg >= 0 ) {	
		MOVQMtoR(t0reg, (int)&cpuRegs.GPR.r[ _Rs_ ].UL[ 0 ]);
		MOVQRtoM((int)&cpuRegs.GPR.r[ _Rd_ ].UL[ 0 ], t0reg);
		_freeMMXreg(t0reg);
	}
	else {
		MOV32MtoR(EAX, (int)&cpuRegs.GPR.r[ _Rs_ ].UL[ 0 ]);
		MOV32MtoR(EDX, (int)&cpuRegs.GPR.r[ _Rs_ ].UL[ 1 ]);
		MOV32RtoM((int)&cpuRegs.GPR.r[ _Rd_ ].UL[ 0 ], EAX);
		MOV32RtoM((int)&cpuRegs.GPR.r[ _Rd_ ].UL[ 1 ], EDX);
	}

	x86SetJ8( j8Ptr[ 0 ] );

	SetMMXstate();
}

EERECOMPILE_CODE0(MOVNtemp, XMMINFO_READS|XMMINFO_READD|XMMINFO_READD|XMMINFO_WRITED);

void recMOVN()
{
	if( _Rs_ == _Rd_ )
		return;

	if (GPR_IS_CONST1(_Rt_)) {
		if (g_cpuConstRegs[_Rt_].UD[0] == 0)
			return;
	} else if (GPR_IS_CONST1(_Rd_))
		_deleteEEreg(_Rd_, 1);

	recMOVNtemp();
}

#else

////////////////////////////////////////////////////
void recLUI( void )
{
	if(!_Rt_) return;
      if ( _Imm_ < 0 )
	   {
		   MOV32ItoM( (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 0 ], (u32)_Imm_ << 16 );	//U
		   MOV32ItoM( (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 1 ], 0xffffffff );	//V
	   }
	   else
	   {
		   MOV32ItoM( (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 0 ], (u32)_Imm_ << 16 );	//U
		   MOV32ItoM( (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 1 ], 0 );			//V
	   } 
}

////////////////////////////////////////////////////
void recMFHI( void ) 
{

   if ( ! _Rd_ )
   {
      return;
   }

	   MOVQMtoR( MM0, (int)&cpuRegs.HI.UD[ 0 ] );
	   MOVQRtoM( (int)&cpuRegs.GPR.r[ _Rd_ ].UD[ 0 ], MM0 );
	   SetMMXstate();

}

////////////////////////////////////////////////////
void recMFLO( void ) 
{

   if ( ! _Rd_ ) 
   {
      return;
   }

	   MOVQMtoR( MM0, (int)&cpuRegs.LO.UD[ 0 ] );
	   MOVQRtoM( (int)&cpuRegs.GPR.r[ _Rd_ ].UD[ 0 ], MM0 );
	   SetMMXstate();
 
}

////////////////////////////////////////////////////
void recMTHI( void ) 
{

	   MOVQMtoR( MM0, (int)&cpuRegs.GPR.r[ _Rs_ ].UD[ 0 ] );
	   MOVQRtoM( (int)&cpuRegs.HI.UD[ 0 ], MM0 );
	   SetMMXstate();
 
}

////////////////////////////////////////////////////
void recMTLO( void )
{

	   MOVQMtoR( MM0, (int)&cpuRegs.GPR.r[ _Rs_ ].UD[ 0 ] );
	   MOVQRtoM( (int)&cpuRegs.LO.UD[ 0 ], MM0 );
	   SetMMXstate();
 
}

////////////////////////////////////////////////////
void recMOVZ( void )
{
   if ( ! _Rd_ ) 
   {
      return;
   }
	   MOV32MtoR( EAX, (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 0 ] );
	   OR32MtoR( EAX, (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 1 ] );
	   j8Ptr[ 0 ] = JNZ8( 0 );

	   MOV32MtoR( EAX, (int)&cpuRegs.GPR.r[ _Rs_ ].UL[ 0 ] );
	   MOV32MtoR( EDX, (int)&cpuRegs.GPR.r[ _Rs_ ].UL[ 1 ] );
	   MOV32RtoM( (int)&cpuRegs.GPR.r[ _Rd_ ].UL[ 0 ], EAX );
	   MOV32RtoM( (int)&cpuRegs.GPR.r[ _Rd_ ].UL[ 1 ], EDX );

	   x86SetJ8( j8Ptr[ 0 ] ); 
 
}

////////////////////////////////////////////////////
void recMOVN( void ) 
{
   if ( ! _Rd_ ) 
   {
      return;
   }

	   MOV32MtoR( EAX, (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 0 ] );
	   OR32MtoR( EAX, (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 1 ] );
	   j8Ptr[ 0 ] = JZ8( 0 );

	   MOV32MtoR( EAX, (int)&cpuRegs.GPR.r[ _Rs_ ].UL[ 0 ] );
	   MOV32MtoR( ECX, (int)&cpuRegs.GPR.r[ _Rs_ ].UL[ 1 ] );
	   MOV32RtoM( (int)&cpuRegs.GPR.r[ _Rd_ ].UL[ 0 ], EAX );
	   MOV32RtoM( (int)&cpuRegs.GPR.r[ _Rd_ ].UL[ 1 ], ECX );

	   x86SetJ8( j8Ptr[ 0 ] );

}

REC_FUNC( MFHI1 );
REC_FUNC( MFLO1 );
REC_FUNC( MTHI1 );
REC_FUNC( MTLO1 );

#endif

} } }
