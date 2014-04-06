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