/*  Pcsx2 - Pc Ps2 Emulator
 *  Copyright (C) 2002-2008  Pcsx2 Team
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

#include "PrecompiledHeader.h"

#include <ctype.h>
#include <time.h>

#ifndef _WIN32
#include <sys/time.h>
#endif

#include "PsxCommon.h"
#include "CDVDiso.h"

static cdvdStruct cdvd;
static int cdCaseopen;

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

//////////////////////////////////////////////////////////////////////////
// -- Cdvd Block Read Cycle Timings --
// These timings are based on a median average block read speed.  In theory the read
// speeds differ based on the location of the sector being read (outer rings have
// a different read speed from inner rings).  But for our purposes an average is good
// enough, since all of Pcsx2's instruction cycle counting is hardly accurate anyway.

// Morale of the story:  don't get too caught up micro-managing your cycle timings. :)

// Note: DVD read times are modified to be faster, because games seem to be a lot more
// concerned with accurate(ish) seek delays and less concerned with actual block read speeds.

static const uint PSX_CD_READSPEED = 153600;   // 1 Byte Time @ x1 (150KB = cd x 1)
static const uint PSX_DVD_READSPEED = 1382400 + 256000; // normal is 1 Byte Time @ x1 (1350KB = dvd x 1).

enum CDVD_MODE_TYPE
{
	MODE_DVDROM,
	MODE_CDROM
};

// if a seek is within this many blocks, read instead of seek.
// I picked 9 as an arbitrary value.  Not sure what the real PS2 uses.
static const int Cdvd_Contigious_Seek = 9;
static const uint Cdvd_Avg_SeekCycles = (PSXCLK*40) / 1000;		// average number of cycles per seek (40ms)



static const char *mg_zones[8] = {"Japan", "USA", "Europe", "Oceania", "Asia", "Russia", "China", "Mexico"};

static const char *nCmdName[0x100]= {
	"CdSync", "CdNop", "CdStandby", "CdStop",
	"CdPause", "CdSeek", "CdRead", "CdReadCDDA",
	"CdReadDVDV", "CdGetToc", "", "NCMD_B",
	"CdReadKey", "", "sceCdReadXCDDA", "sceCdChgSpdlCtrl",
};

static const char *sCmdName[0x100]= {
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
	s32	regparams;	// offset of RegionParams for PStwo
	s32 mac;		// offset of the value written to 0xFFFE0188 and 0xFFFE018C on PStwo
};

#define NVM_FORMAT_MAX	2
NVMLayout nvmlayouts[NVM_FORMAT_MAX] =
{
	{0x000,  0x280, 0x300, 0x200, 0x1C8, 0x1C0, 0x1A0, 0x180, 0x198},	// eeproms from bios v0.00 and up
	{0x146,  0x270, 0x2B0, 0x200, 0x1C8, 0x1E0, 0x1B0, 0x180, 0x198},	// eeproms from bios v1.70 and up
};

#define btoi(b)		((b)/16*10 + (b)%16)		/* BCD to u_char */
#define itob(i)		((i)/10*16 + (i)%10)		/* u_char to BCD */

#define SetResultSize(size) \
	cdvd.ResultC = size; cdvd.ResultP = 0; \
	cdvd.sDataIn&=~0x40;

#define CDVDREAD_INT(eCycle) PSX_INT(IopEvt_CdvdRead, eCycle)
static void CDVD_INT(int eCycle)
{
	if( eCycle == 0 )
		cdvdActionInterrupt();
	else
		PSX_INT(IopEvt_Cdvd, eCycle);
}

// Sets the cdvd IRQ and the reason for the IRQ, and signals the IOP for a branch
// test (which will cause the exception to be handled).
static void cdvdSetIrq( uint id = (1<<Irq_CommandComplete) )
{
	cdvd.PwOff |= id;
	iopIntcIrq( 2 );
	hwIntcIrq(INTC_SBUS);
	psxSetNextBranchDelta( 20 );
}

static int mg_BIToffset(u8 *buffer){
	int i, ofs = 0x20; for (i=0; i<*(u16*)&buffer[0x1A]; i++)ofs+=0x10;
	if (*(u16*)&buffer[0x18] & 1)ofs+=buffer[ofs];
	if ((*(u16*)&buffer[0x18] & 0xF000)==0)ofs+=8;
	return ofs + 0x20;
}

FILE *_cdvdOpenMechaVer() {
	char *ptr;
	int i;
	char file[g_MaxPath];
	string Bios;
	FILE* fd;

	// get the name of the bios file
	Path::Combine( Bios, Config.BiosDir, Config.Bios );
	
	// use the bios filename to get the name of the mecha ver file
	// [TODO] : Upgrade this to use std::string!

	strcpy(file, Bios.c_str());
	ptr = file; i = (int)strlen(file);
	while (i > 0) { if (ptr[i] == '.') break; i--; }
	ptr[i+1] = '\0';
	strcat(file, "MEC");
	
	// if file doesnt exist, create empty one
	fd = fopen(file, "r+b");
	if (fd == NULL) {
		SysPrintf("MEC File Not Found , Creating Blank File\n");
		fd = fopen(file, "wb");
		if (fd == NULL) {
			Msgbox::Alert("_cdvdOpenMechaVer: Error creating %s", params file);
			exit(1);
		}
		fputc(0x03, fd);
		fputc(0x06, fd);
		fputc(0x02, fd);
		fputc(0x00, fd);
	}
	return fd;
}

s32 cdvdGetMechaVer(u8* ver)
{
	FILE* fd = _cdvdOpenMechaVer();
	if (fd == NULL) return 1;
	fseek(fd, 0, SEEK_SET);
	fread(ver, 1, 4, fd);
	fclose(fd);
	return 0;
}

FILE *_cdvdOpenNVM() {
	char *ptr;
	int i;
	string Bios;
	char file[g_MaxPath];
	FILE* fd;

	// get the name of the bios file
	Path::Combine( Bios, Config.BiosDir, Config.Bios );
	
	// use the bios filename to get the name of the nvm file
	// [TODO] : Upgrade this to use std::string!

	strcpy( file, Bios.c_str() );
	ptr = file; i = (int)strlen(file);
	while (i > 0) { if (ptr[i] == '.') break; i--; }
	ptr[i+1] = '\0';
	strcat(file, "NVM");
	
	// if file doesnt exist, create empty one
	fd = fopen(file, "r+b");
	if (fd == NULL) {
		SysPrintf("NVM File Not Found , Creating Blank File\n");
		fd = fopen(file, "wb");
		if (fd == NULL) {
			Msgbox::Alert("_cdvdOpenNVM: Error creating %s", params file);
			exit(1);
		}
		for (i=0; i<1024; i++) fputc(0, fd);
	}
	return fd;
}

// 
// the following 'cdvd' functions all return 0 if successful
// 

s32 cdvdReadNVM(u8 *dst, int offset, int bytes) {
	FILE* fd = _cdvdOpenNVM();
	if (fd == NULL) return 1;
	fseek(fd, offset, SEEK_SET);
	fread(dst, 1, bytes, fd);
	fclose(fd);
	return 0;
}
s32 cdvdWriteNVM(const u8 *src, int offset, int bytes) {
	FILE* fd = _cdvdOpenNVM();
	if (fd == NULL) return 1;
	fseek(fd, offset, SEEK_SET);
	fwrite(src, 1, bytes, fd);
	fclose(fd);
	return 0;
}

#define GET_NVM_DATA(buff, offset, size, fmtOffset)	getNvmData(buff, offset, size, BiosVersion, offsetof(NVMLayout, fmtOffset))
#define SET_NVM_DATA(buff, offset, size, fmtOffset)	setNvmData(buff, offset, size, BiosVersion, offsetof(NVMLayout, fmtOffset))

s32 getNvmData(u8* buffer, s32 offset, s32 size, u32 biosVersion, s32 fmtOffset)
{
	// find the correct bios version
	NVMLayout* nvmLayout = NULL;
	s32 nvmIdx;
	for(nvmIdx=0; nvmIdx<NVM_FORMAT_MAX; nvmIdx++)
	{
		if(nvmlayouts[nvmIdx].biosVer <= biosVersion)
			nvmLayout = &nvmlayouts[nvmIdx];
	}
	if(nvmLayout == NULL)
		return 1;
	
	// get data from eeprom
	return cdvdReadNVM(buffer, *(s32*)(((u8*)nvmLayout)+fmtOffset) + offset, size);
}
s32 setNvmData(const u8* buffer, s32 offset, s32 size, u32 biosVersion, s32 fmtOffset)
{
	// find the correct bios version
	NVMLayout* nvmLayout = NULL;
	s32 nvmIdx;
	for(nvmIdx=0; nvmIdx<NVM_FORMAT_MAX; nvmIdx++)
	{
		if(nvmlayouts[nvmIdx].biosVer <= biosVersion)
			nvmLayout = &nvmlayouts[nvmIdx];
	}
	if(nvmLayout == NULL)
		return 1;
	
	// set data in eeprom
	return cdvdWriteNVM(buffer, *(s32*)(((u8*)nvmLayout)+fmtOffset) + offset, size);
}

s32 cdvdReadConsoleID(u8* id)
{
	return GET_NVM_DATA(id, 0, 8, consoleId);
}
s32 cdvdWriteConsoleID(const u8* id)
{
	return SET_NVM_DATA(id, 0, 8, consoleId);
}

s32 cdvdReadILinkID(u8* id)
{
	return GET_NVM_DATA(id, 0, 8, ilinkId);
}
s32 cdvdWriteILinkID(const u8* id)
{
	return SET_NVM_DATA(id, 0, 8, ilinkId);
}

s32 cdvdReadModelNumber(u8* num, s32 part)
{
	return GET_NVM_DATA(num, part, 8, modelNum);
}
s32 cdvdWriteModelNumber(const u8* num, s32 part)
{
	return SET_NVM_DATA(num, part, 8, modelNum);
}

s32 cdvdReadRegionParams(u8* num)
{
	return GET_NVM_DATA(num, 0, 8, regparams);
}

s32 cdvdWriteRegionParams(const u8* num)
{
	return SET_NVM_DATA(num, 0, 8, regparams);
}

s32 cdvdReadMAC(u8* num)
{
	return GET_NVM_DATA(num, 0, 8, mac);
}

s32 cdvdWriteMAC(const u8* num)
{
	return SET_NVM_DATA(num, 0, 8, mac);
}

