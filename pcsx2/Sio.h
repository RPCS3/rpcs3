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

struct _sio {
	u16 StatReg;
	u16 ModeReg;
	u16 CtrlReg;
	u16 BaudReg;

	u8  buf[256];
	u32 bufcount;
	u32 parp;
	u32 mcdst,rdwr;
	u8  adrH,adrL;
	u32 padst;
	u32 mtapst;
	u32 packetsize;

	u8  terminator;
	u8  mode;
	u8  mc_command;
	u32 lastsector;
	u32 sector;
	u32 k;
	u32 count;

	// Active pad slot for each port.  Not sure if these automatically reset after each read or not.
	u8 activePadSlot[2];
	// Active memcard slot for each port.  Not sure if these automatically reset after each read or not.
	u8 activeMemcardSlot[2];

	int GetMemcardIndex() const
	{
		return (CtrlReg&0x2000) >> 13;
	}

	int GetMultitapPort() const
	{
		return (CtrlReg&0x2000) >> 13;
	}
};

extern _sio sio;

extern void sioInit();
extern u8 sioRead8();
extern void sioWrite8(u8 value);
extern void sioWriteCtrl16(u16 value);
extern void sioInterrupt();
extern void InitializeSIO(u8 value);
extern void sioEjectCard( uint mcdId );

