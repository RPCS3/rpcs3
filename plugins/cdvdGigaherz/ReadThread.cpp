#include "CDVD.h"

const s32 prefetch_max_blocks=16;
s32 prefetch_mode=0;
s32 prefetch_last_lba=0;
s32 prefetch_last_mode=0;
s32 prefetch_left=0;

HANDLE hNotify = INVALID_HANDLE_VALUE;
HANDLE hThread = INVALID_HANDLE_VALUE;
DWORD  pidThread= 0;

enum loadStatus
{
	LoadIdle,
	LoadPending,
	LoadSuccess,
};

typedef struct 
{
	int lsn;
	int mode;
	char data[2352*16]; //we will read in blocks of 16 sectors
} SectorInfo;

//bits: 8 would use 1<<8 entries, or 256*16 sectors
#define CACHE_SIZE 10

const s32 CacheSize=(1<<CACHE_SIZE);
SectorInfo Cache[CacheSize];

bool		threadRequestPending;
SectorInfo	threadRequestInfo;

u32  cdvdSectorHash(int lsn, int mode)
{
	u32 t = 0;

	int i=32;
	int m=CacheSize-1;

	while(i>=0)
	{
		t^=lsn&m;
		lsn>>=CACHE_SIZE;
		i-=CACHE_SIZE;
	}

	return (t^mode)&m;
}

void cdvdCacheUpdate(int lsn, int mode, char* data)
{
	u32 entry = cdvdSectorHash(lsn,mode);

	memcpy(Cache[entry].data,data,2352*16);
	Cache[entry].lsn = lsn;
	Cache[entry].mode = mode;
}

bool cdvdCacheFetch(int lsn, int mode, char* data)
{
	u32 entry = cdvdSectorHash(lsn,mode);

	if((Cache[entry].lsn==lsn) &&
		(Cache[entry].mode==mode))
	{
		memcpy(data,Cache[entry].data,2352*16);
		return true;
	}

	return false;
}

void cdvdCacheReset()
{
	for(int i=0;i<CacheSize;i++)
	{
		Cache[i].lsn=-1;
		Cache[i].mode=-1;
	}
}

void cdvdCallNewDiscCB()
{
	weAreInNewDiskCB=1;
	newDiscCB();
	weAreInNewDiskCB=0;
}

bool cdvdUpdateDiscStatus()
{
	int change = src->DiscChanged();

	if(change==-1) //error getting status (no disc in drive?)
	{
		//try to recreate the device
		src->Reopen();

		if(src->IsOK())
		{
			change = 1;
		}
		else
		{
			curDiskType=CDVD_TYPE_NODISC;
			curTrayStatus=CDVD_TRAY_OPEN;
			return true;
		}
	}
	
	if(change==1)
	{
		if(!disc_has_changed)
		{
			disc_has_changed=1;
			curDiskType=CDVD_TYPE_NODISC;
			curTrayStatus=CDVD_TRAY_OPEN;
			cdvdCallNewDiscCB();
		}
	}
	else
	{
		if(disc_has_changed)
		{
			curDiskType=CDVD_TYPE_NODISC;
			curTrayStatus=CDVD_TRAY_CLOSE;

			// just a test
			src->Reopen();

			disc_has_changed=0;
			cdvdRefreshData();
			cdvdCallNewDiscCB();
		}
	}
	return (change!=0);
}

DWORD CALLBACK cdvdThread(PVOID param)
{
	printf(" * CDVD: IO thread started...\n");
	while(cdvd_is_open)
	{
		DWORD f=0;

		if(!src) break;

		if(cdvdUpdateDiscStatus()) continue;

		if(prefetch_left)
			WaitForSingleObject(hNotify,1);
		else
			WaitForSingleObject(hNotify,250);

		if(cdvd_is_open)
		{
			static SectorInfo info;

			bool handlingRequest = false;

			if(threadRequestPending)
			{
				info=threadRequestInfo;
				handlingRequest = true;
			}
			else
			{
				info.lsn = prefetch_last_lba;
				info.mode = prefetch_last_mode;
			}

			if(threadRequestPending || prefetch_left)
			{
				s32 ret = -1;
				s32 tries=5;

				s32 count = 16;

				s32 left = tracks[0].length-info.lsn;

				if(left<count) count=left;

				do {
					if(info.mode==CDVD_MODE_2048)
						ret = src->ReadSectors2048(info.lsn,count,info.data);
					else
						ret = src->ReadSectors2352(info.lsn,count,info.data);

					if(ret==0)
						break;

					tries--;

				} while((ret<0)&&(tries>0));

				cdvdCacheUpdate(info.lsn,info.mode,info.data);

				if(handlingRequest)
				{
					threadRequestInfo = info;

					handlingRequest = false;
					threadRequestPending = false;

					prefetch_last_lba=info.lsn;
					prefetch_last_mode=info.mode;

					prefetch_left = prefetch_max_blocks;
				}
				else
				{
					prefetch_last_lba+=16;
					prefetch_left--;
				}
			}
		}
	}
	printf(" * CDVD: IO thread finished.\n");
	return 0;
}

