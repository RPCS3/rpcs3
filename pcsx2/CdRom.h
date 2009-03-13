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

#ifndef __CDROM_H__
#define __CDROM_H__

#include "IopCommon.h"
#include "Decode_XA.h"
#include "PS2Edefs.h"

struct cdrStruct
{
	u8 OCUP;
	u8 Reg1Mode;
	u8 Reg2;
	u8 CmdProcess;
	u8 Ctrl;
	u8 Stat;

	u8 StatP;

	u8 Transfer[2352];
	u8 *pTransfer;

	u8 Prev[4];
	u8 Param[8];
	u8 Result[8];

	u8 ParamC;
	u8 ParamP;
	u8 ResultC;
	u8 ResultP;
	u8 ResultReady;
	u8 Cmd;
	u8 Readed;
	unsigned long Reading;

	cdvdTN ResultTN;
	u8 ResultTD[4];
	u8 SetSector[4];
	u8 SetSectorSeek[4];
	u8 Track;
	int Play;
	int CurTrack;
	int Mode, File, Channel, Muted;
	int Reset;
	int RErr;
	int FirstSector;

	xa_decode_t Xa;

	int Init;

	u8 Irq;
	unsigned long eCycle;

	char Unused[4087];
};

extern cdrStruct cdr;

s32  MSFtoLSN(u8 *Time);
void LSNtoMSF(u8 *Time, s32 lsn);
void AddIrqQueue(u8 irq, unsigned long ecycle);

void cdrReset();
void  cdrInterrupt();
void  cdrReadInterrupt();
u8   cdrRead0(void);
u8   cdrRead1(void);
u8   cdrRead2(void);
u8   cdrRead3(void);
void cdrWrite0(u8 rt);
void cdrWrite1(u8 rt);
void cdrWrite2(u8 rt);
void cdrWrite3(u8 rt);

#endif /* __CDROM_H__ */
