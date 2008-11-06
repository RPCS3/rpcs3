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

#include <stdlib.h>
#include "Common.h"
#include "DebugTools/Debug.h"
#include "R5900.h"
#include "InterTables.h"


void MMI() {
#ifdef MMI_LOG
	MMI_LOG("%s\n", disR5900F(cpuRegs.code, cpuRegs.pc));
#endif
	Int_MMIPrintTable[_Funct_]();
}

void MMI0() {
	Int_MMI0PrintTable[_Sa_]();
}

void MMI1() {
	Int_MMI1PrintTable[_Sa_]();
}

void MMI2() {
	Int_MMI2PrintTable[_Sa_]();
}

void MMI3() {
	Int_MMI3PrintTable[_Sa_]();
}

void MMI_Unknown() {
	SysPrintf ("Unknown MMI opcode called\n");
}

//*****************MMI OPCODES*********************************

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

#define _PLZCW(n) { \
	sign = cpuRegs.GPR.r[_Rs_].UL[n] >> 31; \
    for (i=30; i>=0; i--) { \
        if (((cpuRegs.GPR.r[_Rs_].UL[n] >> i) & 0x1) != sign) { \
            break; \
        } \
    } \
    cpuRegs.GPR.r[_Rd_].UL[n] = 30 - i; \
}

void PLZCW() {
    int i;
	u32 sign;

    if (!_Rd_) return;

	_PLZCW (0);
	_PLZCW (1);
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
	s64 temp = (s64)cpuRegs.GPR.r[_Rs_].SL[0] * (s64)cpuRegs.GPR.r[_Rt_].SL[0];

	cpuRegs.LO.UD[1] = (s64)(s32)(temp & 0xffffffff);
	cpuRegs.HI.UD[1] = (s64)(s32)(temp >> 32);

	/* Modified a bit . asadr */
	if (_Rd_) cpuRegs.GPR.r[_Rd_].UD[0] = cpuRegs.LO.UD[1];
}

void MULTU1() {
	u64 tempu = (u64)cpuRegs.GPR.r[_Rs_].UL[0] * (u64)cpuRegs.GPR.r[_Rt_].UL[0];

	cpuRegs.LO.UD[1] = (s32)(tempu & 0xffffffff);
	cpuRegs.HI.UD[1] = (s32)(tempu >> 32);

	if (_Rd_) cpuRegs.GPR.r[_Rd_].UD[0] = cpuRegs.LO.UD[1];
}

void DIV1() {
	if (cpuRegs.GPR.r[_Rt_].SL[0] != 0) {
		cpuRegs.LO.SD[1] = cpuRegs.GPR.r[_Rs_].SL[0] / cpuRegs.GPR.r[_Rt_].SL[0];
		cpuRegs.HI.SD[1] = cpuRegs.GPR.r[_Rs_].SL[0] % cpuRegs.GPR.r[_Rt_].SL[0];
	}
}

void DIVU1() {
	if (cpuRegs.GPR.r[_Rt_].UL[0] != 0) {
		cpuRegs.LO.UD[1] = cpuRegs.GPR.r[_Rs_].UL[0] / cpuRegs.GPR.r[_Rt_].UL[0];
		cpuRegs.HI.UD[1] = cpuRegs.GPR.r[_Rs_].UL[0] % cpuRegs.GPR.r[_Rt_].UL[0];
	}
}

