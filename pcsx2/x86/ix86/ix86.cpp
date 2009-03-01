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
/*
 * ix86 core v0.6.2
 * Authors: linuzappz <linuzappz@pcsx.net>
 *			alexey silinov
 *			goldfinger
 *			zerofrog(@gmail.com)
 *			cottonvibes(@gmail.com)
 */

#pragma once
#include "PrecompiledHeader.h"
#include "System.h"
#include "ix86.h"

u8  *x86Ptr[3]; // 3 rec instances (can be increased if needed)
				// <0> = Main Instance, <1> = VU0 Instance, <2> = VU1 Instance
u8  *j8Ptr[32];
u32 *j32Ptr[32];

PCSX2_ALIGNED16(unsigned int p[4]);
PCSX2_ALIGNED16(unsigned int p2[4]);
PCSX2_ALIGNED16(float f[4]);

XMMSSEType g_xmmtypes[XMMREGS] = { XMMT_INT };
