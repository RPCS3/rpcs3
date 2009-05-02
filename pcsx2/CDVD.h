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

/*
Interrupts - values are flag bits.

0x00	 No interrupt		  
0x01	 Data Ready		  
0x02	 Command Complete 	  
0x03	 Acknowledge (reserved) 
0x04	 End of Data Detected   
0x05	 Error Detected 	  
0x06	 Drive Not Ready	

In limited experimentation I found that PS2 apps respond actively to use of the
'Data Ready' flag -- in that they'll almost immediately initiate a DMA transfer
after receiving an Irq with that as the cause.  But the question is, of course,
*when* to use it.  Adding it into some locations of CDVD reading only slowed
games down and broke things.

Using Drive Not Ready also invokes basic error handling from the Iop Bios, but
without proper emulation of the cdvd status flag it also tends to break things.

*/

enum CdvdIrqId
{
	Irq_None = 0
,	Irq_DataReady = 0
,	Irq_CommandComplete
,	Irq_Acknowledge
,	Irq_EndOfData
,	Irq_Error
,	Irq_NotReady

};

/* is cdvd.Status only for NCMDS? (linuzappz) */
enum cdvdStatus
{
	CDVD_STATUS_NONE            = 0x00, // not sure ;)
	CDVD_STATUS_SEEK_COMPLETE   = 0x0A,
};

enum cdvdready
{
	CDVD_NOTREADY = 0x00,
	CDVD_READY1 = 0x40,
	CDVD_READY2 = 0x4e // This is used in a few places for some reason. 
	//It would be worth checking if this was just a typo made at some point.
};

// Cdvd actions tell the emulator how and when to respond to certain requests.
// Actions are handled by the cdvdInterrupt()
enum cdvdActions
{
	cdvdAction_None = 0
,	cdvdAction_Seek
,	cdvdAction_Standby
,	cdvdAction_Stop
,	cdvdAction_Break
,	cdvdAction_Read			// note: not used yet.
};

//////////////////////////////////////////////////////////////////////////////////////////
// Cdvd Block Read Cycle Timings
//
// The PS2 CDVD effectively has two seek modes -- the normal/slow one (est. avg seeks being
// around 120-160ms), and a faster seek which has an estimated seek time of about 35-40ms.
// Fast seeks happen when the destination sector is within a certain range of the starting
// point, such that abs(start-dest) is less than the value in the tbl_FastSeekDelta.
//
// CDVDs also have a secondary seeking method used when the destination is close enough
// that a contiguous sector read can reach the sector faster than initiating a full seek.
// Typically this value is very low.

enum CDVD_MODE_TYPE
{
	MODE_CDROM = 0,
	MODE_DVDROM,
};

static const uint tbl_FastSeekDelta[3] =
{
	4371,	// CD-ROM
	14764,	// Single-layer DVD-ROM
	13360	// dual-layer DVD-ROM [currently unused]
};

// if a seek is within this many blocks, read instead of seek.
// These values are arbitrary assumptions.  Not sure what the real PS2 uses.
static const uint tbl_ContigiousSeekDelta[3] =
{
	8,		// CD-ROM
	16,		// single-layer DVD-ROM
	16,		// dual-layer DVD-ROM [currently unused]
};

// Note: DVD read times are modified to be faster, because games seem to be a lot more
// concerned with accurate(ish) seek delays and less concerned with actual block read speeds.
// Translation: it's a minor speedhack :D

static const uint PSX_CD_READSPEED = 153600;   // 1 Byte Time @ x1 (150KB = cd x 1)
static const uint PSX_DVD_READSPEED = 1382400 + 256000; // normal is 1 Byte Time @ x1 (1350KB = dvd x 1).


// Legacy Note: FullSeek timing causes many games to load very slow, but it likely not the real problem.
// Games breaking with it set to PSXCLK*40 : "wrath unleashed" and "Shijou Saikyou no Deshi Kenichi".

static const uint Cdvd_FullSeek_Cycles = (PSXCLK*100) / 1000;		// average number of cycles per fullseek (100ms)
static const uint Cdvd_FastSeek_Cycles = (PSXCLK*30) / 1000;		// average number of cycles per fastseek (37ms)

static const __unused char *mg_zones[8] = {"Japan", "USA", "Europe", "Oceania", "Asia", "Russia", "China", "Mexico"};

static const __unused char *nCmdName[0x100]= {
	"CdSync", 
	"CdNop", 
	"CdStandby", 
	"CdStop",
	"CdPause",
	"CdSeek",
	"CdRead", 
	"CdReadCDDA",
	"CdReadDVDV", 
	"CdGetToc", 
	"", 
	"NCMD_B",
	"CdReadKey", 
	"", 
	"sceCdReadXCDDA",
	"sceCdChgSpdlCtrl",
};

