/*  Pcsx2 - Pc Ps2 Emulator
 *  Copyright (C) 2002-2008  Pcsx2 Team
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

#include <cmath>
#include "Common.h"
#include "R5900.h"
#include "R5900OpcodeTables.h"

using namespace std;			// for min / max

// Helper Macros
//****************************************************************

// IEEE 754 Values
#define PosInfinity 0x7f800000
#define NegInfinity 0xff800000
#define posFmax 0x7F7FFFFF
#define negFmax 0xFF7FFFFF


/*	Used in compare function to compensate for differences between IEEE 754 and the FPU.
	Setting it to ~0x00000000 = Compares Exact Value. (comment out this macro for faster Exact Compare method)
	Setting it to ~0x00000001 = Discards the least significant bit when comparing.
	Setting it to ~0x00000003 = Discards the least 2 significant bits when comparing... etc..  */
//#define comparePrecision ~0x00000001

// Operands
#define _Ft_         ( ( cpuRegs.code >> 16 ) & 0x1F )
#define _Fs_         ( ( cpuRegs.code >> 11 ) & 0x1F )
#define _Fd_         ( ( cpuRegs.code >>  6 ) & 0x1F )

// Floats
#define _FtValf_     fpuRegs.fpr[ _Ft_ ].f
#define _FsValf_     fpuRegs.fpr[ _Fs_ ].f
#define _FdValf_     fpuRegs.fpr[ _Fd_ ].f
#define _FAValf_     fpuRegs.ACC.f

// U32's
#define _FtValUl_    fpuRegs.fpr[ _Ft_ ].UL
#define _FsValUl_    fpuRegs.fpr[ _Fs_ ].UL
#define _FdValUl_    fpuRegs.fpr[ _Fd_ ].UL
#define _FAValUl_    fpuRegs.ACC.UL

// FPU Control Reg (FCR31)
#define _ContVal_    fpuRegs.fprc[ 31 ]

// FCR31 Flags
#define FPUflagC	0X00800000
#define FPUflagI	0X00020000
#define FPUflagD	0X00010000
#define FPUflagO	0X00008000
#define FPUflagU	0X00004000
#define FPUflagSI	0X00000040
#define FPUflagSD	0X00000020
#define FPUflagSO	0X00000010
#define FPUflagSU	0X00000008

//****************************************************************

// If we have an infinity value, then Overflow has occured.
#define checkOverflow(xReg, cFlagsToSet, shouldReturn) {  \
	if ( ( xReg & ~0x80000000 ) == PosInfinity ) {  \
		/*SysPrintf( "FPU OVERFLOW!: Changing to +/-Fmax!!!!!!!!!!!!\n" );*/  \
		xReg = ( xReg & 0x80000000 ) | posFmax;  \
		_ContVal_ |= cFlagsToSet;  \
		if ( shouldReturn ) { return; }  \
	}  \
}

// If we have a denormal value, then Underflow has occured.
#define checkUnderflow(xReg, cFlagsToSet, shouldReturn) {  \
	if ( ( ( xReg & 0x7F800000 ) == 0 ) && ( ( xReg & 0x007FFFFF ) != 0 ) ) {  \
		/*SysPrintf( "FPU UNDERFLOW!: Changing to +/-0!!!!!!!!!!!!\n" );*/  \
		xReg &= 0x80000000;  \
		_ContVal_ |= cFlagsToSet;  \
		if ( shouldReturn ) { return; }  \
	}  \
}

/*	Checks if Divide by Zero will occur. (z/y = x)
	cFlagsToSet1 = Flags to set if (z != 0)
	cFlagsToSet2 = Flags to set if (z == 0)
	( Denormals are counted as "0" )
*/
#define checkDivideByZero(xReg, yDivisorReg, zDividendReg, cFlagsToSet1, cFlagsToSet2, shouldReturn) {  \
	if ( ( yDivisorReg & 0x7F800000 ) == 0 ) {  \
		_ContVal_ |= ( ( zDividendReg & 0x7F800000 ) == 0 ) ? cFlagsToSet2 : cFlagsToSet1;  \
		xReg = ( ( yDivisorReg ^ zDividendReg ) & 0x80000000 ) | posFmax;  \
		if ( shouldReturn ) { return; }  \
	}  \
}