s32 cdvdReadConfig(u8* config)
{
	// make sure its in read mode
	if(cdvd.CReadWrite != 0)
	{
		config[0] = 0x80;
		memset(&config[1], 0x00, 15);
		return 1;
	}
	// check if block index is in bounds
	else if(cdvd.CBlockIndex >= cdvd.CNumBlocks)
		return 1;
	else if(
		((cdvd.COffset == 0) && (cdvd.CBlockIndex >= 4))||
		((cdvd.COffset == 1) && (cdvd.CBlockIndex >= 2))||
		((cdvd.COffset == 2) && (cdvd.CBlockIndex >= 7))
		)
	{
		memzero_ptr<16>(config);
		return 0;
	}
	
	// get config data
	if(cdvd.COffset == 0)
		return GET_NVM_DATA(config, (cdvd.CBlockIndex++)*16, 16, config0);	else
	if(cdvd.COffset == 2)
		return GET_NVM_DATA(config, (cdvd.CBlockIndex++)*16, 16, config2);
	else
		return GET_NVM_DATA(config, (cdvd.CBlockIndex++)*16, 16, config1);
}
s32 cdvdWriteConfig(const u8* config)
{
	// make sure its in write mode
	if(cdvd.CReadWrite != 1)
		return 1;
	// check if block index is in bounds
	else if(cdvd.CBlockIndex >= cdvd.CNumBlocks)
		return 1;
	else if(
		((cdvd.COffset == 0) && (cdvd.CBlockIndex >= 4))||
		((cdvd.COffset == 1) && (cdvd.CBlockIndex >= 2))||
		((cdvd.COffset == 2) && (cdvd.CBlockIndex >= 7))
		)
		return 0;
	
	// get config data
	if(cdvd.COffset == 0)
		return SET_NVM_DATA(config, (cdvd.CBlockIndex++)*16, 16, config0);	else
	if(cdvd.COffset == 2)
		return SET_NVM_DATA(config, (cdvd.CBlockIndex++)*16, 16, config2);
	else
		return SET_NVM_DATA(config, (cdvd.CBlockIndex++)*16, 16, config1);
}


void cdvdReadKey(u8 arg0, u16 arg1, u32 arg2, u8* key) {
	char str[g_MaxPath];
	int numbers;
	int letters;
	unsigned int  key_0_3;
	unsigned char key_4;
	unsigned char key_14;
    char exeName[12];
	
	// get main elf name
	GetPS2ElfName(str);
    sprintf(exeName, "%c%c%c%c%c%c%c%c%c%c%c",str[8],str[9],str[10],str[11],str[12],str[13],str[14],str[15],str[16],str[17],str[18]);
	DevCon::Notice("exeName = %s", params &str[8]);
	
	// convert the number characters to a real 32bit number
	numbers =	((((exeName[5] - '0'))*10000)	+
				(((exeName[ 6] - '0'))*1000)	+
				(((exeName[ 7] - '0'))*100)		+
				(((exeName[ 9] - '0'))*10)		+
				(((exeName[10] - '0'))*1)		);
	
	// combine the lower 7 bits of each char
	// to make the 4 letters fit into a single u32
	letters =	(int)((exeName[3]&0x7F)<< 0) |
				(int)((exeName[2]&0x7F)<< 7) |
				(int)((exeName[1]&0x7F)<<14) |
				(int)((exeName[0]&0x7F)<<21);
	
	// calculate magic numbers
	key_0_3 = ((numbers & 0x1FC00) >> 10) | ((0x01FFFFFF & letters) <<  7);	// numbers = 7F  letters = FFFFFF80
	key_4   = ((numbers & 0x0001F) <<  3) | ((0x0E000000 & letters) >> 25);	// numbers = F8  letters = 07
	key_14  = ((numbers & 0x003E0) >>  2) | 0x04;							// numbers = F8  extra   = 04  unused = 03
	
	// clear key values
	memzero_ptr<16>(key);
	
	// store key values
	key[ 0] = (key_0_3&0x000000FF)>> 0;
	key[ 1] = (key_0_3&0x0000FF00)>> 8;
	key[ 2] = (key_0_3&0x00FF0000)>>16;
	key[ 3] = (key_0_3&0xFF000000)>>24;
	key[ 4] = key_4;
	
	if(arg2 == 75)
	{
		key[14] = key_14;
		key[15] = 0x05;
	}
	else if(arg2 == 3075)
	{
		key[15] = 0x01;
	}
	else if(arg2 == 4246)
	{
		// 0x0001F2F707 = sector 0x0001F2F7  dec 0x07
		key[ 0] = 0x07;
		key[ 1] = 0xF7;
		key[ 2] = 0xF2;
		key[ 3] = 0x01;
		key[ 4] = 0x00;
		key[15] = 0x01;
	}
	else
	{
		key[15] = 0x01;
	}
	
	Console::WriteLn( "CDVD.KEY = %02X,%02X,%02X,%02X,%02X,%02X,%02X", params
		cdvd.Key[0],cdvd.Key[1],cdvd.Key[2],cdvd.Key[3],cdvd.Key[4],cdvd.Key[14],cdvd.Key[15] );

	// Now's a good time to reload the ELF info...
	if( ElfCRC == 0 )
	{
		FreezeMMXRegs(1);
		ElfCRC = loadElfCRC( str );
		ElfApplyPatches();
		LoadGameSpecificSettings();
		GSsetGameCRC( ElfCRC, 0 );
		FreezeMMXRegs(0);
	}
}

s32 cdvdGetToc(void* toc)
{
	s32 ret = CDVDgetTOC(toc);
	if(ret == -1) ret = 0x80;
	return ret;
/*
	cdvdTN diskInfo;
	cdvdTD trackInfo;
	u8 _time[3];
	u32 type;
	int	i, err;
			
	//Param[0] is 0 for CdGetToc and any value for cdvdman_call19
	//the code below handles only CdGetToc!
	//if(cdvd.Param[0]==0x01)
	//{
	SysPrintf("CDGetToc Param[0]=%d, Param[1]=%d\n",cdvd.Param[0],cdvd.Param[1]);
	//}
	type = CDVDgetDiskType();
	if (CDVDgetTN(&diskInfo) == -1)	{ diskInfo.etrack = 0;diskInfo.strack = 1; }
	if (CDVDgetTD(0, &trackInfo) == -1) trackInfo.lsn = 0;

	if (type == CDVD_TYPE_CDDA) {
		PSXMu8(HW_DMA3_MADR+ 0) = 0x01;
	} else
	if (type == CDVD_TYPE_PS2DVD) {
		if (trackInfo.lsn >= (2048*1024)) { // dual sided
			PSXMu8(HW_DMA3_MADR+ 0) = 0x24;
		} else {
			PSXMu8(HW_DMA3_MADR+ 0) = 0x04;
		}
	} else
	if (type == CDVD_TYPE_PS2CD) {
		PSXMu8(HW_DMA3_MADR+ 0) = 0x41;
	}

	if (PSXMu8(HW_DMA3_MADR+ 0) & 0x04) {
		PSXMu8(HW_DMA3_MADR+ 1) = 0x02;
		PSXMu8(HW_DMA3_MADR+ 2) = 0xF2;
		PSXMu8(HW_DMA3_MADR+ 3) = 0x00;

		if (PSXMu8(HW_DMA3_MADR+ 0) & 0x20) {
			PSXMu8(HW_DMA3_MADR+ 4) = 0x41;
			PSXMu8(HW_DMA3_MADR+ 5) = 0x95;
		} else {
			PSXMu8(HW_DMA3_MADR+ 4) = 0x86;
			PSXMu8(HW_DMA3_MADR+ 5) = 0x72;
		}
		PSXMu8(HW_DMA3_MADR+ 6) = 0x00;
		PSXMu8(HW_DMA3_MADR+ 7) = 0x00;
		PSXMu8(HW_DMA3_MADR+ 8) = 0x00;
		PSXMu8(HW_DMA3_MADR+ 9) = 0x00;
		PSXMu8(HW_DMA3_MADR+10) = 0x00;
		PSXMu8(HW_DMA3_MADR+11) = 0x00;

		PSXMu8(HW_DMA3_MADR+12) = 0x00;
		PSXMu8(HW_DMA3_MADR+13) = 0x00;
		PSXMu8(HW_DMA3_MADR+14) = 0x00;
		PSXMu8(HW_DMA3_MADR+15) = 0x00;

		PSXMu8(HW_DMA3_MADR+16) = 0x00;
		PSXMu8(HW_DMA3_MADR+17) = 0x03;
		PSXMu8(HW_DMA3_MADR+18) = 0x00;
		PSXMu8(HW_DMA3_MADR+19) = 0x00;

	} else {
		PSXMu8(HW_DMA3_MADR+ 1) = 0x00;
		PSXMu8(HW_DMA3_MADR+ 2) = 0xA0;
		PSXMu8(HW_DMA3_MADR+ 7) = itob(diskInfo.strack);//Number of FirstTrack

		PSXMu8(HW_DMA3_MADR+12) = 0xA1;
		PSXMu8(HW_DMA3_MADR+17) = itob(diskInfo.etrack);//Number of LastTrack

		PSXMu8(HW_DMA3_MADR+22) = 0xA2;//DiskLength
		LSNtoMSF(_time, trackInfo.lsn);
		PSXMu8(HW_DMA3_MADR+27) = itob(_time[2]);
		PSXMu8(HW_DMA3_MADR+28) = itob(_time[1]);

		for (i=diskInfo.strack; i<=diskInfo.etrack; i++) {
			err=CDVDgetTD(i, &trackInfo);
			LSNtoMSF(_time, trackInfo.lsn);
			PSXMu8(HW_DMA3_MADR+i*10+30) = trackInfo.type;
			PSXMu8(HW_DMA3_MADR+i*10+32) = err == -1 ? 0 : itob(i);	  //number
			PSXMu8(HW_DMA3_MADR+i*10+37) = itob(_time[2]);
			PSXMu8(HW_DMA3_MADR+i*10+38) = itob(_time[1]);
			PSXMu8(HW_DMA3_MADR+i*10+39) = itob(_time[0]);
		}
	}
*/
}

s32 cdvdReadSubQ(s32 lsn, cdvdSubQ* subq)
{
	s32 ret = CDVDreadSubQ(lsn, subq);
	if(ret == -1)	ret = 0x80;
	return ret;
}

