/*  PCSX2 - PS2 Emulator for PCs
 *  Copyright (C) 2002-2010  PCSX2 Dev Team
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


#include "IopCommon.h"
#include "CDVD/CDVDaccess.h"

#define btoi(b)		((b)/16*10 + (b)%16)		/* BCD to u_char */
#define itob(i)		((i)/10*16 + (i)%10)		/* u_char to BCD */

static __forceinline s32 msf_to_lsn(u8 *Time)
{
	u32 lsn;

	lsn = Time[2];
	lsn +=(Time[1] - 2) * 75;
	lsn += Time[0] * 75 * 60;
	return lsn;
}

static __forceinline s32 msf_to_lba(u8 m, u8 s, u8 f)
{
	u32 lsn;
	lsn = f;
	lsn += (s - 2) * 75;
	lsn += m * 75 * 60;
	return lsn;
}

static __forceinline void lsn_to_msf(u8 *Time, s32 lsn)
{
	u8 m, s, f;

	lsn += 150;
	m = lsn / 4500; 		// minuten
	lsn = lsn - m * 4500;	// minuten rest
	s = lsn / 75;			// sekunden
	f = lsn - (s * 75);		// sekunden rest
	Time[0] = itob(m);
	Time[1] = itob(s);
	Time[2] = itob(f);
}

static __forceinline void lba_to_msf(s32 lba, u8* m, u8* s, u8* f)
{
	lba += 150;
	*m = lba / (60 * 75);
	*s = (lba / 75) % 60;
	*f = lba % 75;
}

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

extern void cdvdReloadElfInfo();

extern wxString DiscID;
