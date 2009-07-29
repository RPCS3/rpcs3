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
#include "IopCommon.h"
#include "IsoFStools.h"
#include "IsoFSdrv.h"
#include "IsoFileFormats.h"

#include <stdio.h>
#include <fcntl.h>
#include <errno.h>

int detect(isoFile *iso)
{
	u8 buf[2448];
	struct cdVolDesc *volDesc;

	if (isoReadBlock(iso, buf + iso->blockofs, 16) == -1) return -1;
	
	volDesc = (struct cdVolDesc *)(buf + 24);
	
	if (strncmp((char*)volDesc->volID, "CD001", 5)) return 0;

	if (volDesc->rootToc.tocSize == 2048)
		iso->type = ISOTYPE_CD;
	else
		iso->type = ISOTYPE_DVD;

	return 1;
}

int _isoReadDtable(isoFile *iso)
{
	int ret;

	_seekfile(iso->handle, 0, SEEK_END);
	iso->dtablesize = (_tellfile(iso->handle) - 16) / (iso->blocksize + 4);
	iso->dtable = (u32*)malloc(iso->dtablesize * 4);

	for (int i = 0; i < iso->dtablesize; i++)
	{
		_seekfile(iso->handle, 16 + (iso->blocksize + 4) * i, SEEK_SET);
		ret = _readfile(iso->handle, &iso->dtable[i], 4);
		if (ret < 4) return -1;
	}

	return 0;
}

bool tryIsoType(isoFile *iso, u32 size, u32 offset, u32 blockofs)
{
	iso->blocksize = size;
	iso->offset = offset;
	iso->blockofs = blockofs;
	if (detect(iso) == 1) return true;
	
	return false;
}

int isoDetect(isoFile *iso)   // based on florin's CDVDbin detection code :)
{
	char buf[32];
	int len;

	iso->type = ISOTYPE_ILLEGAL;

	len = strlen(iso->filename);

	_seekfile(iso->handle, 0, SEEK_SET);
	_readfile(iso->handle, buf, 4);
	
	if (strncmp(buf, "BDV2", 4) == 0)
	{
		iso->flags = ISOFLAGS_BLOCKDUMP;
		_readfile(iso->handle, &iso->blocksize, 4);
		_readfile(iso->handle, &iso->blocks, 4);
		_readfile(iso->handle, &iso->blockofs, 4);
		_isoReadDtable(iso);
		return (detect(iso) == 1) ? 0 : -1;
	}
	else
	{
		iso->blocks = 16;
	}

	if (tryIsoType(iso, 2048, 0, 24)) return 0;			// ISO 2048
	if (tryIsoType(iso, 2336, 0, 16)) return 0;			// RAW 2336
	if (tryIsoType(iso, 2352, 0, 0)) return 0; 				// RAW 2352
	if (tryIsoType(iso, 2448, 0, 0)) return 0; 				// RAWQ 2448
	if (tryIsoType(iso, 2048, 150 * 2048, 24)) return 0;		// NERO ISO 2048
	if (tryIsoType(iso, 2352, 150 * 2048, 0)) return 0;		// NERO RAW 2352
	if (tryIsoType(iso, 2448, 150 * 2048, 0)) return 0;		// NERO RAWQ 2448
	if (tryIsoType(iso, 2048, -8, 24)) return 0; 			// ISO 2048
	if (tryIsoType(iso, 2352, -8, 0)) return 0;				// RAW 2352
	if (tryIsoType(iso, 2448, -8, 0)) return 0;				// RAWQ 2448

	iso->offset = 0;
	iso->blocksize = 2352;
	iso->blockofs = 0;
	iso->type = ISOTYPE_AUDIO;
	return 0;
}