s32 cdvdCtrlTrayOpen()
{
	s32 ret = CDVDctrlTrayOpen();
	if(ret == -1)	ret = 0x80;
	return ret;
}

s32 cdvdCtrlTrayClose()
{
	s32 ret = CDVDctrlTrayClose();
	if(ret == -1)	ret = 0x80;
	return ret;
}

// Modified by (efp) - 16/01/2006
// checks if tray was opened since last call to this func
s32 cdvdGetTrayStatus()
{
	s32 ret = CDVDgetTrayStatus();
	// get current tray state
	if (cdCaseopen)  return(CDVD_TRAY_OPEN);

	
	if (ret == -1)  return(CDVD_TRAY_CLOSE);
	return(ret);
}

// Note: Is tray status being kept as a var here somewhere?
//   cdvdNewDiskCB() can update it's status as well...

// Modified by (efp) - 16/01/2006
__forceinline void cdvdGetDiskType()
{
	// defs 0.9.0
	if(CDVDnewDiskCB || cdvd.Type != CDVD_TYPE_NODISC)  return;

	// defs.0.8.1
	if(cdvdGetTrayStatus() == CDVD_TRAY_OPEN)
	{
		cdvd.Type = CDVD_TYPE_NODISC;
		return;
	}

	cdvd.Type = CDVDgetDiskType();
	if (cdvd.Type == CDVD_TYPE_PS2CD) // && needReset == 1)
	{
		char str[g_MaxPath];
		if (GetPS2ElfName(str) == 1)
		{
			cdvd.Type = CDVD_TYPE_PSCD;
		} // ENDIF- Does the SYSTEM.CNF file only say "BOOT="? PS1 CD then.
	} // ENDIF- Is the type listed as a PS2 CD?
} // END cdvdGetDiskType()


// check whether disc is single or dual layer
// if its dual layer, check what the disctype is and what sector number
// layer1 starts at
// 
// args:    gets value for dvd type (0=single layer, 1=ptp, 2=otp)
//          gets value for start lsn of layer1
// returns: 1 if on dual layer disc
//          0 if not on dual layer disc
s32 cdvdReadDvdDualInfo(s32* dualType, u32* layer1Start)
{
	u8 toc[2064];
	*dualType = 0;
	*layer1Start = 0;
	
	// if error getting toc, settle for single layer disc ;)
	if(cdvdGetToc(toc))
		return 0;
	if(toc[14] & 0x60)
	{
		if(toc[14] & 0x10)
		{
			// otp dvd
			*dualType = 2;
			*layer1Start = (toc[25]<<16) + (toc[26]<<8) + (toc[27]) - 0x30000 + 1;
		}
		else
		{
			// ptp dvd
			*dualType = 1;
			*layer1Start = (toc[21]<<16) + (toc[22]<<8) + (toc[23]) - 0x30000 + 1;
		}
	}
	else
	{
		// single layer dvd
		*dualType = 0;
		*layer1Start = (toc[21]<<16) + (toc[22]<<8) + (toc[23]) - 0x30000 + 1;
	}
	
	return 1;
}


#include <time.h>

static uint cdvdBlockReadTime( CDVD_MODE_TYPE mode )
{
	return (PSXCLK * cdvd.BlockSize) / (((mode==MODE_CDROM) ? PSX_CD_READSPEED : PSX_DVD_READSPEED) * cdvd.Speed);
}

void cdvdReset()
{
#ifdef _WIN32
	SYSTEMTIME st;
    //Get and set the internal clock to time
	GetSystemTime(&st);
#else
    time_t traw;
    struct tm* ptlocal;
    time(&traw);
    ptlocal = localtime(&traw);
#endif

	memzero_obj(cdvd);

	cdvd.Type = CDVD_TYPE_NODISC;
	cdvd.Spinning = false;

	cdvd.sDataIn = 0x40;
	cdvd.Ready   = 0x4e;
	cdCaseopen = 0;
	cdvd.Speed = 4;
	cdvd.BlockSize = 2064;
	cdvd.Action = cdvdAction_None;
	cdvd.ReadTime = cdvdBlockReadTime( MODE_DVDROM );

    // any random valid date will do
    cdvd.RTC.hour = 1;
    cdvd.RTC.day = 25;
    cdvd.RTC.month = 5;
    cdvd.RTC.year = 7; //2007

#ifndef _DEBUG
#ifdef _WIN32
	cdvd.RTC.second = (u8)(st.wSecond);
	cdvd.RTC.minute = (u8)(st.wMinute);
	cdvd.RTC.hour = (u8)(st.wHour+1)%24;
	cdvd.RTC.day = (u8)(st.wDay);
	cdvd.RTC.month = (u8)(st.wMonth);
	cdvd.RTC.year = (u8)(st.wYear - 2000);
#else
    cdvd.RTC.second = ptlocal->tm_sec;
    cdvd.RTC.minute = ptlocal->tm_min;
    cdvd.RTC.hour = ptlocal->tm_hour;
    cdvd.RTC.day = ptlocal->tm_mday;
    cdvd.RTC.month = ptlocal->tm_mon;
    cdvd.RTC.year = ptlocal->tm_year;
#endif
#endif

}

struct Freeze_v10Compat
{
	u8  Action;
	u32 SeekToSector;
	u32 ReadTime;
	bool Spinning;
};

void SaveState::cdvdFreeze()
{
	if( GetVersion() <= 0x10 )
	{
		// the old cdvd struct didn't save the last few items.
		FreezeLegacy( cdvd, sizeof(Freeze_v10Compat) );
		cdvd.SeekToSector = cdvd.Sector;
		cdvd.Action = cdvdAction_None;
		cdvd.ReadTime = cdvdBlockReadTime( MODE_DVDROM );
		cdvd.Spinning = true;
	}
	else
		Freeze( cdvd );

	if( IsLoading() )
	{
		// Make sure the Cdvd plugin has the expected track loaded into the buffer.
		// If cdvd.Readed is cleared it means we need to load the SeekToSector (ie, a
		// seek is in progress!)

		if( cdvd.Reading )
			cdvd.RErr = CDVDreadTrack( cdvd.Readed ? cdvd.Sector : cdvd.SeekToSector, cdvd.ReadMode);
	}
}

// Modified by (efp) - 16/01/2006
void cdvdNewDiskCB()
{
	cdvd.Type = CDVDgetDiskType();
	if(cdvd.Type == CDVD_TYPE_PS2CD) {
		char str[g_MaxPath];
		if(GetPS2ElfName(str) == 1) {
			cdvd.Type = CDVD_TYPE_PSCD;
		} // ENDIF- Does the SYSTEM.CNF file only say "BOOT="? PS1 CD then.
	}
}

void mechaDecryptBytes(unsigned char* buffer, int size)
{
	int i;

	int shiftAmount = (cdvd.decSet>>4) & 7;
	int doXor = (cdvd.decSet) & 1;
	int doShift = (cdvd.decSet) & 2;
	
	for (i=0; i<size; i++) 
	{
		if (doXor) buffer[i] ^= cdvd.Key[4];
		if (doShift) buffer[i] = (buffer[i]>>shiftAmount) | (buffer[i]<<(8-shiftAmount));
	}
}

int cdvdReadSector() {
	s32 bcr;

	CDR_LOG("SECTOR %d (BCR %x;%x)\n", cdvd.Sector, HW_DMA3_BCR_H16, HW_DMA3_BCR_L16);

	bcr = (HW_DMA3_BCR_H16 * HW_DMA3_BCR_L16) *4;
	if (bcr < cdvd.BlockSize) {
		CDR_LOG( "READBLOCK:  bcr < cdvd.BlockSize; %x < %x\n", bcr, cdvd.BlockSize );
		if (HW_DMA3_CHCR & 0x01000000) {
			HW_DMA3_CHCR &= ~0x01000000;
			psxDmaInterrupt(3);
		}
		return -1;
	}

	const u32 madr = HW_DMA3_MADR;

	// if raw dvd sector 'fill in the blanks'
	if (cdvd.BlockSize == 2064) {
		// get info on dvd type and layer1 start
		u32 layer1Start;
		s32 dualType;
		s32 layerNum;
		u32 lsn = cdvd.Sector;

		cdvdReadDvdDualInfo(&dualType, &layer1Start);
		
		if((dualType == 1) && (lsn >= layer1Start)) {
			// dual layer ptp disc
			layerNum = 1;
			lsn = lsn-layer1Start + 0x30000;
		} else if((dualType == 2) && (lsn >= layer1Start)) {
			// dual layer otp disc
			layerNum = 1;
			lsn = ~(layer1Start+0x30000 - 1);
		} else {
			// single layer disc
			// or on first layer of dual layer disc
			layerNum = 0;
			lsn += 0x30000;
		} // ENDLONGIF- Assumed the other dualType is 0.

		PSXMu8(madr+0) = 0x20 | layerNum;
		PSXMu8(madr+1) = (u8)(lsn >> 16);
		PSXMu8(madr+2) = (u8)(lsn >>  8);
		PSXMu8(madr+3) = (u8)(lsn      );
		
		// sector IED (not calculated at present)
		PSXMu8(madr+4) = 0;
		PSXMu8(madr+5) = 0;
		
		// sector CPR_MAI (not calculated at present)
		PSXMu8(madr+ 6) = 0;
		PSXMu8(madr+ 7) = 0;
		PSXMu8(madr+ 8) = 0;
		PSXMu8(madr+ 9) = 0;
		PSXMu8(madr+10) = 0;
		PSXMu8(madr+11) = 0;
		
		// normal 2048 bytes of sector data
		memcpy_fast(PSXM(madr+12), cdr.pTransfer, 2048);
		
		// 4 bytes of edc (not calculated at present)
		PSXMu8(madr+2060) = 0;
		PSXMu8(madr+2061) = 0;
		PSXMu8(madr+2062) = 0;
		PSXMu8(madr+2063) = 0;
	} else {
		// normal read
		memcpy_fast((u8*)PSXM(madr), cdr.pTransfer, cdvd.BlockSize);
	}
	// decrypt sector's bytes
	if(cdvd.decSet)
		mechaDecryptBytes((u8*)PSXM(madr), cdvd.BlockSize);

	// Added a clear after memory write .. never seemed to be necessary before but *should*
	// be more correct. (air)
	psxCpu->Clear( madr, cdvd.BlockSize/4);

//	SysPrintf("sector %x;%x;%x\n", PSXMu8(madr+0), PSXMu8(madr+1), PSXMu8(madr+2));

	HW_DMA3_BCR_H16-= (cdvd.BlockSize / (HW_DMA3_BCR_L16*4));
	HW_DMA3_MADR+= cdvd.BlockSize;

	return 0;
}

