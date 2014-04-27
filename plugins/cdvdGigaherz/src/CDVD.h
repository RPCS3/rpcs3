/*  PCSX2 - PS2 Emulator for PCs
 *  Copyright (C) 2002-2014 David Quintana [gigaherz]
 *
 *  PCSX2 is free software: you can redistribute it and/or modify it under the terms
 *  of the GNU Lesser General Public License as published by the Free Software Found-
 *  ation, either version 3 of the License, or (at your option) any later version.
 *
 *  PCSX2 is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 *  without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 *  PURPOSE.  See the GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along with PCSX2.
 *  If not, see <http://www.gnu.org/licenses/>.
 */

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
	Source(Source&);
public:
	Source(){};

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
	IOCtlSrc(IOCtlSrc&);

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

extern Source *src;

Source* TryLoaders(const char* fileName);

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
void CfgSetSettingsDir( const char* dir );

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
