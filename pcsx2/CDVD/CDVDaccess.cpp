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

#include "PrecompiledHeader.h"

// TODO: fix this for linux! (hardcoded as _WIN32 only)
#define ENABLE_TIMESTAMPS

#ifdef _WIN32
#include <windows.h>
#endif

#include <ctype.h>
#include <time.h>

#include "IopCommon.h"
#include "IsoFStools.h"
#include "IsoFSdrv.h"
#include "CDVDisoReader.h"

static int diskTypeCached=-1;

int lastReadSize;
static int plsn=0;

static isoFile *blockDumpFile;

/////////////////////////////////////////////////
//
// Disk Type detection stuff (from cdvdGigaherz)
//

int CheckDiskTypeFS(int baseType)
{
	int		f;
	char	buffer[256];//if a file is longer...it should be shorter :D
	char	*pos;
	static struct TocEntry tocEntry;

	IsoFS_init();

	// check if the file exists
	if (IsoFS_findFile("SYSTEM.CNF;1", &tocEntry) == TRUE)
	{
		f=IsoFS_open("SYSTEM.CNF;1", 1);
		IsoFS_read(f, buffer, 256);
		IsoFS_close(f);

		buffer[tocEntry.fileSize]='\0';

		pos=strstr(buffer, "BOOT2");
		if (pos==NULL){
			pos=strstr(buffer, "BOOT");
			if (pos==NULL) {
				return CDVD_TYPE_ILLEGAL;
			}
			return CDVD_TYPE_PSCD;
		}
		return (baseType==CDVD_TYPE_DETCTCD)?CDVD_TYPE_PS2CD:CDVD_TYPE_PS2DVD;
	}

	if (IsoFS_findFile("PSX.EXE;1", &tocEntry) == TRUE)
	{
		return CDVD_TYPE_PSCD;
	}

	if (IsoFS_findFile("VIDEO_TS/VIDEO_TS.IFO;1", &tocEntry) == TRUE)
	{
		return CDVD_TYPE_DVDV;
	}

	return CDVD_TYPE_ILLEGAL; // << Only for discs which aren't ps2 at all.
}

static char bleh[2352];

int FindDiskType(int mType)
{
	int dataTracks=0;
	int audioTracks=0;

	int iCDType = CDVD_TYPE_DETCTDVDS;

	s32 mt=mType;

	cdvdTN tn;

	CDVD.getTN(&tn);

	if((mt<0) || ((mt == CDVD_TYPE_DETCTDVDS) && (tn.strack != tn.etrack)))
	{
		cdvdTD td;
		CDVD.getTD(0,&td);
		if(td.lsn>452849)
		{
			iCDType = CDVD_TYPE_DETCTDVDS;
		}
		else if(DoCDVDreadSector((u8*)bleh,16,CDVD_MODE_2048)==0)
		{
			struct cdVolDesc* volDesc=(struct cdVolDesc *)bleh;
			if(volDesc)
			{                                 
				if(volDesc->rootToc.tocSize==2048) iCDType = CDVD_TYPE_DETCTCD;
				else                               iCDType = CDVD_TYPE_DETCTDVDS;
			}
		}
	}

	DevCon::Status(" * CDVD Disk Open: %d tracks (%d to %d):\n", params tn.etrack-tn.strack+1,tn.strack,tn.etrack);

	audioTracks=dataTracks=0;
	for(int i=tn.strack;i<=tn.etrack;i++)
	{
		cdvdTD td,td2;
		CDVD.getTD(i,&td);

		if(tn.etrack>i)
			CDVD.getTD(i+1,&td2);
		else
			CDVD.getTD(0,&td2);

		int tlength = td2.lsn - td.lsn;

		if(td.type==CDVD_AUDIO_TRACK) 
		{
			audioTracks++;
			DevCon::Status(" * * Track %d: Audio (%d sectors)\n", params i,tlength);
		}
		else 
		{
			dataTracks++;
			DevCon::Status(" * * Track %d: Data (Mode %d) (%d sectors)\n", params i,((td.type==CDVD_MODE1_TRACK)?1:2),tlength);
		}
	}

	if(dataTracks>0)
	{
		iCDType=CheckDiskTypeFS(iCDType);
	}

	if(audioTracks>0)
	{
		if(iCDType==CDVD_TYPE_PS2CD)
		{
			iCDType=CDVD_TYPE_PS2CDDA;
		}
		else if(iCDType==CDVD_TYPE_PSCD)
		{
			iCDType=CDVD_TYPE_PSCDDA;
		}
		else
		{
			iCDType=CDVD_TYPE_CDDA;
		}
	}

	return iCDType;
}

