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


#include "IopCommon.h"
#include "CDVD/CDVDaccess.h"

//extern char isoFileName[];

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
	lsn += 150;
	Time[2] = lsn / 4500;			// minuten
	lsn = lsn - Time[2] * 4500;		// minuten rest
	Time[1] = lsn / 75;				// sekunden
	Time[0] = lsn - Time[1] * 75;		// sekunden rest
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


struct CDVD_API
{
	void (CALLBACK *close)();
		
	// Don't need init or shutdown.  iso/nodisc have no init/shutdown and plugin's
	// is handled by the PluginManager.
	
	// Don't need plugin specific things like freeze, test, or other stuff here.
	// Those are handled by the plugin manager specifically.

	_CDVDopen          open;
	_CDVDreadTrack     readTrack;
	_CDVDgetBuffer     getBuffer;
	_CDVDreadSubQ      readSubQ;
	_CDVDgetTN         getTN;
	_CDVDgetTD         getTD;
	_CDVDgetTOC        getTOC;
	_CDVDgetDiskType   getDiskType;
	_CDVDgetTrayStatus getTrayStatus;
	_CDVDctrlTrayOpen  ctrlTrayOpen;
	_CDVDctrlTrayClose ctrlTrayClose;
	_CDVDnewDiskCB     newDiskCB;

	// special functions, not in external interface yet
	_CDVDreadSector    readSector;
	_CDVDgetBuffer2    getBuffer2;
	_CDVDgetDualInfo   getDualInfo;

	wxString (*getUniqueFilename)();
};

extern void cdvdReset();
extern void cdvdVsync();
extern void cdvdActionInterrupt();
extern void cdvdReadInterrupt();

// We really should not have a function with the exact same name as a callback except for case!
extern void cdvdDetectDisk();
extern void cdvdNewDiskCB();
extern u8 cdvdRead(u8 key);
extern void cdvdWrite(u8 key, u8 rt);

// ----------------------------------------------------------------------------
//  Multiple interface system for CDVD
//  used to provide internal CDVDiso and NoDisc, and external plugin interfaces.
//  ---------------------------------------------------------------------------

// ----------------------------------------------------------------------------
//  Multiple interface system for CDVD
//  used to provide internal CDVDiso and NoDisc, and external plugin interfaces.
//  ---------------------------------------------------------------------------

enum CDVD_SourceType
{
	CDVDsrc_Iso = 0,	// use built in ISO api
	CDVDsrc_Plugin,		// use external plugin
	CDVDsrc_NoDisc,		// use built in CDVDnull
};

extern void CDVDsys_ChangeSource( CDVD_SourceType type );
extern CDVD_API* CDVD;		// currently active CDVD access mode api (either Iso, NoDisc, or Plugin)

extern CDVD_API CDVDapi_Plugin;
extern CDVD_API CDVDapi_Iso;
extern CDVD_API CDVDapi_NoDisc;
