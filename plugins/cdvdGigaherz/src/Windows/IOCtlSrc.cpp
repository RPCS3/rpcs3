#undef _WIN32_WINNT
#define _WIN32_WINNT 0x0600
#include "../CDVD.h"
#pragma warning(disable:4200)
#pragma pack(1)
#include <winioctl.h>
#include "rosddk/ntddcdvd.h"
#include "rosddk/ntddcdrm.h"
#include "rosddk/ntddscsi.h"
#pragma warning(default:4200)
#include <stddef.h>

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

#if FALSE
typedef struct {
  SCSI_PASS_THROUGH_DIRECT spt;
  ULONG Filler;
  UCHAR ucSenseBuf[32];
} SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER, *PSCSI_PASS_THROUGH_DIRECT_WITH_BUFFER;

SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER sptIOCTL;         // global read bufs

char szBuf[1024];

PSCSI_ADDRESS pSA;

DWORD SendIoCtlScsiCommand(HANDLE hDevice, u8 direction,
						   u8  cdbLength,  u8*   cdb,
						   u32 dataLength, void* dataBuffer
						   )
{
	DWORD dwRet;

	memset(szBuf,0,1024);

	pSA=(PSCSI_ADDRESS)szBuf;
	pSA->Length=sizeof(SCSI_ADDRESS);

	if(!DeviceIoControl(hDevice,IOCTL_SCSI_GET_ADDRESS,NULL,
						0,pSA,sizeof(SCSI_ADDRESS),
						&dwRet,NULL))
	{
		return -1;
	}

	memset(&sptIOCTL,0,sizeof(sptIOCTL));

	sptIOCTL.spt.Length             = sizeof(SCSI_PASS_THROUGH_DIRECT);
	sptIOCTL.spt.TimeOutValue       = 60;
	sptIOCTL.spt.SenseInfoLength    = 14;
	sptIOCTL.spt.SenseInfoOffset    = offsetof(SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER, ucSenseBuf);

	sptIOCTL.spt.PathId				= pSA->PortNumber;
	sptIOCTL.spt.TargetId           = pSA->TargetId;
	sptIOCTL.spt.Lun                = pSA->Lun;

	sptIOCTL.spt.DataIn             = (direction>0)?SCSI_IOCTL_DATA_IN:((direction<0)?SCSI_IOCTL_DATA_OUT:SCSI_IOCTL_DATA_UNSPECIFIED);
	sptIOCTL.spt.DataTransferLength = dataLength;
	sptIOCTL.spt.DataBuffer         = dataBuffer;

	sptIOCTL.spt.CdbLength          = cdbLength;
	memcpy(sptIOCTL.spt.Cdb,cdb,cdbLength);

	DWORD code = DeviceIoControl(hDevice,
					IOCTL_SCSI_PASS_THROUGH_DIRECT,
					&sptIOCTL,sizeof(sptIOCTL),
					&sptIOCTL,sizeof(sptIOCTL),
					&dwRet,NULL);
	ApiErrorCheck<DWORD>(code,0,false);
	return code;
}

//0x00 = PHYSICAL STRUCTURE
DWORD ScsiReadStructure(HANDLE hDevice, u32 layer, u32 format, u32 buffer_length, void* buffer)
{
	u8 cdb[12]={0};


	cdb[0]     = 0xAD;
	/*
	cdb[2]     = (unsigned char)((addr >> 24) & 0xFF);
	cdb[3]     = (unsigned char)((addr >> 16) & 0xFF);
	cdb[4]     = (unsigned char)((addr >> 8) & 0xFF);
	cdb[5]     = (unsigned char)(addr & 0xFF);
	*/
	cdb[6]     = layer;
	cdb[7]     = format;
	cdb[8]     = (unsigned char)((buffer_length>>8) & 0xFF);
	cdb[9]     = (unsigned char)((buffer_length) & 0xFF);

	return SendIoCtlScsiCommand(hDevice, 1, 12, cdb, buffer_length, buffer);
}


DWORD ScsiReadBE_2(HANDLE hDevice, u32 addr, u32 count, u8* buffer)
{
	u8 cdb[12]={0};

	cdb[0]     = 0xBE;
	cdb[2]     = (unsigned char)((addr >> 24) & 0xFF);
	cdb[3]     = (unsigned char)((addr >> 16) & 0xFF);
	cdb[4]     = (unsigned char)((addr >> 8) & 0xFF);
	cdb[5]     = (unsigned char)(addr & 0xFF);
	cdb[8]     = count;
	cdb[9]     = 0xF8;
	cdb[10]    = 0x2;

	return SendIoCtlScsiCommand(hDevice, 1, 12, cdb, 2352, buffer);
}

