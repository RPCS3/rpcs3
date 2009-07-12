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

#include <ctype.h>
#include <time.h>

#include "IopCommon.h"
#include "IsoFStools.h"
#include "IsoFSdrv.h"
#include "CDVDisoReader.h"

static int diskTypeCached=-1;

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
		else if(DoCDVDreadSector((u8*)bleh,16)==0)
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
	if(loadFromISO)
		return ISOopen(pTitleFilename);
	else
		return CDVDopen(pTitleFilename);
}

void DoCDVDclose()
{
	if(loadFromISO)
		ISOclose();
	else
		CDVDclose();
}

void DoCDVDshutdown()
{
	if(!loadFromISO)
	{
		if (CDVDshutdown != NULL) CDVDshutdown();
	}
}

s32 DoCDVDreadSector(u8* buffer, u32 lsn)
{
	if(loadFromISO)
		return ISOreadSector(buffer,lsn);
	else
	{
		CDVDreadTrack(lsn,CDVD_MODE_2048);
		void* pbuffer = CDVDgetBuffer();
		if(pbuffer!=NULL)
		{
			memcpy(buffer,pbuffer,2048);
			return 0;
		}
		return -1;
	}
}

s32 DoCDVDreadTrack(u32 lsn, int mode)
{
	if(loadFromISO)
		return ISOreadTrack(lsn, mode);
	else
		return CDVDreadTrack(lsn, mode);
}

// return can be NULL (for async modes)
u8* DoCDVDgetBuffer()
{
	if(loadFromISO)
		return ISOgetBuffer();
	else
		return CDVDgetBuffer();
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