isoFile *isoOpen(const char *filename)
{
	isoFile *iso;

	iso = (isoFile*)malloc(sizeof(isoFile));
	if (iso == NULL) return NULL;

	memset(iso, 0, sizeof(isoFile));
	strcpy(iso->filename, filename);

	iso->handle = _openfile( iso->filename, O_RDONLY);
	if (iso->handle == NULL)
	{
		Console::Error("error loading %s", params iso->filename);
		return NULL;
	}

	if (isoDetect(iso) == -1) return NULL;

	Console::WriteLn("detected blocksize = %d", params iso->blocksize);

	if ((strlen(iso->filename) > 3) && strncmp(iso->filename + (strlen(iso->filename) - 3), "I00", 3) == 0)
	{
		int i;
		
		_closefile(iso->handle);
		iso->flags |= ISOFLAGS_MULTI;
		iso->blocks = 0;
		
		for (i = 0; i < 8; i++)
		{
			iso->filename[strlen(iso->filename) - 1] = '0' + i;
			iso->multih[i].handle = _openfile(iso->filename, O_RDONLY);
			
			if (iso->multih[i].handle == NULL)
			{
				break;
			}
			
			iso->multih[i].slsn = iso->blocks;
			_seekfile(iso->multih[i].handle, 0, SEEK_END);
			iso->blocks += (u32)((_tellfile(iso->multih[i].handle) - iso->offset) / (iso->blocksize));
			iso->multih[i].elsn = iso->blocks - 1;
		}

		if (i == 0)
		{
			return NULL;
		}
	}

	if (iso->flags == 0)
	{
		_seekfile(iso->handle, 0, SEEK_END);
		iso->blocks = (u32)((_tellfile(iso->handle) - iso->offset) / (iso->blocksize));
	}

	Console::WriteLn("isoOpen: %s ok", params iso->filename);
	Console::WriteLn("offset = %d", params iso->offset);
	Console::WriteLn("blockofs = %d", params iso->blockofs);
	Console::WriteLn("blocksize = %d", params iso->blocksize);
	Console::WriteLn("blocks = %d", params iso->blocks);
	Console::WriteLn("type = %d", params iso->type);

	return iso;
}

isoFile *isoCreate(const char *filename, int flags)
{
	isoFile *iso;
	char Zfile[256];

	iso = (isoFile*)malloc(sizeof(isoFile));
	if (iso == NULL) return NULL;

	memset(iso, 0, sizeof(isoFile));
	strcpy(iso->filename, filename);
	
	iso->flags = flags;
	iso->offset = 0;
	iso->blockofs = 24;
	iso->blocksize = 2048;

	if (iso->flags & (ISOFLAGS_Z | ISOFLAGS_Z2 | ISOFLAGS_BZ2))
	{
		sprintf(Zfile, "%s.table", iso->filename);
		iso->htable = _openfile(Zfile, O_WRONLY);
		
		if (iso->htable == NULL) return NULL;
	}

	iso->handle = _openfile(iso->filename, O_WRONLY | O_CREAT);
	
	if (iso->handle == NULL)
	{
		Console::Error("Error loading %s", params iso->filename);
		return NULL;
	}
	
	Console::WriteLn("isoCreate: %s ok", params iso->filename);
	Console::WriteLn("offset = %d", params iso->offset);

	return iso;
}

int  isoSetFormat(isoFile *iso, int blockofs, int blocksize, int blocks)
{
	iso->blocksize = blocksize;
	iso->blocks = blocks;
	iso->blockofs = blockofs;
	
	Console::WriteLn("blockofs = %d", params iso->blockofs);
	Console::WriteLn("blocksize = %d", params iso->blocksize);
	Console::WriteLn("blocks = %d", params iso->blocks);
	
	if (iso->flags & ISOFLAGS_BLOCKDUMP)
	{
		if (_writefile(iso->handle, "BDV2", 4) < 4) return -1;
		if (_writefile(iso->handle, &blocksize, 4) < 4) return -1;
		if (_writefile(iso->handle, &blocks, 4) < 4) return -1;
		if (_writefile(iso->handle, &blockofs, 4) < 4) return -1;
	}

	return 0;
}

s32 MSFtoLSN(u8 *Time)
{
	u32 lsn;

	lsn = Time[2];
	lsn += (Time[1] - 2) * 75;
	lsn += Time[0] * 75 * 60;
	return lsn;
}

void LSNtoMSF(u8 *Time, s32 lsn)
{
	u8 m, s, f;

	lsn += 150;
	m = lsn / 4500; 		// minuten
	lsn = lsn - m * 4500;	// minuten rest
	s = lsn / 75;			// sekunden
	f = lsn - (s * 75);		// sekunden rest
	Time[0] = itob(m);
	Time[1] = itob(s);
	Time[2] = itob(f);
}