// inlined due to being referenced in only one place.
__forceinline void cdvdActionInterrupt()
{
	switch( cdvd.Action )
	{
		case cdvdAction_Seek:
		case cdvdAction_Standby:
			cdvd.Spinning = true;
			cdvd.Ready  = 0x40;
			cdvd.Sector = cdvd.SeekToSector;
			cdvd.Status = CDVD_STATUS_SEEK_COMPLETE;
		break;

		case cdvdAction_Stop:
			cdvd.Spinning = false;
			cdvd.Ready = 0x40;
			cdvd.Sector = 0;
			cdvd.Status = 0;
		break;

		case cdvdAction_Break:
			// Make sure the cdvd action state is pretty well cleared:
			cdvd.Reading = 0;
			cdvd.Readed = 0;
			cdvd.Ready  = 0x4e;		// should be 0x40 or something else?
			cdvd.Status = 0;
			cdvd.RErr = 0;
			cdvd.nCommand = 0;
		break;
	}
	cdvd.Action = cdvdAction_None;

	cdvd.PwOff |= 1<<Irq_CommandComplete;
	psxHu32(0x1070)|= 0x4;
	hwIntcIrq(INTC_SBUS);
}

// inlined due to being referenced in only one place.
__forceinline void cdvdReadInterrupt() {

	//SysPrintf("cdvdReadInterrupt %x %x %x %x %x\n", cpuRegs.interrupt, cdvd.Readed, cdvd.Reading, cdvd.nSectors, (HW_DMA3_BCR_H16 * HW_DMA3_BCR_L16) *4);

	cdvd.Ready   = 0x00;
	if (cdvd.Readed == 0)
	{
		// Seeking finished.  Process the track we requested before, and
		// then schedule another CDVD read int for when the block read finishes.

		// NOTE: The first CD track was read when the seek was initiated, so no need
		// to call CDVDReadTrack here.

		cdvd.Spinning = true;
		cdvd.RetryCntP = 0;
		cdvd.Reading = 1;
		cdvd.Readed = 1;
		cdvd.Status = CDVD_STATUS_SEEK_COMPLETE;
		cdvd.Sector = cdvd.SeekToSector;

		CDR_LOG( "Cdvd Seek Complete > Scheduling block read interrupt at iopcycle=%8.8x.\n",
			psxRegs.cycle + cdvd.ReadTime );

		CDVDREAD_INT(cdvd.ReadTime);
		return;
	}

	if (cdvd.Reading == 1) {
		if (cdvd.RErr == 0) {
			cdr.pTransfer = CDVDgetBuffer();
		} else cdr.pTransfer = NULL;
		if (cdr.pTransfer == NULL) {
			cdvd.RetryCntP++;
			Console::Error("CDVD READ ERROR, sector=%d", params cdvd.Sector);
			if (cdvd.RetryCntP <= cdvd.RetryCnt) {
				cdvd.RErr = CDVDreadTrack(cdvd.Sector, cdvd.ReadMode);
				CDVDREAD_INT(cdvd.ReadTime);
				return;
			}
		}
		cdvd.Reading = 0;
	}

	if (cdvdReadSector() == -1) {
		assert( (int)cdvd.ReadTime > 0 );
		CDVDREAD_INT(cdvd.ReadTime);
		return;
	}

	cdvd.Sector++;

	if (--cdvd.nSectors <= 0)
	{
		cdvd.PwOff |= 1<<Irq_CommandComplete;
		psxHu32(0x1070)|= 0x4;
		hwIntcIrq(INTC_SBUS);

		HW_DMA3_CHCR &= ~0x01000000;
		psxDmaInterrupt(3);
		cdvd.Ready   = 0x4e;

		// All done! :D

		return;
	}

	cdvd.RetryCntP = 0;
	cdvd.Reading = 1;
	cdr.RErr = CDVDreadTrack(cdvd.Sector, cdvd.ReadMode);
	CDVDREAD_INT(cdvd.ReadTime);

	return;
}

u8 monthmap[13] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

void cdvdVsync() {
	cdvd.RTCcount++;
	if (cdvd.RTCcount < ((Config.PsxType & 1) ? 50 : 60)) return;
	cdvd.RTCcount = 0;

	cdvd.RTC.second++;
	if (cdvd.RTC.second < 60) return;
	cdvd.RTC.second = 0;

	cdvd.RTC.minute++;
	if (cdvd.RTC.minute < 60) return;
	cdvd.RTC.minute = 0;

	cdvd.RTC.hour++;
	if (cdvd.RTC.hour < 24) return;
	cdvd.RTC.hour = 0;

	cdvd.RTC.day++;
	if (cdvd.RTC.day <= monthmap[cdvd.RTC.month-1]) return;
	cdvd.RTC.day = 1;

	cdvd.RTC.month++;
	if (cdvd.RTC.month <= 12) return;
	cdvd.RTC.month = 1;

	cdvd.RTC.year++;
	if (cdvd.RTC.year < 100) return;
	cdvd.RTC.year = 0;
}


u8   cdvdRead04(void) { // NCOMMAND
	CDR_LOG("cdvdRead04(NCMD) %x\n", cdvd.nCommand);

	return cdvd.nCommand;
}

u8   cdvdRead05(void) { // N-READY
	CDR_LOG("cdvdRead05(NReady) %x\n", cdvd.Ready);

	return cdvd.Ready;
}

u8   cdvdRead06(void) { // ERROR
	CDR_LOG("cdvdRead06(Error) %x\n", cdvd.Error);
	
	return cdvd.Error;
}

u8   cdvdRead07(void) { // BREAK
	CDR_LOG("cdvdRead07(Break) %x\n", 0);

	return 0;
}

u8   cdvdRead08(void) { // INTR_STAT
	CDR_LOG("cdvdRead08(IntrReason) %x\n", cdvd.PwOff);

	return cdvd.PwOff;
}

u8   cdvdRead0A(void) { // STATUS
	CDR_LOG("cdvdRead0A(Status) %x\n", cdvd.Status);

	return cdvd.Status;
}

u8   cdvdRead0B(void) { // TRAY-STATE (if tray has been opened)
	u8 tray = cdvdGetTrayStatus();
	CDR_LOG("cdvdRead0B(Tray) %x\n", tray);

	return tray;
}

u8   cdvdRead0C(void) { // CRT MINUTE
	CDR_LOG("cdvdRead0C(Min) %x\n", itob((u8)(cdvd.Sector/(60*75))));

	return itob((u8)(cdvd.Sector/(60*75)));
}

u8   cdvdRead0D(void) { // CRT SECOND
	CDR_LOG("cdvdRead0D(Sec) %x\n", itob((u8)((cdvd.Sector/75)%60)+2));

	return itob((u8)((cdvd.Sector/75)%60)+2);
}

u8   cdvdRead0E(void) { // CRT FRAME
	CDR_LOG("cdvdRead0E(Frame) %x\n", itob((u8)(cdvd.Sector%75)));

	return itob((u8)(cdvd.Sector%75));
}

u8   cdvdRead0F(void)	// TYPE
{
	CDR_LOG("cdvdRead0F(Disc Type) %x\n", cdvd.Type);
	cdvdGetDiskType();
	return cdvd.Type;
}

u8   cdvdRead13(void) { // UNKNOWN
	CDR_LOG("cdvdRead13(Unknown) %x\n", 4);
	
	return 4;
}

u8   cdvdRead15(void) { // RSV
	CDR_LOG("cdvdRead15(RSV)\n");
	
	return 0x01; // | 0x80 for ATAPI mode
}

u8   cdvdRead16(void) { // SCOMMAND
	CDR_LOG("cdvdRead16(SCMD) %x\n", cdvd.sCommand);
	
	return cdvd.sCommand;
}

u8   cdvdRead17(void) { // SREADY
	CDR_LOG("cdvdRead17(SReady) %x\n", cdvd.sDataIn);
	
	return cdvd.sDataIn;
}

u8   cdvdRead18(void) { // SDATAOUT
	u8 ret=0;

	if ((cdvd.sDataIn & 0x40) == 0) {
		if (cdvd.ResultP < cdvd.ResultC) {
			cdvd.ResultP++;
			if (cdvd.ResultP >= cdvd.ResultC) cdvd.sDataIn|= 0x40;
			ret = cdvd.Result[cdvd.ResultP-1];
		}
	}
	CDR_LOG("cdvdRead18(SDataOut) %x (ResultC=%d, ResultP=%d)\n", ret, cdvd.ResultC, cdvd.ResultP);
	return ret;
}

u8   cdvdRead20(void) {
	CDR_LOG("cdvdRead20(Key0) %x\n", cdvd.Key[0]);
	
	return cdvd.Key[0];
}

u8   cdvdRead21(void) {
	CDR_LOG("cdvdRead21(Key1) %x\n", cdvd.Key[1]);
	
	return cdvd.Key[1];
}

u8   cdvdRead22(void) {
	CDR_LOG("cdvdRead22(Key2) %x\n", cdvd.Key[2]);
	
	return cdvd.Key[2];
}

u8   cdvdRead23(void) {
	CDR_LOG("cdvdRead23(Key3) %x\n", cdvd.Key[3]);
	
	return cdvd.Key[3];
}

u8   cdvdRead24(void) {
	CDR_LOG("cdvdRead24(Key4) %x\n", cdvd.Key[4]);
	
	return cdvd.Key[4];
}

u8   cdvdRead28(void) {
	CDR_LOG("cdvdRead28(Key5) %x\n", cdvd.Key[5]);
	
	return cdvd.Key[5];
}

u8   cdvdRead29(void) {
	CDR_LOG("cdvdRead29(Key6) %x\n", cdvd.Key[6]);
	
	return cdvd.Key[6];
}

u8   cdvdRead2A(void) {
	CDR_LOG("cdvdRead2A(Key7) %x\n", cdvd.Key[7]);
	
	return cdvd.Key[7];
}

u8   cdvdRead2B(void) {
	CDR_LOG("cdvdRead2B(Key8) %x\n", cdvd.Key[8]);
	
	return cdvd.Key[8];
}

u8   cdvdRead2C(void) {
	CDR_LOG("cdvdRead2C(Key9) %x\n", cdvd.Key[9]);
	
	return cdvd.Key[9];
}