void DetectDiskType()
{
	if (CDVD.getTrayStatus() == CDVD_TRAY_OPEN)
	{
		diskTypeCached = CDVD_TYPE_NODISC;
		return;
	}

	int baseMediaType = CDVD.getDiskType();
	int mType = -1;

	switch(baseMediaType) // Paranoid mode: do not trust the plugin's detection system to work correctly.
	{
	case CDVD_TYPE_CDDA:
	case CDVD_TYPE_PSCD:
	case CDVD_TYPE_PS2CD:
	case CDVD_TYPE_PSCDDA:
	case CDVD_TYPE_PS2CDDA:
		mType=CDVD_TYPE_DETCTCD;
		break;
	case CDVD_TYPE_DVDV:
	case CDVD_TYPE_PS2DVD:
		mType=CDVD_TYPE_DETCTDVDS;
		break;
	case CDVD_TYPE_DETCTDVDS:
	case CDVD_TYPE_DETCTDVDD:
	case CDVD_TYPE_DETCTCD:
		mType=baseMediaType;
		break;
	case CDVD_TYPE_NODISC:
		diskTypeCached = CDVD_TYPE_NODISC;
		return;
	}

	diskTypeCached = FindDiskType(mType);
}

//
/////////////////////////////////////////////////

s32 DoCDVDinit()
{
	diskTypeCached=-1;

	if(CDVD.initCount) *CDVD.initCount++; // used to handle the case where the plugin was inited at boot, but then iso takes over
	return CDVD.init();
}

void DoCDVDshutdown()
{
	if(CDVD.initCount) *CDVD.initCount--;
	CDVD.shutdown();
}

s32 DoCDVDopen(const char* pTitleFilename)
{
	int ret=0;

	ret = CDVD.open(pTitleFilename);

	int cdtype = DoCDVDdetectDiskType();

	if((Config.Blockdump)&&(cdtype != CDVD_TYPE_NODISC))
	{
		char fname_only[MAX_PATH];

		if(CDVD.init == ISO.init)
		{
#ifdef _WIN32
			char fname[MAX_PATH], ext[MAX_PATH];
			_splitpath(isoFileName, NULL, NULL, fname, ext);
			_makepath(fname_only, NULL, NULL, fname, NULL);
#else
			char* p, *plast;

			plast = p = strchr(isoFileName, '/');
			while (p != NULL)
			{
				plast = p;
				p = strchr(p + 1, '/');
			}

			// Lets not create dumps in the plugin directory.
			strcpy(fname_only, "../");
			if (plast != NULL) 
				strcat(fname_only, plast + 1);
			else 
				strcat(fname_only, isoFileName);

			plast = p = strchr(fname_only, '.');

			while (p != NULL)
			{
				plast = p;
				p = strchr(p + 1, '.');
			}

			if (plast != NULL) *plast = 0;
#endif
		}
		else
		{
			strcpy(fname_only, "Untitled");
		}

#if defined(_WIN32) && defined(ENABLE_TIMESTAMPS)
		SYSTEMTIME time;
		GetLocalTime(&time);

		sprintf(
			fname_only+strlen(fname_only),
			" (%04d-%02d-%02d %02d-%02d-%02d).dump",
			time.wYear, time.wMonth, time.wDay,
			time.wHour, time.wMinute, time.wSecond);

		// TODO: implement this for linux
#else
		strcat(fname_only, ".dump");
#endif
		cdvdTD td;
		CDVD.getTD(0, &td);

		int blockofs=0;
		int blocksize=0;
		int blocks = td.lsn;

		switch(cdtype)
		{
		case CDVD_TYPE_PS2DVD:
		case CDVD_TYPE_DVDV:
		case CDVD_TYPE_DETCTDVDS:
		case CDVD_TYPE_DETCTDVDD:
			blockofs = 24;
			blocksize = 2048;
			break;
		default:
			blockofs = 0;
			blocksize= 2352;
			break;
		}

		blockDumpFile = isoCreate(fname_only, ISOFLAGS_BLOCKDUMP);
		if (blockDumpFile) isoSetFormat(blockDumpFile, blockofs, blocksize, blocks);
	}
	else
	{
		blockDumpFile = NULL;
	}

	return ret;
}

s32 DoCDVDreadSector(u8* buffer, u32 lsn, int mode)
{
	int ret = CDVD.readSector(buffer,lsn,mode);

	if(ret==0)
	{
		if (blockDumpFile != NULL)
		{
			isoWriteBlock(blockDumpFile, buffer, plsn);
		}
	}
	return ret;
}

s32 DoCDVDreadTrack(u32 lsn, int mode)
{
	// TEMP: until all the plugins use the new CDVDgetBuffer style
	switch (mode)
	{
	case CDVD_MODE_2352:
		lastReadSize = 2352;
		break;
	case CDVD_MODE_2340:
		lastReadSize = 2340;
		break;
	case CDVD_MODE_2328:
		lastReadSize = 2328;
		break;
	case CDVD_MODE_2048:
		lastReadSize = 2048;
		break;
	}

	return CDVD.readTrack(lsn,mode);
}

// return can be NULL (for async modes)

s32 DoCDVDgetBuffer(u8* buffer)
{
	int ret = CDVD.getBuffer2(buffer);

	if(ret==0)
	{
		if (blockDumpFile != NULL)
		{
			isoWriteBlock(blockDumpFile, buffer, plsn);
		}
	}

	return ret;
}

s32 DoCDVDdetectDiskType()
{
	if(diskTypeCached<0)
		DetectDiskType();

	return diskTypeCached;
}

void DoCDVDresetDiskTypeCache()
{
	diskTypeCached = -1;
}
