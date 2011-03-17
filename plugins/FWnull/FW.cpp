/*  FWnull
 *  Copyright (C) 2004-2010  PCSX2 Dev Team
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

#include <stdlib.h>
#include <string>
using namespace std;

#include "FW.h"

const u8 version  = PS2E_FW_VERSION;
const u8 revision = 0;
const u8 build    = 7;    // increase that with each version

static char *libraryName = "FWnull Driver";

string s_strIniPath="inis/";
string s_strLogPath = "logs/";

u8 phyregs[16];
s8 *fwregs;
Config conf;
PluginLog FWLog;

void (*FWirq)();

void LogInit()
{
	const std::string LogFile(s_strLogPath + "/FWnull.log");
	setLoggingState();
	FWLog.Open(LogFile);
}

EXPORT_C_(void)  FWsetLogDir(const char* dir)
{
	// Get the path to the log directory.
	s_strLogPath = (dir==NULL) ? "logs/" : dir;
	
	// Reload the log file after updated the path
	FWLog.Close();
	LogInit();
}

EXPORT_C_(u32) PS2EgetLibType()
{
	return PS2E_LT_FW;
}

EXPORT_C_(char*) PS2EgetLibName()
{
	return libraryName;
}

EXPORT_C_(u32) PS2EgetLibVersion2(u32 type)
{
	return (version<<16) | (revision<<8) | build;
}

EXPORT_C_(s32) FWinit()
{
	LoadConfig();
	LogInit();
	FWLog.WriteLn("FWnull plugin version %d,%d", revision, build);
	FWLog.WriteLn("Initializing FWnull");

	memset(phyregs, 0, sizeof(phyregs));
	// Initializing our registers.
	fwregs = (s8*)calloc(0x10000,1);
	if (fwregs == NULL)
	{
		FWLog.Message("Error allocating Memory");
		return -1;
	}
	return 0;
}

EXPORT_C_(void) FWshutdown()
{
	// Freeing the registers.
	free(fwregs);
	fwregs = NULL;

	FWLog.Close();
}

EXPORT_C_(s32) FWopen(void *pDsp)
{
	FWLog.WriteLn("Opening FWnull.");

	return 0;
}

EXPORT_C_(void) FWclose()
{
	// Close the plugin.
	FWLog.WriteLn("Closing FWnull.");
}

void PHYWrite()
{
	u8 reg = (PHYACC >> 8) & 0xf;
	u8 data = PHYACC & 0xff;

	phyregs[reg] = data;

	PHYACC &= ~0x4000ffff;
}

void PHYRead()
{
	u8 reg = (PHYACC >> 24) & 0xf;
	u8 data = (PHYACC >> 16) & 0xff;

	PHYACC &= ~0x80000000;

	PHYACC |= phyregs[reg] | (reg << 8);
	
	if(fwRu32(0x8424) & 0x40000000) //RRx interrupt mask
	{
		fwRu32(0x8420) |= 0x40000000;
		FWirq();
	}	
}
EXPORT_C_(u32) FWread32(u32 addr)
{
	u32 ret = 0;

	switch (addr)
	{
		//Node ID Register the top part is default, bottom part i got from my ps2
		case 0x1f808400:
			ret = /*(0x3ff << 22) | 1;*/ 0xffc00001;
		break;
		// Control Register 2
		case 0x1f808410:
			ret = fwRu32(addr); //SCLK OK (Needs to be set when FW is "Ready"
			break;
		//Interrupt 0 Register
		case 0x1f808420:
			ret = fwRu32(addr);
			break;

		//Dunno what this is, but my home console always returns this value 0x10000001
		//Seems to be related to the Node ID however (does some sort of compare/check)
		case 0x1f80847c:
			ret = 0x10000001;
			break;

		// Include other relevant 32 bit addresses we need to catch here.
		default:
			// By default, read fwregs.
			ret = fwRu32(addr);
			break;
	}

	FWLog.WriteLn("FW read mem 0x%x: 0x%x", addr, ret);

	return ret;
}

