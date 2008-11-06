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

#ifndef __VU0_H__
#define __VU0_H__

#include "VU.h"
#define Lcode cpuRegs.code

int  vu0Init();
void vu0Reset();
void vu0ResetRegs();
void vu0Freeze(gzFile f, int Mode);
void vu0Shutdown();

void recResetVU0( void );

void vu0Finish();

extern char *recMemVU0;	/* VU0 blocks */
extern char *recVU0;	   /* VU1 mem */
extern char *recVU0mac;
extern char *recVU0status;
extern char *recVU0clip;
extern char *recVU0Q;
extern char *recVU0cycles;
extern char* recVU0XMMRegs;
extern char *recPtrVU0;

extern u32 vu0recpcold;

#endif /* __VU0_H__ */
