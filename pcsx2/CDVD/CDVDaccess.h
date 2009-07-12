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

#ifndef __CDVD_ACCESS_H__
#define __CDVD_ACCESS_H__

s32 DoCDVDinit();
s32 DoCDVDopen(const char* pTitleFilename);
void DoCDVDclose();
void DoCDVDshutdown();
s32 DoCDVDreadSector(u8* buffer, u32 lsn, int mode);
s32 DoCDVDreadTrack(u32 lsn, int mode);
s32 DoCDVDgetBuffer(u8* buffer);
s32 DoCDVDreadSubQ(u32 lsn, cdvdSubQ* subq);
s32 DoCDVDgetTN(cdvdTN *Buffer);
s32 DoCDVDgetTD(u8 Track, cdvdTD *Buffer);
s32 DoCDVDgetTOC(void* toc);
s32 DoCDVDgetDiskType();
s32 DoCDVDgetTrayStatus();
s32 DoCDVDctrlTrayOpen();
s32 DoCDVDctrlTrayClose();
void DoCDVDnewDiskCB(void (*callback)());
s32 DoCDVDdetectDiskType();
void DoCDVDresetDiskTypeCache();
#endif /* __CDVD_H__ */
