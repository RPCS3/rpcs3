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

#pragma once

extern s32  DoCDVDopen(const char* pTitleFilename);
extern void DoCDVDclose();
extern s32  DoCDVDreadSector(u8* buffer, u32 lsn, int mode);
extern s32  DoCDVDreadTrack(u32 lsn, int mode);
extern s32  DoCDVDgetBuffer(u8* buffer);
//extern s32  DoCDVDreadSubQ(u32 lsn, cdvdSubQ* subq);
extern void DoCDVDnewDiskCB(void (*callback)());
extern s32  DoCDVDdetectDiskType();
extern void DoCDVDresetDiskTypeCache();