enum nCmds
{
	N_CD_SYNC = 0x00, // CdSync
	N_CD_NOP = 0x01, // CdNop
	N_CD_STANDBY = 0x02, // CdStandby
	N_CD_STOP = 0x03, // CdStop
	N_CD_PAUSE = 0x04, // CdPause
	N_CD_SEEK = 0x05, // CdSeek
	N_CD_READ = 0x06, // CdRead
	N_CD_READ_CDDA = 0x07, // CdReadCDDA
	N_DVD_READ = 0x08, // DvdRead
	N_CD_GET_TOC = 0x09, // CdGetToc & cdvdman_call19
	N_CMD_B = 0x0B, // CdReadKey
	N_CD_READ_KEY = 0x0C, // CdReadKey
	N_CD_READ_XCDDA = 0x0E, // CdReadXCDDA
	N_CD_CHG_SPDL_CTRL = 0x0F, // CdChgSpdlCtrl
};

static const __unused char *sCmdName[0x100]= {
	"", "sceCdGetDiscType", "sceCdReadSubQ", "subcommands",//sceCdGetMecaconVersion, read/write console id, read renewal date
	"", "sceCdTrayState", "sceCdTrayCtrl", "",
	"sceCdReadClock", "sceCdWriteClock", "sceCdReadNVM", "sceCdWriteNVM",
	"sceCdSetHDMode", "", "", "sceCdPowerOff",
	"", "", "sceCdReadILinkID", "sceCdWriteILinkID", /*10*/
	"sceAudioDigitalOut", "sceForbidDVDP", "sceAutoAdjustCtrl", "sceCdReadModelNumber",
	"sceWriteModelNumber", "sceCdForbidCD", "sceCdBootCertify", "sceCdCancelPOffRdy",
	"sceCdBlueLEDCtl", "", "sceRm2Read", "sceRemote2_7",//Rm2PortGetConnection?
	"sceRemote2_6", "sceCdWriteWakeUpTime", "sceCdReadWakeUpTime", "", /*20*/
	"sceCdRcBypassCtl", "", "", "",
	"", "sceCdNoticeGameStart", "", "",
	"sceCdXBSPowerCtl", "sceCdXLEDCtl", "sceCdBuzzerCtl", "",
	"", "sceCdSetMediumRemoval", "sceCdGetMediumRemoval", "sceCdXDVRPReset", /*30*/
	"", "", "__sceCdReadRegionParams", "__sceCdReadMAC",
	"__sceCdWriteMAC", "", "", "",
	"", "", "__sceCdWriteRegionParams", "",
	"sceCdOpenConfig", "sceCdReadConfig", "sceCdWriteConfig", "sceCdCloseConfig", /*40*/
	"", "", "", "",
	"", "", "", "",
	"", "", "", "",
	"", "", "", "", /*50*/
	"", "", "", "",
	"", "", "", "",
	"", "", "", "",
	"", "", "", "", /*60*/
	"", "", "", "",
	"", "", "", "",
	"", "", "", "",
	"", "", "", "", /*70*/
	"", "", "", "",
	"", "", "", "",
	"", "", "", "",
	"mechacon_auth_0x80", "mechacon_auth_0x81", "mechacon_auth_0x82", "mechacon_auth_0x83", /*80*/
	"mechacon_auth_0x84", "mechacon_auth_0x85", "mechacon_auth_0x86", "mechacon_auth_0x87",
	"mechacon_auth_0x88", "", "", "",
	"", "sceMgWriteData", "sceMgReadData", "mechacon_auth_0x8F",
	"sceMgWriteHeaderStart", "sceMgReadBITLength", "sceMgWriteDatainLength", "sceMgWriteDataoutLength", /*90*/
	"sceMgReadKbit", "sceMgReadKbit2", "sceMgReadKcon", "sceMgReadKcon2",
	"sceMgReadIcvPs2", "", "", "",
	"", "", "", "",
	/*A0, no sCmds above?*/
};

// NVM (eeprom) layout info
struct NVMLayout {
	u32 biosVer;	// bios version that this eeprom layout is for
	s32 config0;	// offset of 1st config block
	s32 config1;	// offset of 2nd config block
	s32 config2;	// offset of 3rd config block
	s32 consoleId;	// offset of console id (?)
	s32 ilinkId;	// offset of ilink id (ilink mac address)
	s32 modelNum;	// offset of ps2 model number (eg "SCPH-70002")
	s32 regparams;	// offset of RegionParams for PStwo
	s32 mac;		// offset of the value written to 0xFFFE0188 and 0xFFFE018C on PStwo
};

#define NVM_FORMAT_MAX	2
static NVMLayout nvmlayouts[NVM_FORMAT_MAX] =
{
	{0x000,  0x280, 0x300, 0x200, 0x1C8, 0x1C0, 0x1A0, 0x180, 0x198},	// eeproms from bios v0.00 and up
	{0x146,  0x270, 0x2B0, 0x200, 0x1C8, 0x1E0, 0x1B0, 0x180, 0x198},	// eeproms from bios v1.70 and up
};

#define btoi(b)		((b)/16*10 + (b)%16)		/* BCD to u_char */
#define itob(i)		((i)/10*16 + (i)%10)		/* u_char to BCD */

void cdvdReset();
void cdvdVsync();
extern void  cdvdActionInterrupt();
extern void  cdvdReadInterrupt();
void cdvdNewDiskCB();
u8   cdvdRead(u8 key);
void cdvdWrite(u8 key, u8 rt);

#endif /* __CDVD_H__ */