/*	Clears the "Cause Flags" of the Control/Status Reg
	The "EE Core Users Manual" implies that all the Cause flags are cleared every instruction...
	But, the "EE Core Instruction Set Manual" says that only certain Cause Flags are cleared
	for specific instructions... I'm just setting them to clear when the Instruction Set Manual
	says to... (cottonvibes)
*/
#define clearFPUFlags(cFlags) {  \
	_ContVal_ &= ~( cFlags ) ;  \
}

#ifdef comparePrecision
// This compare discards the least-significant bit(s) in order to solve some rounding issues.
	#define C_cond_S(cond) {  \
		FPRreg tempA, tempB;  \
		tempA.UL = _FsValUl_ & comparePrecision;  \
		tempB.UL = _FtValUl_ & comparePrecision;  \
		_ContVal_ = ( ( tempA.f ) cond ( tempB.f ) ) ?  \
					( _ContVal_ | FPUflagC ) :  \
					( _ContVal_ & ~FPUflagC );  \
	}
#else
// Used for Comparing; This compares if the floats are exactly the same.
	#define C_cond_S(cond) {  \
	   _ContVal_ = ( _FsValf_ cond _FtValf_ ) ?  \
				   ( _ContVal_ | FPUflagC ) :  \
				   ( _ContVal_ & ~FPUflagC );  \
	}
#endif

// Conditional Branch
#define BC1(cond)                               \
   if ( ( _ContVal_ & FPUflagC ) cond 0 ) {   \
      intDoBranch( _BranchTarget_ );            \
   }

// Conditional Branch
#define BC1L(cond)                              \
   if ( ( _ContVal_ & FPUflagC ) cond 0 ) {   \
      intDoBranch( _BranchTarget_ );            \
   } else cpuRegs.pc += 4;

