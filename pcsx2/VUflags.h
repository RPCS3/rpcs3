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

#pragma once
#include "VU.h"

extern u32  VU_MACx_UPDATE(VURegs * VU, float x);
extern u32  VU_MACy_UPDATE(VURegs * VU, float y);
extern u32  VU_MACz_UPDATE(VURegs * VU, float z);
extern u32  VU_MACw_UPDATE(VURegs * VU, float w);
extern void VU_MACx_CLEAR(VURegs * VU);
extern void VU_MACy_CLEAR(VURegs * VU);
extern void VU_MACz_CLEAR(VURegs * VU);
extern void VU_MACw_CLEAR(VURegs * VU);
extern void VU_STAT_UPDATE(VURegs * VU);
