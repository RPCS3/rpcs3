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

namespace R5900 {
namespace Interpreter {
namespace OpcodeImpl {

	/////////////////////////////////////////////////////////////////
	// Non-MMI Instructions!
	//
	// Several instructions in the MMI opcode class are actually just regular
	// instructions which have been added to "extend" the R5900's instruction
	// set.  They're here, because if not here they'd be homeless.
	//  - The Pcsx2 team, doing their part to fight homelessness

	void MADD() {
		s64 temp = (s64)((u64)cpuRegs.LO.UL[0] | ((u64)cpuRegs.HI.UL[0] << 32)) +
				  ((s64)cpuRegs.GPR.r[_Rs_].SL[0] * (s64)cpuRegs.GPR.r[_Rt_].SL[0]);

		cpuRegs.LO.SD[0] = (s32)(temp & 0xffffffff);
		cpuRegs.HI.SD[0] = (s32)(temp >> 32);

		if (_Rd_) cpuRegs.GPR.r[_Rd_].SD[0] = cpuRegs.LO.SD[0];
	}

	void MADDU() {
		u64 tempu =	(u64)((u64)cpuRegs.LO.UL[0] | ((u64)cpuRegs.HI.UL[0] << 32)) +
				   ((u64)cpuRegs.GPR.r[_Rs_].UL[0] * (u64)cpuRegs.GPR.r[_Rt_].UL[0]);

		cpuRegs.LO.SD[0] = (s32)(tempu & 0xffffffff);
		cpuRegs.HI.SD[0] = (s32)(tempu >> 32);

		if (_Rd_) cpuRegs.GPR.r[_Rd_].SD[0] = cpuRegs.LO.SD[0];
	}

	void MADD1() {
		s64 temp = (s64)((u64)cpuRegs.LO.UL[2] | ((u64)cpuRegs.HI.UL[2] << 32)) +
				  ((s64)cpuRegs.GPR.r[_Rs_].SL[0] * (s64)cpuRegs.GPR.r[_Rt_].SL[0]);

		cpuRegs.LO.SD[1] = (s32)(temp & 0xffffffff);
		cpuRegs.HI.SD[1] = (s32)(temp >> 32);

		if (_Rd_) cpuRegs.GPR.r[_Rd_].SD[0] = cpuRegs.LO.SD[1];
	}

	void MADDU1() {
		u64 tempu = (u64)((u64)cpuRegs.LO.UL[2] | ((u64)cpuRegs.HI.UL[2] << 32)) +
				   ((u64)cpuRegs.GPR.r[_Rs_].UL[0] * (u64)cpuRegs.GPR.r[_Rt_].UL[0]);

		cpuRegs.LO.SD[1] = (s32)(tempu & 0xffffffff);
		cpuRegs.HI.SD[1] = (s32)(tempu >> 32);

		if (_Rd_) cpuRegs.GPR.r[_Rd_].SD[0] = cpuRegs.LO.SD[1];
	}

	void MFHI1() {
		if (!_Rd_) return;
		cpuRegs.GPR.r[_Rd_].UD[0] = cpuRegs.HI.UD[1];
	}

	void MFLO1() {
		if (!_Rd_) return;
		cpuRegs.GPR.r[_Rd_].UD[0] = cpuRegs.LO.UD[1];
	}

	void MTHI1() {
		cpuRegs.HI.UD[1] = cpuRegs.GPR.r[_Rs_].UD[0];
	}

	void MTLO1() {
		cpuRegs.LO.UD[1] = cpuRegs.GPR.r[_Rs_].UD[0];
	}

	void MULT1() {
		s64 temp = (s64)cpuRegs.GPR.r[_Rs_].SL[0] * cpuRegs.GPR.r[_Rt_].SL[0];

		// Sign-extend into 64 bits:
		cpuRegs.LO.SD[1] = (s32)(temp & 0xffffffff);
		cpuRegs.HI.SD[1] = (s32)(temp >> 32);

		if (_Rd_) cpuRegs.GPR.r[_Rd_].UD[0] = cpuRegs.LO.UD[1];
	}

	void MULTU1() {
		u64 tempu = (u64)cpuRegs.GPR.r[_Rs_].UL[0] * cpuRegs.GPR.r[_Rt_].UL[0];

		// According to docs, sign-extend into 64 bits even though it's an unsigned mult.
		cpuRegs.LO.SD[1] = (s32)(tempu & 0xffffffff);
		cpuRegs.HI.SD[1] = (s32)(tempu >> 32);

		if (_Rd_) cpuRegs.GPR.r[_Rd_].UD[0] = cpuRegs.LO.UD[1];
	}

	void DIV1() {
		if (cpuRegs.GPR.r[_Rs_].UL[0] == 0x80000000 && cpuRegs.GPR.r[_Rt_].UL[0] == 0xffffffff)
		{
				cpuRegs.LO.SD[1] = (s32)0x80000000;
				cpuRegs.HI.SD[1] = (s32)0x0;
		}
		else if (cpuRegs.GPR.r[_Rt_].SL[0] != 0)
		{
			cpuRegs.LO.SD[1] = cpuRegs.GPR.r[_Rs_].SL[0] / cpuRegs.GPR.r[_Rt_].SL[0];
			cpuRegs.HI.SD[1] = cpuRegs.GPR.r[_Rs_].SL[0] % cpuRegs.GPR.r[_Rt_].SL[0];
		}
		else
		{
			cpuRegs.LO.SD[1] = (cpuRegs.GPR.r[_Rs_].SL[0] < 0) ? 1 : -1;
			cpuRegs.HI.SD[1] = cpuRegs.GPR.r[_Rs_].SL[0];
		}
	}

