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

#define REC_STORES
#define REC_LOADS
#define NEWLWC1
#define NEWSWC
#define NEWLQC
#define NEWSQC

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

	//no int 3? i love to get my hands dirty ;p - Raz
	//write8(0xCC);

	// Load EDX with the destination.
	// 64/128 bit modes load the result directly into the cpuRegs.GPR struct.

	if( _Rt_ )
		MOV32ItoR(EDX, (uptr)&cpuRegs.GPR.r[ _Rt_ ].UL[ 0 ] );
	else
		MOV32ItoR(EDX, (uptr)&dummyValue[0] );

	if( GPR_IS_CONST1( _Rs_ ) )
	{
		iFlushCall(FLUSH_EXCEPTION);
		_eeOnLoadWrite(_Rt_);
		_deleteEEreg(_Rt_, 0);
		u32 srcadr = g_cpuConstRegs[_Rs_].UL[0] + _Imm_;
		if( bits == 128 ) srcadr &= ~0x0f;
		vtlb_DynGenRead64_Const( bits, srcadr );
	}
	else
	{
		iFlushCall(FLUSH_EXCEPTION);
		// Load ECX with the source memory address that we're reading from.
		_eeMoveGPRtoR(ECX, _Rs_);
		if ( _Imm_ != 0 )
			ADD32ItoR( ECX, _Imm_ );
		if( bits == 128 )		// force 16 byte alignment on 128 bit reads
			AND32ItoR(ECX,~0x0F);	// emitter automatically encodes this as an 8-bit sign-extended imm8

		_eeOnLoadWrite(_Rt_);
		_deleteEEreg(_Rt_, 0);

		vtlb_DynGenRead64(bits);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////
//
void recLoad32( u32 bits, bool sign )
{
	jASSUME( bits <= 32 );

	//no int 3? i love to get my hands dirty ;p - Raz
	//write8(0xCC);

	// 8/16/32 bit modes return the loaded value in EAX.

	if( GPR_IS_CONST1( _Rs_ ) )
	{
		iFlushCall(FLUSH_EXCEPTION);
		_eeOnLoadWrite(_Rt_);
		_deleteEEreg(_Rt_, 0);

		u32 srcadr = g_cpuConstRegs[_Rs_].UL[0] + _Imm_;
		vtlb_DynGenRead32_Const( bits, sign, srcadr );
	}
	else
	{
		iFlushCall(FLUSH_EXCEPTION);
		// Load ECX with the source memory address that we're reading from.
		_eeMoveGPRtoR(ECX, _Rs_);
		if ( _Imm_ != 0 )
			ADD32ItoR( ECX, _Imm_ );

		_eeOnLoadWrite(_Rt_);
		_deleteEEreg(_Rt_, 0);
		vtlb_DynGenRead32(bits, sign);
	}

	if( _Rt_ )
	{
		// EAX holds the loaded value, so sign extend as needed:
		if (sign)
			CDQ();
		else
			XOR32RtoR(EDX,EDX);

		MOV32RtoM( (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 0 ], EAX );
		MOV32RtoM( (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 1 ], EDX );
	}
}

//////////////////////////////////////////////////////////////////////////////////////////
//

// edxAlreadyAssigned - set to true if edx already holds the value being written (used by SWL/SWR)
void recStore(u32 sz, bool edxAlreadyAssigned=false)
{
        // Performance note: Const prop for the store address is good, always.
        // Constprop for the value being stored is not really worthwhile (better to use register
        // allocation -- simpler code and just as fast)

        // Load EDX first with the value being written, or the address of the value
        // being written (64/128 bit modes).  TODO: use register allocation, if the
        // value is allocated to a register.

        if( !edxAlreadyAssigned )
        {
                if( sz < 64 )
                {
                        _eeMoveGPRtoR(EDX, _Rt_);
                }
                else if (sz==128 || sz==64)
                {
                        _deleteEEreg(_Rt_, 1);          // flush register to mem
                        MOV32ItoR(EDX,(int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 0 ]);
                }
        }

        // Load ECX with the destination address, or issue a direct optimized write
        // if the address is a constant propagation.

        if( GPR_IS_CONST1( _Rs_ ) )
        {
                iFlushCall(FLUSH_EXCEPTION);
                u32 dstadr = g_cpuConstRegs[_Rs_].UL[0] + _Imm_;
                if( sz == 128 ) dstadr &= ~0x0f;
                vtlb_DynGenWrite_Const( sz, dstadr );
        }
        else
        {
                iFlushCall(FLUSH_EXCEPTION);
                _eeMoveGPRtoR(ECX, _Rs_);

                if ( _Imm_ != 0 )
                        ADD32ItoR(ECX, _Imm_);
                if (sz==128)
                        AND32ItoR(ECX,~0x0F);

                vtlb_DynGenWrite(sz);
        }
}


//////////////////////////////////////////////////////////////////////////////////////////
//
void recLB( void )  { if(_Rt_) recLoad32(8,true); }
void recLBU( void ) { if(_Rt_) recLoad32(8,false); }
void recLH( void )  { if(_Rt_) recLoad32(16,true); }
void recLHU( void ) { if(_Rt_) recLoad32(16,false); }
void recLW( void )  { if(_Rt_) recLoad32(32,true); }
void recLWU( void ) { if(_Rt_) recLoad32(32,false); }
void recLD( void )  { if(_Rt_) recLoad64(64,false); }
void recLQ( void )  { if(_Rt_) recLoad64(128,false); }

void recSB( void )  { recStore(8); }
void recSH( void )  { recStore(16); }
void recSW( void )  { recStore(32); }
void recSQ( void )  { recStore(128); }
void recSD( void )  { recStore(64); }

//////////////////////////////////////////////////////////////////////////////////////////
// Non-recompiled Implementations Start Here -->
// (LWL/SWL, LWR/SWR, etc)

////////////////////////////////////////////////////

static const s32 LWL_MASK[4] = { 0xffffff, 0x0000ffff, 0x000000ff, 0x00000000 };
static const s32 LWR_MASK[4] = { 0x000000, 0xff000000, 0xffff0000, 0xffffff00 };
static const u8 LWL_SHIFT[4] = { 24, 16, 8, 0 };
static const u8 LWR_SHIFT[4] = { 0, 8, 16, 24 };

void recLWL( void )
{
	if(!_Rt_) return;
#ifdef REC_LOADS

	iFlushCall(FLUSH_EXCEPTION);
	_eeOnLoadWrite(_Rt_);
	_deleteEEreg(_Rt_, 1);

	if( GPR_IS_CONST1( _Rs_ ) )
	{
		u32 srcadr = g_cpuConstRegs[_Rs_].UL[0] + _Imm_;
		u32 shift = srcadr & 0x3;
		srcadr &= ~0x3;

		vtlb_DynGenRead32_Const( 32, true, srcadr );

		if( GPR_IS_CONST1( _Rt_ ) )
		{
			int res = g_cpuConstRegs[_Rt_].UL[0] & LWL_MASK[shift];
			MOV32ItoR( EDX, res );
		}
		else
		{
			_eeMoveGPRtoR(EDX, _Rt_);		
			AND32ItoR( EDX, LWL_MASK[shift] );
		}
		
		SHL32ItoR( EAX, LWL_SHIFT[shift] );
		OR32RtoR( EAX, EDX );
	}
	else
	{
		// Load ECX with the source memory address that we're reading from.
		_eeMoveGPRtoR(ECX, _Rs_);
		if ( _Imm_ != 0 )
			ADD32ItoR( ECX, _Imm_ );

		MOV32RtoR(EDX, ECX);
		AND32ItoR(EDX, 0x3);
		SHL32ItoR(EDX, 3);
		
		AND32ItoR(ECX,~0x3);
		
		
		vtlb_DynGenRead32(32, true);

		MOV32RtoR(ECX, EDX);
		MOV32ItoR( EBX, 0xffffff );
		SHR32CLtoR( EBX );

		MOV32ItoR( ECX, 24 );
		SUB32RtoR(ECX, EDX);
		SHL32CLtoR( EAX );// eax << ecx
		
		_eeMoveGPRtoR(EDX, _Rt_);

		AND32RtoR( EDX, EBX );
		
		OR32RtoR( EAX, EDX );
	}
	
	// EAX holds the loaded value, so sign extend as needed:
	CDQ();

	MOV32RtoM( (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 0 ], EAX );
	MOV32RtoM( (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 1 ], EDX );
#else
	
	iFlushCall(FLUSH_EXCEPTION);
	_deleteEEreg(_Rs_, 1);
	_eeOnLoadWrite(_Rt_);
	_deleteEEreg(_Rt_, 1);

	recCall(LWL);
#endif
}

////////////////////////////////////////////////////
void recLWR( void )
{
	if(!_Rt_) return;
#ifdef REC_LOADS
	iFlushCall(FLUSH_EXCEPTION);
	_eeOnLoadWrite(_Rt_);
	_deleteEEreg(_Rt_, 1);

	if( GPR_IS_CONST1( _Rs_ ) )
	{
		u32 srcadr = g_cpuConstRegs[_Rs_].UL[0] + _Imm_;
		u32 shift = srcadr & 0x3;
		srcadr &= ~0x3;
		//DevCon.Warning("LWR Const");
		vtlb_DynGenRead32_Const( 32, true, srcadr );

		if( GPR_IS_CONST1( _Rt_ ) )
		{
			int res = g_cpuConstRegs[_Rt_].UL[0] & LWR_MASK[shift];
			MOV32ItoR( EDX, res );
		}
		else
		{
			_eeMoveGPRtoR(EDX, _Rt_);
			AND32ItoR( EDX, LWR_MASK[shift] );
		}
		
		SHR32ItoR( EAX, LWR_SHIFT[shift] );
		OR32RtoR( EAX, EDX );
	}
	else
	{
		
		//iFlushCall(FLUSH_EXCEPTION);
		// Load ECX with the source memory address that we're reading from.
		_eeMoveGPRtoR(ECX, _Rs_);
		if ( _Imm_ != 0 )
			ADD32ItoR( ECX, _Imm_ );

		MOV32RtoR(EDX, ECX);
		AND32ItoR(EDX, 0x3);
		SHL32ItoR(EDX, 3);
		
		AND32ItoR(ECX,~0x3);

		vtlb_DynGenRead32(32, true);
		MOV32RtoR(ECX, EDX);
		MOV32ItoR( EBX, 0xffffff00 );
		SHR32CLtoR( EAX ); // eax << ecx

		MOV32ItoR( ECX, 24 );
		SUB32RtoR(ECX, EDX);
		SHL32CLtoR( EBX );// ebx << ecx
		
		_eeMoveGPRtoR(EDX, _Rt_);

		AND32RtoR( EDX, EBX );
		
		OR32RtoR( EAX, EDX );
	}

	// EAX holds the loaded value, so sign extend as needed:
	CDQ();

	MOV32RtoM( (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 0 ], EAX );
	MOV32RtoM( (int)&cpuRegs.GPR.r[ _Rt_ ].UL[ 1 ], EDX );
#else	
	iFlushCall(FLUSH_EXCEPTION);
	_deleteEEreg(_Rs_, 1);
	_eeOnLoadWrite(_Rt_);
	_deleteEEreg(_Rt_, 1);

	recCall(LWR);
#endif
}

static const u32 SWL_MASK[4] = { 0xffffff00, 0xffff0000, 0xff000000, 0x00000000 };
static const u32 SWR_MASK[4] = { 0x00000000, 0x000000ff, 0x0000ffff, 0x00ffffff };

static const u8 SWR_SHIFT[4] = { 0, 8, 16, 24 };
static const u8 SWL_SHIFT[4] = { 24, 16, 8, 0 };

////////////////////////////////////////////////////
void recSWL( void )
{

#ifdef REC_STORES
	iFlushCall(FLUSH_EXCEPTION);
	_eeOnLoadWrite(_Rt_);
	_deleteEEreg(_Rt_, 1);

	if( GPR_IS_CONST1( _Rs_ ) )
	{
		u32 addr = g_cpuConstRegs[_Rs_].UL[0] + _Imm_;
		u32 shift = addr & 3;
		vtlb_DynGenRead32_Const( 32, false, addr & ~3 );

		// Prep eax/edx for producing the writeback result:
		//DevCon.Warning("SWL Const");
		if(_Rt_)
		{
			if( GPR_IS_CONST1( _Rt_ ) )
			{
				int res;
				res = g_cpuConstRegs[_Rt_].UL[0] >> SWL_SHIFT[shift];
			}
			else
			{
				_eeMoveGPRtoR(EDX, _Rt_);				
				SHR32ItoR( EDX, SWL_SHIFT[shift] );
			}
		}
		else XOR32RtoR( EDX, EDX );

		AND32ItoR( EAX, SWL_MASK[shift] );		
		OR32RtoR( EDX, EAX );

		vtlb_DynGenWrite_Const( 32, addr & ~0x3 );
	}
	else
	{

		_eeMoveGPRtoR(ECX, _Rs_);
		//DevCon.Warning("SWL No Const");
		if ( _Imm_ != 0 )
			ADD32ItoR(ECX, _Imm_);


		MOV32RtoR(EDX, ECX);
		AND32ItoR(EDX, 0x3);
		SHL32ItoR(EDX, 3);

		AND32ItoR(ECX,~0x3);
		
		vtlb_DynGenRead32(32, false);
		MOV32RtoR( ECX, EDX );
		MOV32ItoR( EBX, 0xffffff00 );
		SHL32CLtoR( EBX ); // ebx << ecx
		AND32RtoR( EAX, EBX );
		if(_Rt_)
		{
			MOV32ItoR( ECX, 24 );
			SUB32RtoR( ECX, EDX );

			_eeMoveGPRtoR(EDX, _Rt_);				
			SHR32CLtoR( EDX ); // edx >> ecx
		} 
		else XOR32RtoR( EDX, EDX );
		OR32RtoR( EDX, EAX );
		
		_eeMoveGPRtoR(ECX, _Rs_);

		if ( _Imm_ != 0 )
			ADD32ItoR(ECX, _Imm_);

		AND32ItoR(ECX,~0x3);

		vtlb_DynGenWrite(32);
	}
#else
	iFlushCall(FLUSH_EXCEPTION);
	_deleteEEreg(_Rs_, 1);
	_deleteEEreg(_Rt_, 1);
	recCall(SWL);
#endif
}

////////////////////////////////////////////////////
void recSWR( void )
{

#ifdef REC_STORES
	iFlushCall(FLUSH_EXCEPTION);
	_eeOnLoadWrite(_Rt_);
	_deleteEEreg(_Rt_, 1);
	if( GPR_IS_CONST1( _Rs_ ) )
	{
		u32 addr = g_cpuConstRegs[_Rs_].UL[0] + _Imm_;
		u32 shift = addr & 3;
		vtlb_DynGenRead32_Const( 32, false, addr & ~3 );

		// Prep eax/edx for producing the writeback result:
		//DevCon.Warning("SWR Const");
		if(_Rt_)
		{
			if( GPR_IS_CONST1( _Rt_ ) )
			{
				int res;
				res = g_cpuConstRegs[_Rt_].UL[0] << SWR_SHIFT[shift];
				MOV32ItoR( EDX, res );
			}
			else
			{
				_eeMoveGPRtoR(EDX, _Rt_);				
				SHL32ItoR( EDX, SWR_SHIFT[shift] );
			}
		}
		else XOR32RtoR( EDX, EDX );
		AND32ItoR( EAX, SWR_MASK[shift] );
		OR32RtoR( EDX, EAX );

		vtlb_DynGenWrite_Const( 32, addr & ~0x3 );
	}
	else
	{		
		_eeMoveGPRtoR(ECX, _Rs_);
		//DevCon.Warning("SWR No Const");
		if ( _Imm_ != 0 )
			ADD32ItoR(ECX, _Imm_);
		MOV32RtoR(EDX, ECX); //Move to EBX for shift
		AND32ItoR(ECX,~0x3);  //Mask final bit of address
		
		vtlb_DynGenRead32(32, false); //Read in to EAX

		AND32ItoR(EDX, 0x3); //Mask shift bits
		SHL32ItoR(EDX, 3);   //Multiply by 8

		if(_Rt_)
		{
			MOV32RtoR( ECX, EDX ); //Copy shift in to ECX
			_eeMoveGPRtoR(EBX, _Rt_);	//Move Rt in to EDX
			SHL32CLtoR( EBX ); // Rt << shift (ecx)
		}
		else XOR32RtoR( EBX, EBX );
		MOV32ItoR( ECX, 24 );  //Move 24 in to ECX
		SUB32RtoR( ECX, EDX ); //Take the shift from it (so if shift is 1, itll do 24 - 8 = 16)
		MOV32ItoR( EDX, 0xffffff ); //Move the mask in to where the shift was
		SHR32CLtoR( EDX ); // mask >> 24-shift

		AND32RtoR( EAX, EDX ); //And the Mask with the original memory in EAX
		
		OR32RtoR( EBX, EAX ); //Or our result of the _Rt_ shift to it	
		MOV32RtoR( EDX, EBX );
		_eeMoveGPRtoR(ECX, _Rs_);

		if ( _Imm_ != 0 )
			ADD32ItoR(ECX, _Imm_);

		AND32ItoR(ECX,~0x3);

		//EDX holds data to be written back
		vtlb_DynGenWrite(32); //Write back to memory
	}
#else
	iFlushCall(FLUSH_EXCEPTION);
	_deleteEEreg(_Rs_, 1);
	_deleteEEreg(_Rt_, 1);
	recCall(SWR);
#endif
}

////////////////////////////////////////////////////
void recLDL( void )
{
	
	if(!_Rt_) return;
#ifdef REC_LOADS
	iFlushCall(FLUSH_EXCEPTION);
	_eeOnLoadWrite(_Rt_);
	_deleteEEreg(_Rt_, 1);

	if( GPR_IS_CONST1( _Rs_ ) )
	{
		u32 srcadr = g_cpuConstRegs[_Rs_].UL[0] + _Imm_;
		u32 shift = srcadr & 0x7;

		if(shift == 7)
		{
			//DevCon.Warning("LDL 7");
			srcadr &= ~0x7;			
			MOV32ItoR(EDX, (uptr)&cpuRegs.GPR.r[ _Rt_ ].UL[ 0 ] );
			
			vtlb_DynGenRead64_Const( 64, srcadr );
		}
		else if(shift == 3)
		{
			//DevCon.Warning("LDL threeeee");
			srcadr &= ~0x7;
			vtlb_DynGenRead32_Const( 32, false, srcadr );
			MOV32RtoM((uptr)&cpuRegs.GPR.r[ _Rt_ ].UL[ 1 ] , EAX);
			
		}
		else
		{
			//DevCon.Warning("LDL Const Interpreter Drop Back %x", shift);
			recCall(LDL);
		}
	}
	else
	{
		    
			//DevCon.Warning("Interpreter %x", shift);
			_deleteEEreg(_Rt_, 0);
			_eeOnLoadWrite(_Rt_);
			recCall(LDL);
	}
#else
	iFlushCall(FLUSH_EXCEPTION);
	_deleteEEreg(_Rs_, 1);
	_eeOnLoadWrite(_Rt_);
	_deleteEEreg(_Rt_, 1);
	recCall(LDL);
#endif
}

////////////////////////////////////////////////////
void recLDR( void )
{
	if(!_Rt_) return;
	
#ifdef REC_LOADS
	iFlushCall(FLUSH_EXCEPTION);
	_eeOnLoadWrite(_Rt_);
	_deleteEEreg(_Rt_, 1);
	if( GPR_IS_CONST1( _Rs_ ) )
	{
		u32 srcadr = g_cpuConstRegs[_Rs_].UL[0] + _Imm_;
		u32 shift = srcadr & 0x7;

		if(shift == 0)
		{
			//DevCon.Warning("LDR 0");

			MOV32ItoR(EDX, (uptr)&cpuRegs.GPR.r[ _Rt_ ].UL[ 0 ] );
			
			vtlb_DynGenRead64_Const( 64, srcadr );
		}
		else if(shift == 4)
		{
			//DevCon.Warning("LDR 4");		

			vtlb_DynGenRead32_Const( 32, false, srcadr );
			MOV32RtoM((uptr)&cpuRegs.GPR.r[ _Rt_ ].UL[ 0 ] , EAX);
		}
		else
		{
			//DevCon.Warning("LDR Const Interpreter Drop Back %x", shift);
			recCall(LDR);
		}
	}
	else
	{
			//DevCon.Warning("Interpreter %x", shift);
			_deleteEEreg(_Rt_, 0);
			_eeOnLoadWrite(_Rt_);
			recCall(LDR);
	}
#else
	iFlushCall(FLUSH_EXCEPTION);
	_deleteEEreg(_Rs_, 1);
	_eeOnLoadWrite(_Rt_);
	_deleteEEreg(_Rt_, 1);
	recCall(LDR);
#endif
}

////////////////////////////////////////////////////

static const u64 SDL_MASK[8] =
{	0xffffffffffffff00LL, 0xffffffffffff0000LL, 0xffffffffff000000LL, 0xffffffff00000000LL,
	0xffffff0000000000LL, 0xffff000000000000LL, 0xff00000000000000LL, 0x0000000000000000LL
};
static const u64 SDR_MASK[8] =
{	0x0000000000000000LL, 0x00000000000000ffLL, 0x000000000000ffffLL, 0x0000000000ffffffLL,
	0x00000000ffffffffLL, 0x000000ffffffffffLL, 0x0000ffffffffffffLL, 0x00ffffffffffffffLL
};

void recSDL( void )
{

#ifdef REC_STORES
	iFlushCall(FLUSH_EXCEPTION);
	_eeOnLoadWrite(_Rt_);
	_deleteEEreg(_Rt_, 1);

	if( GPR_IS_CONST1( _Rs_ ) )
	{
		
		if( GPR_IS_CONST1( _Rt_ ) )DevCon.Warning("Yay SDL!");
		u32 addr = g_cpuConstRegs[_Rs_].UL[0] + _Imm_;
		u32 shift = addr & 7;
		//DevCon.Warning("SDL %x", shift);
		if(shift == 7)
		{
			//DevCon.Warning("T SDL 7");
			if( GPR_IS_CONST1( _Rt_ ) ) MOV32ItoR(EDX, (uptr)&g_cpuConstRegs[_Rt_].UL[0]);
			else MOV32ItoR(EDX, (uptr)&cpuRegs.GPR.r[ _Rt_ ].UL[ 0 ] );
			vtlb_DynGenWrite_Const( 64, addr & ~0x7 );
		}
		else if(shift == 3)
		{
			//DevCon.Warning("T SDL 3");
			if( GPR_IS_CONST1( _Rt_ ) ) MOV32ItoR(EDX, (uptr)&g_cpuConstRegs[_Rt_].UL[1]);
			else MOV32MtoR(EDX, (uptr)&cpuRegs.GPR.r[ _Rt_ ].UL[ 1 ] );
			vtlb_DynGenWrite_Const( 32, addr & ~0x7 );
		}
		else
		{
			//DevCon.Warning("SDL Const Interpreter Drop Back %x", shift);
			iFlushCall(FLUSH_INTERPRETER);
			recCall(SDL);
		}
	}
	else
	{
		iFlushCall(FLUSH_INTERPRETER);
		recCall(SDL);
		
	}
#else
	iFlushCall(FLUSH_EXCEPTION);
	_deleteEEreg(_Rs_, 1);
	_deleteEEreg(_Rt_, 1);
	recCall(SDL);
#endif
}

////////////////////////////////////////////////////
void recSDR( void )
{
#ifdef REC_STORES
	if( GPR_IS_CONST1( _Rs_ ) )
	{
		iFlushCall(FLUSH_EXCEPTION);
		_eeOnLoadWrite(_Rt_);
		_deleteEEreg(_Rt_, 1);

		u32 addr = g_cpuConstRegs[_Rs_].UL[0] + _Imm_;
		u32 shift = addr & 7;

		if( GPR_IS_CONST1( _Rt_ ) )DevCon.Warning("Yay SDR!");
		
		if(shift == 0)
		{
			//DevCon.Warning("T SDR 0");
			if( GPR_IS_CONST1( _Rt_ ) ) MOV32ItoR(EDX, (uptr)&g_cpuConstRegs[_Rt_].UL[0]);
			else MOV32ItoR(EDX, (uptr)&cpuRegs.GPR.r[ _Rt_ ].UL[ 0 ] );
			vtlb_DynGenWrite_Const( 64, addr );
		}
		else if(shift == 4)
		{
			//DevCon.Warning("T SDR 4");
			if( GPR_IS_CONST1( _Rt_ ) ) MOV32ItoR(EDX, (uptr)&g_cpuConstRegs[_Rt_].UL[0]);
			else MOV32MtoR(EDX, (uptr)&cpuRegs.GPR.r[ _Rt_ ].UL[ 0 ] );
			vtlb_DynGenWrite_Const( 32, addr );
		}
		else
		{
			//DevCon.Warning("SDR Const Interpreter Drop Back %x", shift);
			iFlushCall(FLUSH_INTERPRETER);
			recCall(SDR);
		}
	}
	else
	{
		iFlushCall(FLUSH_INTERPRETER);
		recCall(SDR);
	}
#else
	iFlushCall(FLUSH_EXCEPTION);
	_deleteEEreg(_Rs_, 1);
	_deleteEEreg(_Rt_, 1);
	recCall(SDR);
#endif
}

//////////////////////////////////////////////////////////////////////////////////////////
/*********************************************************
* Load and store for COP1                                *
* Format:  OP rt, offset(base)                           *
*********************************************************/

////////////////////////////////////////////////////

void recLWC1( void )
{
#ifdef NEWLWC1
	iFlushCall(FLUSH_EXCEPTION);
	_deleteEEreg(_Rs_, 1);
	_deleteFPtoXMMreg(_Rt_, 2);

	if( GPR_IS_CONST1( _Rs_ ) )
	{
		int addr = g_cpuConstRegs[_Rs_].UL[0] + _Imm_;
		vtlb_DynGenRead32_Const(32, false, addr);
	}
	else
	{
		MOV32MtoR( ECX, (int)&cpuRegs.GPR.r[ _Rs_ ].UL[ 0 ] );
		if ( _Imm_ != 0 )
			ADD32ItoR( ECX, _Imm_ );
		vtlb_DynGenRead32(32, false);
	}
		
	MOV32RtoM( (int)&fpuRegs.fpr[ _Rt_ ].UL, EAX );
#else
	iFlushCall(FLUSH_EXCEPTION);
	_deleteEEreg(_Rs_, 1);
	_deleteFPtoXMMreg(_Rt_, 2);

	MOV32MtoR( ECX, (int)&cpuRegs.GPR.r[ _Rs_ ].UL[ 0 ] );
	if ( _Imm_ != 0 )
		ADD32ItoR( ECX, _Imm_ );

	vtlb_DynGenRead32(32, false);
	MOV32RtoM( (int)&fpuRegs.fpr[ _Rt_ ].UL, EAX );
#endif
}

////////////////////////////////////////////////////

void recSWC1( void )
{
#ifdef NEWSWC
	iFlushCall(FLUSH_EXCEPTION);
	_deleteEEreg(_Rs_, 1);
	_deleteFPtoXMMreg(_Rt_, 1);

	MOV32MtoR(EDX, (int)&fpuRegs.fpr[ _Rt_ ].UL );

	if( GPR_IS_CONST1( _Rs_ ) )
	{
		int addr = g_cpuConstRegs[_Rs_].UL[0] + _Imm_;
		vtlb_DynGenWrite_Const(32, addr);
	}
	else
	{
		MOV32MtoR( ECX, (int)&cpuRegs.GPR.r[ _Rs_ ].UL[ 0 ] );
		if ( _Imm_ != 0 )
			ADD32ItoR( ECX, _Imm_ );
		vtlb_DynGenWrite(32);
	}
#else
	iFlushCall(FLUSH_EXCEPTION);
	_deleteEEreg(_Rs_, 1);
	_deleteFPtoXMMreg(_Rt_, 0);

	MOV32MtoR( ECX, (int)&cpuRegs.GPR.r[ _Rs_ ].UL[ 0 ] );
	if ( _Imm_ != 0 )
		ADD32ItoR( ECX, _Imm_ );

	MOV32MtoR(EDX, (int)&fpuRegs.fpr[ _Rt_ ].UL );
	vtlb_DynGenWrite(32);
#endif
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
	
#ifdef NEWLQC
	iFlushCall(FLUSH_EXCEPTION);
	_deleteEEreg(_Rs_, 1);
	_deleteVFtoXMMreg(_Ft_, 0, 2);

	if ( _Rt_ )
		MOV32ItoR(EDX, (int)&VU0.VF[_Ft_].UD[0] );
	else
		MOV32ItoR(EDX, (int)&dummyValue[0] );

	if( GPR_IS_CONST1( _Rs_ ) )
	{
		int addr = g_cpuConstRegs[_Rs_].UL[0] + _Imm_;

		vtlb_DynGenRead64_Const(128, addr);
	}
	else
	{
		MOV32MtoR( ECX, (int)&cpuRegs.GPR.r[ _Rs_ ].UL[ 0 ] );

		if ( _Imm_ != 0 )
			ADD32ItoR( ECX, _Imm_);	

		vtlb_DynGenRead64(128);
	}
#else
	iFlushCall(FLUSH_EXCEPTION);
	_deleteEEreg(_Rs_, 1);
	_deleteVFtoXMMreg(_Ft_, 0, 2);

	MOV32MtoR( ECX, (int)&cpuRegs.GPR.r[ _Rs_ ].UL[ 0 ] );
	if ( _Imm_ != 0 )
		ADD32ItoR( ECX, _Imm_);

	if ( _Rt_ )
		MOV32ItoR(EDX, (int)&VU0.VF[_Ft_].UD[0] );
	else
		MOV32ItoR(EDX, (int)&dummyValue[0] );

	vtlb_DynGenRead64(128);
#endif
}

////////////////////////////////////////////////////



void recSQC2( void )
{
#ifdef NEWSQC
	iFlushCall(FLUSH_EXCEPTION);
	_deleteEEreg(_Rs_, 1);
	_deleteVFtoXMMreg(_Ft_, 0, 1); //Want to flush it but not clear it

	MOV32ItoR(EDX, (int)&VU0.VF[_Ft_].UD[0] );
	if( GPR_IS_CONST1( _Rs_ ) )
	{
		int addr = g_cpuConstRegs[_Rs_].UL[0] + _Imm_;
		vtlb_DynGenWrite_Const(128, addr);
	}
	else
	{
		MOV32MtoR( ECX, (int)&cpuRegs.GPR.r[ _Rs_ ].UL[ 0 ] );
		if ( _Imm_ != 0 )
			ADD32ItoR( ECX, _Imm_ );

		vtlb_DynGenWrite(128);
	}
#else
	iFlushCall(FLUSH_EXCEPTION);
	_deleteEEreg(_Rs_, 1);
	_deleteVFtoXMMreg(_Ft_, 0, 0);

	MOV32MtoR( ECX, (int)&cpuRegs.GPR.r[ _Rs_ ].UL[ 0 ] );
	if ( _Imm_ != 0 )
		ADD32ItoR( ECX, _Imm_ );

	MOV32ItoR(EDX, (int)&VU0.VF[_Ft_].UD[0] );
	vtlb_DynGenWrite(128);
#endif
	
}

#endif

} } }	// end namespace R5900::Dynarec::OpcodeImpl

using namespace R5900::Dynarec;
using namespace R5900::Dynarec::OpcodeImpl;

void SetFastMemory(int bSetFast) {}
