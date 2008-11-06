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

#include <math.h>
#include "Common.h"
#include "R5900.h"
#include "InterTables.h"

// Helper Macros
//****************************************************************
#define _Ft_         ( ( cpuRegs.code >> 16 ) & 0x1F )
#define _Fs_         ( ( cpuRegs.code >> 11 ) & 0x1F )
#define _Fd_         ( ( cpuRegs.code >>  6 ) & 0x1F )

#define _FtValf_     fpuRegs.fpr[ _Ft_ ].f
#define _FsValf_     fpuRegs.fpr[ _Fs_ ].f
#define _FdValf_     fpuRegs.fpr[ _Fd_ ].f
#define _FAValf_     fpuRegs.ACC.f

#define _ContVal_    fpuRegs.fprc[ 31 ]

// Testing
#define _FtValUl_    fpuRegs.fpr[ _Ft_ ].UL
#define _FsValUl_    fpuRegs.fpr[ _Fs_ ].UL
#define _FdValUl_    fpuRegs.fpr[ _Fd_ ].UL
#define _FAValUl_    fpuRegs.ACC.UL

//****************************************************************

void COP1() {
#ifdef FPU_LOG
	FPU_LOG("%s\n", disR5900F(cpuRegs.code, cpuRegs.pc));
#endif
	Int_COP1PrintTable[_Rs_]();
}

/*********************************************************
* Load and store for COP1                                *
* Format:  OP rt, offset(base)                           *
*********************************************************/



void LWC1() {
	s32 addr;

	addr = cpuRegs.GPR.r[_Rs_].UL[0] + (s16)(cpuRegs.code);// ((cpuRegs.code & 0x8000 ? 0xFFFF8000 : 0)| (cpuRegs.code & 0x7fff));
	memRead32(addr, &fpuRegs.fpr[_Rt_].UL);
}

void SWC1() {
	s32 addr;
	addr = cpuRegs.GPR.r[_Rs_].UL[0] + (s16)(cpuRegs.code);//((cpuRegs.code & 0x8000 ? 0xFFFF8000 : 0)| (cpuRegs.code & 0x7fff));
	memWrite32(addr,  fpuRegs.fpr[_Rt_].UL); 
}

void COP1_BC1() {
	Int_COP1BC1PrintTable[_Rt_]();
}

void COP1_S() {
	Int_COP1SPrintTable[_Funct_]();
}

void COP1_W() {
	Int_COP1WPrintTable[_Funct_]();
}

void COP1_Unknown() {
#ifdef FPU_LOG
	FPU_LOG("Unknown FPU opcode called\n");
#endif
}



void MFC1() {
	if ( !_Rt_ ) return;
	cpuRegs.GPR.r[_Rt_].UD[0] = (s32)_FsValUl_;
}

void CFC1() {
	if ( !_Rt_ || ( _Fs_ != 0 && _Fs_ != 31 ) ) return;
	cpuRegs.GPR.r[_Rt_].UD[0] = (s32)fpuRegs.fprc[_Fs_];
}

void MTC1() {
	_FsValUl_ = cpuRegs.GPR.r[_Rt_].UL[0];
}

void CTC1() {
	if(_Fs_!=31) return;
	fpuRegs.fprc[_Fs_] = cpuRegs.GPR.r[_Rt_].UL[0];
}

#define C_cond_S(cond)                       \
   _ContVal_ = ( _FsValf_ cond _FtValf_ ) ?  \
               ( _ContVal_ | 0x00800000 ) :  \
               ( _ContVal_ & ~0x00800000 );

void C_F()  { _ContVal_ &= ~0x00800000;} //clears C regardless
void C_EQ() { C_cond_S(==); }
void C_LT() { C_cond_S(<); }
void C_LE() { C_cond_S(<=); }

#define BC1(cond)                               \
   if ( ( _ContVal_ & 0x00800000 ) cond 0 ) {   \
      intDoBranch( _BranchTarget_ );            \
   }
void BC1F() { BC1(==); }
void BC1T() { BC1(!=); }

#define BC1L(cond)                              \
   if ( ( _ContVal_ & 0x00800000 ) cond 0 ) {   \
      intDoBranch( _BranchTarget_ );            \
   } else cpuRegs.pc += 4;
void BC1FL() { BC1L(==); } // Equal to 0
void BC1TL() { BC1L(!=); } // different from 0


void ADD_S()   { _FdValf_  = _FsValf_ + _FtValf_; }  
void SUB_S()   { _FdValf_  = _FsValf_ - _FtValf_; }  
void MUL_S()   { _FdValf_  = _FsValf_ * _FtValf_; }
void MOV_S()   { _FdValf_  = _FsValf_; } 
void NEG_S()   { _FdValf_  = -_FsValf_; } 
void ADDA_S()  { _FAValf_  = _FsValf_ + _FtValf_; } 
void SUBA_S()  { _FAValf_  = _FsValf_ - _FtValf_; } 
void MULA_S()  { _FAValf_  = _FsValf_ * _FtValf_; }
void MADD_S()  { _FdValf_  = _FAValf_ + ( _FsValf_ * _FtValf_ ); } 
void MSUB_S()  { _FdValf_  = _FAValf_ - ( _FsValf_ * _FtValf_ ); } 
void MADDA_S() { _FAValf_ += _FsValf_ * _FtValf_; } 
void MSUBA_S() { _FAValf_ -= _FsValf_ * _FtValf_; }
void ABS_S()   { _FdValf_  = fpufabsf(_FsValf_); }  
void MAX_S()   { _FdValf_  = max( _FsValf_, _FtValf_ ); }
void MIN_S()   { _FdValf_  = min( _FsValf_, _FtValf_ ); }
void DIV_S()   { if ( _FtValf_ ) { _FdValf_ = _FsValf_ / _FtValf_; } }
void SQRT_S()  { 
	//if ( _FtValf_ >= 0.0f ) { _FdValf_ = fpusqrtf( _FtValf_ ); }
	//else {
		_FdValf_ = fpusqrtf(fpufabsf(_FtValf_));
	//}
} 
void CVT_S()   { _FdValf_ = (float)(s32)_FsValUl_; }
void CVT_W()   {
	if ( ( _FsValUl_ & 0x7F800000 ) <= 0x4E800000 ) { _FdValUl_ = (s32)(float)_FsValf_; }
	else if ( _FsValUl_ & 0x80000000 ) { _FdValUl_ = 0x80000000; }
	else { _FdValUl_ = 0x7fffffff; }
}

void RSQRT_S() {
	if ( _FtValf_ >= 0.0f ) {
		float tmp = fpusqrtf( _FtValf_ );
		if ( tmp != 0 ) { _FdValf_ = _FsValf_ / tmp; }
	}
}

//3322 2222 2222 1111 1111 1100 0000 0000
//1098 7654 3210 9876 5432 1098 7654 3210
//0000 0000 0000 0000 0000 0000 0000 0000
