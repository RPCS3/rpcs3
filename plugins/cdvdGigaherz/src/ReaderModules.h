#pragma once

#include "FileStream.h"

class Reader: public Source //abstract class as base for Reader modules
{
	Reader(Reader&);
public:
	Reader(){};

	//virtual destructor
	virtual ~Reader() {}

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

	//added members
	static Reader* TryLoad(const char* fileName);
};

class PlainIso: public Reader
{
	FileStream* fileSource;

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

	PlainIso(PlainIso&);
public:

	PlainIso(const char* fileName);
	virtual ~PlainIso();

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

	static Reader* TryLoad(const char* fileName);
};