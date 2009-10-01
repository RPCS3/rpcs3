#ifndef __CDVD_H__
#define __CDVD_H__

#include <windows.h>
#include <stdio.h>

#define CDVDdefs
#include <PS2Edefs.h>

typedef struct _track
{
	u32 start_lba;
	u32 length;
	u32 type;
} track;

extern int strack;
extern int etrack;
extern track tracks[100];

extern int curDiskType;
extern int curTrayStatus;

typedef struct _toc_entry {
    UCHAR SessionNumber;
    UCHAR Control      : 4;
    UCHAR Adr          : 4;
    UCHAR Reserved1;
    UCHAR Point;
    UCHAR MsfExtra[3];
    UCHAR Zero;
    UCHAR Msf[3];
} toc_entry;

typedef struct _toc_data 
{
    UCHAR Length[2];
    UCHAR FirstCompleteSession;
    UCHAR LastCompleteSession;

    toc_entry Descriptors[255];
} toc_data;

extern toc_data cdtoc;


class Source //abstract class as base for source modules
{
public:
	//virtual destructor
	virtual ~Source()
	{
	}

	//virtual members
	virtual s32 GetSectorCount()=0;
	virtual s32 ReadTOC(char *toc,int size)=0;
	virtual s32 ReadSectors2048(u32 sector, u32 count, char *buffer)=0;
	virtual s32 ReadSectors2352(u32 sector, u32 count, char *buffer)=0;
	virtual s32 GetLayerBreakAddress()=0;

	virtual s32 GetMediaType()=0;

	virtual s32 IsOK()=0;
	virtual s32 Reopen()=0;

	virtual s32 DiscChanged()=0;
};

class IOCtlSrc: public Source
{
private:
	HANDLE device;

	bool OpenOK;

	s32 last_read_mode;

	s32 last_sector_count;

	char sectorbuffer[32*2048];

	char fName[256];

	DWORD sessID;

	bool tocCached;
	char tocCacheData[2048];

	bool mediaTypeCached;
	int  mediaType;

	bool discSizeCached;
	s32  discSize;

	bool layerBreakCached;
	s32  layerBreak;

public:
	IOCtlSrc(const char* fileName);

	//virtual destructor
	virtual ~IOCtlSrc();

	//virtual members
	virtual s32 GetSectorCount();
	virtual s32 ReadTOC(char *toc,int size);
	virtual s32 ReadSectors2048(u32 sector, u32 count, char *buffer);
	virtual s32 ReadSectors2352(u32 sector, u32 count, char *buffer);
	virtual s32 GetLayerBreakAddress();

	virtual s32 GetMediaType();

	virtual s32 IsOK();
	virtual s32 Reopen();

	virtual s32 DiscChanged();
};

class FileSrc: public Source
{
private:
	HANDLE fileSource;

	bool OpenOK;

	s32 sector_count;

	char sectorbuffer[32*2048];

	char fName[256];

	DWORD sessID;

	bool tocCached;
	char tocCacheData[2048];

	bool mediaTypeCached;
	int  mediaType;

	bool discSizeCached;
	s32  discSize;

	bool layerBreakCached;
	s32  layerBreak;

public:
	FileSrc(const char* fileName);

	//virtual destructor
	virtual ~FileSrc();

	//virtual members
	virtual s32 GetSectorCount();
	virtual s32 ReadTOC(char *toc,int size);
	virtual s32 ReadSectors2048(u32 sector, u32 count, char *buffer);
	virtual s32 ReadSectors2352(u32 sector, u32 count, char *buffer);
	virtual s32 GetLayerBreakAddress();

	virtual s32 GetMediaType();

	virtual s32 IsOK();
	virtual s32 Reopen();

	virtual s32 DiscChanged();
};

extern Source *src;

int FindDiskType();

void configure();

extern char source_drive;
extern char source_file[];

extern HINSTANCE hinst;

#define MSF_TO_LBA(m,s,f) ((m*60+s)*75+f-150)

s32 cdvdDirectReadSector(s32 first, s32 mode, char *buffer);

s32 cdvdGetMediaType();

void ReadSettings();
void WriteSettings();

extern char csrc[];
extern BOOL cdvd_is_open;
extern int weAreInNewDiskCB;

extern void (*newDiscCB)();

extern s32 disc_has_changed;

s32 cdvdStartThread();
void cdvdStopThread();
s32 cdvdRequestSector(u32 sector, s32 mode);
s32 cdvdRequestComplete();
char* cdvdGetSector(s32 sector, s32 mode);
s32 cdvdDirectReadSector(s32 first, s32 mode, char *buffer);
s32 cdvdGetMediaType();
s32 cdvdRefreshData();
s32 cdvdParseTOC();

#endif /* __CDVD_H__ */