namespace R5900 {
namespace Interpreter {
namespace OpcodeImpl {
namespace COP1 {

//****************************************************************
// FPU Opcodes
//****************************************************************

float fpuDouble(u32 f)
{
	switch(f & 0x7f800000){
		case 0x0:
			f &= 0x80000000;
			return *(float*)&f;
			break;
		case 0x7f800000:
			f = (f & 0x80000000)|0x7f7fffff;
			return *(float*)&f;
			break;
		default:
			return *(float*)&f;
			break;
	}
}

void ABS_S() {
	_FdValUl_ = _FsValUl_ & 0x7fffffff;
	clearFPUFlags( FPUflagO | FPUflagU );
}  

void ADD_S() {
	_FdValf_  = fpuDouble( _FsValUl_ ) + fpuDouble( _FtValUl_ );
	checkOverflow( _FdValUl_, FPUflagO | FPUflagSO, 1 );
	checkUnderflow( _FdValUl_, FPUflagU | FPUflagSU, 1 );
}

void ADDA_S() {
	_FAValf_  = fpuDouble( _FsValUl_ ) + fpuDouble( _FtValUl_ );
	checkOverflow( _FAValUl_, FPUflagO | FPUflagSO, 1 );
	checkUnderflow( _FAValUl_, FPUflagU | FPUflagSU, 1 );
} 

void BC1F() {
	BC1(==);
}

void BC1FL() {
	BC1L(==); // Equal to 0
}

void BC1T() {
	BC1(!=);
}

void BC1TL() {
	BC1L(!=); // different from 0
}

void C_EQ() {
	C_cond_S(==);
}

void C_F() {
	clearFPUFlags( FPUflagC ); //clears C regardless
}

void C_LE() {
	C_cond_S(<=);
}

void C_LT() {
	C_cond_S(<);
}

void CFC1() {
	if ( !_Rt_ || ( (_Fs_ != 0) && (_Fs_ != 31) ) ) return;
	cpuRegs.GPR.r[_Rt_].SD[0] = (s64)fpuRegs.fprc[_Fs_];
}

void CTC1() {
	if ( _Fs_ != 31 ) return;
	fpuRegs.fprc[_Fs_] = cpuRegs.GPR.r[_Rt_].UL[0];
}

void CVT_S() {
	_FdValf_ = (float)(*(s32*)&_FsValUl_);
	_FdValf_ = fpuDouble( _FdValUl_ );
}

void CVT_W() {
	if ( ( _FsValUl_ & 0x7F800000 ) <= 0x4E800000 ) { _FdValUl_ = (s32)_FsValf_; }
	else if ( ( _FsValUl_ & 0x80000000 ) == 0 ) { _FdValUl_ = 0x7fffffff; }
	else { _FdValUl_ = 0x80000000; }
}

void DIV_S() {
	checkDivideByZero( _FdValUl_, _FtValUl_, _FsValUl_, FPUflagD | FPUflagSD, FPUflagI | FPUflagSI, 1 );
	_FdValf_ = fpuDouble( _FsValUl_ ) / fpuDouble( _FtValUl_ );
	checkOverflow( _FdValUl_, 0, 1);
	checkUnderflow( _FdValUl_, 0, 1 );
}

/*	The Instruction Set manual has an overly complicated way of
	determining the flags that are set. Hopefully this shorter
	method provides a similar outcome and is faster. (cottonvibes)
*/
void MADD_S() {
	FPRreg temp;
	temp.f = fpuDouble( _FsValUl_ ) * fpuDouble( _FtValUl_ );
	_FdValf_  = fpuDouble( _FAValUl_ ) + fpuDouble( temp.UL );
	checkOverflow( _FdValUl_, FPUflagO | FPUflagSO, 1 );
	checkUnderflow( _FdValUl_, FPUflagU | FPUflagSU, 1 );
} 

void MADDA_S() {
	_FAValf_ += fpuDouble( _FsValUl_ ) * fpuDouble( _FtValUl_ );
	checkOverflow( _FAValUl_, FPUflagO | FPUflagSO, 1 );
	checkUnderflow( _FAValUl_, FPUflagU | FPUflagSU, 1 );
} 

void MAX_S() {
	_FdValf_  = max( _FsValf_, _FtValf_ );
	clearFPUFlags( FPUflagO | FPUflagU );
}

void MFC1() {
	if ( !_Rt_ ) return;
	cpuRegs.GPR.r[_Rt_].SD[0] = (s64)_FsValUl_;
}

void MIN_S() {
	_FdValf_  = min( _FsValf_, _FtValf_ );
	clearFPUFlags( FPUflagO | FPUflagU );
}

void MOV_S() {
	_FdValUl_  = _FsValUl_;
} 

void MSUB_S() {
	FPRreg temp;
	temp.f = fpuDouble( _FsValUl_ ) * fpuDouble( _FtValUl_ );
	_FdValf_  = fpuDouble( _FAValUl_ ) - fpuDouble( temp.UL );
	checkOverflow( _FdValUl_, FPUflagO | FPUflagSO, 1 );
	checkUnderflow( _FdValUl_, FPUflagU | FPUflagSU, 1 );
} 

void MSUBA_S() {
	_FAValf_ -= fpuDouble( _FsValUl_ ) * fpuDouble( _FtValUl_ );
	checkOverflow( _FAValUl_, FPUflagO | FPUflagSO, 1 );
	checkUnderflow( _FAValUl_, FPUflagU | FPUflagSU, 1 );
}

void MTC1() {
	_FsValUl_ = cpuRegs.GPR.r[_Rt_].UL[0];
}

void MUL_S() {
	_FdValf_  = fpuDouble( _FsValUl_ ) * fpuDouble( _FtValUl_ );
	checkOverflow( _FdValUl_, FPUflagO | FPUflagSO, 1 );
	checkUnderflow( _FdValUl_, FPUflagU | FPUflagSU, 1 );
}

void MULA_S() {
	_FAValf_  = fpuDouble( _FsValUl_ ) * fpuDouble( _FtValUl_ );
	checkOverflow( _FAValUl_, FPUflagO | FPUflagSO, 1 );
	checkUnderflow( _FAValUl_, FPUflagU | FPUflagSU, 1 );
}

void NEG_S() {
	_FdValUl_  = (_FsValUl_ ^ 0x80000000);
	clearFPUFlags( FPUflagO | FPUflagU );
} 

void RSQRT_S() {
	FPRreg temp;
	if ( ( _FtValUl_ & 0x7F800000 ) == 0 ) { // Ft is zero (Denormals are Zero)
		_ContVal_ |= FPUflagD | FPUflagSD;
		_FdValUl_ = ( ( _FsValUl_ ^ _FtValUl_ ) & 0x80000000 ) | posFmax;
		return;
	}
	else if ( _FtValUl_ & 0x80000000 ) { // Ft is negative
		_ContVal_ |= FPUflagI | FPUflagSI;
		temp.f = sqrt( fabs( fpuDouble( _FtValUl_ ) ) );
		_FdValf_ = fpuDouble( _FsValUl_ ) / fpuDouble( temp.UL );
	}
	else { _FdValf_ = fpuDouble( _FsValUl_ ) / sqrt( fpuDouble( _FtValUl_ ) ); } // Ft is positive and not zero

	checkOverflow( _FdValUl_, 0, 1 );
	checkUnderflow( _FdValUl_, 0, 1 );
}

void SQRT_S() {
	if ( ( _FtValUl_ & 0xFF800000 ) == 0x80000000 ) { _FdValUl_ = 0x80000000; } // If Ft = -0
	else if ( _FtValUl_ & 0x80000000 ) { // If Ft is Negative
		_ContVal_ |= FPUflagI | FPUflagSI;
		_FdValf_ = sqrt( fabs( fpuDouble( _FtValUl_ ) ) );
	}
	else { _FdValf_ = sqrt( fpuDouble( _FtValUl_ ) ); } // If Ft is Positive
	clearFPUFlags( FPUflagD );
}

void SUB_S() {
	_FdValf_  = fpuDouble( _FsValUl_ ) - fpuDouble( _FtValUl_ );
	checkOverflow( _FdValUl_, FPUflagO | FPUflagSO, 1 );
	checkUnderflow( _FdValUl_, FPUflagU | FPUflagSU, 1 );
}  

void SUBA_S() {
	_FAValf_  = fpuDouble( _FsValUl_ ) - fpuDouble( _FtValUl_ );
	checkOverflow( _FAValUl_, FPUflagO | FPUflagSO, 1 );
	checkUnderflow( _FAValUl_, FPUflagU | FPUflagSU, 1 );
} 

}	// End Namespace COP1

/////////////////////////////////////////////////////////////////////
// COP1 (FPU)  Load/Store Instructions

// These are actually EE opcodes but since they're related to FPU registers and such they
// seem more appropriately located here.

void LWC1() {
	u32 addr;
	addr = cpuRegs.GPR.r[_Rs_].UL[0] + (s32)(s16)(cpuRegs.code & 0xffff);
	if (addr & 0x00000003) { Console::Error( "FPU (LWC1 Opcode): Invalid Memory Address" ); return; }  // Should signal an exception?
	memRead32(addr, &fpuRegs.fpr[_Rt_].UL);
}

void SWC1() {
	u32 addr;
	addr = cpuRegs.GPR.r[_Rs_].UL[0] + (s32)(s16)(cpuRegs.code & 0xffff);
	if (addr & 0x00000003) { Console::Error( "FPU (SWC1 Opcode): Invalid Memory Address" ); return; }  // Should signal an exception?
	memWrite32(addr,  fpuRegs.fpr[_Rt_].UL); 
}

} } }
