/*  PCSX2 - PS2 Emulator for PCs
 *  Copyright (C) 2002-2014 David Quintana [gigaherz]
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

#pragma once
#include "..\dev9.h"

u8 CALLBACK smap_read8(u32 addr);
u16 CALLBACK smap_read16(u32 addr);
u32 CALLBACK smap_read32(u32 addr);

void CALLBACK smap_write8(u32 addr, u8 value);
void CALLBACK smap_write16(u32 addr, u16 value);
void CALLBACK smap_write32(u32 addr, u32 value);

void CALLBACK smap_readDMA8Mem(u32 *pMem, int size);
void CALLBACK smap_writeDMA8Mem(u32 *pMem, int size);