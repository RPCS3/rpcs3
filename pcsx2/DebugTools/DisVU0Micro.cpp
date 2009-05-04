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

#include "Debug.h"

static char ostr[1024];

// Type deffinition of our functions
#define DisFInterface  (u32 code, u32 pc)
#define DisFInterfaceT (u32, u32)
#define DisFInterfaceN (code, pc)

typedef char* (*TdisR5900F)DisFInterface;

// These macros are used to assemble the disassembler functions
#define MakeDisF(fn, b) \
	char* fn DisFInterface { \
		sprintf (ostr, "%8.8x %8.8x:", pc, code); \
		b; /*ostr[(strlen(ostr) - 1)] = 0;*/ return ostr; \
	}

//Lower/Upper instructions can use that..
#define _Ft_ ((code >> 16) & 0x1F)  // The rt part of the instruction register 
#define _Fs_ ((code >> 11) & 0x1F)  // The rd part of the instruction register 
#define _Fd_ ((code >>  6) & 0x1F)  // The sa part of the instruction register
#define _It_ (_Ft_ & 15)
#define _Is_ (_Fs_ & 15)
#define _Id_ (_Fd_ & 15)

#define dName(i)	sprintf(ostr, "%s %-7s,", ostr, i)
#define dNameU(i)	{ char op[256]; sprintf(op, "%s.%s%s%s%s", i, _X ? "x" : "", _Y ? "y" : "", _Z ? "z" : "", _W ? "w" : ""); sprintf(ostr, "%s %-7s,", ostr, op); }


#define dCP2128f(i)		sprintf(ostr, "%s w=%f z=%f y=%f x=%f (%s),", ostr, VU0.VF[i].f.w, VU0.VF[i].f.z, VU0.VF[i].f.y, VU0.VF[i].f.x, disRNameCP2f[i])
#define dCP232x(i)		sprintf(ostr, "%s x=%f (%s),", ostr, VU0.VF[i].f.x, disRNameCP2f[i])
#define dCP232y(i)		sprintf(ostr, "%s y=%f (%s),", ostr, VU0.VF[i].f.y, disRNameCP2f[i])
#define dCP232z(i)		sprintf(ostr, "%s z=%f (%s),", ostr, VU0.VF[i].f.z, disRNameCP2f[i])
#define dCP232w(i)		sprintf(ostr, "%s w=%f (%s),", ostr, VU0.VF[i].f.w, disRNameCP2f[i])
#define dCP2ACCf()		sprintf(ostr, "%s w=%f z=%f y=%f x=%f (ACC),", ostr, VU0.ACC.f.w, VU0.ACC.f.z, VU0.ACC.f.y, VU0.ACC.f.x)
#define dCP232i(i)		sprintf(ostr, "%s %8.8x (%s),", ostr, VU0.VI[i].UL, disRNameCP2i[i])
#define dCP232iF(i)		sprintf(ostr, "%s %f (%s),", ostr, VU0.VI[i].F, disRNameCP2i[i])
#define dCP232f(i, j)	sprintf(ostr, "%s Q %s=%f (%s),", ostr, CP2VFnames[j], VU0.VF[i].F[j], disRNameCP2f[i])
#define dImm5()			sprintf(ostr, "%s %d,", ostr, (code >> 6) & 0x1f)
#define dImm11()		sprintf(ostr, "%s %d,", ostr, code & 0x7ff)
#define dImm15()		sprintf(ostr, "%s %d,", ostr, ( ( code >> 10 ) & 0x7800 ) | ( code & 0x7ff ))

#define _X ((code>>24) & 0x1)
#define _Y ((code>>23) & 0x1)
#define _Z ((code>>22) & 0x1)
#define _W ((code>>21) & 0x1)

#define _Fsf_ ((code >> 21) & 0x03)
#define _Ftf_ ((code >> 23) & 0x03)


/*********************************************************
* Unknown instruction (would generate an exception)      *
* Format:  ?                                             *
*********************************************************/
//extern char* disNULL DisFInterface;
static MakeDisF(disNULL,		dName("*** Bad OP ***");)

#include "DisVUmicro.h"
#include "DisVUops.h"
#include "VU.h"

_disVUOpcodes(VU0);
_disVUTables(VU0);