	void DIVU1()
	{
		if (cpuRegs.GPR.r[_Rt_].UL[0] != 0)
		{
			// note: DIVU has no sign extension when assigning back to 64 bits
			// note 2: reference material strongly disagrees. (air)
			cpuRegs.LO.SD[1] = (s32)(cpuRegs.GPR.r[_Rs_].UL[0] / cpuRegs.GPR.r[_Rt_].UL[0]);
			cpuRegs.HI.SD[1] = (s32)(cpuRegs.GPR.r[_Rs_].UL[0] % cpuRegs.GPR.r[_Rt_].UL[0]);
		}
		else
		{
			cpuRegs.LO.SD[1] = -1;
			cpuRegs.HI.SD[1] = cpuRegs.GPR.r[_Rs_].SL[0];
		}
	}

namespace MMI {

//*****************MMI OPCODES*********************************

static __fi void _PLZCW(int n)
{
	// This function counts the number of "like" bits in the source register, starting
	// with the MSB and working its way down, and returns the result MINUS ONE.
	// So 0xff00 would return 7, not 8.

	int c = 0;
	s32 i = cpuRegs.GPR.r[_Rs_].SL[n];

	// Negate the source based on the sign bit.  This allows us to use a simple
	// unified bit test of the MSB for either condition.
	if( i >= 0 ) i = ~i;

	// shift first, compare, then increment.  This excludes the sign bit from our final count.
	while( i <<= 1, i < 0 ) c++;

	cpuRegs.GPR.r[_Rd_].UL[n] = c;
}

void PLZCW() {
    if (!_Rd_) return;

	_PLZCW (0);
	_PLZCW (1);
}

__fi void PMFHL_CLAMP(u16& dst, s32 src)
{
    if      (src >  0x7fff)	dst = 0x7fff;
    else if (src < -0x8000)	dst = 0x8000;
    else					dst = (u16)src;
}

void PMFHL() {
	if (!_Rd_) return;

	switch (_Sa_) {
		case 0x00: // LW
			cpuRegs.GPR.r[_Rd_].UL[0] = cpuRegs.LO.UL[0];
			cpuRegs.GPR.r[_Rd_].UL[1] = cpuRegs.HI.UL[0];
			cpuRegs.GPR.r[_Rd_].UL[2] = cpuRegs.LO.UL[2];
			cpuRegs.GPR.r[_Rd_].UL[3] = cpuRegs.HI.UL[2];
			break;

		case 0x01: // UW
			cpuRegs.GPR.r[_Rd_].UL[0] = cpuRegs.LO.UL[1];
			cpuRegs.GPR.r[_Rd_].UL[1] = cpuRegs.HI.UL[1];
			cpuRegs.GPR.r[_Rd_].UL[2] = cpuRegs.LO.UL[3];
			cpuRegs.GPR.r[_Rd_].UL[3] = cpuRegs.HI.UL[3];
			break;

		case 0x02: // SLW
			{
				s64 TempS64 = ((u64)cpuRegs.HI.UL[0] << 32) | (u64)cpuRegs.LO.UL[0];
				if (TempS64 >= 0x000000007fffffffLL) {
					cpuRegs.GPR.r[_Rd_].UD[0] = 0x000000007fffffffLL;
				} else if (TempS64 <= 0xffffffff80000000LL) {
					cpuRegs.GPR.r[_Rd_].UD[0] = 0xffffffff80000000LL;
				} else {
					cpuRegs.GPR.r[_Rd_].UD[0] = (s64)cpuRegs.LO.SL[0];
				}

				TempS64 = ((u64)cpuRegs.HI.UL[2] << 32) | (u64)cpuRegs.LO.UL[2];
				if (TempS64 >= 0x000000007fffffffLL) {
					cpuRegs.GPR.r[_Rd_].UD[1] = 0x000000007fffffffLL;
				} else if (TempS64 <= 0xffffffff80000000LL) {
					cpuRegs.GPR.r[_Rd_].UD[1] = 0xffffffff80000000LL;
				} else {
					cpuRegs.GPR.r[_Rd_].UD[1] = (s64)cpuRegs.LO.SL[2];
				}
			}
			break;

		case 0x03: // LH
			cpuRegs.GPR.r[_Rd_].US[0] = cpuRegs.LO.US[0];
			cpuRegs.GPR.r[_Rd_].US[1] = cpuRegs.LO.US[2];
			cpuRegs.GPR.r[_Rd_].US[2] = cpuRegs.HI.US[0];
			cpuRegs.GPR.r[_Rd_].US[3] = cpuRegs.HI.US[2];
			cpuRegs.GPR.r[_Rd_].US[4] = cpuRegs.LO.US[4];
			cpuRegs.GPR.r[_Rd_].US[5] = cpuRegs.LO.US[6];
			cpuRegs.GPR.r[_Rd_].US[6] = cpuRegs.HI.US[4];
			cpuRegs.GPR.r[_Rd_].US[7] = cpuRegs.HI.US[6];
			break;

		case 0x04: // SH
			PMFHL_CLAMP(cpuRegs.GPR.r[_Rd_].US[0], cpuRegs.LO.UL[0]);
			PMFHL_CLAMP(cpuRegs.GPR.r[_Rd_].US[1], cpuRegs.LO.UL[1]);
			PMFHL_CLAMP(cpuRegs.GPR.r[_Rd_].US[2], cpuRegs.HI.UL[0]);
			PMFHL_CLAMP(cpuRegs.GPR.r[_Rd_].US[3], cpuRegs.HI.UL[1]);
			PMFHL_CLAMP(cpuRegs.GPR.r[_Rd_].US[4], cpuRegs.LO.UL[2]);
			PMFHL_CLAMP(cpuRegs.GPR.r[_Rd_].US[5], cpuRegs.LO.UL[3]);
			PMFHL_CLAMP(cpuRegs.GPR.r[_Rd_].US[6], cpuRegs.HI.UL[2]);
			PMFHL_CLAMP(cpuRegs.GPR.r[_Rd_].US[7], cpuRegs.HI.UL[3]);
			break;
	}
}

void PMTHL() {
	if (_Sa_ != 0) return;

	cpuRegs.LO.UL[0] = cpuRegs.GPR.r[_Rs_].UL[0];
	cpuRegs.HI.UL[0] = cpuRegs.GPR.r[_Rs_].UL[1];
	cpuRegs.LO.UL[2] = cpuRegs.GPR.r[_Rs_].UL[2];
	cpuRegs.HI.UL[2] = cpuRegs.GPR.r[_Rs_].UL[3];
}

static __fi void _PSLLH(int n)
{
	cpuRegs.GPR.r[_Rd_].US[n] = cpuRegs.GPR.r[_Rt_].US[n] << ( _Sa_ & 0xf );
}

void PSLLH() {
	if (!_Rd_) return;

	_PSLLH(0); _PSLLH(1); _PSLLH(2); _PSLLH(3);
	_PSLLH(4); _PSLLH(5); _PSLLH(6); _PSLLH(7);
}

static __fi void _PSRLH(int n)
{
	cpuRegs.GPR.r[_Rd_].US[n] = cpuRegs.GPR.r[_Rt_].US[n] >> ( _Sa_ & 0xf );
}

void PSRLH () {
	if (!_Rd_) return;

	_PSRLH(0); _PSRLH(1); _PSRLH(2); _PSRLH(3);
	_PSRLH(4); _PSRLH(5); _PSRLH(6); _PSRLH(7);
}

static __fi void _PSRAH(int n)
{
	cpuRegs.GPR.r[_Rd_].US[n] = cpuRegs.GPR.r[_Rt_].SS[n] >> ( _Sa_ & 0xf );
}

void PSRAH() {
	if (!_Rd_) return;

	_PSRAH(0); _PSRAH(1); _PSRAH(2); _PSRAH(3);
	_PSRAH(4); _PSRAH(5); _PSRAH(6); _PSRAH(7);
}

static __fi void _PSLLW(int n)
{
	cpuRegs.GPR.r[_Rd_].UL[n] = cpuRegs.GPR.r[_Rt_].UL[n] << _Sa_;
}

void PSLLW() {
	if (!_Rd_) return;

	_PSLLW(0); _PSLLW(1); _PSLLW(2); _PSLLW(3);
}

static __fi void _PSRLW(int n)
{
	cpuRegs.GPR.r[_Rd_].UL[n] = cpuRegs.GPR.r[_Rt_].UL[n] >> _Sa_;
}

void PSRLW() {
	if (!_Rd_) return;

	_PSRLW(0); _PSRLW(1); _PSRLW(2); _PSRLW(3);
}

static __fi void _PSRAW(int n)
{
	cpuRegs.GPR.r[_Rd_].UL[n] = cpuRegs.GPR.r[_Rt_].SL[n] >> _Sa_;
}

void PSRAW() {
	if (!_Rd_) return;

	_PSRAW(0); _PSRAW(1); _PSRAW(2); _PSRAW(3);
}

//*****************END OF MMI OPCODES**************************
//*************************MMI0 OPCODES************************

static __fi void _PADDW(int n)
{
	cpuRegs.GPR.r[_Rd_].UL[n] = cpuRegs.GPR.r[_Rs_].UL[n] + cpuRegs.GPR.r[_Rt_].UL[n];
}

void PADDW() {
	if (!_Rd_) return;

	_PADDW(0); _PADDW(1); _PADDW(2); _PADDW(3);
}

static __fi void _PSUBW(int n)
{
	cpuRegs.GPR.r[_Rd_].UL[n] = cpuRegs.GPR.r[_Rs_].UL[n] - cpuRegs.GPR.r[_Rt_].UL[n];
}

void PSUBW() {
	if (!_Rd_) return;

	_PSUBW(0); _PSUBW(1); _PSUBW(2); _PSUBW(3);
}

static __fi void _PCGTW(int n)
{
	if (cpuRegs.GPR.r[_Rs_].SL[n] > cpuRegs.GPR.r[_Rt_].SL[n])
		cpuRegs.GPR.r[_Rd_].UL[n] = 0xFFFFFFFF;
	else
		cpuRegs.GPR.r[_Rd_].UL[n] = 0x00000000;
}

void PCGTW() {
	if (!_Rd_) return;

	_PCGTW(0);  _PCGTW(1);  _PCGTW(2);  _PCGTW(3);
}

static __fi void _PMAXW(int n)
{
	if (cpuRegs.GPR.r[_Rs_].SL[n] > cpuRegs.GPR.r[_Rt_].SL[n])
		cpuRegs.GPR.r[_Rd_].UL[n] = cpuRegs.GPR.r[_Rs_].UL[n];
	else
		cpuRegs.GPR.r[_Rd_].UL[n] = cpuRegs.GPR.r[_Rt_].UL[n];
}

void PMAXW() {
	if (!_Rd_) return;

	_PMAXW(0);  _PMAXW(1);  _PMAXW(2);  _PMAXW(3);
}

static __fi void _PADDH(int n)
{
	cpuRegs.GPR.r[_Rd_].US[n] = cpuRegs.GPR.r[_Rs_].US[n] + cpuRegs.GPR.r[_Rt_].US[n];
}

void PADDH() {
	if (!_Rd_) return;

	_PADDH(0);  _PADDH(1);  _PADDH(2);  _PADDH(3);
	_PADDH(4);  _PADDH(5);  _PADDH(6);  _PADDH(7);
}

static __fi void _PSUBH(int n)
{
	cpuRegs.GPR.r[_Rd_].US[n] = cpuRegs.GPR.r[_Rs_].US[n] - cpuRegs.GPR.r[_Rt_].US[n];
}

void PSUBH() {
	if (!_Rd_) return;

	_PSUBH(0);  _PSUBH(1);  _PSUBH(2);  _PSUBH(3);
	_PSUBH(4);  _PSUBH(5);  _PSUBH(6);  _PSUBH(7);
}

static __fi void _PCGTH(int n)
{
	if (cpuRegs.GPR.r[_Rs_].SS[n] > cpuRegs.GPR.r[_Rt_].SS[n])
		cpuRegs.GPR.r[_Rd_].US[n] = 0xFFFF;
	else
		cpuRegs.GPR.r[_Rd_].US[n] = 0x0000;
}

void PCGTH() {
	if (!_Rd_) return;

	_PCGTH(0);  _PCGTH(1);  _PCGTH(2);  _PCGTH(3);
	_PCGTH(4);  _PCGTH(5);  _PCGTH(6);  _PCGTH(7);
}

static __fi void _PMAXH(int n)
{
	if (cpuRegs.GPR.r[_Rs_].SS[n] > cpuRegs.GPR.r[_Rt_].SS[n])
		cpuRegs.GPR.r[_Rd_].US[n] =  cpuRegs.GPR.r[_Rs_].US[n];
	else
		cpuRegs.GPR.r[_Rd_].US[n] = cpuRegs.GPR.r[_Rt_].US[n];
}

void PMAXH() {
	if (!_Rd_) return;

	_PMAXH(0);  _PMAXH(1);  _PMAXH(2);  _PMAXH(3);
	_PMAXH(4);  _PMAXH(5);  _PMAXH(6);  _PMAXH(7);
}

static __fi void _PADDB(int n)
{
	cpuRegs.GPR.r[_Rd_].SC[n] = cpuRegs.GPR.r[_Rs_].SC[n] + cpuRegs.GPR.r[_Rt_].SC[n];
}

void PADDB() {
	int i;
	if (!_Rd_) return;

	for( i=0; i<16; i++ )
		_PADDB( i );
}

static __fi void _PSUBB(int n)
{
	cpuRegs.GPR.r[_Rd_].SC[n] = cpuRegs.GPR.r[_Rs_].SC[n] - cpuRegs.GPR.r[_Rt_].SC[n];
}

void PSUBB() {
	int i;
	if (!_Rd_) return;

	for( i=0; i<16; i++ )
		_PSUBB( i );
}

static __fi void _PCGTB(int n)
{
	if (cpuRegs.GPR.r[_Rs_].SC[n] > cpuRegs.GPR.r[_Rt_].SC[n])
		cpuRegs.GPR.r[_Rd_].UC[n] = 0xFF;
	else
		cpuRegs.GPR.r[_Rd_].UC[n] = 0x00;
}

void PCGTB() {
	int i;
	if (!_Rd_) return;

	for( i=0; i<16; i++ )
		_PCGTB( i );
}

static __fi void _PADDSW(int n)
{
	s64 sTemp64;

	sTemp64 = (s64)cpuRegs.GPR.r[_Rs_].SL[n] + (s64)cpuRegs.GPR.r[_Rt_].SL[n];
	if (sTemp64 > 0x7FFFFFFF)
		cpuRegs.GPR.r[_Rd_].UL[n] = 0x7FFFFFFF;
	else if ((sTemp64 < (s32)0x80000000) )
		cpuRegs.GPR.r[_Rd_].UL[n] = 0x80000000LL;
	else
		cpuRegs.GPR.r[_Rd_].UL[n] = (s32)sTemp64;
}

void PADDSW() {
	if (!_Rd_) return;

	_PADDSW(0); _PADDSW(1); _PADDSW(2); _PADDSW(3);
}

static __fi void _PSUBSW(int n)
{
	s64 sTemp64;

	sTemp64 = (s64)cpuRegs.GPR.r[_Rs_].SL[n] - (s64)cpuRegs.GPR.r[_Rt_].SL[n];

	if (sTemp64 >= 0x7FFFFFFF)
		cpuRegs.GPR.r[_Rd_].UL[n] = 0x7FFFFFFF;
	else if ((sTemp64 < (s32)0x80000000))
		cpuRegs.GPR.r[_Rd_].UL[n] = 0x80000000;
	else
		cpuRegs.GPR.r[_Rd_].UL[n] = (s32)sTemp64;
}

void PSUBSW() {
	if (!_Rd_) return;

	_PSUBSW(0);
	_PSUBSW(1);
	_PSUBSW(2);
	_PSUBSW(3);
}

void PEXTLW() {
	GPR_reg Rs, Rt;

	if (!_Rd_) return;

	Rs = cpuRegs.GPR.r[_Rs_]; Rt = cpuRegs.GPR.r[_Rt_];
	cpuRegs.GPR.r[_Rd_].UL[0] = Rt.UL[0];
	cpuRegs.GPR.r[_Rd_].UL[1] = Rs.UL[0];
	cpuRegs.GPR.r[_Rd_].UL[2] = Rt.UL[1];
	cpuRegs.GPR.r[_Rd_].UL[3] = Rs.UL[1];
}

void PPACW() {
	GPR_reg Rs, Rt;

	if (!_Rd_) return;

	Rs = cpuRegs.GPR.r[_Rs_]; Rt = cpuRegs.GPR.r[_Rt_];
	cpuRegs.GPR.r[_Rd_].UL[0] = Rt.UL[0];
	cpuRegs.GPR.r[_Rd_].UL[1] = Rt.UL[2];
	cpuRegs.GPR.r[_Rd_].UL[2] = Rs.UL[0];
	cpuRegs.GPR.r[_Rd_].UL[3] = Rs.UL[2];
}

__fi void  _PADDSH(int n)
{
	s32 sTemp32;
	sTemp32 = (s32)cpuRegs.GPR.r[_Rs_].SS[n] + (s32)cpuRegs.GPR.r[_Rt_].SS[n];

	if (sTemp32 > 0x7FFF)
		cpuRegs.GPR.r[_Rd_].US[n] = 0x7FFF;
	else if ((sTemp32 < (s32)0xffff8000) )
		cpuRegs.GPR.r[_Rd_].US[n] = 0x8000;
	else
		cpuRegs.GPR.r[_Rd_].US[n] = (s16)sTemp32;
}

void PADDSH() {
	if (!_Rd_) return;

	_PADDSH(0); _PADDSH(1); _PADDSH(2); _PADDSH(3);
	_PADDSH(4); _PADDSH(5); _PADDSH(6); _PADDSH(7);
}

__fi void  _PSUBSH(int n)
{
	s32 sTemp32;
	sTemp32 = (s32)cpuRegs.GPR.r[_Rs_].SS[n] - (s32)cpuRegs.GPR.r[_Rt_].SS[n];

	if (sTemp32 >= 0x7FFF)
		cpuRegs.GPR.r[_Rd_].US[n] = 0x7FFF;
	else if ((sTemp32 < (s32)0xffff8000) )
		cpuRegs.GPR.r[_Rd_].US[n] = 0x8000;
	else
		cpuRegs.GPR.r[_Rd_].US[n] = (s16)sTemp32;
}

void PSUBSH() {
	if (!_Rd_) return;

	_PSUBSH(0); _PSUBSH(1); _PSUBSH(2); _PSUBSH(3);
	_PSUBSH(4); _PSUBSH(5); _PSUBSH(6); _PSUBSH(7);
}

void PEXTLH() {
	GPR_reg Rs, Rt;

	if (!_Rd_) return;

	Rs = cpuRegs.GPR.r[_Rs_]; Rt = cpuRegs.GPR.r[_Rt_];
	cpuRegs.GPR.r[_Rd_].US[0] = Rt.US[0];
	cpuRegs.GPR.r[_Rd_].US[1] = Rs.US[0];
	cpuRegs.GPR.r[_Rd_].US[2] = Rt.US[1];
	cpuRegs.GPR.r[_Rd_].US[3] = Rs.US[1];
	cpuRegs.GPR.r[_Rd_].US[4] = Rt.US[2];
	cpuRegs.GPR.r[_Rd_].US[5] = Rs.US[2];
	cpuRegs.GPR.r[_Rd_].US[6] = Rt.US[3];
	cpuRegs.GPR.r[_Rd_].US[7] = Rs.US[3];
}

void PPACH() {
	GPR_reg Rs, Rt;

	if (!_Rd_) return;

	Rs = cpuRegs.GPR.r[_Rs_]; Rt = cpuRegs.GPR.r[_Rt_];
	cpuRegs.GPR.r[_Rd_].US[0] = Rt.US[0];
	cpuRegs.GPR.r[_Rd_].US[1] = Rt.US[2];
	cpuRegs.GPR.r[_Rd_].US[2] = Rt.US[4];
	cpuRegs.GPR.r[_Rd_].US[3] = Rt.US[6];
	cpuRegs.GPR.r[_Rd_].US[4] = Rs.US[0];
	cpuRegs.GPR.r[_Rd_].US[5] = Rs.US[2];
	cpuRegs.GPR.r[_Rd_].US[6] = Rs.US[4];
	cpuRegs.GPR.r[_Rd_].US[7] = Rs.US[6];
}

__fi void  _PADDSB(int n)
{
	s16 sTemp16;
	sTemp16 = (s16)cpuRegs.GPR.r[_Rs_].SC[n] + (s16)cpuRegs.GPR.r[_Rt_].SC[n];

	if (sTemp16 > 0x7F)
		cpuRegs.GPR.r[_Rd_].UC[n] = 0x7F;
	else if (sTemp16 < /*(s16)0xff80*/(s16)-128) // be sure
		cpuRegs.GPR.r[_Rd_].UC[n] = 0x80;
	else
		cpuRegs.GPR.r[_Rd_].UC[n] = (s8)sTemp16;
}

void PADDSB() {
	int i;
	if (!_Rd_) return;

	for( i=0; i<16; i++ )
		_PADDSB(i);
}

static __fi void _PSUBSB( u8 n )
{
	s16 sTemp16;
	sTemp16 = (s16)cpuRegs.GPR.r[_Rs_].SC[n] - (s16)cpuRegs.GPR.r[_Rt_].SC[n];

	if (sTemp16 >= 0x7F)
		cpuRegs.GPR.r[_Rd_].UC[n] = 0x7F;
	else if (sTemp16 < /*(s16)0xff80*/(s16)-128) // be sure
		cpuRegs.GPR.r[_Rd_].UC[n] = 0x80;
	else
		cpuRegs.GPR.r[_Rd_].UC[n] = (s8)sTemp16;
}

void PSUBSB() {
	int i;
	if (!_Rd_) return;

	for( i=0; i<16; i++ )
		_PSUBSB(i);
}

void PEXTLB() {
	GPR_reg Rs, Rt;

	if (!_Rd_) return;

	Rs = cpuRegs.GPR.r[_Rs_]; Rt = cpuRegs.GPR.r[_Rt_];
	cpuRegs.GPR.r[_Rd_].UC[0]  = Rt.UC[0];
	cpuRegs.GPR.r[_Rd_].UC[1]  = Rs.UC[0];
	cpuRegs.GPR.r[_Rd_].UC[2]  = Rt.UC[1];
	cpuRegs.GPR.r[_Rd_].UC[3]  = Rs.UC[1];

	cpuRegs.GPR.r[_Rd_].UC[4]  = Rt.UC[2];
	cpuRegs.GPR.r[_Rd_].UC[5]  = Rs.UC[2];
	cpuRegs.GPR.r[_Rd_].UC[6]  = Rt.UC[3];
	cpuRegs.GPR.r[_Rd_].UC[7]  = Rs.UC[3];

	cpuRegs.GPR.r[_Rd_].UC[8]  = Rt.UC[4];
	cpuRegs.GPR.r[_Rd_].UC[9]  = Rs.UC[4];
	cpuRegs.GPR.r[_Rd_].UC[10] = Rt.UC[5];
	cpuRegs.GPR.r[_Rd_].UC[11] = Rs.UC[5];

	cpuRegs.GPR.r[_Rd_].UC[12] = Rt.UC[6];
	cpuRegs.GPR.r[_Rd_].UC[13] = Rs.UC[6];
	cpuRegs.GPR.r[_Rd_].UC[14] = Rt.UC[7];
	cpuRegs.GPR.r[_Rd_].UC[15] = Rs.UC[7];
}

void PPACB() {
	GPR_reg Rs, Rt;

	if (!_Rd_) return;

	Rs = cpuRegs.GPR.r[_Rs_]; Rt = cpuRegs.GPR.r[_Rt_];
	cpuRegs.GPR.r[_Rd_].UC[0]  = Rt.UC[0];
	cpuRegs.GPR.r[_Rd_].UC[1]  = Rt.UC[2];
	cpuRegs.GPR.r[_Rd_].UC[2]  = Rt.UC[4];
	cpuRegs.GPR.r[_Rd_].UC[3]  = Rt.UC[6];

	cpuRegs.GPR.r[_Rd_].UC[4]  = Rt.UC[8];
	cpuRegs.GPR.r[_Rd_].UC[5]  = Rt.UC[10];
	cpuRegs.GPR.r[_Rd_].UC[6]  = Rt.UC[12];
	cpuRegs.GPR.r[_Rd_].UC[7]  = Rt.UC[14];

	cpuRegs.GPR.r[_Rd_].UC[8]  = Rs.UC[0];
	cpuRegs.GPR.r[_Rd_].UC[9]  = Rs.UC[2];
	cpuRegs.GPR.r[_Rd_].UC[10] = Rs.UC[4];
	cpuRegs.GPR.r[_Rd_].UC[11] = Rs.UC[6];

	cpuRegs.GPR.r[_Rd_].UC[12] = Rs.UC[8];
	cpuRegs.GPR.r[_Rd_].UC[13] = Rs.UC[10];
	cpuRegs.GPR.r[_Rd_].UC[14] = Rs.UC[12];
	cpuRegs.GPR.r[_Rd_].UC[15] = Rs.UC[14];
}

__fi void  _PEXT5(int n)
{
	cpuRegs.GPR.r[_Rd_].UL[n] =
		((cpuRegs.GPR.r[_Rt_].UL[n] & 0x0000001F) <<  3) |
		((cpuRegs.GPR.r[_Rt_].UL[n] & 0x000003E0) <<  6) |
		((cpuRegs.GPR.r[_Rt_].UL[n] & 0x00007C00) <<  9) |
		((cpuRegs.GPR.r[_Rt_].UL[n] & 0x00008000) << 16);
}

void PEXT5() {
	if (!_Rd_) return;

	_PEXT5(0); _PEXT5(1); _PEXT5(2); _PEXT5(3);
}

__fi void  _PPAC5(int n)
{
	cpuRegs.GPR.r[_Rd_].UL[n] =
		((cpuRegs.GPR.r[_Rt_].UL[n] >>  3) & 0x0000001F) |
		((cpuRegs.GPR.r[_Rt_].UL[n] >>  6) & 0x000003E0) |
		((cpuRegs.GPR.r[_Rt_].UL[n] >>  9) & 0x00007C00) |
		((cpuRegs.GPR.r[_Rt_].UL[n] >> 16) & 0x00008000);
}

void PPAC5() {
	if (!_Rd_) return;

	_PPAC5(0); _PPAC5(1); _PPAC5(2); _PPAC5(3);
}

//***END OF MMI0 OPCODES******************************************
//**********MMI1 OPCODES**************************************

static __fi void _PABSW(int n)
{
	if (cpuRegs.GPR.r[_Rt_].UL[n] == 0x80000000)
		cpuRegs.GPR.r[_Rd_].UL[n] = 0x7fffffff; //clamp
	else if (cpuRegs.GPR.r[_Rt_].SL[n] < 0)
		cpuRegs.GPR.r[_Rd_].UL[n] = - cpuRegs.GPR.r[_Rt_].SL[n];
	else
		cpuRegs.GPR.r[_Rd_].UL[n] = cpuRegs.GPR.r[_Rt_].SL[n];
}

void PABSW() {
	if (!_Rd_) return;

	_PABSW(0);  _PABSW(1);  _PABSW(2);  _PABSW(3);
}

static __fi void _PCEQW(int n)
{
	if (cpuRegs.GPR.r[_Rs_].UL[n] == cpuRegs.GPR.r[_Rt_].UL[n])
		cpuRegs.GPR.r[_Rd_].UL[n] = 0xFFFFFFFF;
	else
		cpuRegs.GPR.r[_Rd_].UL[n] = 0x00000000;
}

void PCEQW() {
	if (!_Rd_) return;

	_PCEQW(0); _PCEQW(1); _PCEQW(2); _PCEQW(3);
}

static __fi void _PMINW( u8 n )
{
	if (cpuRegs.GPR.r[_Rs_].SL[n] < cpuRegs.GPR.r[_Rt_].SL[n])
		cpuRegs.GPR.r[_Rd_].SL[n] = cpuRegs.GPR.r[_Rs_].SL[n];
	else
		cpuRegs.GPR.r[_Rd_].SL[n] = cpuRegs.GPR.r[_Rt_].SL[n];
}

void PMINW() {
	if (!_Rd_) return;

	_PMINW(0); _PMINW(1); _PMINW(2); _PMINW(3);
}

void PADSBH() {
	if (!_Rd_) return;

	_PSUBH(0); _PSUBH(1); _PSUBH(2); _PSUBH(3);
	_PADDH(4); _PADDH(5); _PADDH(6); _PADDH(7);
}

static __fi void _PABSH(int n)
{
	if (cpuRegs.GPR.r[_Rt_].US[n] == 0x8000)
		cpuRegs.GPR.r[_Rd_].US[n] = 0x7fff; //clamp
	else if (cpuRegs.GPR.r[_Rt_].SS[n] < 0)
		cpuRegs.GPR.r[_Rd_].US[n] = - cpuRegs.GPR.r[_Rt_].SS[n];
	else
		cpuRegs.GPR.r[_Rd_].US[n] = cpuRegs.GPR.r[_Rt_].SS[n];
}

void PABSH() {
	if (!_Rd_) return;

	_PABSH(0); _PABSH(1); _PABSH(2); _PABSH(3);
	_PABSH(4); _PABSH(5); _PABSH(6); _PABSH(7);
}

static __fi void  _PCEQH( u8 n )
{
	if (cpuRegs.GPR.r[_Rs_].US[n] == cpuRegs.GPR.r[_Rt_].US[n])
		cpuRegs.GPR.r[_Rd_].US[n] = 0xFFFF;
	else
		cpuRegs.GPR.r[_Rd_].US[n] = 0x0000;
}

void PCEQH() {
	if (!_Rd_) return;

	_PCEQH(0); _PCEQH(1); _PCEQH(2); _PCEQH(3);
	_PCEQH(4); _PCEQH(5); _PCEQH(6); _PCEQH(7);
}

static __fi void  _PMINH( u8 n )
{
	if (cpuRegs.GPR.r[_Rs_].SS[n] < cpuRegs.GPR.r[_Rt_].SS[n])
		cpuRegs.GPR.r[_Rd_].US[n] = cpuRegs.GPR.r[_Rs_].US[n];
	else
		cpuRegs.GPR.r[_Rd_].US[n] = cpuRegs.GPR.r[_Rt_].US[n];
}

void PMINH() {
	if (!_Rd_) return;

	_PMINH(0); _PMINH(1); _PMINH(2); _PMINH(3);
	_PMINH(4); _PMINH(5); _PMINH(6); _PMINH(7);
}

__fi void  _PCEQB(int n)
{
	if (cpuRegs.GPR.r[_Rs_].UC[n] == cpuRegs.GPR.r[_Rt_].UC[n])
		cpuRegs.GPR.r[_Rd_].UC[n] = 0xFF;
	else
		cpuRegs.GPR.r[_Rd_].UC[n] = 0x00;
}

void PCEQB() {
	int i;
	if (!_Rd_) return;

	for( i=0; i<16; i++ )
		_PCEQB(i);
}

__fi void  _PADDUW(int n)
{
	s64 tmp;
	tmp = (s64)cpuRegs.GPR.r[_Rs_].UL[n] + (s64)cpuRegs.GPR.r[_Rt_].UL[n];

	if (tmp > 0xffffffff)
		 cpuRegs.GPR.r[_Rd_].UL[n] = 0xffffffff;
	else
		cpuRegs.GPR.r[_Rd_].UL[n] = (u32)tmp;
}

void PADDUW () {
	if (!_Rd_) return;

	_PADDUW(0); _PADDUW(1); _PADDUW(2); _PADDUW(3);
}

__fi void  _PSUBUW(int n)
{
	s64 sTemp64;
	sTemp64 = (s64)cpuRegs.GPR.r[_Rs_].UL[n] - (s64)cpuRegs.GPR.r[_Rt_].UL[n];

	if (sTemp64 <= 0x0)
		cpuRegs.GPR.r[_Rd_].UL[n] = 0x0;
	else
		cpuRegs.GPR.r[_Rd_].UL[n] = (u32)sTemp64;
}

void PSUBUW() {
	if (!_Rd_) return;

	_PSUBUW(0); _PSUBUW(1); _PSUBUW(2); _PSUBUW(3);
}

void PEXTUW() {
	GPR_reg Rs, Rt;

	if (!_Rd_) return;

	Rs = cpuRegs.GPR.r[_Rs_]; Rt = cpuRegs.GPR.r[_Rt_];
	cpuRegs.GPR.r[_Rd_].UL[0] = Rt.UL[2];
	cpuRegs.GPR.r[_Rd_].UL[1] = Rs.UL[2];
	cpuRegs.GPR.r[_Rd_].UL[2] = Rt.UL[3];
	cpuRegs.GPR.r[_Rd_].UL[3] = Rs.UL[3];
}

__fi void  _PADDUH(int n)
{
	s32 sTemp32;
	sTemp32 = (s32)cpuRegs.GPR.r[_Rs_].US[n] + (s32)cpuRegs.GPR.r[_Rt_].US[n];

	if (sTemp32 > 0xFFFF)
		cpuRegs.GPR.r[_Rd_].US[n] = 0xFFFF;
	else
		cpuRegs.GPR.r[_Rd_].US[n] = (u16)sTemp32;
}

void PADDUH() {
	if (!_Rd_) return;

	_PADDUH(0); _PADDUH(1); _PADDUH(2); _PADDUH(3);
	_PADDUH(4); _PADDUH(5); _PADDUH(6); _PADDUH(7);
}

__fi void  _PSUBUH(int n)
{
	s32 sTemp32;
	sTemp32 = (s32)cpuRegs.GPR.r[_Rs_].US[n] - (s32)cpuRegs.GPR.r[_Rt_].US[n];

	if (sTemp32 <= 0x0)
		cpuRegs.GPR.r[_Rd_].US[n] = 0x0;
	else
		cpuRegs.GPR.r[_Rd_].US[n] = (u16)sTemp32;
}

void PSUBUH() {
	if (!_Rd_) return;

	_PSUBUH(0); _PSUBUH(1); _PSUBUH(2); _PSUBUH(3);
	_PSUBUH(4); _PSUBUH(5); _PSUBUH(6); _PSUBUH(7);
}

void PEXTUH() {
	GPR_reg Rs, Rt;

	if (!_Rd_) return;

	Rs = cpuRegs.GPR.r[_Rs_]; Rt = cpuRegs.GPR.r[_Rt_];
	cpuRegs.GPR.r[_Rd_].US[0] = Rt.US[4];
	cpuRegs.GPR.r[_Rd_].US[1] = Rs.US[4];
	cpuRegs.GPR.r[_Rd_].US[2] = Rt.US[5];
	cpuRegs.GPR.r[_Rd_].US[3] = Rs.US[5];

	cpuRegs.GPR.r[_Rd_].US[4] = Rt.US[6];
	cpuRegs.GPR.r[_Rd_].US[5] = Rs.US[6];
	cpuRegs.GPR.r[_Rd_].US[6] = Rt.US[7];
	cpuRegs.GPR.r[_Rd_].US[7] = Rs.US[7];
}

__fi void  _PADDUB(int n)
{
	u16 Temp16;
	Temp16 = (u16)cpuRegs.GPR.r[_Rs_].UC[n] + (u16)cpuRegs.GPR.r[_Rt_].UC[n];

	if (Temp16 > 0xFF)
		cpuRegs.GPR.r[_Rd_].UC[n] = 0xFF;
	else
		cpuRegs.GPR.r[_Rd_].UC[n] = (u8)Temp16;
}

void PADDUB() {
	int i;
	if (!_Rd_) return;

	for( i=0; i<16; i++ )
		 _PADDUB(i);
}

__fi void  _PSUBUB(int n) {
	s16 sTemp16;
	sTemp16 = (s16)cpuRegs.GPR.r[_Rs_].UC[n] - (s16)cpuRegs.GPR.r[_Rt_].UC[n];

	if (sTemp16 <= 0x0)
		cpuRegs.GPR.r[_Rd_].UC[n] = 0x0;
	else
		cpuRegs.GPR.r[_Rd_].UC[n] = (u8)sTemp16;
}

void PSUBUB() {
	int i;
	if (!_Rd_) return;

	for( i=0; i<16; i++ )
		 _PSUBUB(i);
}

void PEXTUB() {
	GPR_reg Rs, Rt;

	if (!_Rd_) return;

	Rs = cpuRegs.GPR.r[_Rs_]; Rt = cpuRegs.GPR.r[_Rt_];
	cpuRegs.GPR.r[_Rd_].UC[0]  = Rt.UC[8];
	cpuRegs.GPR.r[_Rd_].UC[1]  = Rs.UC[8];
	cpuRegs.GPR.r[_Rd_].UC[2]  = Rt.UC[9];
	cpuRegs.GPR.r[_Rd_].UC[3]  = Rs.UC[9];
	cpuRegs.GPR.r[_Rd_].UC[4]  = Rt.UC[10];
	cpuRegs.GPR.r[_Rd_].UC[5]  = Rs.UC[10];
	cpuRegs.GPR.r[_Rd_].UC[6]  = Rt.UC[11];
	cpuRegs.GPR.r[_Rd_].UC[7]  = Rs.UC[11];
	cpuRegs.GPR.r[_Rd_].UC[8]  = Rt.UC[12];
	cpuRegs.GPR.r[_Rd_].UC[9]  = Rs.UC[12];
	cpuRegs.GPR.r[_Rd_].UC[10] = Rt.UC[13];
	cpuRegs.GPR.r[_Rd_].UC[11] = Rs.UC[13];
	cpuRegs.GPR.r[_Rd_].UC[12] = Rt.UC[14];
	cpuRegs.GPR.r[_Rd_].UC[13] = Rs.UC[14];
	cpuRegs.GPR.r[_Rd_].UC[14] = Rt.UC[15];
	cpuRegs.GPR.r[_Rd_].UC[15] = Rs.UC[15];
}

//int saZero = 0;
void QFSRV() {				// JayteeMaster: changed a bit to avoid screw up
	GPR_reg Rd;
	if (!_Rd_) return;

	u32 sa_amt = cpuRegs.sa << 3;
	if (sa_amt == 0) {
		cpuRegs.GPR.r[_Rd_].UD[0] = cpuRegs.GPR.r[_Rt_].UD[0];
		cpuRegs.GPR.r[_Rd_].UD[1] = cpuRegs.GPR.r[_Rt_].UD[1];
		//saZero++;
		//if( saZero >= 388800 )
			//Console.WriteLn( "SA Is Zero, Bitch: %d zeros and counting.", saZero );
	} else {
		//Console.WriteLn( "SA Properly Valued at: %d (after %d zeros)", sa_amt, saZero );
		//saZero = 0;
		if (sa_amt < 64) {
			/*
			cpuRegs.GPR.r[_Rd_].UD[0] = cpuRegs.GPR.r[_Rt_].UD[0] >> sa_amt;
			cpuRegs.GPR.r[_Rd_].UD[1] = cpuRegs.GPR.r[_Rt_].UD[1] >> sa_amt;
			cpuRegs.GPR.r[_Rd_].UD[0]|= cpuRegs.GPR.r[_Rt_].UD[1] << (64 - sa_amt);
			cpuRegs.GPR.r[_Rd_].UD[1]|= cpuRegs.GPR.r[_Rs_].UD[0] << (64 - sa_amt);
			*/
			Rd.UD[0] = cpuRegs.GPR.r[_Rt_].UD[0] >> sa_amt;
			Rd.UD[1] = cpuRegs.GPR.r[_Rt_].UD[1] >> sa_amt;
			Rd.UD[0]|= cpuRegs.GPR.r[_Rt_].UD[1] << (64 - sa_amt);
			Rd.UD[1]|= cpuRegs.GPR.r[_Rs_].UD[0] << (64 - sa_amt);
			cpuRegs.GPR.r[_Rd_] = Rd;
		} else {
			/*
			cpuRegs.GPR.r[_Rd_].UD[0] = cpuRegs.GPR.r[_Rt_].UD[1] >> (sa_amt - 64);
			cpuRegs.GPR.r[_Rd_].UD[1] = cpuRegs.GPR.r[_Rs_].UD[0] >> (sa_amt - 64);
			cpuRegs.GPR.r[_Rd_].UD[0]|= cpuRegs.GPR.r[_Rs_].UD[0] << (128 - sa_amt);
			cpuRegs.GPR.r[_Rd_].UD[1]|= cpuRegs.GPR.r[_Rs_].UD[1] << (128 - sa_amt);
			*/
			Rd.UD[0] = cpuRegs.GPR.r[_Rt_].UD[1] >> (sa_amt - 64);
			Rd.UD[1] = cpuRegs.GPR.r[_Rs_].UD[0] >> (sa_amt - 64);
			Rd.UD[0]|= cpuRegs.GPR.r[_Rs_].UD[0] << (128 - sa_amt);
			Rd.UD[1]|= cpuRegs.GPR.r[_Rs_].UD[1] << (128 - sa_amt);
			cpuRegs.GPR.r[_Rd_] = Rd;
		}
	}
}

//********END OF MMI1 OPCODES***********************************

//*********MMI2 OPCODES***************************************

static __fi void _PMADDW(int dd, int ss)
{
	s64 temp = (s64)((s64)cpuRegs.LO.SL[ss] | ((s64)cpuRegs.HI.SL[ss] << 32)) +
			   ((s64)cpuRegs.GPR.r[_Rs_].SL[ss] * (s64)cpuRegs.GPR.r[_Rt_].SL[ss]);

	cpuRegs.LO.SD[dd] = (s32)(temp & 0xffffffff);
	cpuRegs.HI.SD[dd] = (s32)(temp >> 32);

	if (_Rd_) cpuRegs.GPR.r[_Rd_].SD[dd] = temp;
}

void PMADDW() {
	_PMADDW(0, 0);
	_PMADDW(1, 2);
}

void PSLLVW() {
	if (!_Rd_) return;

	cpuRegs.GPR.r[_Rd_].SD[0] = (s64)(s32)(cpuRegs.GPR.r[_Rt_].UL[0] <<
										  (cpuRegs.GPR.r[_Rs_].UL[0] & 0x1F));
	cpuRegs.GPR.r[_Rd_].SD[1] = (s64)(s32)(cpuRegs.GPR.r[_Rt_].UL[2] <<
										  (cpuRegs.GPR.r[_Rs_].UL[2] & 0x1F));
}

void PSRLVW() {
	if (!_Rd_) return;

	cpuRegs.GPR.r[_Rd_].SD[0] = (s64)(s32)(cpuRegs.GPR.r[_Rt_].UL[0] >>
										  (cpuRegs.GPR.r[_Rs_].UL[0] & 0x1F));
	cpuRegs.GPR.r[_Rd_].SD[1] = (s64)(s32)(cpuRegs.GPR.r[_Rt_].UL[2] >>
										  (cpuRegs.GPR.r[_Rs_].UL[2] & 0x1F));
}

__fi void  _PMSUBW(int dd, int ss)
{
	s64 temp = (s64)((s64)cpuRegs.LO.SL[ss] | ((s64)cpuRegs.HI.SL[ss] << 32)) -
			   ((s64)cpuRegs.GPR.r[_Rs_].SL[ss] * (s64)cpuRegs.GPR.r[_Rt_].SL[ss]);

	cpuRegs.LO.SD[dd] = (s32)(temp & 0xffffffff);
	cpuRegs.HI.SD[dd] = (s32)(temp >> 32);

	if (_Rd_) cpuRegs.GPR.r[_Rd_].SD[dd] = temp;
}

void PMSUBW() {
	_PMSUBW(0, 0);
	_PMSUBW(1, 2);
}

void PMFHI() {
	if (!_Rd_) return;

	cpuRegs.GPR.r[_Rd_].UD[0] = cpuRegs.HI.UD[0];
	cpuRegs.GPR.r[_Rd_].UD[1] = cpuRegs.HI.UD[1];
}

void PMFLO() {
	if (!_Rd_) return;

	cpuRegs.GPR.r[_Rd_].UD[0] = cpuRegs.LO.UD[0];
	cpuRegs.GPR.r[_Rd_].UD[1] = cpuRegs.LO.UD[1];
}

void PINTH() {
	GPR_reg Rs, Rt;

	if (!_Rd_) return;

	Rs = cpuRegs.GPR.r[_Rs_]; Rt = cpuRegs.GPR.r[_Rt_];
	cpuRegs.GPR.r[_Rd_].US[0] = Rt.US[0];
	cpuRegs.GPR.r[_Rd_].US[1] = Rs.US[4];
	cpuRegs.GPR.r[_Rd_].US[2] = Rt.US[1];
	cpuRegs.GPR.r[_Rd_].US[3] = Rs.US[5];
	cpuRegs.GPR.r[_Rd_].US[4] = Rt.US[2];
	cpuRegs.GPR.r[_Rd_].US[5] = Rs.US[6];
	cpuRegs.GPR.r[_Rd_].US[6] = Rt.US[3];
	cpuRegs.GPR.r[_Rd_].US[7] = Rs.US[7];
}

__fi void  _PMULTW(int dd, int ss)
{
	s64 temp = (s64)cpuRegs.GPR.r[_Rs_].SL[ss] * (s64)cpuRegs.GPR.r[_Rt_].SL[ss];

	cpuRegs.LO.UD[dd] = (s32)(temp & 0xffffffff);
	cpuRegs.HI.UD[dd] = (s32)(temp >> 32);

	if (_Rd_) cpuRegs.GPR.r[_Rd_].SD[dd] = temp;
}

void PMULTW() {
	_PMULTW(0, 0);
	_PMULTW(1, 2);
}

__fi void  _PDIVW(int dd, int ss)
{
	if (cpuRegs.GPR.r[_Rs_].UL[ss] == 0x80000000 && cpuRegs.GPR.r[_Rt_].UL[ss] == 0xffffffff)
	{
		cpuRegs.LO.SD[dd] = (s32)0x80000000;
		cpuRegs.HI.SD[dd] = (s32)0;
	}
	else if (cpuRegs.GPR.r[_Rt_].SL[ss] != 0)
	{
		cpuRegs.LO.SD[dd] = cpuRegs.GPR.r[_Rs_].SL[ss] / cpuRegs.GPR.r[_Rt_].SL[ss];
		cpuRegs.HI.SD[dd] = cpuRegs.GPR.r[_Rs_].SL[ss] % cpuRegs.GPR.r[_Rt_].SL[ss];
	}
	else
	{
		cpuRegs.LO.SD[dd] = (cpuRegs.GPR.r[_Rs_].SL[ss] < 0) ? 1 : -1;
		cpuRegs.HI.SD[dd] = cpuRegs.GPR.r[_Rs_].SL[ss];
	}
}

void PDIVW() {
	_PDIVW(0, 0);
	_PDIVW(1, 2);
}

void PCPYLD() {
	if (!_Rd_) return;

	// note: first _Rs_, since the other way when _Rd_ equals
	// _Rs_ or _Rt_ this would screw up
	cpuRegs.GPR.r[_Rd_].UD[1] = cpuRegs.GPR.r[_Rs_].UD[0];
	cpuRegs.GPR.r[_Rd_].UD[0] = cpuRegs.GPR.r[_Rt_].UD[0];
}

void PMADDH() {			// JayteeMaster: changed a bit to avoid screw up
	s32 temp;

    temp = cpuRegs.LO.UL[0] + (s32)cpuRegs.GPR.r[_Rs_].SS[0] * (s32)cpuRegs.GPR.r[_Rt_].SS[0];
    cpuRegs.LO.UL[0] = temp;
    /* if (_Rd_) cpuRegs.GPR.r[_Rd_].UL[0] = temp; */

    temp = cpuRegs.LO.UL[1] + (s32)cpuRegs.GPR.r[_Rs_].SS[1] * (s32)cpuRegs.GPR.r[_Rt_].SS[1];
    cpuRegs.LO.UL[1] = temp;

    temp = cpuRegs.HI.UL[0] + (s32)cpuRegs.GPR.r[_Rs_].SS[2] * (s32)cpuRegs.GPR.r[_Rt_].SS[2];
    cpuRegs.HI.UL[0] = temp;
    /* if (_Rd_) cpuRegs.GPR.r[_Rd_].UL[1] = temp; */

    temp = cpuRegs.HI.UL[1] + (s32)cpuRegs.GPR.r[_Rs_].SS[3] * (s32)cpuRegs.GPR.r[_Rt_].SS[3];
    cpuRegs.HI.UL[1] = temp;

    temp = cpuRegs.LO.UL[2] + (s32)cpuRegs.GPR.r[_Rs_].SS[4] * (s32)cpuRegs.GPR.r[_Rt_].SS[4];
    cpuRegs.LO.UL[2] = temp;
    /* if (_Rd_) cpuRegs.GPR.r[_Rd_].UL[2] = temp; */

    temp = cpuRegs.LO.UL[3] + (s32)cpuRegs.GPR.r[_Rs_].SS[5] * (s32)cpuRegs.GPR.r[_Rt_].SS[5];
    cpuRegs.LO.UL[3] = temp;

    temp = cpuRegs.HI.UL[2] + (s32)cpuRegs.GPR.r[_Rs_].SS[6] * (s32)cpuRegs.GPR.r[_Rt_].SS[6];
    cpuRegs.HI.UL[2] = temp;
    /* if (_Rd_) cpuRegs.GPR.r[_Rd_].UL[3] = temp; */

    temp = cpuRegs.HI.UL[3] + (s32)cpuRegs.GPR.r[_Rs_].SS[7] * (s32)cpuRegs.GPR.r[_Rt_].SS[7];
    cpuRegs.HI.UL[3] = temp;

	if (_Rd_) {
		cpuRegs.GPR.r[_Rd_].UL[0] = cpuRegs.LO.UL[0];
		cpuRegs.GPR.r[_Rd_].UL[1] = cpuRegs.HI.UL[0];
		cpuRegs.GPR.r[_Rd_].UL[2] = cpuRegs.LO.UL[2];
		cpuRegs.GPR.r[_Rd_].UL[3] = cpuRegs.HI.UL[2];
	}

}

// JayteeMaster: changed a bit to avoid screw up
__fi void  _PHMADH_LO(int dd, int n)
{
	s32 firsttemp =		   (s32)cpuRegs.GPR.r[_Rs_].SS[n+1] * (s32)cpuRegs.GPR.r[_Rt_].SS[n+1];
	s32 temp = firsttemp + (s32)cpuRegs.GPR.r[_Rs_].SS[n]   * (s32)cpuRegs.GPR.r[_Rt_].SS[n];

	cpuRegs.LO.UL[dd] = temp;
	cpuRegs.LO.UL[dd+1] = firsttemp;
}

__fi void  _PHMADH_HI(int dd, int n)
{
	s32 firsttemp =		   (s32)cpuRegs.GPR.r[_Rs_].SS[n+1] * (s32)cpuRegs.GPR.r[_Rt_].SS[n+1];
	s32 temp = firsttemp + (s32)cpuRegs.GPR.r[_Rs_].SS[n]   * (s32)cpuRegs.GPR.r[_Rt_].SS[n];

	cpuRegs.HI.UL[dd] = temp;
	cpuRegs.HI.UL[dd+1] = firsttemp;
}

void PHMADH() {				// JayteeMaster: changed a bit to avoid screw up. Also used 0,2,4,6 instead of 0,1,2,3
	_PHMADH_LO(0, 0);
	_PHMADH_HI(0, 2);
	_PHMADH_LO(2, 4);
	_PHMADH_HI(2, 6);
	if (_Rd_) {
		cpuRegs.GPR.r[_Rd_].UL[0] = cpuRegs.LO.UL[0];
		cpuRegs.GPR.r[_Rd_].UL[1] = cpuRegs.HI.UL[0];
		cpuRegs.GPR.r[_Rd_].UL[2] = cpuRegs.LO.UL[2];
		cpuRegs.GPR.r[_Rd_].UL[3] = cpuRegs.HI.UL[2];
	}
}

void PAND() {
	if (!_Rd_) return;

	cpuRegs.GPR.r[_Rd_].UD[0] = cpuRegs.GPR.r[_Rs_].UD[0] & cpuRegs.GPR.r[_Rt_].UD[0];
	cpuRegs.GPR.r[_Rd_].UD[1] = cpuRegs.GPR.r[_Rs_].UD[1] & cpuRegs.GPR.r[_Rt_].UD[1];
}

void PXOR() {
	if (!_Rd_) return;

	cpuRegs.GPR.r[_Rd_].UD[0] = cpuRegs.GPR.r[_Rs_].UD[0] ^ cpuRegs.GPR.r[_Rt_].UD[0];
	cpuRegs.GPR.r[_Rd_].UD[1] = cpuRegs.GPR.r[_Rs_].UD[1] ^ cpuRegs.GPR.r[_Rt_].UD[1];
}

void PMSUBH() {			// JayteeMaster: changed a bit to avoid screw up
	s32 temp;

    temp = cpuRegs.LO.UL[0] - (s32)cpuRegs.GPR.r[_Rs_].SS[0] * (s32)cpuRegs.GPR.r[_Rt_].SS[0];
    cpuRegs.LO.UL[0] = temp;
    /*if (_Rd_) cpuRegs.GPR.r[_Rd_].UL[0] = temp;*/

    temp = cpuRegs.LO.UL[1] - (s32)cpuRegs.GPR.r[_Rs_].SS[1] * (s32)cpuRegs.GPR.r[_Rt_].SS[1];
    cpuRegs.LO.UL[1] = temp;

    temp = cpuRegs.HI.UL[0] - (s32)cpuRegs.GPR.r[_Rs_].SS[2] * (s32)cpuRegs.GPR.r[_Rt_].SS[2];
    cpuRegs.HI.UL[0] = temp;
    /*if (_Rd_) cpuRegs.GPR.r[_Rd_].UL[1] = temp;*/

    temp = cpuRegs.HI.UL[1] - (s32)cpuRegs.GPR.r[_Rs_].SS[3] * (s32)cpuRegs.GPR.r[_Rt_].SS[3];
    cpuRegs.HI.UL[1] = temp;

    temp = cpuRegs.LO.UL[2] - (s32)cpuRegs.GPR.r[_Rs_].SS[4] * (s32)cpuRegs.GPR.r[_Rt_].SS[4];
    cpuRegs.LO.UL[2] = temp;
    /*if (_Rd_) cpuRegs.GPR.r[_Rd_].UL[2] = temp;*/

    temp = cpuRegs.LO.UL[3] - (s32)cpuRegs.GPR.r[_Rs_].SS[5] * (s32)cpuRegs.GPR.r[_Rt_].SS[5];
    cpuRegs.LO.UL[3] = temp;

    temp = cpuRegs.HI.UL[2] - (s32)cpuRegs.GPR.r[_Rs_].SS[6] * (s32)cpuRegs.GPR.r[_Rt_].SS[6];
    cpuRegs.HI.UL[2] = temp;
    /*if (_Rd_) cpuRegs.GPR.r[_Rd_].UL[3] = temp;*/

    temp = cpuRegs.HI.UL[3] - (s32)cpuRegs.GPR.r[_Rs_].SS[7] * (s32)cpuRegs.GPR.r[_Rt_].SS[7];
    cpuRegs.HI.UL[3] = temp;

	if (_Rd_) {
		cpuRegs.GPR.r[_Rd_].UL[0] = cpuRegs.LO.UL[0];
		cpuRegs.GPR.r[_Rd_].UL[1] = cpuRegs.HI.UL[0];
		cpuRegs.GPR.r[_Rd_].UL[2] = cpuRegs.LO.UL[2];
		cpuRegs.GPR.r[_Rd_].UL[3] = cpuRegs.HI.UL[2];
	}
}

// JayteeMaster: changed a bit to avoid screw up
static __fi void _PHMSBH_LO(int dd, int n, int rdd)
{
	s32 firsttemp =        (s32)cpuRegs.GPR.r[_Rs_].SS[n+1] * (s32)cpuRegs.GPR.r[_Rt_].SS[n+1];
	s32 temp = firsttemp - (s32)cpuRegs.GPR.r[_Rs_].SS[n]   * (s32)cpuRegs.GPR.r[_Rt_].SS[n];

	cpuRegs.LO.UL[dd] = temp;
	cpuRegs.LO.UL[dd+1] = ~firsttemp;
}
static __fi void _PHMSBH_HI(int dd, int n, int rdd)
{
	s32 firsttemp =        (s32)cpuRegs.GPR.r[_Rs_].SS[n+1] * (s32)cpuRegs.GPR.r[_Rt_].SS[n+1];
	s32 temp = firsttemp - (s32)cpuRegs.GPR.r[_Rs_].SS[n]   * (s32)cpuRegs.GPR.r[_Rt_].SS[n];

	cpuRegs.HI.UL[dd] = temp;
	cpuRegs.HI.UL[dd+1] = ~firsttemp;
}

void PHMSBH() {		// JayteeMaster: changed a bit to avoid screw up
	_PHMSBH_LO(0, 0, 0);
	_PHMSBH_HI(0, 2, 1);
	_PHMSBH_LO(2, 4, 2);
	_PHMSBH_HI(2, 6, 3);
	if (_Rd_) {
		cpuRegs.GPR.r[_Rd_].UL[0] = cpuRegs.LO.UL[0];
		cpuRegs.GPR.r[_Rd_].UL[1] = cpuRegs.HI.UL[0];
		cpuRegs.GPR.r[_Rd_].UL[2] = cpuRegs.LO.UL[2];
		cpuRegs.GPR.r[_Rd_].UL[3] = cpuRegs.HI.UL[2];
	}
}

void PEXEH() {
	GPR_reg Rt;

	if (!_Rd_) return;

	Rt = cpuRegs.GPR.r[_Rt_];
	cpuRegs.GPR.r[_Rd_].US[0] = Rt.US[2];
	cpuRegs.GPR.r[_Rd_].US[1] = Rt.US[1];
	cpuRegs.GPR.r[_Rd_].US[2] = Rt.US[0];
	cpuRegs.GPR.r[_Rd_].US[3] = Rt.US[3];
	cpuRegs.GPR.r[_Rd_].US[4] = Rt.US[6];
	cpuRegs.GPR.r[_Rd_].US[5] = Rt.US[5];
	cpuRegs.GPR.r[_Rd_].US[6] = Rt.US[4];
	cpuRegs.GPR.r[_Rd_].US[7] = Rt.US[7];
}

void PREVH () {
	GPR_reg Rt;

	if (!_Rd_) return;

	Rt = cpuRegs.GPR.r[_Rt_];
	cpuRegs.GPR.r[_Rd_].US[0] = Rt.US[3];
	cpuRegs.GPR.r[_Rd_].US[1] = Rt.US[2];
	cpuRegs.GPR.r[_Rd_].US[2] = Rt.US[1];
	cpuRegs.GPR.r[_Rd_].US[3] = Rt.US[0];
	cpuRegs.GPR.r[_Rd_].US[4] = Rt.US[7];
	cpuRegs.GPR.r[_Rd_].US[5] = Rt.US[6];
	cpuRegs.GPR.r[_Rd_].US[6] = Rt.US[5];
	cpuRegs.GPR.r[_Rd_].US[7] = Rt.US[4];
}

void PMULTH() {			// JayteeMaster: changed a bit to avoid screw up
	s32 temp;

    temp = (s32)cpuRegs.GPR.r[_Rs_].SS[0] * (s32)cpuRegs.GPR.r[_Rt_].SS[0];
    cpuRegs.LO.UL[0] = temp;
    /*if (_Rd_) cpuRegs.GPR.r[_Rd_].UL[0] = temp;*/

    temp = (s32)cpuRegs.GPR.r[_Rs_].SS[1] * (s32)cpuRegs.GPR.r[_Rt_].SS[1];
    cpuRegs.LO.UL[1] = temp;

    temp = (s32)cpuRegs.GPR.r[_Rs_].SS[2] * (s32)cpuRegs.GPR.r[_Rt_].SS[2];
    cpuRegs.HI.UL[0] = temp;
    /*if (_Rd_) cpuRegs.GPR.r[_Rd_].UL[1] = temp;*/

    temp = (s32)cpuRegs.GPR.r[_Rs_].SS[3] * (s32)cpuRegs.GPR.r[_Rt_].SS[3];
    cpuRegs.HI.UL[1] = temp;

    temp = (s32)cpuRegs.GPR.r[_Rs_].SS[4] * (s32)cpuRegs.GPR.r[_Rt_].SS[4];
    cpuRegs.LO.UL[2] = temp;
    /*if (_Rd_) cpuRegs.GPR.r[_Rd_].UL[2] = temp;*/

    temp = (s32)cpuRegs.GPR.r[_Rs_].SS[5] * (s32)cpuRegs.GPR.r[_Rt_].SS[5];
    cpuRegs.LO.UL[3] = temp;

    temp = (s32)cpuRegs.GPR.r[_Rs_].SS[6] * (s32)cpuRegs.GPR.r[_Rt_].SS[6];
    cpuRegs.HI.UL[2] = temp;
    /*if (_Rd_) cpuRegs.GPR.r[_Rd_].UL[3] = temp;*/

    temp = (s32)cpuRegs.GPR.r[_Rs_].SS[7] * (s32)cpuRegs.GPR.r[_Rt_].SS[7];
    cpuRegs.HI.UL[3] = temp;

	if (_Rd_) {
		cpuRegs.GPR.r[_Rd_].UL[0] = cpuRegs.LO.UL[0];
		cpuRegs.GPR.r[_Rd_].UL[1] = cpuRegs.HI.UL[0];
		cpuRegs.GPR.r[_Rd_].UL[2] = cpuRegs.LO.UL[2];
		cpuRegs.GPR.r[_Rd_].UL[3] = cpuRegs.HI.UL[2];
	}
}

__fi void  _PDIVBW(int n)
{
	if (cpuRegs.GPR.r[_Rs_].UL[n] == 0x80000000 && cpuRegs.GPR.r[_Rt_].US[0] == 0xffff)
	{
		cpuRegs.LO.SL[n] = (s32)0x80000000;
		cpuRegs.HI.SL[n] = (s32)0x0;
	}
    else if (cpuRegs.GPR.r[_Rt_].US[0] != 0)
    {
        cpuRegs.LO.SL[n] = cpuRegs.GPR.r[_Rs_].SL[n] / cpuRegs.GPR.r[_Rt_].SS[0];
        cpuRegs.HI.SL[n] = cpuRegs.GPR.r[_Rs_].SL[n] % cpuRegs.GPR.r[_Rt_].SS[0];
    }
	else
	{
		cpuRegs.LO.SL[n] = (cpuRegs.GPR.r[_Rs_].SL[n] < 0) ? 1 : -1;
		cpuRegs.HI.SL[n] = cpuRegs.GPR.r[_Rs_].SL[n];
	}
}

void PDIVBW() {
	_PDIVBW(0); _PDIVBW(1); _PDIVBW(2); _PDIVBW(3);
}

void PEXEW() {
	GPR_reg Rt;

	if (!_Rd_) return;

	Rt = cpuRegs.GPR.r[_Rt_];
	cpuRegs.GPR.r[_Rd_].UL[0] = Rt.UL[2];
	cpuRegs.GPR.r[_Rd_].UL[1] = Rt.UL[1];
	cpuRegs.GPR.r[_Rd_].UL[2] = Rt.UL[0];
	cpuRegs.GPR.r[_Rd_].UL[3] = Rt.UL[3];
}

void PROT3W() {
	GPR_reg Rt;

	if (!_Rd_) return;

	Rt = cpuRegs.GPR.r[_Rt_];
	cpuRegs.GPR.r[_Rd_].UL[0] = Rt.UL[1];
	cpuRegs.GPR.r[_Rd_].UL[1] = Rt.UL[2];
	cpuRegs.GPR.r[_Rd_].UL[2] = Rt.UL[0];
	cpuRegs.GPR.r[_Rd_].UL[3] = Rt.UL[3];
}

//*****END OF MMI2 OPCODES***********************************

//*************************MMI3 OPCODES************************

static __fi void _PMADDUW(int dd, int ss)
{
	u64 tempu =	(u64)((u64)cpuRegs.LO.UL[ss] | ((u64)cpuRegs.HI.UL[ss] << 32)) + \
			   ((u64)cpuRegs.GPR.r[_Rs_].UL[ss] * (u64)cpuRegs.GPR.r[_Rt_].UL[ss]);

	cpuRegs.LO.SD[dd] = (s32)(tempu & 0xffffffff);
	cpuRegs.HI.SD[dd] = (s32)(tempu >> 32);

	if (_Rd_) cpuRegs.GPR.r[_Rd_].UD[dd] = tempu;
}

void PMADDUW() {
	_PMADDUW(0, 0);
	_PMADDUW(1, 2);
}

void PSRAVW() {
	if (!_Rd_) return;

	cpuRegs.GPR.r[_Rd_].SD[0] = (s64)(cpuRegs.GPR.r[_Rt_].SL[0] >>
									 (cpuRegs.GPR.r[_Rs_].UL[0] & 0x1F));
	cpuRegs.GPR.r[_Rd_].SD[1] = (s64)(cpuRegs.GPR.r[_Rt_].SL[2] >>
									 (cpuRegs.GPR.r[_Rs_].UL[2] & 0x1F));
}

void PMTHI() {
	cpuRegs.HI.UD[0] = cpuRegs.GPR.r[_Rs_].UD[0];
	cpuRegs.HI.UD[1] = cpuRegs.GPR.r[_Rs_].UD[1];
}

void PMTLO() {
	cpuRegs.LO.UD[0] = cpuRegs.GPR.r[_Rs_].UD[0];
	cpuRegs.LO.UD[1] = cpuRegs.GPR.r[_Rs_].UD[1];
}

void PINTEH() {
	GPR_reg Rs, Rt;

	if (!_Rd_) return;

	Rs = cpuRegs.GPR.r[_Rs_]; Rt = cpuRegs.GPR.r[_Rt_];
	cpuRegs.GPR.r[_Rd_].US[0] = Rt.US[0];
	cpuRegs.GPR.r[_Rd_].US[1] = Rs.US[0];
	cpuRegs.GPR.r[_Rd_].US[2] = Rt.US[2];
	cpuRegs.GPR.r[_Rd_].US[3] = Rs.US[2];
	cpuRegs.GPR.r[_Rd_].US[4] = Rt.US[4];
	cpuRegs.GPR.r[_Rd_].US[5] = Rs.US[4];
	cpuRegs.GPR.r[_Rd_].US[6] = Rt.US[6];
	cpuRegs.GPR.r[_Rd_].US[7] = Rs.US[6];
}

__fi void  _PMULTUW(int dd, int ss)
{
   u64 tempu = (u64)cpuRegs.GPR.r[_Rs_].UL[ss] * (u64)cpuRegs.GPR.r[_Rt_].UL[ss];

   cpuRegs.LO.UD[dd] = (s32)(tempu & 0xffffffff);
   cpuRegs.HI.UD[dd] = (s32)(tempu >> 32);

   if (_Rd_) cpuRegs.GPR.r[_Rd_].UD[dd] = tempu;
}


void PMULTUW() {
	_PMULTUW(0, 0);
	_PMULTUW(1, 2);
}

__fi void  _PDIVUW(int dd, int ss)
{
	if (cpuRegs.GPR.r[_Rt_].UL[ss] != 0) {
		cpuRegs.LO.SD[dd] = (s32)(cpuRegs.GPR.r[_Rs_].UL[ss] / cpuRegs.GPR.r[_Rt_].UL[ss]);
		cpuRegs.HI.SD[dd] = (s32)(cpuRegs.GPR.r[_Rs_].UL[ss] % cpuRegs.GPR.r[_Rt_].UL[ss]);
	}
	else
	{
		cpuRegs.LO.SD[dd] = -1;
		cpuRegs.HI.SD[dd] = cpuRegs.GPR.r[_Rs_].SL[ss];
	}
}

void PDIVUW() {
	_PDIVUW(0, 0);
	_PDIVUW(1, 2);
}

void PCPYUD() {
	if (!_Rd_) return;

	// note: first _Rs_, since the other way when _Rd_ equals
	// _Rs_ or _Rt_ this would screw up
	cpuRegs.GPR.r[_Rd_].UD[0] = cpuRegs.GPR.r[_Rs_].UD[1];
	cpuRegs.GPR.r[_Rd_].UD[1] = cpuRegs.GPR.r[_Rt_].UD[1];
}

void POR() {
	if (!_Rd_) return;

	cpuRegs.GPR.r[_Rd_].UD[0] = cpuRegs.GPR.r[_Rs_].UD[0] | cpuRegs.GPR.r[_Rt_].UD[0];
	cpuRegs.GPR.r[_Rd_].UD[1] = cpuRegs.GPR.r[_Rs_].UD[1] | cpuRegs.GPR.r[_Rt_].UD[1];
}

void PNOR () {
	if (!_Rd_) return;

	cpuRegs.GPR.r[_Rd_].UD[0] = ~(cpuRegs.GPR.r[_Rs_].UD[0] | cpuRegs.GPR.r[_Rt_].UD[0]);
	cpuRegs.GPR.r[_Rd_].UD[1] = ~(cpuRegs.GPR.r[_Rs_].UD[1] | cpuRegs.GPR.r[_Rt_].UD[1]);
}

void PEXCH() {
	GPR_reg Rt;

	if (!_Rd_) return;

	Rt = cpuRegs.GPR.r[_Rt_];
	cpuRegs.GPR.r[_Rd_].US[0] = Rt.US[0];
	cpuRegs.GPR.r[_Rd_].US[1] = Rt.US[2];
	cpuRegs.GPR.r[_Rd_].US[2] = Rt.US[1];
	cpuRegs.GPR.r[_Rd_].US[3] = Rt.US[3];
	cpuRegs.GPR.r[_Rd_].US[4] = Rt.US[4];
	cpuRegs.GPR.r[_Rd_].US[5] = Rt.US[6];
	cpuRegs.GPR.r[_Rd_].US[6] = Rt.US[5];
	cpuRegs.GPR.r[_Rd_].US[7] = Rt.US[7];
}

void PCPYH() {
	GPR_reg Rt;

	if (!_Rd_) return;

	Rt = cpuRegs.GPR.r[_Rt_];
	cpuRegs.GPR.r[_Rd_].US[0] = Rt.US[0];
	cpuRegs.GPR.r[_Rd_].US[1] = Rt.US[0];
	cpuRegs.GPR.r[_Rd_].US[2] = Rt.US[0];
	cpuRegs.GPR.r[_Rd_].US[3] = Rt.US[0];
	cpuRegs.GPR.r[_Rd_].US[4] = Rt.US[4];
	cpuRegs.GPR.r[_Rd_].US[5] = Rt.US[4];
	cpuRegs.GPR.r[_Rd_].US[6] = Rt.US[4];
	cpuRegs.GPR.r[_Rd_].US[7] = Rt.US[4];
}

void PEXCW() {
	GPR_reg Rt;

	if (!_Rd_) return;

	Rt = cpuRegs.GPR.r[_Rt_];
	cpuRegs.GPR.r[_Rd_].UL[0] = Rt.UL[0];
	cpuRegs.GPR.r[_Rd_].UL[1] = Rt.UL[2];
	cpuRegs.GPR.r[_Rd_].UL[2] = Rt.UL[1];
	cpuRegs.GPR.r[_Rd_].UL[3] = Rt.UL[3];
}

//**********************END OF MMI3 OPCODES********************

// obs:
// QFSRV not verified

}}} } // end namespace R5900::Interpreter::OpcodeImpl::MMI