DWORD ScsiRead10(HANDLE hDevice, u32 addr, u32 count, u8* buffer)
{
	u8 cdb[10]={0};

	cdb[0]     = 0x28;
	//cdb[1]     = lun<<5;
	cdb[2]     = (unsigned char)((addr >> 24) & 0xFF);
	cdb[3]     = (unsigned char)((addr >> 16) & 0xFF);
	cdb[4]     = (unsigned char)((addr >> 8) & 0xFF);
	cdb[5]     = (unsigned char)(addr & 0xFF);
	cdb[8]     = count;

	return SendIoCtlScsiCommand(hDevice, 1, 10, cdb, 2352, buffer);
}

DWORD ScsiReadTOC(HANDLE hDevice, u32 addr, u8* buffer, bool use_msf)
{
	u8 cdb[12]={0};

	cdb[0]     = 0x43;
	cdb[7]     = 0x03;
	cdb[8]     = 0x24;

	if(use_msf) cdb[1]=2;

	return SendIoCtlScsiCommand(hDevice, 1, 10, cdb, 2352, buffer);
}

s32 IOCtlSrc::GetSectorCount()
{
	DWORD size;

	LARGE_INTEGER li;
	int plain_sectors = 0;

	if(GetFileSizeEx(device,&li))
	{
		return li.QuadPart / 2048;
	}

	GET_LENGTH_INFORMATION info;
	if(DeviceIoControl(device, IOCTL_DISK_GET_LENGTH_INFO, NULL, 0, &info, sizeof(info), &size, NULL))
	{
		return info.Length.QuadPart / 2048;
	}

	CDROM_READ_TOC_EX tocrq={0};

	tocrq.Format = CDROM_READ_TOC_EX_FORMAT_FULL_TOC;
	tocrq.Msf=1;
	tocrq.SessionTrack=1;

	CDROM_TOC_FULL_TOC_DATA *ftd=(CDROM_TOC_FULL_TOC_DATA*)sectorbuffer;

	if(DeviceIoControl(device,IOCTL_CDROM_READ_TOC_EX,&tocrq,sizeof(tocrq),ftd, 2048, &size, NULL))
	{
		for(int i=0;i<101;i++)
		{
			if(ftd->Descriptors[i].Point==0xa2)
			{
				if(ftd->Descriptors[i].SessionNumber==ftd->LastCompleteSession)
				{
					int min=ftd->Descriptors[i].Msf[0];
					int sec=ftd->Descriptors[i].Msf[1];
					int frm=ftd->Descriptors[i].Msf[2];

					return MSF_TO_LBA(min,sec,frm);
				}
			}
		}
	}

	int sectors1=-1;

	if(ScsiReadStructure(device,0,DvdPhysicalDescriptor, sizeof(dld), &dld)!=0)
	{
		if(dld.ld.EndLayerZeroSector>0) // OTP?
		{
			sectors1 = dld.ld.EndLayerZeroSector - dld.ld.StartingDataSector;
		}
		else //PTP or single layer
		{
			sectors1 = dld.ld.EndDataSector - dld.ld.StartingDataSector;
		}

		if(ScsiReadStructure(device,1,DvdPhysicalDescriptor, sizeof(dld), &dld)!=0)
		{
			// PTP second layer
			//sectors1 += dld.ld.EndDataSector - dld.ld.StartingDataSector;
			if(dld.ld.EndLayerZeroSector>0) // OTP?
			{
				sectors1 += dld.ld.EndLayerZeroSector - dld.ld.StartingDataSector;
			}
			else //PTP
			{
				sectors1 += dld.ld.EndDataSector - dld.ld.StartingDataSector;
			}
		}

		return sectors1;
	}

	return -1;
}

s32 IOCtlSrc::GetLayerBreakAddress()
{
	DWORD size;
	if(ScsiReadStructure(device,0,DvdPhysicalDescriptor, sizeof(dld), &dld)!=0)
	{
		if(dld.ld.EndLayerZeroSector>0) // OTP?
		{
			return dld.ld.EndLayerZeroSector - dld.ld.StartingDataSector;
		}
		else //PTP or single layer
		{
			u32 s1 = dld.ld.EndDataSector - dld.ld.StartingDataSector;

			if(ScsiReadStructure(device,1,DvdPhysicalDescriptor, sizeof(dld), &dld)!=0)
			{
				//PTP
				return s1;
			}

			// single layer
			return 0;
		}
	}

	return -1;
}
#endif