s32 cdvdStartThread()
{

	hNotify = CreateEvent(NULL,FALSE,FALSE,NULL);
	if(hNotify==INVALID_HANDLE_VALUE)
		return -1;

	cdvd_is_open=true;
	hThread = CreateThread(NULL,0,cdvdThread,NULL,0,&pidThread);

	if(hThread==INVALID_HANDLE_VALUE)
		return -1;

	SetThreadPriority(hThread,THREAD_PRIORITY_NORMAL);

	cdvdCacheReset();

	return 0;
}

void cdvdStopThread()
{
	cdvd_is_open=false;
	PulseEvent(hNotify);
	if(WaitForSingleObject(hThread,4000)==WAIT_TIMEOUT)
	{
		TerminateThread(hThread,0);
	}
	CloseHandle(hThread);
	CloseHandle(hNotify);
}

s32 cdvdRequestSector(u32 sector, s32 mode)
{
	if(sector>=tracks[0].length)
		return -1;

	sector&=~15; //align to 16-sector block

	threadRequestInfo.lsn = sector;
	threadRequestInfo.mode = mode;
	threadRequestPending = false;
	if(cdvdCacheFetch(sector,mode,threadRequestInfo.data))
	{
		return 0;
	}

	threadRequestPending = true;

	PulseEvent(hNotify);

	return 0;
}

s32 cdvdRequestComplete()
{
	return !threadRequestPending;
}

s8* cdvdGetSector(s32 sector, s32 mode)
{
	while(threadRequestPending)
	{
		Sleep(1);
	}

	s32 offset;

	if(mode==CDVD_MODE_2048)
	{
		offset = 2048*(sector-threadRequestInfo.lsn);
		return threadRequestInfo.data + offset;
	}

	offset   = 2352*(sector-threadRequestInfo.lsn);
	s8* data = threadRequestInfo.data + offset;

	switch(mode)
	{
		case CDVD_MODE_2328:
			 return data + 24;
		case CDVD_MODE_2340:
			 return data + 12;
	}
	return data;
}

s32 cdvdDirectReadSector(s32 first, s32 mode, char *buffer)
{
	static char data[16*2352];

	if((u32)first>=tracks[0].length)
		return -1;

	s32 sector = first&(~15); //align to 16-sector block

	if(!cdvdCacheFetch(sector,mode,data))
	{
		s32 ret = -1;
		s32 tries=5;

		s32 count = 16;

		s32 left = tracks[0].length-sector;

		if(left<count) count=left;

		do {
			if(mode==CDVD_MODE_2048)
				ret = src->ReadSectors2048(sector,count,data);
			else
				ret = src->ReadSectors2352(sector,count,data);

			if(ret==0)
				break;

			tries--;

		} while((ret<0)&&(tries>0));

		cdvdCacheUpdate(sector,mode,data);
	}

	s32 offset;

	if(mode==CDVD_MODE_2048)
	{
		offset = 2048*(first-sector);
		memcpy(buffer,data + offset,2048); 
		return 0;
	}
	
	offset  = 2352*(first-sector);
	s8* bfr = data + offset;

	switch(mode)
	{
		case CDVD_MODE_2328:
			memcpy(buffer,bfr+24,2328); 
			return 0;
		case CDVD_MODE_2340:
			memcpy(buffer,bfr+12,2340); 
			return 0;
		default:
			memcpy(buffer,bfr+12,2352); 
			return 0;
	}
	return 0;
}

s32 cdvdGetMediaType()
{
	return src->GetMediaType();
}

s32 refreshes=0;

s32 cdvdRefreshData()
{
	char *diskTypeName="Unknown";

	//read TOC from device
	cdvdParseTOC();

	if((etrack==0)||(strack>etrack))
	{
		curDiskType=CDVD_TYPE_NODISC;
	}
	else
	{
		curDiskType = FindDiskType();
	}
	
	curTrayStatus = CDVD_TRAY_CLOSE;

	switch(curDiskType)
	{
		case CDVD_TYPE_ILLEGAL:   diskTypeName="Illegal Disc"; break;
		case CDVD_TYPE_DVDV:      diskTypeName="DVD-Video"; break;
		case CDVD_TYPE_CDDA:      diskTypeName="CD-Audio"; break;
		case CDVD_TYPE_PS2DVD:    diskTypeName="PS2 DVD"; break;
		case CDVD_TYPE_PS2CDDA:   diskTypeName="PS2 CD+Audio"; break;
		case CDVD_TYPE_PS2CD:     diskTypeName="PS2 CD"; break;
		case CDVD_TYPE_PSCDDA:    diskTypeName="PS1 CD+Audio"; break;
		case CDVD_TYPE_PSCD:      diskTypeName="PS1 CD"; break;
		case CDVD_TYPE_UNKNOWN:   diskTypeName="Unknown"; break;
		case CDVD_TYPE_DETCTDVDD: diskTypeName="Detecting (Single-Layer DVD)"; break;
		case CDVD_TYPE_DETCTDVDS: diskTypeName="Detecting (Double-Layer DVD)"; break;
		case CDVD_TYPE_DETCTCD:   diskTypeName="Detecting (CD)"; break;
		case CDVD_TYPE_DETCT:     diskTypeName="Detecting"; break;
		case CDVD_TYPE_NODISC:    diskTypeName="No Disc"; break;
	}

	printf(" * CDVD: Disk Type: %s\n",diskTypeName);

	cdvdCacheReset();

	return 0;
}
