
#include <stdio.h>

#include "CDVD.h"
#include "isofs/CDVDlib.h"
#include "isofs/CDVDiso.h"
#include "isofs/CDVDisodrv.h"

char	buffer[2048]; //if the file is longer...it should be shorter :D

int CheckDiskType(int baseType)
{
	int		f;
	char	*pos;
	static struct TocEntry tocEntry;

	CDVDFS_init();

	// check if the file exists
	if (CDVD_findfile("SYSTEM.CNF;1", &tocEntry) == TRUE)
	{
		f=CDVDFS_open("SYSTEM.CNF;1", 1);
		CDVDFS_read(f, buffer, 2047);
		CDVDFS_close(f);
		
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

	if (CDVD_findfile("PSX.EXE;1", &tocEntry) == TRUE)
	{
		return CDVD_TYPE_PSCD;
	}

	if (CDVD_findfile("VIDEO_TS/VIDEO_TS.IFO;1", &tocEntry) == TRUE)
	{
		return CDVD_TYPE_DVDV;
	}

	return CDVD_TYPE_ILLEGAL;
}

char bleh[2352];

int FindDiskType()
{
	int dataTracks=0;
	int audioTracks=0;

	int iCDType = CDVD_TYPE_DETCTDVDS;

	s32 mt=cdvdGetMediaType();

	if(mt<0)
	{
		if(tracks[0].length>452849)
		{
			iCDType = CDVD_TYPE_DETCTDVDS;
		}
		else if(cdvdDirectReadSector(16,CDVD_MODE_2048,bleh)==0)
		{
			struct cdVolDesc* volDesc=(struct cdVolDesc *)bleh;
			if(volDesc)
			{                                 
				if(volDesc->rootToc.tocSize==2048) iCDType = CDVD_TYPE_DETCTCD;
				else                               iCDType = CDVD_TYPE_DETCTDVDS;
			}
		}
	}

	fprintf(stderr," * CDVD Disk Open: %d tracks (%d to %d):\n",etrack-strack+1,strack,etrack);

	audioTracks=dataTracks=0;
	for(int i=strack;i<=etrack;i++)
	{
		if(tracks[i].type==CDVD_AUDIO_TRACK) 
		{
			audioTracks++;
			fprintf(stderr," * * Track %d: Audio (%d sectors)\n",i,tracks[i].length);
		}
		else 
		{
			dataTracks++;
			fprintf(stderr," * * Track %d: Data (Mode %d) (%d sectors)\n",i,((tracks[i].type==CDVD_MODE1_TRACK)?1:2),tracks[i].length);
		}
	}

	if(dataTracks>0)
	{
		iCDType=CheckDiskType(iCDType);
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