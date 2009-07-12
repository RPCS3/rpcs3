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

#include <ctype.h>
#include <time.h>

#include "IopCommon.h"
#include "IsoFStools.h"
#include "IsoFSdrv.h"
#include "CDVDisoReader.h"

static int diskTypeCached=-1;

static int psize;
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

	DoCDVDgetTN(&tn);

	if((mt<0) || ((mt == CDVD_TYPE_DETCTDVDS) && (tn.strack != tn.etrack)))
	{
		cdvdTD td;
		DoCDVDgetTD(0,&td);
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

	fprintf(stderr," * CDVD Disk Open: %d tracks (%d to %d):\n",tn.etrack-tn.strack+1,tn.strack,tn.etrack);

	audioTracks=dataTracks=0;
	for(int i=tn.strack;i<=tn.etrack;i++)
	{
		cdvdTD td,td2;
		DoCDVDgetTD(i,&td);
		DoCDVDgetTD(i+1,&td2);

		int tlength = td2.lsn - td.lsn;

		if(td.type==CDVD_AUDIO_TRACK) 
		{
			audioTracks++;
			fprintf(stderr," * * Track %d: Audio (%d sectors)\n",i,tlength);
		}
		else 
		{
			dataTracks++;
			fprintf(stderr," * * Track %d: Data (Mode %d) (%d sectors)\n",i,((td.type==CDVD_MODE1_TRACK)?1:2),tlength);
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
	if (DoCDVDgetTrayStatus() == CDVD_TRAY_OPEN)
	{
		diskTypeCached = CDVD_TYPE_NODISC;
		return;
	}

	int baseMediaType = DoCDVDgetDiskType();
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
	if(!loadFromISO)
		return CDVDinit();

	diskTypeCached=-1;

	return 0;
}

s32 DoCDVDopen(const char* pTitleFilename)
{
	int ret=0;
	if(loadFromISO)
		ret = ISOopen(pTitleFilename);
	else
		ret = CDVDopen(pTitleFilename);

	if (Config.Blockdump)
	{
		char fname_only[MAX_PATH];

		if(loadFromISO)
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
#else
		// TODO: implement this
		strcat(fname_only, ".dump");
#endif

		blockDumpFile = isoCreate(fname_only, ISOFLAGS_BLOCKDUMP);
		if (blockDumpFile) isoSetFormat(blockDumpFile, iso->blockofs, iso->blocksize, iso->blocks);
	}
	else
	{
		blockDumpFile = NULL;
	}

	return ret;
}

void DoCDVDclose()
{
	if(loadFromISO)
		ISOclose();
	else
		CDVDclose();

	if (blockDumpFile != NULL) isoClose(blockDumpFile);
}

void DoCDVDshutdown()
{
	if(!loadFromISO)
	{
		if (CDVDshutdown != NULL) CDVDshutdown();
	}
}

s32 DoCDVDreadSector(u8* buffer, u32 lsn, int mode)
{
	int ret;

	if(loadFromISO)
		ret = ISOreadSector(buffer,lsn,mode);
	else
	{
		CDVDreadTrack(lsn,mode);
		void* pbuffer = CDVDgetBuffer();
		if(pbuffer!=NULL)
		{
			switch(mode)
			{
			case CDVD_MODE_2048:
				memcpy(buffer,pbuffer,2048);
				break;
			case CDVD_MODE_2328:
				memcpy(buffer,pbuffer,2328);
				break;
			case CDVD_MODE_2340:
				memcpy(buffer,pbuffer,2340);
				break;
			case CDVD_MODE_2352:
				memcpy(buffer,pbuffer,2352);
				break;
			}
			ret = 0;
		}
		else ret = -1;
	}


	if(ret==0)
	{
		if (blockDumpFile != NULL)
		{
			isoWriteBlock(blockDumpFile, pbuffer, plsn);
		}
	}
	return ret;
}

s32 DoCDVDreadTrack(u32 lsn, int mode)
{
	if(loadFromISO)
		return ISOreadTrack(lsn, mode);
	else
	{
		// TEMP: until I fix all the plugins to use the new CDVDgetBuffer style
		switch (mode)
		{
		case CDVD_MODE_2352:
			psize = 2352;
			break;
		case CDVD_MODE_2340:
			psize = 2340;
			break;
		case CDVD_MODE_2328:
			psize = 2328;
			break;
		case CDVD_MODE_2048:
			psize = 2048;
			break;
		}
		return CDVDreadTrack(lsn, mode);
	}
}

// return can be NULL (for async modes)
s32 DoCDVDgetBuffer(u8* buffer)
{
	int ret;

	if(loadFromISO)
		ret = ISOgetBuffer(buffer);
	else
	{
		// TEMP: until I fix all the plugins to use this function style
		u8* pb = CDVDgetBuffer();
		if(pb!=NULL)
		{
			memcpy(buffer,pb,psize);
			ret=0;
		}
		else ret= -1;
	}

	if(ret==0)
	{
		if (blockDumpFile != NULL)
		{
			isoWriteBlock(blockDumpFile, pbuffer, plsn);
		}
	}
	return ret;
}

s32 DoCDVDreadSubQ(u32 lsn, cdvdSubQ* subq)
{
	if(loadFromISO)
		return ISOreadSubQ(lsn,subq);
	else
		return CDVDreadSubQ(lsn,subq);
}

s32 DoCDVDgetTN(cdvdTN *Buffer)
{
	if(loadFromISO)
		return ISOgetTN(Buffer);
	else
		return CDVDgetTN(Buffer);
}

s32 DoCDVDgetTD(u8 Track, cdvdTD *Buffer)
{
	if(loadFromISO)
		return ISOgetTD(Track,Buffer);
	else
		return CDVDgetTD(Track,Buffer);
}

s32 DoCDVDgetTOC(void* toc)
{
	if(loadFromISO)
		return ISOgetTOC(toc);
	else
		return CDVDgetTOC(toc);
}

s32 DoCDVDgetDiskType()
{
	if(loadFromISO)
		return ISOgetDiskType();
	else
		return CDVDgetDiskType();
}

s32 DoCDVDdetectDiskType()
{
	if(diskTypeCached<0)
		DetectDiskType();

	return diskTypeCached;
}

void DoCDVDresetDiskTypeCache()
{
	diskTypeCached=-1;
}

s32 DoCDVDgetTrayStatus()
{
	if(loadFromISO)
		return ISOgetTrayStatus();
	else
		return CDVDgetTrayStatus();

}

s32 DoCDVDctrlTrayOpen()
{
	if(loadFromISO)
		return ISOctrlTrayOpen();
	else
		return CDVDctrlTrayOpen();
}

s32 DoCDVDctrlTrayClose()
{
	if(loadFromISO)
		return ISOctrlTrayClose();
	else
		return CDVDctrlTrayClose();
}

void DoCDVDnewDiskCB(void (*callback)())
{
	if(!loadFromISO)
	{
		if (CDVDnewDiskCB) CDVDnewDiskCB(callback);
	}
}
