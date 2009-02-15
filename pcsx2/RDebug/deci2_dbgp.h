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

#ifndef __DECI2DBGP_H__
#define __DECI2DBGP_H__

#include "Common.h"
#include "deci2.h"

void D2_DBGP(const u8 *inbuffer, u8 *outbuffer, char *message, char *eepc, char *ioppc, char *eecy, char *iopcy);
void sendBREAK(u8 source, u16 id, u8 code, u8 result, u8 count);

#endif//__DECI2DBGP_H__