#define PMFHL_CLAMP(dst, src) \
    if ((int)src > (int)0x00007fff) dst = 0x7fff; \
    else \
    if ((int)src < (int)0xffff8000) dst = 0x8000; \
    else dst = (u16)src;

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
				u64 TempU64 = ((u64)cpuRegs.HI.UL[0] << 32) | (u64)cpuRegs.LO.UL[0];
				if (TempU64 >= 0x000000007fffffffLL) {
					cpuRegs.GPR.r[_Rd_].UD[0] = 0x000000007fffffffLL;
				} else if (TempU64 <= 0xffffffff80000000LL) {
					cpuRegs.GPR.r[_Rd_].UD[0] = 0xffffffff80000000LL;
				} else {
					cpuRegs.GPR.r[_Rd_].UD[0] = (s64)cpuRegs.LO.SL[0];
				}

				TempU64 = ((u64)cpuRegs.HI.UL[2] << 32) | (u64)cpuRegs.LO.UL[2];
				if (TempU64 >= 0x000000007fffffffLL) {
					cpuRegs.GPR.r[_Rd_].UD[1] = 0x000000007fffffffLL;
				} else if (TempU64 <= 0xffffffff80000000LL) {
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

#define _PSLLH(n) \
	cpuRegs.GPR.r[_Rd_].US[n] = cpuRegs.GPR.r[_Rt_].US[n] << ( _Sa_ & 0xf );

void PSLLH() {
	if (!_Rd_) return;

	_PSLLH(0); _PSLLH(1); _PSLLH(2); _PSLLH(3);
	_PSLLH(4); _PSLLH(5); _PSLLH(6); _PSLLH(7);
}

#define _PSRLH(n) \
	cpuRegs.GPR.r[_Rd_].US[n] = cpuRegs.GPR.r[_Rt_].US[n] >> ( _Sa_ & 0xf );

void PSRLH () {
	if (!_Rd_) return;

	_PSRLH(0); _PSRLH(1); _PSRLH(2); _PSRLH(3);
	_PSRLH(4); _PSRLH(5); _PSRLH(6); _PSRLH(7);
}

#define _PSRAH(n) \
	cpuRegs.GPR.r[_Rd_].US[n] = cpuRegs.GPR.r[_Rt_].SS[n] >> ( _Sa_ & 0xf );

void PSRAH() {
	if (!_Rd_) return;

	_PSRAH(0); _PSRAH(1); _PSRAH(2); _PSRAH(3);
	_PSRAH(4); _PSRAH(5); _PSRAH(6); _PSRAH(7);
}

#define _PSLLW(n) \
	cpuRegs.GPR.r[_Rd_].UL[n] = cpuRegs.GPR.r[_Rt_].UL[n] << _Sa_;

void PSLLW() {
	if (!_Rd_) return;

	_PSLLW(0); _PSLLW(1); _PSLLW(2); _PSLLW(3);
}

#define _PSRLW(n) \
	cpuRegs.GPR.r[_Rd_].UL[n] = cpuRegs.GPR.r[_Rt_].UL[n] >> _Sa_;

void PSRLW() {
	if (!_Rd_) return;

	_PSRLW(0); _PSRLW(1); _PSRLW(2); _PSRLW(3);
}

#define _PSRAW(n) \
	cpuRegs.GPR.r[_Rd_].UL[n] = cpuRegs.GPR.r[_Rt_].SL[n] >> _Sa_;

void PSRAW() {
	if (!_Rd_) return;

	_PSRAW(0); _PSRAW(1); _PSRAW(2); _PSRAW(3);
}

//*****************END OF MMI OPCODES**************************
//*************************MMI0 OPCODES************************

#define _PADDW(n) \
	cpuRegs.GPR.r[_Rd_].UL[n] = cpuRegs.GPR.r[_Rs_].UL[n] + cpuRegs.GPR.r[_Rt_].UL[n];

void PADDW() {
	if (!_Rd_) return;

	_PADDW(0); _PADDW(1); _PADDW(2); _PADDW(3);
}

#define _PSUBW(n) \
	cpuRegs.GPR.r[_Rd_].UL[n] = cpuRegs.GPR.r[_Rs_].UL[n] - cpuRegs.GPR.r[_Rt_].UL[n];

void PSUBW() {
	if (!_Rd_) return;

	_PSUBW(0); _PSUBW(1); _PSUBW(2); _PSUBW(3);
}

#define _PCGTW(n) \
	cpuRegs.GPR.r[_Rd_].UL[n] = \
		(cpuRegs.GPR.r[_Rs_].SL[n] > cpuRegs.GPR.r[_Rt_].SL[n]) ? \
		 0xFFFFFFFF : 0x00000000;

void PCGTW() {
	if (!_Rd_) return;

	_PCGTW(0);  _PCGTW(1);  _PCGTW(2);  _PCGTW(3);
}

#define _PMAXW(n) \
	cpuRegs.GPR.r[_Rd_].UL[n] = \
		(cpuRegs.GPR.r[_Rs_].SL[n] > cpuRegs.GPR.r[_Rt_].SL[n]) ? \
		 cpuRegs.GPR.r[_Rs_].UL[n] : cpuRegs.GPR.r[_Rt_].UL[n];

void PMAXW() {
	if (!_Rd_) return;

	_PMAXW(0);  _PMAXW(1);  _PMAXW(2);  _PMAXW(3);
}

#define _PADDH(n) \
	cpuRegs.GPR.r[_Rd_].US[n] = cpuRegs.GPR.r[_Rs_].US[n] + cpuRegs.GPR.r[_Rt_].US[n];

void PADDH() {
	if (!_Rd_) return;

	_PADDH(0);  _PADDH(1);  _PADDH(2);  _PADDH(3);
	_PADDH(4);  _PADDH(5);  _PADDH(6);  _PADDH(7);
}

#define _PSUBH(n) \
	cpuRegs.GPR.r[_Rd_].US[n] = cpuRegs.GPR.r[_Rs_].US[n] - cpuRegs.GPR.r[_Rt_].US[n];

void PSUBH() {
	if (!_Rd_) return;

	_PSUBH(0);  _PSUBH(1);  _PSUBH(2);  _PSUBH(3);
	_PSUBH(4);  _PSUBH(5);  _PSUBH(6);  _PSUBH(7);
}

#define _PCGTH(n) \
	cpuRegs.GPR.r[_Rd_].US[n] = \
		(cpuRegs.GPR.r[_Rs_].SS[n] > cpuRegs.GPR.r[_Rt_].SS[n]) ? \
		 0xFFFF : 0x0000;

void PCGTH() {
	if (!_Rd_) return;

	_PCGTH(0);  _PCGTH(1);  _PCGTH(2);  _PCGTH(3);
	_PCGTH(4);  _PCGTH(5);  _PCGTH(6);  _PCGTH(7);
}

#define _PMAXH(n) \
	cpuRegs.GPR.r[_Rd_].US[n] = \
		(cpuRegs.GPR.r[_Rs_].SS[n] > cpuRegs.GPR.r[_Rt_].SS[n]) ? \
		 cpuRegs.GPR.r[_Rs_].US[n] : cpuRegs.GPR.r[_Rt_].US[n];

void PMAXH() {
	if (!_Rd_) return;

	_PMAXH(0);  _PMAXH(1);  _PMAXH(2);  _PMAXH(3);
	_PMAXH(4);  _PMAXH(5);  _PMAXH(6);  _PMAXH(7);
}

#define _PADDB(n) \
	cpuRegs.GPR.r[_Rd_].SC[n] = cpuRegs.GPR.r[_Rs_].SC[n] + cpuRegs.GPR.r[_Rt_].SC[n];

void PADDB() {
	if (!_Rd_) return;

	_PADDB(0);  _PADDB(1);  _PADDB(2);  _PADDB(3);
	_PADDB(4);  _PADDB(5);  _PADDB(6);  _PADDB(7);
	_PADDB(8);  _PADDB(9);  _PADDB(10); _PADDB(11);
	_PADDB(12); _PADDB(13); _PADDB(14); _PADDB(15);
}

#define _PSUBB(n) \
	cpuRegs.GPR.r[_Rd_].SC[n] = cpuRegs.GPR.r[_Rs_].SC[n] - cpuRegs.GPR.r[_Rt_].SC[n];

void PSUBB() {
	if (!_Rd_) return;

	_PSUBB(0);  _PSUBB(1);  _PSUBB(2);  _PSUBB(3);
	_PSUBB(4);  _PSUBB(5);  _PSUBB(6);  _PSUBB(7);
	_PSUBB(8);  _PSUBB(9);  _PSUBB(10); _PSUBB(11);
	_PSUBB(12); _PSUBB(13); _PSUBB(14); _PSUBB(15);
}

#define _PCGTB(n) \
    cpuRegs.GPR.r[_Rd_].UC[n] = (cpuRegs.GPR.r[_Rs_].SC[n] > cpuRegs.GPR.r[_Rt_].SC[n]) ? \
                                 0xFF : 0x00;

void PCGTB() {
	if (!_Rd_) return;

	_PCGTB(0);  _PCGTB(1);  _PCGTB(2);  _PCGTB(3);
	_PCGTB(4);  _PCGTB(5);  _PCGTB(6);  _PCGTB(7);
	_PCGTB(8);  _PCGTB(9);  _PCGTB(10); _PCGTB(11);
	_PCGTB(12); _PCGTB(13); _PCGTB(14); _PCGTB(15);
}

#define _PADDSW(n) \
    sTemp64 = (s64)cpuRegs.GPR.r[_Rs_].SL[n] + (s64)cpuRegs.GPR.r[_Rt_].SL[n]; \
    if (sTemp64 > 0x7FFFFFFF) { \
        cpuRegs.GPR.r[_Rd_].UL[n] = 0x7FFFFFFF; \
    } else \
    if ((sTemp64 < (s32)0x80000000) ) { \
        cpuRegs.GPR.r[_Rd_].UL[n] = 0x80000000LL; \
    } else { \
        cpuRegs.GPR.r[_Rd_].UL[n] = (s32)sTemp64; \
    }

void PADDSW() {
	s64 sTemp64;

	if (!_Rd_) return;

	_PADDSW(0); _PADDSW(1); _PADDSW(2); _PADDSW(3);
}

#define _PSUBSW(n) \
    sTemp64 = (s64)cpuRegs.GPR.r[_Rs_].SL[n] - (s64)cpuRegs.GPR.r[_Rt_].SL[n]; \
    if (sTemp64 >= 0x7FFFFFFF) { \
        cpuRegs.GPR.r[_Rd_].UL[n] = 0x7FFFFFFF; \
    } else \
    if ((sTemp64 < (s32)0x80000000) ) { \
        cpuRegs.GPR.r[_Rd_].UL[n] = 0x80000000; \
    } else { \
        cpuRegs.GPR.r[_Rd_].UL[n] = (s32)sTemp64; \
    }

void PSUBSW() {
	s64 sTemp64;

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

#define _PADDSH(n) \
    sTemp32 = (s32)cpuRegs.GPR.r[_Rs_].SS[n] + (s32)cpuRegs.GPR.r[_Rt_].SS[n]; \
    if (sTemp32 > 0x7FFF) { \
        cpuRegs.GPR.r[_Rd_].US[n] = 0x7FFF; \
    } else \
    if ((sTemp32 < (s32)0xffff8000) ) { \
        cpuRegs.GPR.r[_Rd_ ].US[n] = 0x8000; \
    } else { \
        cpuRegs.GPR.r[_Rd_ ].US[n] = (s16)sTemp32; \
    }

void PADDSH() {
	s32 sTemp32;

	if (!_Rd_) return;

	_PADDSH(0); _PADDSH(1); _PADDSH(2); _PADDSH(3);
	_PADDSH(4); _PADDSH(5); _PADDSH(6); _PADDSH(7);
}

#define _PSUBSH(n) \
    sTemp32 = (s32)cpuRegs.GPR.r[_Rs_].SS[n] - (s32)cpuRegs.GPR.r[_Rt_].SS[n]; \
    if (sTemp32 >= 0x7FFF) { \
        cpuRegs.GPR.r[_Rd_].US[n] = 0x7FFF; \
    } else \
    if ((sTemp32 < (s32)0xffff8000) ) { \
        cpuRegs.GPR.r[_Rd_].US[n] = 0x8000; \
    } else { \
        cpuRegs.GPR.r[_Rd_].US[n] = (s16)sTemp32; \
    }

void PSUBSH() {
	s32 sTemp32;

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

#define _PADDSB(n) \
    sTemp16 = (s16)cpuRegs.GPR.r[_Rs_].SC[n] + (s16)cpuRegs.GPR.r[_Rt_].SC[n]; \
    if (sTemp16 > 0x7F) { \
        cpuRegs.GPR.r[_Rd_].UC[n] = 0x7F; \
    } else \
    if ((sTemp16 < 0x180) && (sTemp16 >= 0x100)) { \
        cpuRegs.GPR.r[_Rd_].UC[n] = 0x80; \
    } else { \
        cpuRegs.GPR.r[_Rd_].UC[n] = (s8)sTemp16; \
    }

void PADDSB() {
	s16 sTemp16;

	if (!_Rd_) return;

	_PADDSB(0);  _PADDSB(1);  _PADDSB(2);  _PADDSB(3);
	_PADDSB(4);  _PADDSB(5);  _PADDSB(6);  _PADDSB(7);
	_PADDSB(8);  _PADDSB(9);  _PADDSB(10); _PADDSB(11);
	_PADDSB(12); _PADDSB(13); _PADDSB(14); _PADDSB(15);
}

#define _PSUBSB(n) \
    sTemp16 = (s16)cpuRegs.GPR.r[_Rs_].SC[n] - (s16)cpuRegs.GPR.r[_Rt_].SC[n]; \
    if (sTemp16 >= 0x7F) { \
        cpuRegs.GPR.r[_Rd_].UC[n] = 0x7F; \
    } else \
    if ((sTemp16 < 0x180) && (sTemp16 >= 0x100)) { \
        cpuRegs.GPR.r[_Rd_].UC[n] = 0x80; \
    } else { \
        cpuRegs.GPR.r[_Rd_].UC[n] = (s8)sTemp16; \
    }

void PSUBSB() {
	s16 sTemp16;

	if (!_Rd_) return;

	_PSUBSB(0);  _PSUBSB(1);  _PSUBSB(2);  _PSUBSB(3);
	_PSUBSB(4);  _PSUBSB(5);  _PSUBSB(6);  _PSUBSB(7);
	_PSUBSB(8);  _PSUBSB(9);  _PSUBSB(10); _PSUBSB(11);
	_PSUBSB(12); _PSUBSB(13); _PSUBSB(14); _PSUBSB(15);
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

#define _PEXT5(n) \
	cpuRegs.GPR.r[_Rd_].UL[n] = \
		((cpuRegs.GPR.r[_Rt_].UL[n] & 0x0000001F) <<  3) | \
		((cpuRegs.GPR.r[_Rt_].UL[n] & 0x000003E0) <<  6) | \
		((cpuRegs.GPR.r[_Rt_].UL[n] & 0x00007C00) <<  9) | \
		((cpuRegs.GPR.r[_Rt_].UL[n] & 0x00008000) << 16);

void PEXT5() {
	if (!_Rd_) return;

	_PEXT5(0); _PEXT5(1); _PEXT5(2); _PEXT5(3);
}

#define _PPAC5(n) \
	cpuRegs.GPR.r[_Rd_].UL[n] = \
		((cpuRegs.GPR.r[_Rt_].UL[n] >>  3) & 0x0000001F) | \
		((cpuRegs.GPR.r[_Rt_].UL[n] >>  6) & 0x000003E0) | \
		((cpuRegs.GPR.r[_Rt_].UL[n] >>  9) & 0x00007C00) | \
		((cpuRegs.GPR.r[_Rt_].UL[n] >> 16) & 0x00008000);

void PPAC5() {
	if (!_Rd_) return;

	_PPAC5(0); _PPAC5(1); _PPAC5(2); _PPAC5(3);
}

//***END OF MMI0 OPCODES******************************************
//**********MMI1 OPCODES**************************************

#define _PABSW(n) \
	cpuRegs.GPR.r[_Rd_].UL[n] = abs(cpuRegs.GPR.r[_Rt_].SL[n]);

void PABSW() {
	if (!_Rd_) return;

	_PABSW(0);  _PABSW(1);  _PABSW(2);  _PABSW(3);
}

#define _PCEQW(n) \
	cpuRegs.GPR.r[_Rd_].UL[n] = \
		(cpuRegs.GPR.r[_Rs_].UL[n] == cpuRegs.GPR.r[_Rt_].UL[n]) ? \
		 0xFFFFFFFF : 0x00000000;

void PCEQW() {
	if (!_Rd_) return;

	_PCEQW(0); _PCEQW(1); _PCEQW(2); _PCEQW(3);
}

#define _PMINW(n) \
	cpuRegs.GPR.r[_Rd_].SL[n] = \
		(cpuRegs.GPR.r[_Rs_].SL[n] < cpuRegs.GPR.r[_Rt_].SL[n]) ? \
		 cpuRegs.GPR.r[_Rs_].SL[n] : cpuRegs.GPR.r[_Rt_].SL[n];

void PMINW() {
	if (!_Rd_) return;

	_PMINW(0); _PMINW(1); _PMINW(2); _PMINW(3);
}

void PADSBH() {
	if (!_Rd_) return;

	_PSUBH(0); _PSUBH(1); _PSUBH(2); _PSUBH(3);
	_PADDH(4); _PADDH(5); _PADDH(6); _PADDH(7);
}

#define _PABSH(n) \
	cpuRegs.GPR.r[_Rd_].US[n] = abs(cpuRegs.GPR.r[_Rt_].SS[n]);

void PABSH() {
	if (!_Rd_) return;

	_PABSH(0); _PABSH(1); _PABSH(2); _PABSH(3);
	_PABSH(4); _PABSH(5); _PABSH(6); _PABSH(7);
}

#define _PCEQH(n) \
	cpuRegs.GPR.r[_Rd_].US[n] = \
		(cpuRegs.GPR.r[_Rs_].US[n] == cpuRegs.GPR.r[_Rt_].US[n]) ? 0xFFFF : 0x0000;

void PCEQH() {
	if (!_Rd_) return;

	_PCEQH(0); _PCEQH(1); _PCEQH(2); _PCEQH(3);
	_PCEQH(4); _PCEQH(5); _PCEQH(6); _PCEQH(7);
}

#define _PMINH(n) \
	cpuRegs.GPR.r[_Rd_].US[n] = \
		(cpuRegs.GPR.r[_Rs_].SS[n] < cpuRegs.GPR.r[_Rt_].SS[n]) ? \
		 cpuRegs.GPR.r[_Rs_].US[n] : cpuRegs.GPR.r[_Rt_].US[n];

void PMINH() {
	if (!_Rd_) return;

	_PMINH(0); _PMINH(1); _PMINH(2); _PMINH(3);
	_PMINH(4); _PMINH(5); _PMINH(6); _PMINH(7);
}

#define _PCEQB(n) \
	cpuRegs.GPR.r[_Rd_].UC[n] = (cpuRegs.GPR.r[_Rs_].UC[n] == \
								 cpuRegs.GPR.r[_Rt_].UC[n]) ? 0xFF : 0x00;

void PCEQB() {
	if (!_Rd_) return;

	_PCEQB(0);  _PCEQB(1);  _PCEQB(2);  _PCEQB(3);
	_PCEQB(4);  _PCEQB(5);  _PCEQB(6);  _PCEQB(7);
	_PCEQB(8);  _PCEQB(9);  _PCEQB(10); _PCEQB(11);
	_PCEQB(12); _PCEQB(13); _PCEQB(14); _PCEQB(15);
}

#define _PADDUW(n) \
	tmp = (s64)cpuRegs.GPR.r[_Rs_].UL[n] + (s64)cpuRegs.GPR.r[_Rt_].UL[n]; \
	if (tmp > 0xffffffff) \
		 cpuRegs.GPR.r[_Rd_].UL[n] = 0xffffffff; \
	else cpuRegs.GPR.r[_Rd_].UL[n] = (u32)tmp;

void PADDUW () {
	s64 tmp;

	if (!_Rd_) return;

	_PADDUW(0); _PADDUW(1); _PADDUW(2); _PADDUW(3);
}

#define _PSUBUW(n) \
	sTemp64 = (s64)cpuRegs.GPR.r[_Rs_].UL[n] - (s64)cpuRegs.GPR.r[_Rt_].UL[n]; \
	if (sTemp64 <= 0x0) { \
		cpuRegs.GPR.r[_Rd_].UL[n] = 0x0; \
	} else { \
		cpuRegs.GPR.r[_Rd_].UL[n] = (u32)sTemp64; \
	}

void PSUBUW() {
	s64 sTemp64;

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

#define _PADDUH(n) \
	sTemp32 = (s32)cpuRegs.GPR.r[_Rs_].US[n] + (s32)cpuRegs.GPR.r[_Rt_].US[n]; \
	if (sTemp32 > 0xFFFF) { \
		cpuRegs.GPR.r[_Rd_].US[n] = 0xFFFF; \
	} else { \
		cpuRegs.GPR.r[_Rd_].US[n] = (u16)sTemp32; \
	}

void PADDUH() {
	s32 sTemp32;

	if (!_Rd_) return;

	_PADDUH(0); _PADDUH(1); _PADDUH(2); _PADDUH(3);
	_PADDUH(4); _PADDUH(5); _PADDUH(6); _PADDUH(7);
}

#define _PSUBUH(n) \
	sTemp32 = (s32)cpuRegs.GPR.r[_Rs_].US[n] - (s32)cpuRegs.GPR.r[_Rt_].US[n]; \
	if (sTemp32 <= 0x0) { \
		cpuRegs.GPR.r[_Rd_].US[n] = 0x0; \
	} else { \
		cpuRegs.GPR.r[_Rd_].US[n] = (u16)sTemp32; \
	}

void PSUBUH() {
	s32 sTemp32;

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

#define _PADDUB(n) \
	Temp16 = (u16)cpuRegs.GPR.r[_Rs_].UC[n] + (u16)cpuRegs.GPR.r[_Rt_].UC[n]; \
	if (Temp16 > 0xFF) { \
		cpuRegs.GPR.r[_Rd_].UC[n] = 0xFF; \
	} else { \
		cpuRegs.GPR.r[_Rd_].UC[n] = (u8)Temp16; \
	}

void PADDUB() {
	u16 Temp16;

	if (!_Rd_) return;

	_PADDUB(0);  _PADDUB(1);  _PADDUB(2);  _PADDUB(3);
	_PADDUB(4);  _PADDUB(5);  _PADDUB(6);  _PADDUB(7);
	_PADDUB(8);  _PADDUB(9);  _PADDUB(10); _PADDUB(11);
	_PADDUB(12); _PADDUB(13); _PADDUB(14); _PADDUB(15);
}

#define _PSUBUB(n) \
	sTemp16 = (s16)cpuRegs.GPR.r[_Rs_].UC[n] - (s16)cpuRegs.GPR.r[_Rt_].UC[n]; \
	if (sTemp16 <= 0x0) { \
		cpuRegs.GPR.r[_Rd_].UC[n] = 0x0; \
	} else { \
		cpuRegs.GPR.r[_Rd_].UC[n] = (u8)sTemp16; \
	}

void PSUBUB() {
	s16 sTemp16;

	if (!_Rd_) return;

	_PSUBUB(0);  _PSUBUB(1);  _PSUBUB(2);  _PSUBUB(3);
	_PSUBUB(4);  _PSUBUB(5);  _PSUBUB(6);  _PSUBUB(7);
	_PSUBUB(8);  _PSUBUB(9);  _PSUBUB(10); _PSUBUB(11);
	_PSUBUB(12); _PSUBUB(13); _PSUBUB(14); _PSUBUB(15);
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

void QFSRV() {				// JayteeMaster: changed a bit to avoid screw up
	GPR_reg Rd;
	if (!_Rd_) return;

	if (cpuRegs.sa == 0) {
		cpuRegs.GPR.r[_Rd_].UD[0] = cpuRegs.GPR.r[_Rt_].UD[0];
		cpuRegs.GPR.r[_Rd_].UD[1] = cpuRegs.GPR.r[_Rt_].UD[1];
	} else {
		if (cpuRegs.sa < 64) {
			/*
			cpuRegs.GPR.r[_Rd_].UD[0] = cpuRegs.GPR.r[_Rt_].UD[0] >> cpuRegs.sa;
			cpuRegs.GPR.r[_Rd_].UD[1] = cpuRegs.GPR.r[_Rt_].UD[1] >> cpuRegs.sa;
			cpuRegs.GPR.r[_Rd_].UD[0]|= cpuRegs.GPR.r[_Rt_].UD[1] << (64 - cpuRegs.sa);
			cpuRegs.GPR.r[_Rd_].UD[1]|= cpuRegs.GPR.r[_Rs_].UD[0] << (64 - cpuRegs.sa);
			*/
			Rd.UD[0] = cpuRegs.GPR.r[_Rt_].UD[0] >> cpuRegs.sa;
			Rd.UD[1] = cpuRegs.GPR.r[_Rt_].UD[1] >> cpuRegs.sa;
			Rd.UD[0]|= cpuRegs.GPR.r[_Rt_].UD[1] << (64 - cpuRegs.sa);
			Rd.UD[1]|= cpuRegs.GPR.r[_Rs_].UD[0] << (64 - cpuRegs.sa);
			cpuRegs.GPR.r[_Rd_] = Rd;
		} else {
			/*
			cpuRegs.GPR.r[_Rd_].UD[0] = cpuRegs.GPR.r[_Rt_].UD[1] >> (cpuRegs.sa - 64);
			cpuRegs.GPR.r[_Rd_].UD[1] = cpuRegs.GPR.r[_Rs_].UD[0] >> (cpuRegs.sa - 64);
			cpuRegs.GPR.r[_Rd_].UD[0]|= cpuRegs.GPR.r[_Rs_].UD[0] << (128 - cpuRegs.sa);
			cpuRegs.GPR.r[_Rd_].UD[1]|= cpuRegs.GPR.r[_Rs_].UD[1] << (128 - cpuRegs.sa);
			*/
			Rd.UD[0] = cpuRegs.GPR.r[_Rt_].UD[1] >> (cpuRegs.sa - 64);
			Rd.UD[1] = cpuRegs.GPR.r[_Rs_].UD[0] >> (cpuRegs.sa - 64);
			Rd.UD[0]|= cpuRegs.GPR.r[_Rs_].UD[0] << (128 - cpuRegs.sa);
			Rd.UD[1]|= cpuRegs.GPR.r[_Rs_].UD[1] << (128 - cpuRegs.sa);
			cpuRegs.GPR.r[_Rd_] = Rd;
		}
	}
}

//********END OF MMI1 OPCODES***********************************

//*********MMI2 OPCODES***************************************

#define _PMADDW(dd, ss) { \
	s64 temp =	(s64)((s64)cpuRegs.LO.SL[ss] | ((s64)cpuRegs.HI.SL[ss] << 32)) + \
			   ((s64)cpuRegs.GPR.r[_Rs_].SL[ss] * (s64)cpuRegs.GPR.r[_Rt_].SL[ss]); \
 \
	cpuRegs.LO.SD[dd] = (s32)(temp & 0xffffffff); \
	cpuRegs.HI.SD[dd] = (s32)(temp >> 32); \
 \
	if (_Rd_) cpuRegs.GPR.r[_Rd_].SD[dd] = temp; \
}

void PMADDW() {
	_PMADDW(0, 0);
	_PMADDW(1, 2);
}

void PSLLVW() {
	if (!_Rd_) return;

	cpuRegs.GPR.r[_Rd_].UD[0] = (s32)(cpuRegs.GPR.r[_Rt_].UL[0] <<
									 (cpuRegs.GPR.r[_Rs_].UL[0] & 0x1F));
	cpuRegs.GPR.r[_Rd_].UD[1] = (s32)(cpuRegs.GPR.r[_Rt_].UL[2] <<
									 (cpuRegs.GPR.r[_Rs_].UL[2] & 0x1F));
}

void PSRLVW() {
	if (!_Rd_) return;

	cpuRegs.GPR.r[_Rd_].UD[0] = (cpuRegs.GPR.r[_Rt_].UL[0] >>
								(cpuRegs.GPR.r[_Rs_].UL[0] & 0x1F));
	cpuRegs.GPR.r[_Rd_].UD[1] = (cpuRegs.GPR.r[_Rt_].UL[2] >>
								(cpuRegs.GPR.r[_Rs_].UL[2] & 0x1F));
}

#define _PMSUBW(dd, ss) { \
	s64 temp =	(s64)((s64)cpuRegs.LO.SL[ss] | ((s64)cpuRegs.HI.SL[ss] << 32)) - \
			   ((s64)cpuRegs.GPR.r[_Rs_].SL[ss] * (s64)cpuRegs.GPR.r[_Rt_].SL[ss]); \
 \
	cpuRegs.LO.SD[dd] = (s32)(temp & 0xffffffff); \
	cpuRegs.HI.SD[dd] = (s32)(temp >> 32); \
 \
	if (_Rd_) cpuRegs.GPR.r[_Rd_].SD[dd] = temp; \
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

#define _PMULTW(dd, ss) { \
    s64 temp = (s64)cpuRegs.GPR.r[_Rs_].SL[ss] * (s64)cpuRegs.GPR.r[_Rt_].SL[ss]; \
 \
    cpuRegs.LO.UD[dd] = (s32)(temp & 0xffffffff); \
    cpuRegs.HI.UD[dd] = (s32)(temp >> 32); \
 \
    if (_Rd_) { \
        cpuRegs.GPR.r[_Rd_].SD[dd] = temp; \
    } \
}

void PMULTW() {
	_PMULTW(0, 0);
	_PMULTW(1, 2);
}

#define _PDIVW(dd, ss) \
    if (cpuRegs.GPR.r[_Rt_].UL[ss] != 0) { \
        cpuRegs.LO.SD[dd] = cpuRegs.GPR.r[_Rs_].SL[ss] / cpuRegs.GPR.r[_Rt_].SL[ss]; \
        cpuRegs.HI.SD[dd] = cpuRegs.GPR.r[_Rs_].SL[ss] % cpuRegs.GPR.r[_Rt_].SL[ss]; \
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
#define _PHMADH(hlr, dd, n) { \
	s32 temp = (s32)cpuRegs.GPR.r[_Rs_].SS[n+1] * (s32)cpuRegs.GPR.r[_Rt_].SS[n+1] + \
			   (s32)cpuRegs.GPR.r[_Rs_].SS[n]   * (s32)cpuRegs.GPR.r[_Rt_].SS[n]; \
 \
	cpuRegs.hlr.UL[dd] = temp; \
	/*if (_Rd_) cpuRegs.GPR.r[_Rd_].UL[n] = temp; */\
}

void PHMADH() {				// JayteeMaster: changed a bit to avoid screw up. Also used 0,2,4,6 instead of 0,1,2,3
	_PHMADH(LO, 0, 0);
	_PHMADH(HI, 0, 2);
	_PHMADH(LO, 2, 4);
	_PHMADH(HI, 2, 6);
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
#define _PHMSBH(hlr, dd, n, rdd) { \
	s32 temp = (s32)cpuRegs.GPR.r[_Rs_].SS[n+1] * (s32)cpuRegs.GPR.r[_Rt_].SS[n+1] - \
			   (s32)cpuRegs.GPR.r[_Rs_].SS[n]   * (s32)cpuRegs.GPR.r[_Rt_].SS[n]; \
 \
	cpuRegs.hlr.UL[dd] = temp; \
	/*if (_Rd_) cpuRegs.GPR.r[_Rd_].UL[rdd] = temp;*/ \
}

void PHMSBH() {		// JayteeMaster: changed a bit to avoid screw up
	_PHMSBH(LO, 0, 0, 0);
	_PHMSBH(HI, 0, 2, 1);
	_PHMSBH(LO, 2, 4, 2);
	_PHMSBH(HI, 2, 6, 3);
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

#define _PDIVBW(n) \
	cpuRegs.LO.UL[n] = (s32)(cpuRegs.GPR.r[_Rs_].SL[n] / cpuRegs.GPR.r[_Rt_].SS[0]); \
	cpuRegs.HI.UL[n] = (s16)(cpuRegs.GPR.r[_Rs_].SL[n] % cpuRegs.GPR.r[_Rt_].SS[0]); \

void PDIVBW() {
	if (cpuRegs.GPR.r[_Rt_].US[0] == 0) return;

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

#define _PMADDUW(dd, ss) { \
	u64 tempu =	(u64)((u64)cpuRegs.LO.UL[ss] | ((u64)cpuRegs.HI.UL[ss] << 32)) + \
			   ((u64)cpuRegs.GPR.r[_Rs_].UL[ss] * (u64)cpuRegs.GPR.r[_Rt_].UL[ss]); \
 \
	cpuRegs.LO.SD[dd] = (s32)(tempu & 0xffffffff); \
	cpuRegs.HI.SD[dd] = (s32)(tempu >> 32); \
 \
	if (_Rd_) cpuRegs.GPR.r[_Rd_].UD[dd] = tempu; \
}

void PMADDUW() {
	_PMADDUW(0, 0);
	_PMADDUW(1, 2);
}

void PSRAVW() {
	if (!_Rd_) return;

	cpuRegs.GPR.r[_Rd_].UD[0] = (cpuRegs.GPR.r[_Rt_].SL[0] >>
								(cpuRegs.GPR.r[_Rs_].UL[0] & 0x1F));
	cpuRegs.GPR.r[_Rd_].UD[1] = (cpuRegs.GPR.r[_Rt_].SL[2] >>
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

#define _PMULTUW(dd, ss) { \
    u64 tempu = (u64)cpuRegs.GPR.r[_Rs_].UL[ss] * (u64)cpuRegs.GPR.r[_Rt_].UL[ss]; \
 \
    cpuRegs.LO.UD[dd] = (s32)(tempu & 0xffffffff); \
    cpuRegs.HI.UD[dd] = (s32)(tempu >> 32); \
 \
    if (_Rd_) { \
        cpuRegs.GPR.r[_Rd_].UD[dd] = tempu; \
    } \
}

void PMULTUW() {
	_PMULTUW(0, 0);
	_PMULTUW(1, 2);
}

#define _PDIVUW(dd, ss) \
    if (cpuRegs.GPR.r[_Rt_].UL[ss] != 0) { \
        cpuRegs.LO.UD[dd] = (u64)cpuRegs.GPR.r[_Rs_].UL[ss] / (u64)cpuRegs.GPR.r[_Rt_].UL[ss]; \
        cpuRegs.HI.UD[dd] = (u64)cpuRegs.GPR.r[_Rs_].UL[ss] % (u64)cpuRegs.GPR.r[_Rt_].UL[ss]; \
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

