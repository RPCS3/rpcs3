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

#ifndef __CDVD_H__
#define __CDVD_H__

#include "IopCommon.h"
#include "CDVD/CDVDaccess.h"

extern char isoFileName[];

#define btoi(b)		((b)/16*10 + (b)%16)		/* BCD to u_char */
#define itob(i)		((i)/10*16 + (i)%10)		/* u_char to BCD */

struct cdvdRTC {
	u8 status;
	u8 second;
	u8 minute;
	u8 hour;
	u8 pad;
	u8 day;
	u8 month;
	u8 year;
};

struct cdvdStruct {
	u8 nCommand;
	u8 Ready;
	u8 Error;
	u8 PwOff;
	u8 Status;
	u8 Type;
	u8 sCommand;
	u8 sDataIn;
	u8 sDataOut;
	u8 HowTo;

	u8 Param[32];
	u8 Result[32];

	u8 ParamC;
	u8 ParamP;
	u8 ResultC;
	u8 ResultP;

	u8 CBlockIndex;
	u8 COffset;
	u8 CReadWrite;
	u8 CNumBlocks;

	int RTCcount;
	cdvdRTC RTC;

	u32 Sector;
	int nSectors;
	int Readed; // change to bool. --arcum42
	int Reading; // same here.
	int ReadMode;
	int BlockSize; // Total bytes transfered at 1x speed
	int Speed;
	int RetryCnt;
	int RetryCntP;
	int RErr;
	int SpindlCtrl;

	u8 Key[16];
	u8 KeyXor;
	u8 decSet;

	u8  mg_buffer[65536];
	int mg_size;
	int mg_maxsize;
	int mg_datatype;//0-data(encrypted); 1-header
	u8	mg_kbit[16];//last BIT key 'seen'
	u8	mg_kcon[16];//last content key 'seen'

	u8  Action;			// the currently scheduled emulated action
	u32 SeekToSector;	// Holds the destination sector during seek operations.
	u32 ReadTime;		// Avg. time to read one block of data (in Iop cycles)
	bool Spinning;		// indicates if the Cdvd is spinning or needs a spinup delay
};

extern void cdvdReset();
extern void cdvdVsync();
extern void cdvdActionInterrupt();
extern void cdvdReadInterrupt();

// We really should not have a function with the exact same name as a callback except for case!
extern void cdvdNewDiskCB();
extern u8 cdvdRead(u8 key);
extern void cdvdWrite(u8 key, u8 rt);

// Platform dependent system time assignment (see WinMisc / LnxMisc)
extern void cdvdSetSystemTime(cdvdStruct& setme);
		
#endif /* __CDVD_H__ */