u8   cdvdRead30(void) {
	CDR_LOG("cdvdRead30(Key10) %x\n", cdvd.Key[10]);
	
	return cdvd.Key[10];
}

u8   cdvdRead31(void) {
	CDR_LOG("cdvdRead31(Key11) %x\n", cdvd.Key[11]);
	
	return cdvd.Key[11];
}

u8   cdvdRead32(void) {
	CDR_LOG("cdvdRead32(Key12) %x\n", cdvd.Key[12]);
	
	return cdvd.Key[12];
}

u8   cdvdRead33(void) {
	CDR_LOG("cdvdRead33(Key13) %x\n", cdvd.Key[13]);
	
	return cdvd.Key[13];
}

u8   cdvdRead34(void) {
	CDR_LOG("cdvdRead34(Key14) %x\n", cdvd.Key[14]);
	
	return cdvd.Key[14];
}

u8   cdvdRead38(void) {		// valid parts of key data (first and last are valid)
	CDR_LOG("cdvdRead38(KeysValid) %x\n", cdvd.Key[15]);
	
	return cdvd.Key[15];
}

u8   cdvdRead39(void) {	// KEY-XOR
	CDR_LOG("cdvdRead39(KeyXor) %x\n", cdvd.KeyXor);
	
	return cdvd.KeyXor;
}

u8   cdvdRead3A(void) {	// DEC_SET
	CDR_LOG("cdvdRead3A(DecSet) %x\n", cdvd.decSet);
	
	SysPrintf("DecSet Read: %02X\n", cdvd.decSet);
	return cdvd.decSet;
}


// Returns the number of IOP cycles until the event completes.
static uint cdvdStartSeek( uint newsector )
{
	cdvd.SeekToSector = newsector;

	uint delta = abs(cdvd.SeekToSector - cdvd.Sector);
	uint seektime;

	cdvd.Ready = 0;
	cdvd.Reading = 0;
	cdvd.Readed = 0;
	cdvd.Status = 0;

	if( !cdvd.Spinning )
	{
		CDR_LOG( "CdSpinUp > Simulating CdRom Spinup Time, and seek to sector %d\n", cdvd.SeekToSector );
		seektime = PSXCLK / 3;		// 333ms delay
		cdvd.Spinning = true;
	}
	else if( (Cdvd_Contigious_Seek >= 0) && (delta >= Cdvd_Contigious_Seek) )
	{
		CDR_LOG( "CdSeek Begin > to sector %d, from %d - delta=%d\n", cdvd.SeekToSector, cdvd.Sector, delta );
		seektime = Cdvd_Avg_SeekCycles;
	}
	else
	{
		CDR_LOG( "CdSeek Begin > Contigious block without seek - delta=%d sectors\n", delta );
		
		// seektime is the time it takes to read to the destination block:
		seektime = delta * cdvd.ReadTime;

		if( delta == 0 )
		{
			cdvd.Status = CDVD_STATUS_SEEK_COMPLETE;
			cdvd.Readed = 1;
			cdvd.RetryCntP = 0;

			// setting Readed to zero skips the seek logic, which means the next call to
			// cdvdReadInterrupt will load a block.  So make sure it's properly scheduled
			// based on sector read speeds:
			seektime = cdvd.ReadTime;
		}
	}

	return seektime;
}

void cdvdWrite04(u8 rt) { // NCOMMAND
	CDR_LOG("cdvdWrite04: NCMD %s (%x) (ParamP = %x)\n", nCmdName[rt], rt, cdvd.ParamP);

	cdvd.nCommand = rt;
	cdvd.Status = CDVD_STATUS_NONE;
	cdvd.PwOff = Irq_None;		// good or bad?

	switch (rt) {
		case 0x00: // CdSync
		case 0x01: // CdNop_
			cdvdSetIrq();
		break;
		
		case 0x02: // CdStandby

			// Seek to sector zero.  The cdvdStartSeek function will simulate
			// spinup times if needed.

			DevCon::Notice( "CdStandby : %d", params rt );
			cdvd.Action = cdvdAction_Standby;
			cdvd.ReadTime = cdvdBlockReadTime( MODE_DVDROM );
			CDVD_INT( cdvdStartSeek( 0 ) );
		break;

		case 0x03: // CdStop
			DevCon::Notice( "CdStop : %d", params rt );
			cdvd.Action = cdvdAction_Stop;
			CDVD_INT( PSXCLK / 6 );		// 166ms delay?
		break;

		// from an emulation point of view there is not much need to do anything for this one
		case 0x04: // CdPause
			cdvdSetIrq();
		break;

		case 0x05: // CdSeek
			cdvd.Action = cdvdAction_Seek;
			cdvd.ReadTime = cdvdBlockReadTime( MODE_DVDROM );
			CDVD_INT( cdvdStartSeek( *(uint*)(cdvd.Param+0) ) );
		break;
		
		case 0x06: // CdRead
			cdvd.SeekToSector = *(uint*)(cdvd.Param+0);
			cdvd.nSectors = *(int*)(cdvd.Param+4);
			cdvd.RetryCnt = (cdvd.Param[8] == 0) ? 0x100 : cdvd.Param[8];
			cdvd.SpindlCtrl = cdvd.Param[9];
			cdvd.Speed = 24;
			switch (cdvd.Param[10]) {
				case 2: cdvd.ReadMode = CDVD_MODE_2340; cdvd.BlockSize = 2340; break;
				case 1: cdvd.ReadMode = CDVD_MODE_2328; cdvd.BlockSize = 2328; break;
				case 0: default: cdvd.ReadMode = CDVD_MODE_2048; cdvd.BlockSize = 2048; break;
			}

			CDR_LOG( "CdRead > startSector=%d, nSectors=%d, RetryCnt=%x, Speed=%x(%x), ReadMode=%x(%x) (1074=%x)\n",
				cdvd.Sector, cdvd.nSectors, cdvd.RetryCnt, cdvd.Speed, cdvd.Param[9], cdvd.ReadMode, cdvd.Param[10], psxHu32(0x1074));

			if (Config.cdvdPrint) SysPrintf("CdRead: Reading Sector %d(%d Blocks of Size %d) at Speed=%dx\n", cdvd.Sector, cdvd.nSectors,cdvd.BlockSize,cdvd.Speed);

			cdvd.ReadTime = cdvdBlockReadTime( MODE_CDROM );
			CDVDREAD_INT( cdvdStartSeek( cdvd.SeekToSector ) );

			// Read-ahead by telling the plugin about the track now.
			// This helps improve performance on actual from-cd emulation
			// (ie, not using the hard drive)
			cdvd.RErr = CDVDreadTrack( cdvd.SeekToSector, cdvd.ReadMode );

			// Set the reading block flag.  If a seek is pending then Readed will
			// take priority in the handler anyway.  If the read is contigeous then
			// this'll skip the seek delay.
			cdvd.Reading = 1;
		break;

		case 0x07: // CdReadCDDA
		case 0x0E: // CdReadXCDDA
			cdvd.SeekToSector = *(int*)(cdvd.Param+0);
			cdvd.nSectors = *(int*)(cdvd.Param+4);
			if (cdvd.Param[8] == 0) cdvd.RetryCnt = 0x100;
			else cdvd.RetryCnt = cdvd.Param[8];
			cdvd.SpindlCtrl = cdvd.Param[9];
			switch (cdvd.Param[9]) {
				case 0x01: cdvd.Speed =  1; break;
				case 0x02: cdvd.Speed =  2; break;
				case 0x03: cdvd.Speed =  4; break;
				case 0x04: cdvd.Speed = 12; break;
				default:   cdvd.Speed = 24; break;
			}
			switch (cdvd.Param[10]) {
				case 1: cdvd.ReadMode = CDVD_MODE_2368; cdvd.BlockSize = 2368; break;
				case 2:
				case 0: cdvd.ReadMode = CDVD_MODE_2352; cdvd.BlockSize = 2352; break;
			}

			CDR_LOG( "CdReadCDDA > startSector=%d, nSectors=%d, RetryCnt=%x, Speed=%xx(%x), ReadMode=%x(%x) (1074=%x)\n",
				cdvd.Sector, cdvd.nSectors, cdvd.RetryCnt, cdvd.Speed, cdvd.Param[9], cdvd.ReadMode, cdvd.Param[10], psxHu32(0x1074));

			if (Config.cdvdPrint) SysPrintf("CdAudioRead: Reading Sector %d(%d Blocks of Size %d) at Speed=%dx\n", cdvd.Sector, cdvd.nSectors,cdvd.BlockSize,cdvd.Speed);
			
			cdvd.ReadTime = cdvdBlockReadTime( MODE_CDROM );
			CDVDREAD_INT( cdvdStartSeek( cdvd.SeekToSector ) );

			// Read-ahead by telling the plugin about the track now.
			// This helps improve performance on actual from-cd emulation
			// (ie, not using the hard drive)
			cdvd.RErr = CDVDreadTrack( cdvd.SeekToSector, cdvd.ReadMode );

			// Set the reading block flag.  If a seek is pending then Readed will
			// take priority in the handler anyway.  If the read is contigeous then
			// this'll skip the seek delay.
			cdvd.Reading = 1;
		break;

		case 0x08: // DvdRead
			cdvd.SeekToSector   = *(int*)(cdvd.Param+0);
			cdvd.nSectors = *(int*)(cdvd.Param+4);
			if (cdvd.Param[8] == 0) cdvd.RetryCnt = 0x100;
			else cdvd.RetryCnt = cdvd.Param[8];
			cdvd.SpindlCtrl = cdvd.Param[9];
			cdvd.Speed = 4;
			cdvd.ReadMode = CDVD_MODE_2048;
			cdvd.BlockSize = 2064;	// Why oh why was it 2064
		
			CDR_LOG( "DvdRead > startSector=%d, nSectors=%d, RetryCnt=%x, Speed=%x(%x), ReadMode=%x(%x) (1074=%x)\n",
				cdvd.Sector, cdvd.nSectors, cdvd.RetryCnt, cdvd.Speed, cdvd.Param[9], cdvd.ReadMode, cdvd.Param[10], psxHu32(0x1074));
			
			if (Config.cdvdPrint) SysPrintf("DvdRead: Reading Sector %d(%d Blocks of Size %d) at Speed=%dx\n", cdvd.Sector, cdvd.nSectors,cdvd.BlockSize,cdvd.Speed);
			
			cdvd.ReadTime = cdvdBlockReadTime( MODE_DVDROM );
			CDVDREAD_INT( cdvdStartSeek( cdvd.SeekToSector ) );
			
			// Read-ahead by telling the plugin about the track now.
			// This helps improve performance on actual from-cd emulation
			// (ie, not using the hard drive)
			cdvd.RErr = CDVDreadTrack( cdvd.SeekToSector, cdvd.ReadMode );

			// Set the reading block flag.  If a seek is pending then Readed will
			// take priority in the handler anyway.  If the read is contigeous then
			// this'll skip the seek delay.
			cdvd.Reading = 1;
		break;

		case 0x09: // CdGetToc & cdvdman_call19
			//Param[0] is 0 for CdGetToc and any value for cdvdman_call19
			//the code below handles only CdGetToc!
			//if(cdvd.Param[0]==0x01)
			//{
			DevCon::WriteLn("CDGetToc Param[0]=%d, Param[1]=%d", params cdvd.Param[0],cdvd.Param[1]);
			//}
			cdvdGetToc( PSXM( HW_DMA3_MADR ) );
			cdvdSetIrq( (1<<Irq_CommandComplete) ); //| (1<<Irq_DataReady) );
			HW_DMA3_CHCR &= ~0x01000000;
			psxDmaInterrupt(3);
			break;
		
		case 0x0C: // CdReadKey
		{
			u8  arg0 = cdvd.Param[0];
			u16 arg1 = cdvd.Param[1] | (cdvd.Param[2]<<8);
			u32 arg2 = cdvd.Param[3] | (cdvd.Param[4]<<8) | (cdvd.Param[5]<<16) | (cdvd.Param[6]<<24);
			DevCon::WriteLn("cdvdReadKey(%d, %d, %d)\n", params arg0, arg1, arg2);
			cdvdReadKey(arg0, arg1, arg2, cdvd.Key);
			cdvd.KeyXor = 0x00;
			cdvdSetIrq();
		}
		break;

		case 0x0F: // CdChgSpdlCtrl
			Console::Notice("sceCdChgSpdlCtrl(%d)", params cdvd.Param[0]);
			cdvdSetIrq();
		break;

		default:
			Console::Notice("NCMD Unknown %x", params rt);
			cdvdSetIrq();
		break;
	}
	cdvd.ParamP = 0;
	cdvd.ParamC = 0;
}