EXPORT_C_(void) FWwrite32(u32 addr, u32 value)
{
	switch (addr)
	{
//		Include other memory locations we want to catch here.
//		For example:
//
//		case 0x1f808400:
//		case 0x1f808414:
//		case 0x1f808420:
//		case 0x1f808428:
//		case 0x1f808430:
//

		//PHY access
		case 0x1f808414:
		//If in read mode (top bit set) we read the PHY register requested then set the RRx interrupt if it's enabled
		//Im presuming we send that back to pcsx2 then. This register stores the result, plus whatever was written (minus the read/write flag
		fwRu32(addr) = value; //R/W Bit cleaned in underneath function
		if(value & 0x40000000) //Writing to PHY
		{
			PHYWrite();
		}
		else if(value & 0x80000000) //Reading from PHY
		{
			PHYRead();
		}
		break;

		//Control Register 0
		case 0x1f808408:
			//This enables different functions of the link interface
			//Just straight writes, should brobably struct these later.
			//Default written settings (on unreal tournament) are
			//Urcv M = 1
			//RSP 0 = 1
			//Retlim = 0xF
			//Cyc Tmr En = 1
			//Bus ID Rst = 1
			//Rcv Self ID = 1
			fwRu32(addr) = value;
		//	if((value & 0x800000) && (fwRu32(0x842C) & 0x2))
		//	{
		//		fwRu32(0x8428) |= 0x2;
		//		FWirq();
		//	}
			fwRu32(addr) &= ~0x800000;
			break;
		//Control Register 2
		case 0x1f808410:// fwRu32(addr) = value; break;
			//Ignore writes to this for now, apart from 0x2 which is Link Power Enable
			//0x8 is SCLK OK (Ready) which should be set for emulation
			fwRu32(addr) = 0x8 /*| value & 0x2*/;
			break;
		//Interrupt 0 Register
		case 0x1f808420:
		//Interrupt 1 Register
		case 0x1f808428:
		//Interrupt 2 Register
		case 0x1f808430:
			//Writes of 1 clear the corresponding bits
			fwRu32(addr) &= ~value;
			break;
		//Interrupt 0 Register Mask
		case 0x1f808424:
		//Interrupt 1 Register Mask
		case 0x1f80842C:
		//Interrupt 2 Register Mask
		case 0x1f808434:
			//These are direct writes (as it's a mask!)
			fwRu32(addr) = value;
			break;
		//DMA Control and Status Register 0
		case 0x1f8084B8:
			fwRu32(addr) = value;
			break;
		//DMA Control and Status Register 1
		case 0x1f808538:
			fwRu32(addr) = value;
			break;
		default:
			// By default, just write it to fwregs.
			fwRu32(addr) = value;
			break;
	}
	FWLog.WriteLn("FW write mem 0x%x: 0x%x", addr, value);
}

EXPORT_C_(void) FWirqCallback(void (*callback)())
{
	// Register FWirq, so we can trigger an interrupt with it later.
	FWirq = callback;
}

EXPORT_C_(void) FWsetSettingsDir(const char* dir)
{
	// Find out from pcsx2 where we are supposed to put our ini file.
    s_strIniPath = (dir == NULL) ? "inis/" : dir;
}

EXPORT_C_(s32) FWfreeze(int mode, freezeData *data)
{
	// This should store or retrieve any information, for if emulation
	// gets suspended, or for savestates.
	switch(mode)
	{
		case FREEZE_LOAD:
			// Load previously saved data.
			break;
		case FREEZE_SAVE:
			// Save data.
			break;
		case FREEZE_SIZE:
			// return the size of the data.
			break;
	}
	return 0;
}

EXPORT_C_(s32) FWtest()
{
	// 0 if the plugin works, non-0 if it doesn't.
	return 0;
}
