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

// stop compiling if NORECBUILD build (only for Visual Studio)
#if !(defined(_MSC_VER) && defined(PCSX2_NORECBUILD))

#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "Common.h"
#include "InterTables.h"
#include "ix86/ix86.h"
#include "iR5900.h"


#ifdef _WIN32
#pragma warning(disable:4244)
#pragma warning(disable:4761)
#endif

/*********************************************************
* Shift arithmetic with constant shift                   *
* Format:  OP rd, rt, sa                                 *
*********************************************************/
#ifndef MOVE_RECOMPILE

REC_FUNC(LUI);
REC_FUNC(MFLO);
REC_FUNC(MFHI);
REC_FUNC(MTLO);
REC_FUNC(MTHI);
REC_FUNC(MOVZ);
REC_FUNC(MOVN);

REC_FUNC( MFHI1 );
REC_FUNC( MFLO1 );
REC_FUNC( MTHI1 );
REC_FUNC( MTLO1 );

#else
REC_FUNC(MFLO, _Rd_);
REC_FUNC(MFHI, _Rd_);
REC_FUNC(MTLO, 0);
REC_FUNC(MTHI, 0);
REC_FUNC(MOVZ, _Rd_);
REC_FUNC(MOVN, _Rd_);

REC_FUNC( MFHI1, _Rd_ );
REC_FUNC( MFLO1, _Rd_ );
REC_FUNC( MTHI1, 0 );
REC_FUNC( MTLO1, 0 );

/*********************************************************
* Load higher 16 bits of the first word in GPR with imm  *
* Format:  OP rt, immediate                              *
*********************************************************/

////////////////////////////////////////////////////
void recLUI( void ) {
	int rtreg;

	if (!_Rt_) return;

	MOV64I32toM((uptr)&cpuRegs.GPR.r[ _Rt_ ].UL[ 0 ], (s32)(_Imm_ << 16));
}

#endif

#endif // PCSX2_NORECBUILD