void cdvdWrite05(u8 rt) { // NDATAIN
	CDR_LOG("cdvdWrite05(NDataIn) %x\n", rt);

	if (cdvd.ParamP < 32) {
		cdvd.Param[cdvd.ParamP++] = rt;
		cdvd.ParamC++;
	}
}

void cdvdWrite06(u8 rt) { // HOWTO
	CDR_LOG("cdvdWrite06(HowTo) %x\n", rt);
	cdvd.HowTo = rt;
}

void cdvdWrite07(u8 rt)		// BREAK
{
	CDR_LOG("cdvdWrite07(Break) %x\n", rt);

	// If we're already in a Ready state or already Breaking, then do nothing:
	if( cdvd.Ready != 0 || cdvd.Action == cdvdAction_Break )
		return;

	DbgCon::Notice("*PCSX2*: CDVD BREAK %x", params rt);

	// Aborts any one of several CD commands:
	// Pause, Seek, Read, Status, Standby, and Stop

	psxRegs.interrupt &= ~( (1<<IopEvt_Cdvd) | (1<<IopEvt_CdvdRead) );

	cdvd.Action = cdvdAction_Break;
	CDVD_INT( 64 );

	// Clear the cdvd status:
	cdvd.Readed = 0;
	cdvd.Reading = 0;
	cdvd.Status = CDVD_STATUS_NONE;
	//cdvd.nCommand = 0;
}

void cdvdWrite08(u8 rt) { // INTR_STAT
	CDR_LOG("cdvdWrite08(IntrReason) = ACK(%x)\n", rt);
	cdvd.PwOff &= ~rt;
}

void cdvdWrite0A(u8 rt) { // STATUS
	CDR_LOG("cdvdWrite0A(Status) %x\n", rt);
}

void cdvdWrite0F(u8 rt) { // TYPE
	CDR_LOG("cdvdWrite0F(Type) %x\n", rt);
	DevCon::WriteLn("*PCSX2*: CDVD TYPE %x\n", params rt);
}

void cdvdWrite14(u8 rt) { // PS1 MODE??
	u32 cycle = psxRegs.cycle;

	if (rt == 0xFE)	Console::Notice("*PCSX2*: go PS1 mode DISC SPEED = FAST\n");
	else			Console::Notice("*PCSX2*: go PS1 mode DISC SPEED = %dX\n", params rt);

	psxReset();
	psxHu32(0x1f801450) = 0x8;
	psxHu32(0x1f801078) = 1;
	psxRegs.cycle = cycle;
}