#define RETURN(v) {OpenOK=v; return;}

s32 IOCtlSrc::Reopen()
{
	if(device!=INVALID_HANDLE_VALUE)
	{
		DWORD size;
		DeviceIoControl(device,IOCTL_DVD_END_SESSION,&sessID,sizeof(DVD_SESSION_ID),NULL,0,&size, NULL);
		CloseHandle(device);
	}

	DWORD share = FILE_SHARE_READ;
	DWORD flags = FILE_ATTRIBUTE_READONLY | FILE_FLAG_SEQUENTIAL_SCAN;
	DWORD size;

	OpenOK = false;

	device = CreateFile(fName, GENERIC_READ|GENERIC_WRITE|FILE_READ_ATTRIBUTES, share, NULL, OPEN_EXISTING, flags, 0);
	if(device==INVALID_HANDLE_VALUE)
	{
		device = CreateFile(fName, GENERIC_READ|FILE_READ_ATTRIBUTES, share, NULL, OPEN_EXISTING, flags, 0);
		if(device==INVALID_HANDLE_VALUE)
			return -1;
	}

	sessID=0;
	DeviceIoControl(device,IOCTL_DVD_START_SESSION,NULL,0,&sessID,sizeof(DVD_SESSION_ID), &size, NULL);

	tocCached = false;
	mediaTypeCached = false;
	discSizeCached = false;
	layerBreakCached = false;

	OpenOK=true;
	return 0;
}

IOCtlSrc::IOCtlSrc(const char* fileName)
{
	device=INVALID_HANDLE_VALUE;

	strcpy_s(fName,256,fileName);

	Reopen();
}

IOCtlSrc::~IOCtlSrc()
{
	if(OpenOK)
	{
		DWORD size;
		DeviceIoControl(device,IOCTL_DVD_END_SESSION,&sessID,sizeof(DVD_SESSION_ID),NULL,0,&size, NULL);

		CloseHandle(device);
	}
}

struct mycrap
{
	DWORD shit;
	DVD_LAYER_DESCRIPTOR ld;
};

DVD_READ_STRUCTURE dvdrs;
mycrap dld;
DISK_GEOMETRY dg;
CDROM_READ_TOC_EX tocrq={0};

s32 IOCtlSrc::GetSectorCount()
{
	DWORD size;

	LARGE_INTEGER li;
	int plain_sectors = 0;

	if(discSizeCached)
		return discSize;

	if(GetFileSizeEx(device,&li))
	{
		discSizeCached = true;
		discSize = (s32)(li.QuadPart / 2048);
		return discSize;
	}

	GET_LENGTH_INFORMATION info;
	if(DeviceIoControl(device, IOCTL_DISK_GET_LENGTH_INFO, NULL, 0, &info, sizeof(info), &size, NULL))
	{
		discSizeCached = true;
		discSize = (s32)(info.Length.QuadPart / 2048);
		return discSize;
	}

	memset(&tocrq,0,sizeof(CDROM_READ_TOC_EX));
	tocrq.Format = CDROM_READ_TOC_EX_FORMAT_FULL_TOC;
	tocrq.Msf=1;
	tocrq.SessionTrack=1;

	CDROM_TOC_FULL_TOC_DATA *ftd=(CDROM_TOC_FULL_TOC_DATA*)sectorbuffer;

	if(DeviceIoControl(device,IOCTL_CDROM_READ_TOC_EX,&tocrq,sizeof(tocrq),ftd, 2048, &size, NULL))
	{
		for(int i=0;i<101;i++)
		{
			if(ftd->Descriptors[i].Point==0xa2)
			{
				if(ftd->Descriptors[i].SessionNumber==ftd->LastCompleteSession)
				{
					int min=ftd->Descriptors[i].Msf[0];
					int sec=ftd->Descriptors[i].Msf[1];
					int frm=ftd->Descriptors[i].Msf[2];

					discSizeCached = true;
					discSize = (s32)MSF_TO_LBA(min,sec,frm);
					return discSize;
				}
			}
		}
	}

	int sectors1=-1;

	dvdrs.BlockByteOffset.QuadPart=0;
	dvdrs.Format=DvdPhysicalDescriptor;
	dvdrs.SessionId=sessID;
	dvdrs.LayerNumber=0;
	if(DeviceIoControl(device,IOCTL_DVD_READ_STRUCTURE,&dvdrs,sizeof(dvdrs),&dld, sizeof(dld), &size, NULL)!=0)
	{
		if(dld.ld.EndLayerZeroSector>0) // OTP?
		{
			sectors1 = dld.ld.EndLayerZeroSector - dld.ld.StartingDataSector;
		}
		else //PTP or single layer
		{
			sectors1 = dld.ld.EndDataSector - dld.ld.StartingDataSector;
		}
		dvdrs.BlockByteOffset.QuadPart=0;
		dvdrs.Format=DvdPhysicalDescriptor;
		dvdrs.SessionId=sessID;
		dvdrs.LayerNumber=1;
		if(DeviceIoControl(device,IOCTL_DVD_READ_STRUCTURE,&dvdrs,sizeof(dvdrs),&dld, sizeof(dld), &size, NULL)!=0)
		{
			// PTP second layer
			//sectors1 += dld.ld.EndDataSector - dld.ld.StartingDataSector;
			if(dld.ld.EndLayerZeroSector>0) // OTP?
			{
				sectors1 += dld.ld.EndLayerZeroSector - dld.ld.StartingDataSector;
			}
			else //PTP
			{
				sectors1 += dld.ld.EndDataSector - dld.ld.StartingDataSector;
			}
		}

		discSizeCached = true;
		discSize = sectors1;
		return discSize;
	}

	return -1;
}

