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

#include "Debug.h"
#include "VUmicro.h"

static char ostr[1024];

// Type deffinition of our functions
#define DisFInterface  (u32 code, u32 pc)
#define DisFInterfaceT (u32, u32)
#define DisFInterfaceN (code, pc)

typedef char* (*TdisR5900F)DisFInterface;

// These macros are used to assemble the disassembler functions
#define MakeDisF(fn, b) \
	char* fn DisFInterface { \
		if( !CHECK_VU1REC ) sprintf (ostr, "%8.8x %8.8x:", pc, code); \
		else ostr[0] = 0; \
		b; /*ostr[(strlen(ostr) - 1)] = 0;*/ return ostr; \
	}

//Lower/Upper instructions can use that..
#define _Ft_ ((code >> 16) & 0x1F)  // The rt part of the instruction register 
#define _Fs_ ((code >> 11) & 0x1F)  // The rd part of the instruction register 
#define _Fd_ ((code >>  6) & 0x1F)  // The sa part of the instruction register
#define _It_ (_Ft_ & 15)
#define _Is_ (_Fs_ & 15)
#define _Id_ (_Fd_ & 15)

#define dName(i) sprintf(ostr, "%s %-12s", ostr, i); \

#define dNameU(i) { \
	char op[256]; sprintf(op, "%s.%s%s%s%s", i, _X ? "x" : "", _Y ? "y" : "", _Z ? "z" : "", _W ? "w" : ""); \
	sprintf(ostr, "%s %-12s", ostr, op); \
}

#define dCP2128f(i) { \
	if( CHECK_VU1REC ) sprintf(ostr, "%s %s,", ostr, disRNameCP2f[i]); \
	else sprintf(ostr, "%s w=%f (%8.8x) z=%f (%8.8x) y=%f (%8.8xl) x=%f (%8.8x) (%s),", ostr, VU1.VF[i].f.w, VU1.VF[i].UL[3], VU1.VF[i].f.z, VU1.VF[i].UL[2], VU1.VF[i].f.y, VU1.VF[i].UL[1], VU1.VF[i].f.x, VU1.VF[i].UL[0], disRNameCP2f[i]); \
} \

#define dCP232x(i) { \
	if( CHECK_VU1REC ) sprintf(ostr, "%s %s,", ostr, disRNameCP2f[i]); \
	else sprintf(ostr, "%s x=%f (%s),", ostr, VU1.VF[i].f.x, disRNameCP2f[i]); \
} \

#define dCP232y(i) { \
	if( CHECK_VU1REC ) sprintf(ostr, "%s %s,", ostr, disRNameCP2f[i]); \
	else sprintf(ostr, "%s y=%f (%s),", ostr, VU1.VF[i].f.y, disRNameCP2f[i]); \
} \

#define dCP232z(i) { \
	if( CHECK_VU1REC ) sprintf(ostr, "%s %s,", ostr, disRNameCP2f[i]); \
	else sprintf(ostr, "%s z=%f (%s),", ostr, VU1.VF[i].f.z, disRNameCP2f[i]); \
} 

#define dCP232w(i) { \
	if( CHECK_VU1REC ) sprintf(ostr, "%s %s,", ostr, disRNameCP2f[i]); \
	else sprintf(ostr, "%s w=%f (%s),", ostr, VU1.VF[i].f.w, disRNameCP2f[i]); \
}

#define dCP2ACCf() { \
	if( CHECK_VU1REC ) sprintf(ostr, "%s ACC,", ostr); \
	else sprintf(ostr, "%s w=%f z=%f y=%f x=%f (ACC),", ostr, VU1.ACC.f.w, VU1.ACC.f.z, VU1.ACC.f.y, VU1.ACC.f.x); \
} \

#define dCP232i(i) { \
	if( CHECK_VU1REC ) sprintf(ostr, "%s %s,", ostr, disRNameCP2i[i]); \
	else sprintf(ostr, "%s %8.8x (%s),", ostr, VU1.VI[i].UL, disRNameCP2i[i]); \
}

#define dCP232iF(i) { \
	if( CHECK_VU1REC ) sprintf(ostr, "%s %s,", ostr, disRNameCP2i[i]); \
	else sprintf(ostr, "%s %f (%s),", ostr, VU1.VI[i].F, disRNameCP2i[i]); \
}

#define dCP232f(i, j) { \
	if( CHECK_VU1REC ) sprintf(ostr, "%s %s%s,", ostr, disRNameCP2f[i], CP2VFnames[j]); \
	else sprintf(ostr, "%s %s=%f (%s),", ostr, CP2VFnames[j], VU1.VF[i].F[j], disRNameCP2f[i]); \
}

#define dImm5()			sprintf(ostr, "%s %d,", ostr, (s16)((code >> 6) & 0x10 ? 0xfff0 | ((code >> 6) & 0xf) : (code >> 6) & 0xf))
#define dImm11()		sprintf(ostr, "%s %d,", ostr, (s16)(code & 0x400 ? 0xfc00 | (code & 0x3ff) : code & 0x3ff))
#define dImm15()		sprintf(ostr, "%s %d,", ostr, ( ( code >> 10 ) & 0x7800 ) | ( code & 0x7ff ))

#define _X ((code>>24) & 0x1)
#define _Y ((code>>23) & 0x1)
#define _Z ((code>>22) & 0x1)
#define _W ((code>>21) & 0x1)

#define _Fsf_ ((code >> 21) & 0x03)
#define _Ftf_ ((code >> 23) & 0x03)

/*********************************************************
* Unknown instruction (would generate an exception)       *
* Format:  ?                                             *
*********************************************************/
//extern char* disNULL DisFInterface;
static MakeDisF(disNULL,		dName("*** Bad OP ***");)

#include "DisVUmicro.h"
#include "DisVUops.h"

_disVUOpcodes(VU1);
_disVUTables(VU1);

