#undef _WIN32_WINNT
#define _WIN32_WINNT 0x0600
#include "CDVD.h"
#pragma warning(disable:4200)
#pragma pack(1)
#include <winioctl.h>
#include "rosddk/ntddcdvd.h"
#include "rosddk/ntddcdrm.h"
#include "rosddk/ntddscsi.h"
#pragma warning(default:4200)
#include <stddef.h>

#include "ReaderModules.h"
#include "SectorConverters.h"

#include <string>

#define RETURN(v) {OpenOK=v; return;}

s32 PlainIso::Reopen()
{
	if(fileSource)
	{
		delete fileSource;
		fileSource = NULL;
	}

	OpenOK = false;

	fileSource = new FileStream(fName);

	tocCached = false;
	mediaTypeCached = false;
	discSizeCached = false;
	layerBreakCached = false;

	OpenOK=true;
	return 0;
}

PlainIso::PlainIso(const char* fileName)
{
	fileSource=NULL;

	strcpy_s(fName,256,fileName);

	Reopen();
}

PlainIso::~PlainIso()
{
	if(fileSource && OpenOK)
	{
		delete fileSource;
	}
}

s32 PlainIso::GetSectorCount()
{
	int plain_sectors = 0;

	if(discSizeCached)
		return discSize;

	s64 length = fileSource->getLength();

	discSizeCached = true;
	discSize = (s32)(length / 2048);
	return discSize;
}

s32 PlainIso::GetLayerBreakAddress()
{
	return 0;
}

s32 PlainIso::GetMediaType()
{
	if(mediaTypeCached)
		return mediaType;

	// assume single layer DVD, for now
	mediaTypeCached = true;
	mediaType = 0;
	return mediaType;
}

#define MKDESCRIPTOR(n,ses,ctr,adr,pt,me,se,fe,tm,ts,tf) \
	ftd->Descriptors[n].SessionNumber	= ses; \
	ftd->Descriptors[n].Control			= ctr; \
	ftd->Descriptors[n].Adr				= adr; \
	ftd->Descriptors[n].Reserved1		= 0; \
	ftd->Descriptors[n].Point			= pt; \
	ftd->Descriptors[n].MsfExtra[0]		= me; \
	ftd->Descriptors[n].MsfExtra[1]		= se; \
	ftd->Descriptors[n].MsfExtra[2]		= fe; \
	ftd->Descriptors[n].Zero			= 0; \
	ftd->Descriptors[n].Msf[0]			= tm; \
	ftd->Descriptors[n].Msf[1]			= ts; \
	ftd->Descriptors[n].Msf[2]			= tf;

#define MSF_TO_LBA(m,s,f) ((m*60+s)*75+f-150)
#define LBA_TO_MSF(m,s,f,l) \
	m = (l)/(75*60); \
	s = (((l)/75)%60); \
	f = ((l)%75);


s32 PlainIso::ReadTOC(char *toc,int msize)
{
	DWORD size=0;

	if(GetMediaType()>=0)
		return -1;

	if(!tocCached)
	{
		CDROM_TOC_FULL_TOC_DATA *ftd=(CDROM_TOC_FULL_TOC_DATA*)tocCacheData;

		// Build basic TOC
		int length = 6 * sizeof(ftd->Descriptors[0]) + 2;
		ftd->Length[0] = length>>8;
		ftd->Length[1] = length;
		ftd->FirstCompleteSession=1;
		ftd->LastCompleteSession=2;

		int dm,ds,df;

		LBA_TO_MSF(dm,ds,df,sector_count);

		MKDESCRIPTOR(0,1,0x00,1,0xA0,0,0,0,1,0,0);
		MKDESCRIPTOR(1,1,0x00,1,0xA1,0,0,0,1,0,0);
		MKDESCRIPTOR(2,1,0x00,1,0xA2,0,0,0,dm,ds,df);
		MKDESCRIPTOR(3,1,0x00,5,0xB0,0,0,0,0,0,0);
		MKDESCRIPTOR(4,1,0x00,5,0xC0,0,0,0,0,0,0);
		MKDESCRIPTOR(5,1,0x04,1,0x01,0,0,0,0,2,0);

		tocCached = true;
	}

	memcpy(toc,tocCacheData,min(2048,msize));

	return 0;
}

s32 PlainIso::ReadSectors2048(u32 sector, u32 count, char *buffer)
{
	if(!OpenOK) return -1;

	fileSource->seek(sector*(u64)2048);

	s32 bytes = count * 2048;
	s32 size = fileSource->read((byte*)buffer,bytes);

	if(size!=bytes)
	{
		return -1;
	}

	return 0;
}


s32 PlainIso::ReadSectors2352(u32 sector, u32 count, char *buffer)
{
	if(!OpenOK) return -1;

	fileSource->seek(sector*(u64)2352);

	for(uint i=0;i<count;i++)
	{
		s32 size = fileSource->read((byte*)sectorbuffer,2352);

		if(size!=2352)
		{
			return -1;
		}

		Convert<2048,2352>(buffer+2048*i,sectorbuffer,sector+i);
	}

	return 0;
}

s32 PlainIso::DiscChanged()
{
	DWORD size=0;

	if(!OpenOK) return -1;

	return 0;
}

s32 PlainIso::IsOK()
{
	return OpenOK;
}

Reader* PlainIso::TryLoad(const char* fName)
{
	std::string fileName = fName;
	std::string::size_type pos = fileName.find_last_of('.');

	if(pos == std::string::npos) // no "." found, error.
		return NULL;

	std::string extension = fileName.substr(pos);

	if((extension.compare(".iso")!=0)&&
		(extension.compare(".ISO")!=0))	// assume valid
		return NULL;

	return new PlainIso(fName);
}