s32 IOCtlSrc::GetLayerBreakAddress()
{
	DWORD size;
	DWORD code;

	if(GetMediaType()<0)
		return -1;

	if(layerBreakCached)
		return layerBreak;

	dvdrs.BlockByteOffset.QuadPart=0;
	dvdrs.Format=DvdPhysicalDescriptor;
	dvdrs.SessionId=sessID;
	dvdrs.LayerNumber=0;
	if(code=DeviceIoControl(device,IOCTL_DVD_READ_STRUCTURE,&dvdrs,sizeof(dvdrs),&dld, sizeof(dld), &size, NULL)!=0)
	{
		if(dld.ld.EndLayerZeroSector>0) // OTP?
		{
			layerBreakCached = true;
			layerBreak = dld.ld.EndLayerZeroSector - dld.ld.StartingDataSector;
			return layerBreak;
		}
		else //PTP or single layer
		{
			u32 s1 = dld.ld.EndDataSector - dld.ld.StartingDataSector;

			dvdrs.BlockByteOffset.QuadPart=0;
			dvdrs.Format=DvdPhysicalDescriptor;
			dvdrs.SessionId=sessID;
			dvdrs.LayerNumber=1;

			if(DeviceIoControl(device,IOCTL_DVD_READ_STRUCTURE,&dvdrs,sizeof(dvdrs),&dld, sizeof(dld), &size, NULL)!=0)
			{
				//PTP
				layerBreakCached = true;
				layerBreak = s1;
				return layerBreak;
			}

			// single layer
			layerBreakCached = true;
			layerBreak = 0;
			return layerBreak;
		}
	}

	//if not a cd, and fails, assume single layer
	return 0;
}

s32 IOCtlSrc::GetMediaType()
{
	DWORD size;
	DWORD code;

	if(mediaTypeCached)
		return mediaType;

	memset(&tocrq,0,sizeof(CDROM_READ_TOC_EX));
	tocrq.Format = CDROM_READ_TOC_EX_FORMAT_FULL_TOC;
	tocrq.Msf=1;
	tocrq.SessionTrack=1;

	CDROM_TOC_FULL_TOC_DATA *ftd=(CDROM_TOC_FULL_TOC_DATA*)sectorbuffer;

	if(DeviceIoControl(device,IOCTL_CDROM_READ_TOC_EX,&tocrq,sizeof(tocrq),ftd, 2048, &size, NULL))
	{
		mediaTypeCached = true;
		mediaType = -1;
		return mediaType;
	}

	dvdrs.BlockByteOffset.QuadPart=0;
	dvdrs.Format=DvdPhysicalDescriptor;
	dvdrs.SessionId=sessID;
	dvdrs.LayerNumber=0;
	if(code=DeviceIoControl(device,IOCTL_DVD_READ_STRUCTURE,&dvdrs,sizeof(dvdrs),&dld, sizeof(dld), &size, NULL)!=0)
	{
		if(dld.ld.EndLayerZeroSector>0) // OTP?
		{
			mediaTypeCached = true;
			mediaType = 2;
			return mediaType;
		}
		else //PTP or single layer
		{
			u32 s1 = dld.ld.EndDataSector - dld.ld.StartingDataSector;

			dvdrs.BlockByteOffset.QuadPart=0;
			dvdrs.Format=DvdPhysicalDescriptor;
			dvdrs.SessionId=sessID;
			dvdrs.LayerNumber=1;

			if(DeviceIoControl(device,IOCTL_DVD_READ_STRUCTURE,&dvdrs,sizeof(dvdrs),&dld, sizeof(dld), &size, NULL)!=0)
			{
				//PTP
				mediaTypeCached = true;
				mediaType = 2;
				return mediaType;
			}

			// single layer
			mediaTypeCached = true;
			mediaType = 0;
			return mediaType;
		}
	}

	//if not a cd, and fails, assume single layer
	mediaTypeCached = true;
	mediaType = 0;
	return mediaType;
}

