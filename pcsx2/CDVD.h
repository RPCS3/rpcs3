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
	int Readed;
	int Reading;
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

void cdvdReset();
void cdvdVsync();
extern void  cdvdActionInterrupt();
extern void  cdvdReadInterrupt();
void cdvdNewDiskCB();
u8   cdvdRead04(void);
u8   cdvdRead05(void);
u8   cdvdRead06(void);
u8   cdvdRead07(void);
u8   cdvdRead08(void);
u8   cdvdRead0A(void);
u8   cdvdRead0B(void);
u8   cdvdRead0C(void);
u8   cdvdRead0D(void);
u8   cdvdRead0E(void);
u8   cdvdRead0F(void);
u8   cdvdRead13(void);
u8   cdvdRead15(void);
u8   cdvdRead16(void);
u8   cdvdRead17(void);
u8   cdvdRead18(void);
u8   cdvdRead20(void);
u8   cdvdRead21(void);
u8   cdvdRead22(void);
u8   cdvdRead23(void);
u8   cdvdRead24(void);
u8   cdvdRead28(void);
u8   cdvdRead29(void);
u8   cdvdRead2A(void);
u8   cdvdRead2B(void);
u8   cdvdRead2C(void);
u8   cdvdRead30(void);
u8   cdvdRead31(void);
u8   cdvdRead32(void);
u8   cdvdRead33(void);
u8   cdvdRead34(void);
u8   cdvdRead38(void);
u8   cdvdRead39(void);
u8   cdvdRead3A(void);
void cdvdWrite04(u8 rt);
void cdvdWrite05(u8 rt);
void cdvdWrite06(u8 rt);
void cdvdWrite07(u8 rt);
void cdvdWrite08(u8 rt);
void cdvdWrite0A(u8 rt);
void cdvdWrite0F(u8 rt);
void cdvdWrite14(u8 rt);
void cdvdWrite16(u8 rt);
void cdvdWrite17(u8 rt);
void cdvdWrite18(u8 rt);
void cdvdWrite3A(u8 rt);

// Platform dependent system time assignment (see WinMisc / LnxMisc)
extern void cdvdSetSystemTime( cdvdStruct& setme );

#endif /* __CDVD_H__ */