void cdvdWrite16(u8 rt)		 // SCOMMAND
{
//	cdvdTN	diskInfo;
//	cdvdTD	trackInfo;
//	int i, lbn, type, min, sec, frm, address;
	int address;
	u8 tmp;
	
	CDR_LOG("cdvdWrite16: SCMD %s (%x) (ParamP = %x)\n", sCmdName[rt], rt, cdvd.ParamP);

	cdvd.sCommand = rt;
	switch (rt) {

//		case 0x01: // GetDiscType - from cdvdman (0:1)
//			SetResultSize(1);
//			cdvd.Result[0] = 0;
//			break;

		case 0x02: // CdReadSubQ  (0:11)
			SetResultSize(11);
			cdvd.Result[0] = cdvdReadSubQ(cdvd.Sector, (cdvdSubQ*)&cdvd.Result[1]);
			break;

		case 0x03: // Mecacon-command
			if(cdvd.Param[0]==0x00) {
				// get mecha version (1:4)
				SetResultSize(4);
				cdvdGetMechaVer(&cdvd.Result[0]);
			}
			else if(cdvd.Param[0]==0x44) {
				// write console ID (9:1)
				SetResultSize(1);
				cdvd.Result[0] = cdvdWriteConsoleID(&cdvd.Param[1]);
			}
			else if(cdvd.Param[0]==0x45) {
				// read console ID (1:9)
				SetResultSize(9);
				cdvd.Result[0] = cdvdReadConsoleID(&cdvd.Result[1]);
			}
			else if(cdvd.Param[0]==0xFD) {
				// _sceCdReadRenewalDate (1:6) BCD
				SetResultSize(6);
				cdvd.Result[0] = 0;
				cdvd.Result[1] = 0x04;//year
				cdvd.Result[2] = 0x12;//month
				cdvd.Result[3] = 0x10;//day
				cdvd.Result[4] = 0x01;//hour
				cdvd.Result[5] = 0x30;//min
			} else {
				SetResultSize(1);
				cdvd.Result[0] = 0x80;
				SysPrintf("*Unknown Mecacon Command param[0]=%02X\n", cdvd.Param[0]);
			}
			break;
		
		case 0x05: // CdTrayReqState  (0:1) - resets the tray open detection
			SetResultSize(1);
			cdvd.Result[0] = 0;
			break;

		case 0x06: // CdTrayCtrl  (1:1)
			SetResultSize(1);
			if(cdvd.Param[0] == 0)
				cdvd.Result[0] = cdvdCtrlTrayOpen();
			else
				cdvd.Result[0] = cdvdCtrlTrayClose();
			break;

		case 0x08: // CdReadRTC (0:8)
			SetResultSize(8);
			cdvd.Result[0] = 0;
			cdvd.Result[1] = itob(cdvd.RTC.second); //Seconds
			cdvd.Result[2] = itob(cdvd.RTC.minute); //Minutes
			cdvd.Result[3] = itob((cdvd.RTC.hour+8) %24); //Hours
			
			cdvd.Result[4] = 0; //Nothing
			cdvd.Result[5] = itob(cdvd.RTC.day); //Day 
			if(cdvd.Result[3] <= 7) cdvd.Result[5] += 1;
			cdvd.Result[6] = itob(cdvd.RTC.month)+0x80; //Month
			cdvd.Result[7] = itob(cdvd.RTC.year); //Year
			/*SysPrintf("RTC Read Sec %x Min %x Hr %x Day %x Month %x Year %x\n", cdvd.Result[1], cdvd.Result[2],
				cdvd.Result[3], cdvd.Result[5], cdvd.Result[6], cdvd.Result[7]);
			SysPrintf("RTC Read Real Sec %d Min %d Hr %d Day %d Month %d Year %d\n", cdvd.RTC.second, cdvd.RTC.minute,
				cdvd.RTC.hour, cdvd.RTC.day, cdvd.RTC.month, cdvd.RTC.year);*/
   
			break;

		case 0x09: // sceCdWriteRTC (7:1)
			
			cdvd.Result[0] = 0;
			cdvd.RTC.pad = 0;

			cdvd.RTC.second = btoi(cdvd.Param[cdvd.ParamP-7]);
			cdvd.RTC.minute = btoi(cdvd.Param[cdvd.ParamP-6]) % 60;
			cdvd.RTC.hour = (btoi(cdvd.Param[cdvd.ParamP-5])+16) % 24;
			cdvd.RTC.day = btoi(cdvd.Param[cdvd.ParamP-3]);
			if(cdvd.Param[cdvd.ParamP-5] <= 7) cdvd.RTC.day -= 1;
			cdvd.RTC.month = btoi(cdvd.Param[cdvd.ParamP-2]-0x80);
			cdvd.RTC.year = btoi(cdvd.Param[cdvd.ParamP-1]);
			/*SysPrintf("RTC write incomming Sec %x Min %x Hr %x Day %x Month %x Year %x\n", cdvd.Param[cdvd.ParamP-7], cdvd.Param[cdvd.ParamP-6],
				cdvd.Param[cdvd.ParamP-5], cdvd.Param[cdvd.ParamP-3], cdvd.Param[cdvd.ParamP-2], cdvd.Param[cdvd.ParamP-1]);
			SysPrintf("RTC Write Sec %d Min %d Hr %d Day %d Month %d Year %d\n", cdvd.RTC.second, cdvd.RTC.minute,
				cdvd.RTC.hour, cdvd.RTC.day, cdvd.RTC.month, cdvd.RTC.year);*/
			//memcpy_fast((u8*)&cdvd.RTC, cdvd.Param, 7);
			SetResultSize(1);
			break;

		case 0x0A: // sceCdReadNVM (2:3)
			address = (cdvd.Param[0]<<8) | cdvd.Param[1];
			if (address < 512) {
				SetResultSize(3);
				cdvd.Result[0] = cdvdReadNVM(&cdvd.Result[1], address*2, 2);
				// swap bytes around
				tmp				= cdvd.Result[1];
				cdvd.Result[1]	= cdvd.Result[2];
				cdvd.Result[2]	= tmp;
			} else {
				SetResultSize(1);
				cdvd.Result[0] = 0xff;
			}
			break;

		case 0x0B: // sceCdWriteNVM (4:1)
			address = (cdvd.Param[0]<<8) | cdvd.Param[1];
			SetResultSize(1);
			if (address < 512) {
				// swap bytes around
				tmp				= cdvd.Param[2];
				cdvd.Param[2]	= cdvd.Param[3];
				cdvd.Param[3]	= tmp;
				cdvd.Result[0] = cdvdWriteNVM(&cdvd.Param[2], address*2, 2);
			} else {
				cdvd.Result[0] = 0xff;
			}
			break;

//		case 0x0C: // sceCdSetHDMode (1:1) 
//			break;


		case 0x0F: // sceCdPowerOff (0:1)- Call74 from Xcdvdman 
			SetResultSize(1);
			cdvd.Result[0] = 0;
			break;

		case 0x12: // sceCdReadILinkId (0:9)
			SetResultSize(9);
			cdvd.Result[0] = cdvdReadILinkID(&cdvd.Result[1]);
			break;

		case 0x13: // sceCdWriteILinkID (8:1)
			SetResultSize(1);
			cdvd.Result[0] = cdvdWriteILinkID(&cdvd.Param[1]);
			break;

		case 0x14: // CdCtrlAudioDigitalOut (1:1)
			//parameter can be 2, 0, ...
			SetResultSize(1);
			cdvd.Result[0] = 0;		//8 is a flag; not used
			break;

		case 0x15: // sceCdForbidDVDP (0:1) 
			//SysPrintf("sceCdForbidDVDP\n");
			SetResultSize(1);
			cdvd.Result[0] = 5;
			break;

		case 0x16: // AutoAdjustCtrl - from cdvdman (1:1)
			SetResultSize(1);
			cdvd.Result[0] = 0;
			break;

		case 0x17: // CdReadModelNumber (1:9) - from xcdvdman
			SetResultSize(9);
			cdvd.Result[0] = cdvdReadModelNumber(&cdvd.Result[1], cdvd.Param[0]);
			break;

		case 0x18: // CdWriteModelNumber (9:1) - from xcdvdman
			SetResultSize(1);
			cdvd.Result[0] = cdvdWriteModelNumber(&cdvd.Param[1], cdvd.Param[0]);
			break;

//		case 0x19: // sceCdForbidRead (0:1) - from xcdvdman
//			break;

		case 0x1A: // sceCdBootCertify (4:1)//(4:16 in psx?)
			SetResultSize(1);//on input there are 4 bytes: 1;?10;J;C for 18000; 1;60;E;C for 39002 from ROMVER
			cdvd.Result[0]=1;//i guess that means okay
			break;

		case 0x1B: // sceCdCancelPOffRdy (0:1) - Call73 from Xcdvdman (1:1)
			SetResultSize(1);
			cdvd.Result[0] = 0;
			break;

		case 0x1C: // sceCdBlueLEDCtl (1:1) - Call72 from Xcdvdman
			SetResultSize(1);
			cdvd.Result[0] = 0;
			break;

//		case 0x1D: // cdvdman_call116 (0:5) - In V10 Bios 
//			break;

		case 0x1E: // sceRemote2Read (0:5) - // 00 14 AA BB CC -> remote key code
			SetResultSize(5);
			cdvd.Result[0] = 0x00;
			cdvd.Result[1] = 0x14;
			cdvd.Result[2] = 0x00;
			cdvd.Result[3] = 0x00;
			cdvd.Result[4] = 0x00;
			break;

//		case 0x1F: // sceRemote2_7 (2:1) - cdvdman_call117
//			break;

		case 0x20: // sceRemote2_6 (0:3)	// 00 01 00
			SetResultSize(3);
			cdvd.Result[0] = 0x00;
			cdvd.Result[1] = 0x01;
			cdvd.Result[2] = 0x00;
			break;

//		case 0x21: // sceCdWriteWakeUpTime (8:1) 
//			break;

		case 0x22: // sceCdReadWakeUpTime (0:10) 
			SetResultSize(10);
			cdvd.Result[0] = 0;
			cdvd.Result[1] = 0;
			cdvd.Result[2] = 0;
			cdvd.Result[3] = 0;
			cdvd.Result[4] = 0;
			cdvd.Result[5] = 0;
			cdvd.Result[6] = 0;
			cdvd.Result[7] = 0;
			cdvd.Result[8] = 0;
			cdvd.Result[9] = 0;
			break;

		case 0x24: // sceCdRCBypassCtrl (1:1) - In V10 Bios 
			// FIXME: because PRId<0x23, the bit 0 of sio2 don't get updated 0xBF808284
			SetResultSize(1);
			cdvd.Result[0] = 0;
		break;

//		case 0x25: // cdvdman_call120 (1:1) - In V10 Bios 
//			break;

//		case 0x26: // cdvdman_call128 (0,3) - In V10 Bios 
//			break;

//		case 0x27: // cdvdman_call148 (0:13) - In V10 Bios 
//			break;

//		case 0x28: // cdvdman_call150 (1:1) - In V10 Bios 
//			break;

		case 0x29: //sceCdNoticeGameStart (1:1)
			SetResultSize(1);
			cdvd.Result[0] = 0;
			break;

//		case 0x2C: //sceCdXBSPowerCtl (2:2)
//			break;

//		case 0x2D: //sceCdXLEDCtl (2:2)
//			break;

//		case 0x2E: //sceCdBuzzerCtl (0:1)
//			break;

//		case 0x2F: //cdvdman_call167 (16:1)
//			break;

//		case 0x30: //cdvdman_call169 (1:9)
//			break;

		case 0x31: //sceCdSetMediumRemoval (1:1)
			SetResultSize(1);
			cdvd.Result[0] = 0;
			break;

		case 0x32: //sceCdGetMediumRemoval (0:2)
			SetResultSize(2);
			cdvd.Result[0] = 0;
			cdvd.Result[0] = 0;
			break;

//		case 0x33: //sceCdXDVRPReset (1:1)
//			break;

		case 0x36: //cdvdman_call189 [__sceCdReadRegionParams - made up name] (0:15) i think it is 16, not 15
			SetResultSize(15);
			cdvdGetMechaVer(&cdvd.Result[1]);
			cdvd.Result[0] = cdvdReadRegionParams(&cdvd.Result[3]);//size==8
			SysPrintf("REGION PARAMS = %s %s\n", mg_zones[cdvd.Result[1]], &cdvd.Result[3]);
			cdvd.Result[1] = 1 << cdvd.Result[1];	//encryption zone; see offset 0x1C in encrypted headers
			//////////////////////////////////////////
			cdvd.Result[2] = 0;						//??
//			cdvd.Result[3] == ROMVER[4] == *0xBFC7FF04
//			cdvd.Result[4] == OSDVER[4] == CAP			Jjpn, Aeng, Eeng, Heng, Reng, Csch, Kkor?
//			cdvd.Result[5] == OSDVER[5] == small
//			cdvd.Result[6] == OSDVER[6] == small
//			cdvd.Result[7] == OSDVER[7] == small
//			cdvd.Result[8] == VERSTR[0x22] == *0xBFC7FF52
//			cdvd.Result[9] == DVDID						J U O E A R C M
//			cdvd.Result[10]== 0;					//??
			cdvd.Result[11] = 0;					//??
			cdvd.Result[12] = 0;					//??
			//////////////////////////////////////////
			cdvd.Result[13] = 0;					//0xFF - 77001
			cdvd.Result[14] = 0;					//??
			break;

		case 0x37: //called from EECONF [sceCdReadMAC - made up name] (0:9)
			SetResultSize(9);
			cdvd.Result[0] = cdvdReadMAC(&cdvd.Result[1]);
			break;

		case 0x38: //used to fix the MAC back after accidentally trashed it :D [sceCdWriteMAC - made up name] (8:1)
			SetResultSize(1);
			cdvd.Result[0] = cdvdWriteMAC(&cdvd.Param[0]);
			break;

		case 0x3E: //[__sceCdWriteRegionParams - made up name] (15:1) [Florin: hum, i was expecting 14:1]
			SetResultSize(1);
			cdvd.Result[0] = cdvdWriteRegionParams(&cdvd.Param[2]);
			break;

		case 0x40: // CdOpenConfig (3:1)
			cdvd.CReadWrite = cdvd.Param[0];
			cdvd.COffset    = cdvd.Param[1];
			cdvd.CNumBlocks = cdvd.Param[2];
			cdvd.CBlockIndex= 0;
			SetResultSize(1);
			cdvd.Result[0] = 0;
			break;

		case 0x41: // CdReadConfig (0:16)
			SetResultSize(16);
			cdvdReadConfig(&cdvd.Result[0]);
			break;

		case 0x42: // CdWriteConfig (16:1)
			SetResultSize(1);
			cdvd.Result[0] = cdvdWriteConfig(&cdvd.Param[0]);
			break;

		case 0x43: // CdCloseConfig (0:1)
			cdvd.CReadWrite = 0;
			cdvd.COffset    = 0;
			cdvd.CNumBlocks = 0;
			cdvd.CBlockIndex= 0;
			SetResultSize(1);
			cdvd.Result[0] = 0;
			break;

		case 0x80: // secrman: __mechacon_auth_0x80
			cdvd.mg_datatype = 0;//data
			SetResultSize(1);//in:1
			cdvd.Result[0] = 0;
			break;

		case 0x81: // secrman: __mechacon_auth_0x81
			cdvd.mg_datatype = 0;//data
			SetResultSize(1);//in:1
			cdvd.Result[0] = 0;
			break;

		case 0x82: // secrman: __mechacon_auth_0x82
			SetResultSize(1);//in:16
			cdvd.Result[0] = 0;
			break;

		case 0x83: // secrman: __mechacon_auth_0x83
			SetResultSize(1);//in:8
			cdvd.Result[0] = 0;
			break;

		case 0x84: // secrman: __mechacon_auth_0x84
			SetResultSize(1+8+4);//in:0
			cdvd.Result[0] = 0;
			
			cdvd.Result[1] = 0x21;
			cdvd.Result[2] = 0xdc;
			cdvd.Result[3] = 0x31;
			cdvd.Result[4] = 0x96;
			cdvd.Result[5] = 0xce;
			cdvd.Result[6] = 0x72;
			cdvd.Result[7] = 0xe0;
			cdvd.Result[8] = 0xc8;

			cdvd.Result[9]  = 0x69;
			cdvd.Result[10] = 0xda;
			cdvd.Result[11] = 0x34;
			cdvd.Result[12] = 0x9b;
			break;

		case 0x85: // secrman: __mechacon_auth_0x85
			SetResultSize(1+4+8);//in:0
			cdvd.Result[0] = 0;
			
			cdvd.Result[1] = 0xeb;
			cdvd.Result[2] = 0x01;
			cdvd.Result[3] = 0xc7;
			cdvd.Result[4] = 0xa9;

			cdvd.Result[ 5] = 0x3f;
			cdvd.Result[ 6] = 0x9c;
			cdvd.Result[ 7] = 0x5b;
			cdvd.Result[ 8] = 0x19;
			cdvd.Result[ 9] = 0x31;
			cdvd.Result[10] = 0xa0;
			cdvd.Result[11] = 0xb3;
			cdvd.Result[12] = 0xa3;
			break;

		case 0x86: // secrman: __mechacon_auth_0x86
			SetResultSize(1);//in:16
			cdvd.Result[0] = 0;
			break;

		case 0x87: // secrman: __mechacon_auth_0x87
			SetResultSize(1);//in:8
			cdvd.Result[0] = 0;
			break;

		case 0x8D: // sceMgWriteData
			SetResultSize(1);//in:length<=16
			if (cdvd.mg_size + cdvd.ParamC > cdvd.mg_maxsize)
				cdvd.Result[0] = 0x80;
			else{
				memcpy_fast(cdvd.mg_buffer + cdvd.mg_size, cdvd.Param, cdvd.ParamC);
				cdvd.mg_size += cdvd.ParamC;
				cdvd.Result[0] = 0; // 0 complete ; 1 busy ; 0x80 error
			}
			break;

		case 0x8E: // sceMgReadData
			SetResultSize( std::min(16, cdvd.mg_size) );
			memcpy_fast(cdvd.Result, cdvd.mg_buffer, cdvd.ResultC);
			cdvd.mg_size -= cdvd.ResultC;
			memcpy_fast(cdvd.mg_buffer, cdvd.mg_buffer+cdvd.ResultC, cdvd.mg_size);
			break;

		case 0x88: // secrman: __mechacon_auth_0x88	//for now it is the same; so, fall;)
		case 0x8F: // secrman: __mechacon_auth_0x8F
			SetResultSize(1);//in:0
			if (cdvd.mg_datatype == 1){// header data
                u64* psrc, *pdst;
				int bit_ofs, i;

				if (cdvd.mg_maxsize != cdvd.mg_size)				goto fail_pol_cal;
				if (cdvd.mg_size < 0x20)							goto fail_pol_cal;
				if (cdvd.mg_size != *(u16*)&cdvd.mg_buffer[0x14])	goto fail_pol_cal;
				SysPrintf("[MG] ELF_size=0x%X Hdr_size=0x%X unk=0x%X flags=0x%X count=%d zones=",
					*(u32*)&cdvd.mg_buffer[0x10], *(u16*)&cdvd.mg_buffer[0x14], *(u16*)&cdvd.mg_buffer[0x16],
					*(u16*)&cdvd.mg_buffer[0x18], *(u16*)&cdvd.mg_buffer[0x1A]);
				for (i=0; i<8; i++)
					if (cdvd.mg_buffer[0x1C] & (1<<i))
						SysPrintf("%s ", mg_zones[i]);
				SysPrintf("\n");
				bit_ofs = mg_BIToffset(cdvd.mg_buffer);
                psrc = (u64*)&cdvd.mg_buffer[bit_ofs-0x20];
                pdst = (u64*)cdvd.mg_kbit;
                pdst[0] = psrc[0]; pdst[1] = psrc[1];//memcpy(cdvd.mg_kbit, &cdvd.mg_buffer[bit_ofs-0x20], 0x10);
                pdst = (u64*)cdvd.mg_kcon;
                pdst[0] = psrc[2]; pdst[1] = psrc[3];//memcpy(cdvd.mg_kcon, &cdvd.mg_buffer[bit_ofs-0x10], 0x10);

				if (cdvd.mg_buffer[bit_ofs+5] || cdvd.mg_buffer[bit_ofs+6] || cdvd.mg_buffer[bit_ofs+7])goto fail_pol_cal;
				if (cdvd.mg_buffer[bit_ofs+4] * 16 + bit_ofs + 8 + 16 != *(u16*)&cdvd.mg_buffer[0x14]){
fail_pol_cal:
					SysPrintf("[MG] ERROR - Make sure the file is already decrypted!!!\n");
					cdvd.Result[0] = 0x80;
					break;
				}
			}
			cdvd.Result[0] = 0; // 0 complete ; 1 busy ; 0x80 error
			break;

		case 0x90: // sceMgWriteHeaderStart
			cdvd.mg_size = 0;
			cdvd.mg_datatype = 1;//header data
			SysPrintf("[MG] hcode=%d cnum=%d a2=%d length=0x%X\n",
				cdvd.Param[0], cdvd.Param[3], cdvd.Param[4], cdvd.mg_maxsize = cdvd.Param[1] | (((int)cdvd.Param[2])<<8));
			SetResultSize(1);//in:5
			cdvd.Result[0] = 0; // 0 complete ; 1 busy ; 0x80 error
			break;

		case 0x91: // sceMgReadBITLength
			SetResultSize(3);//in:0
			{
				int bit_ofs = mg_BIToffset(cdvd.mg_buffer);
				memcpy_fast(cdvd.mg_buffer, &cdvd.mg_buffer[bit_ofs], 8+16*cdvd.mg_buffer[bit_ofs+4]);
			}
			cdvd.mg_maxsize = 0; // don't allow any write
			cdvd.mg_size = 8+16*cdvd.mg_buffer[4];//new offset, i just moved the data
			SysPrintf("[MG] BIT count=%d\n", cdvd.mg_buffer[4]);

			cdvd.Result[0] = cdvd.mg_datatype==1 ? 0 : 0x80; // 0 complete ; 1 busy ; 0x80 error
			cdvd.Result[1] = (cdvd.mg_size >> 0) & 0xFF;
			cdvd.Result[2] = (cdvd.mg_size >> 8) & 0xFF;
			break;

		case 0x92: // sceMgWriteDatainLength
			cdvd.mg_size = 0;
			cdvd.mg_datatype = 0;//data (encrypted)
			cdvd.mg_maxsize = cdvd.Param[0] | (((int)cdvd.Param[1])<<8);
			SetResultSize(1);//in:2
			cdvd.Result[0] = 0; // 0 complete ; 1 busy ; 0x80 error
			break;

		case 0x93: // sceMgWriteDataoutLength
			SetResultSize(1);//in:2
			if (((cdvd.Param[0] | (((int)cdvd.Param[1])<<8)) == cdvd.mg_size) && (cdvd.mg_datatype == 0)){
				cdvd.mg_maxsize = 0; // don't allow any write
				cdvd.Result[0] = 0; // 0 complete ; 1 busy ; 0x80 error
			}else
				cdvd.Result[0] = 0x80;
			break;

		case 0x94: // sceMgReadKbit - read first half of BIT key
			SetResultSize(1+8);//in:0
			cdvd.Result[0] = 0;

            ((int*)(cdvd.Result+1))[0] = ((int*)cdvd.mg_kbit)[0];
            ((int*)(cdvd.Result+1))[1] = ((int*)cdvd.mg_kbit)[1];//memcpy(cdvd.Result+1, cdvd.mg_kbit, 8);
            break;

		case 0x95: // sceMgReadKbit2 - read second half of BIT key
			SetResultSize(1+8);//in:0
			cdvd.Result[0] = 0;
            ((int*)(cdvd.Result+1))[0] = ((int*)(cdvd.mg_kbit+8))[0];
            ((int*)(cdvd.Result+1))[1] = ((int*)(cdvd.mg_kbit+8))[1];//memcpy(cdvd.Result+1, cdvd.mg_kbit+8, 8);
			break;

		case 0x96: // sceMgReadKcon - read first half of content key
			SetResultSize(1+8);//in:0
			cdvd.Result[0] = 0;
            ((int*)(cdvd.Result+1))[0] = ((int*)cdvd.mg_kcon)[0];
            ((int*)(cdvd.Result+1))[1] = ((int*)cdvd.mg_kcon)[1];//memcpy(cdvd.Result+1, cdvd.mg_kcon, 8);
			break;

		case 0x97: // sceMgReadKcon2 - read second half of content key
			SetResultSize(1+8);//in:0
			cdvd.Result[0] = 0;
            ((int*)(cdvd.Result+1))[0] = ((int*)(cdvd.mg_kcon+8))[0];
            ((int*)(cdvd.Result+1))[1] = ((int*)(cdvd.mg_kcon+8))[1];//memcpy(cdvd.Result+1, cdvd.mg_kcon+8, 8);
			break;

		default:
			// fake a 'correct' command
			SysPrintf("SCMD Unknown %x\n", rt);
			SetResultSize(1);		//in:0
			cdvd.Result[0] = 0;		// 0 complete ; 1 busy ; 0x80 error
			break;
	}
		//SysPrintf("SCMD - %x\n", rt);
		cdvd.ParamP = 0; cdvd.ParamC = 0;
}

void cdvdWrite17(u8 rt) { // SDATAIN
	CDR_LOG("cdvdWrite17(SDataIn) %x\n", rt);
	
	if (cdvd.ParamP < 32) {
		cdvd.Param[cdvd.ParamP++] = rt;
		cdvd.ParamC++;
	}
}

void cdvdWrite18(u8 rt) { // SDATAOUT
	CDR_LOG("cdvdWrite18(SDataOut) %x\n", rt);
	SysPrintf("*PCSX2* SDATAOUT\n");
}

void cdvdWrite3A(u8 rt) { // DEC-SET
	CDR_LOG("cdvdWrite3A(DecSet) %x\n", rt);
	cdvd.decSet = rt;
	SysPrintf("DecSet Write: %02X\n", cdvd.decSet);
}