s32 IOCtlSrc::ReadTOC(char *toc,int msize)
{
	DWORD size=0;

	if(GetMediaType()>=0)
		return -1;

	if(!tocCached)
	{
		memset(&tocrq,0,sizeof(CDROM_READ_TOC_EX));
		tocrq.Format = CDROM_READ_TOC_EX_FORMAT_FULL_TOC;
		tocrq.Msf=1;
		tocrq.SessionTrack=1;

		CDROM_TOC_FULL_TOC_DATA *ftd=(CDROM_TOC_FULL_TOC_DATA*)tocCacheData;

		if(!OpenOK) return -1;

		int code = DeviceIoControl(device,IOCTL_CDROM_READ_TOC_EX,&tocrq,sizeof(tocrq),tocCacheData, 2048, &size, NULL);

		if(code==0)
			return -1;

		tocCached = true;
	}

	memcpy(toc,tocCacheData,min(2048,msize));

	return 0;
}

s32 IOCtlSrc::ReadSectors2048(u32 sector, u32 count, char *buffer)
{
	RAW_READ_INFO rri;

	DWORD size=0;

	if(!OpenOK) return -1;

	rri.DiskOffset.QuadPart=sector*(u64)2048;
	rri.SectorCount=count;

	//fall back to standard reading
	if(SetFilePointer(device,rri.DiskOffset.LowPart,&rri.DiskOffset.HighPart,FILE_BEGIN)==-1)
	{
		if(GetLastError()!=0)
			return -1;
	}

	if(ReadFile(device,buffer,2048*count,&size,NULL)==0)
	{
		return -1;
	}

	if(size!=(2048*count))
	{
		return -1;
	}

	return 0;
}


s32 IOCtlSrc::ReadSectors2352(u32 sector, u32 count, char *buffer)
{
	RAW_READ_INFO rri;

	DWORD size=0;

	if(!OpenOK) return -1;

	rri.DiskOffset.QuadPart=sector*(u64)2048;
	rri.SectorCount=count;

	rri.TrackMode=(TRACK_MODE_TYPE)last_read_mode;
	if(DeviceIoControl(device,IOCTL_CDROM_RAW_READ,&rri,sizeof(rri),buffer, 2352*count, &size, NULL)==0)
	{
		rri.TrackMode=CDDA;
		if(DeviceIoControl(device,IOCTL_CDROM_RAW_READ,&rri,sizeof(rri),buffer, 2352*count, &size, NULL)==0)
		{
			rri.TrackMode=XAForm2;
			if(DeviceIoControl(device,IOCTL_CDROM_RAW_READ,&rri,sizeof(rri),buffer, 2352*count, &size, NULL)==0)
			{
				rri.TrackMode=YellowMode2;
				if(DeviceIoControl(device,IOCTL_CDROM_RAW_READ,&rri,sizeof(rri),buffer, 2352*count, &size, NULL)==0)
				{
					return -1;
				}
			}
		}
	}

	last_read_mode=rri.TrackMode;

	if(size!=(2352*count))
	{
		return -1;
	}

	return 0;
}

s32 IOCtlSrc::DiscChanged()
{
	DWORD size=0;

	if(!OpenOK) return -1;

	int ret = DeviceIoControl(device,IOCTL_STORAGE_CHECK_VERIFY,NULL,0,NULL,0, &size, NULL);

	if(ret==0)
	{
		tocCached = false;
		mediaTypeCached = false;
		discSizeCached = false;
		layerBreakCached = false;

		if(sessID!=0)
		{
			DeviceIoControl(device,IOCTL_DVD_END_SESSION,&sessID,sizeof(DVD_SESSION_ID),NULL,0,&size, NULL);
			sessID=0;
		}
		return 1;
	}

	if(sessID==0)
	{
		DeviceIoControl(device,IOCTL_DVD_START_SESSION,NULL,0,&sessID,sizeof(DVD_SESSION_ID), &size, NULL);
	}

	return 0;
}

s32 IOCtlSrc::IsOK()
{
	return OpenOK;
}
