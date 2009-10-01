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

#include "SectorConverters.h"
template<class T>
bool ApiErrorCheck(T t,T okValue,bool cmpEq)
{
	bool cond=(t==okValue);

	if(!cmpEq) cond=!cond;

	if(!cond)
	{
		static char buff[1024];
		DWORD ec = GetLastError();
		FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM,NULL,ec,0,buff,1023,NULL);
		MessageBoxEx(0,buff,"ERROR?!",MB_OK,0);
	}
	return cond;
}

#define RETURN(v) {OpenOK=v; return;}

s32 FileSrc::Reopen()
{
	if(fileSource!=INVALID_HANDLE_VALUE)
	{
		CloseHandle(fileSource);
	}

	DWORD share = FILE_SHARE_READ;
	DWORD flags = FILE_ATTRIBUTE_READONLY | FILE_FLAG_SEQUENTIAL_SCAN;

	OpenOK = false;

	fileSource = CreateFile(fName, GENERIC_READ|FILE_READ_ATTRIBUTES, share, NULL, OPEN_EXISTING, flags, 0);

	tocCached = false;
	mediaTypeCached = false;
	discSizeCached = false;
	layerBreakCached = false;

	OpenOK=true;
	return 0;
}

FileSrc::FileSrc(const char* fileName)
{
	fileSource=INVALID_HANDLE_VALUE;

	strcpy_s(fName,256,fileName);

	Reopen();
}

FileSrc::~FileSrc()
{
	if(OpenOK)
	{
		CloseHandle(fileSource);
	}
}

s32 FileSrc::GetSectorCount()
{
	LARGE_INTEGER li;
	int plain_sectors = 0;

	if(discSizeCached)
		return discSize;

	if(GetFileSizeEx(fileSource,&li))
	{
		discSizeCached = true;
		discSize = (s32)(li.QuadPart / 2048);
		return discSize;
	}

	return -1;
}

s32 FileSrc::GetLayerBreakAddress()
{
	return 0;
}

s32 FileSrc::GetMediaType()
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


s32 FileSrc::ReadTOC(char *toc,int msize)
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

s32 FileSrc::ReadSectors2048(u32 sector, u32 count, char *buffer)
{
	DWORD size=0;
	LARGE_INTEGER Offset;

	if(!OpenOK) return -1;
	
	Offset.QuadPart=sector*(u64)2048;

	//fall back to standard reading
	if(SetFilePointer(fileSource,Offset.LowPart,&Offset.HighPart,FILE_BEGIN)==-1)
	{
		if(GetLastError()!=0)
			return -1;
	}

	if(ReadFile(fileSource,buffer,2048*count,&size,NULL)==0)
	{
		return -1;
	}

	if(size!=(2048*count))
	{
		return -1;
	}

	return 0;
}


s32 FileSrc::ReadSectors2352(u32 sector, u32 count, char *buffer)
{
	DWORD size=0;
	LARGE_INTEGER Offset;

	if(!OpenOK) return -1;

	assert(count <= 32);

	Offset.QuadPart=sector*(u64)2048;

	//fall back to standard reading
	if(SetFilePointer(fileSource,Offset.LowPart,&Offset.HighPart,FILE_BEGIN)==-1)
	{
		if(GetLastError()!=0)
			return -1;
	}

	if(ReadFile(fileSource,sectorbuffer,2048*count,&size,NULL)==0)
	{
		return -1;
	}

	if(size!=(2048*count))
	{
		return -1;
	}

	for(uint i=0;i<count;i++)
	{
		Convert<2048,2352>(buffer+2352*i,sectorbuffer+2048*i,sector+i);
	}

	return 0;
}

s32 FileSrc::DiscChanged()
{
	DWORD size=0;

	if(!OpenOK) return -1;

	return 0;
}

s32 FileSrc::IsOK()
{
	return OpenOK;
}