int _isoReadBlock(isoFile *iso, u8 *dst, int lsn)
{
	u64 ofs = (u64)lsn * iso->blocksize + iso->offset;
	int ret;

	memset(dst, 0, iso->blockofs);
	_seekfile(iso->handle, ofs, SEEK_SET);
	
	ret = _readfile(iso->handle, dst, iso->blocksize);
	
	if (ret < iso->blocksize)
	{
		Console::Error("read error %d in _isoReadBlock", params ret);
		return -1;
	}

	return 0;
}

int _isoReadBlockD(isoFile *iso, u8 *dst, int lsn)
{
	int ret;

//	Console::WriteLn("_isoReadBlockD %d, blocksize=%d, blockofs=%d\n", params lsn, iso->blocksize, iso->blockofs);
	
	memset(dst, 0, iso->blockofs);
	for (int i = 0; i < iso->dtablesize;i++)
	{
		if (iso->dtable[i] != lsn) continue;

		_seekfile(iso->handle, 16 + i * (iso->blocksize + 4) + 4, SEEK_SET);
		ret = _readfile(iso->handle, dst, iso->blocksize);
		
		if (ret < iso->blocksize) return -1;

		return 0;
	}
	Console::WriteLn("Block %d not found in dump", params lsn);

	return -1;
}

int _isoReadBlockM(isoFile *iso, u8 *dst, int lsn)
{
	u64 ofs;
	int ret, i;

	for (i = 0; i < 8; i++)
	{
		if ((lsn >= iso->multih[i].slsn) && (lsn <= iso->multih[i].elsn))
		{
			break;
		}
	}
	
	if (i == 8) return -1;

	ofs = (u64)(lsn - iso->multih[i].slsn) * iso->blocksize + iso->offset;
	
//	Console::WriteLn("_isoReadBlock %d, blocksize=%d, blockofs=%d\n", params lsn, iso->blocksize, iso->blockofs);
	
	memset(dst, 0, iso->blockofs);
	_seekfile(iso->multih[i].handle, ofs, SEEK_SET);
	ret = _readfile(iso->multih[i].handle, dst, iso->blocksize);
	
	if (ret < iso->blocksize)
	{
		Console::WriteLn("read error %d in _isoReadBlockM", params ret);
		return -1;
	}

	return 0;
}

int isoReadBlock(isoFile *iso, u8 *dst, int lsn)
{
	int ret;

	if (lsn > iso->blocks)
	{
		Console::WriteLn("isoReadBlock: %d > %d", params lsn, iso->blocks);
		return -1;
	}
	
	if (iso->flags & ISOFLAGS_BLOCKDUMP)
		ret = _isoReadBlockD(iso, dst, lsn);
	else if (iso->flags & ISOFLAGS_MULTI)
		ret = _isoReadBlockM(iso, dst, lsn);
	else
		ret = _isoReadBlock(iso, dst, lsn);
	
	if (ret == -1) return -1;

	if (iso->type == ISOTYPE_CD)
	{
		LSNtoMSF(dst + 12, lsn);
		dst[15] = 2;
	}

	return 0;
}


int _isoWriteBlock(isoFile *iso, u8 *src, int lsn)
{
	int ret;
	u64 ofs = (u64)lsn * iso->blocksize + iso->offset;

	_seekfile(iso->handle, ofs, SEEK_SET);
	ret = _writefile(iso->handle, src, iso->blocksize);
	if (ret < iso->blocksize) return -1;

	return 0;
}

int _isoWriteBlockD(isoFile *iso, u8 *src, int lsn)
{
	int ret;

//	Console::WriteLn("_isoWriteBlock %d (ofs=%d)", params iso->blocksize, ofs);
	
	ret = _writefile(iso->handle, &lsn, 4);
	if (ret < 4) return -1;
	ret = _writefile(iso->handle, src, iso->blocksize);
	
//	Console::WriteLn("_isoWriteBlock %d", params ret);
	
	if (ret < iso->blocksize) return -1;

	return 0;
}

int isoWriteBlock(isoFile *iso, u8 *src, int lsn)
{
	int ret;

	if (iso->flags & ISOFLAGS_BLOCKDUMP)
		ret = _isoWriteBlockD(iso, src, lsn);
	else
		ret = _isoWriteBlock(iso, src, lsn);
	
	if (ret == -1) return -1;
	return 0;
}

void isoClose(isoFile *iso)
{
	if (iso->handle) _closefile(iso->handle);
	if (iso->htable) _closefile(iso->htable);
	if (iso->buffer) free(iso->buffer);
	
	free(iso);
}

