#include "PrecompiledHeader.h"
#define __USE_LARGEFILE64
#define __USE_FILE_OFFSET64
#define _FILE_OFFSET_BITS 64
#define _LARGEFILE_SOURCE 
#define _LARGEFILE64_SOURCE

#include "IopCommon.h"
#include "IsoFStools.h"
#include "IsoFSdrv.h"
#include "IsoFileFormats.h"

#include <stdio.h>
#include <fcntl.h>

#ifdef _WIN32
#include <windows.h>

void *_openfile(const char *filename, int flags)
{
	HANDLE handle;

//	printf("_openfile %s, %d\n", filename, flags & O_RDONLY);
	if (flags & O_WRONLY)
	{
		int _flags = CREATE_NEW;
		if (flags & O_CREAT) _flags = CREATE_ALWAYS;
		handle = CreateFile(filename, GENERIC_WRITE, 0, NULL, _flags, 0, NULL);
	}
	else
	{
		handle = CreateFile(filename, GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
	}

	return handle == INVALID_HANDLE_VALUE ? NULL : handle;
}

u64 _tellfile(void *handle)
{
	u64 ofs;
	PLONG _ofs = (LONG*) & ofs;
	_ofs[1] = 0;
	_ofs[0] = SetFilePointer(handle, 0, &_ofs[1], FILE_CURRENT);
	return ofs;
}

int _seekfile(void *handle, u64 offset, int whence)
{
	u64 ofs = (u64)offset;
	PLONG _ofs = (LONG*) & ofs;
//	printf("_seekfile %p, %d_%d\n", handle, _ofs[1], _ofs[0]);
	if (whence == SEEK_SET)
	{
		SetFilePointer(handle, _ofs[0], &_ofs[1], FILE_BEGIN);
	}
	else
	{
		SetFilePointer(handle, _ofs[0], &_ofs[1], FILE_END);
	}
	return 0;
}

int _readfile(void *handle, void *dst, int size)
{
	DWORD ret;

//	printf("_readfile %p %d\n", handle, size);
	ReadFile(handle, dst, size, &ret, NULL);
//	printf("_readfile ret %d; %d\n", ret, GetLastError());
	return ret;
}

int _writefile(void *handle, void *src, int size)
{
	DWORD ret;

//	printf("_writefile %p, %d\n", handle, size);
//	_seekfile(handle, _tellfile(handle));
	WriteFile(handle, src, size, &ret, NULL);
//	printf("_writefile ret %d\n", ret);
	return ret;
}

void _closefile(void *handle)
{
	CloseHandle(handle);
}

#else

void *_openfile(const char *filename, int flags)
{
	printf("_openfile %s %x\n", filename, flags);

	if (flags & O_WRONLY)
		return fopen64(filename, "wb");
	else 
		return fopen64(filename, "rb");
}

#include <errno.h>

u64 _tellfile(void *handle)
{
	s64 cursize = ftell(handle);
	if (cursize == -1)
	{
		// try 64bit
		cursize = ftello64(handle);
		if (cursize < -1)
		{
			// zero top 32 bits
			cursize &= 0xffffffff;
		}
	}
	return cursize;
}

int _seekfile(void *handle, u64 offset, int whence)
{
	int seekerr = fseeko64(handle, offset, whence);
	
	if (seekerr == -1) printf("failed to seek\n");
	
	return seekerr;
}

int _readfile(void *handle, void *dst, int size)
{
	return fread(dst, 1, size, handle);
}

int _writefile(void *handle, void *src, int size)
{
	return fwrite(src, 1, size, handle);
}

void _closefile(void *handle)
{
	fclose(handle);
}

#endif

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
	int i;

	_seekfile(iso->handle, 0, SEEK_END);
	iso->dtablesize = (_tellfile(iso->handle) - 16) / (iso->blocksize + 4);
	iso->dtable = (u32*)malloc(iso->dtablesize * 4);

	for (i = 0; i < iso->dtablesize; i++)
	{
		_seekfile(iso->handle, 16 + (iso->blocksize + 4)*i, SEEK_SET);
		ret = _readfile(iso->handle, &iso->dtable[i], 4);
		if (ret < 4) return -1;
	}

	return 0;
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
		return detect(iso) == 1 ? 0 : -1;
	}
	else
	{
		iso->blocks = 16;
	}

	// ISO 2048
	iso->blocksize = 2048;
	iso->offset = 0;
	iso->blockofs = 24;
	if (detect(iso) == 1) return 0;

	// RAW 2336
	iso->blocksize = 2336;
	iso->offset = 0;
	iso->blockofs = 16;
	if (detect(iso) == 1) return 0;

	// RAW 2352
	iso->blocksize = 2352;
	iso->offset = 0;
	iso->blockofs = 0;
	if (detect(iso) == 1) return 0;

	// RAWQ 2448
	iso->blocksize = 2448;
	iso->offset = 0;
	iso->blockofs = 0;
	if (detect(iso) == 1) return 0;

	// NERO ISO 2048
	iso->blocksize = 2048;
	iso->offset = 150 * 2048;
	iso->blockofs = 24;
	if (detect(iso) == 1) return 0;

	// NERO RAW 2352
	iso->blocksize = 2352;
	iso->offset = 150 * 2048;
	iso->blockofs = 0;
	if (detect(iso) == 1) return 0;

	// NERO RAWQ 2448
	iso->blocksize = 2448;
	iso->offset = 150 * 2048;
	iso->blockofs = 0;
	if (detect(iso) == 1) return 0;

	// ISO 2048
	iso->blocksize = 2048;
	iso->offset = -8;
	iso->blockofs = 24;
	if (detect(iso) == 1) return 0;

	// RAW 2352
	iso->blocksize = 2352;
	iso->offset = -8;
	iso->blockofs = 0;
	if (detect(iso) == 1) return 0;

	// RAWQ 2448
	iso->blocksize = 2448;
	iso->offset = -8;
	iso->blockofs = 0;
	if (detect(iso) == 1) return 0;

	iso->offset = 0;
	iso->blocksize = 2352;
	iso->type = ISOTYPE_AUDIO;
	return 0;

	return -1;
}

isoFile *isoOpen(const char *filename)
{
	isoFile *iso;
	int i;

	iso = (isoFile*)malloc(sizeof(isoFile));
	if (iso == NULL) return NULL;

	memset(iso, 0, sizeof(isoFile));
	strcpy(iso->filename, filename);

	iso->handle = _openfile(iso->filename, O_RDONLY);
	if (iso->handle == NULL)
	{
		printf("Error loading %s\n", iso->filename);
		return NULL;
	}

	if (isoDetect(iso) == -1) return NULL;

	printf("detected blocksize = %d\n", iso->blocksize);

	if (strlen(iso->filename) > 3 && strncmp(iso->filename + (strlen(iso->filename) - 3), "I00", 3) == 0)
	{
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
			iso->blocks += (u32)((_tellfile(iso->multih[i].handle) - iso->offset) /
			                     (iso->blocksize));
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
		iso->blocks = (u32)((_tellfile(iso->handle) - iso->offset) /
		                    (iso->blocksize));
	}


	printf("isoOpen: %s ok\n", iso->filename);
	printf("offset = %d\n", iso->offset);
	printf("blockofs = %d\n", iso->blockofs);
	printf("blocksize = %d\n", iso->blocksize);
	printf("blocks = %d\n", iso->blocks);
	printf("type = %d\n", iso->type);

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
	iso->blocksize = CD_FRAMESIZE_RAW;
	iso->blocksize = 2048;

	if (iso->flags & (ISOFLAGS_Z | ISOFLAGS_Z2 | ISOFLAGS_BZ2))
	{
		sprintf(Zfile, "%s.table", iso->filename);
		iso->htable = _openfile(Zfile, O_WRONLY);
		if (iso->htable == NULL)
		{
			return NULL;
		}
	}

	iso->handle = _openfile(iso->filename, O_WRONLY | O_CREAT);
	if (iso->handle == NULL)
	{
		printf("Error loading %s\n", iso->filename);
		return NULL;
	}
	printf("isoCreate: %s ok\n", iso->filename);
	printf("offset = %d\n", iso->offset);

	return iso;
}

int  isoSetFormat(isoFile *iso, int blockofs, int blocksize, int blocks)
{
	iso->blocksize = blocksize;
	iso->blocks = blocks;
	iso->blockofs = blockofs;
	printf("blockofs = %d\n", iso->blockofs);
	printf("blocksize = %d\n", iso->blocksize);
	printf("blocks = %d\n", iso->blocks);
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
		printf("read error %d\n", ret);
		return -1;
	}

	return 0;
}

int _isoReadBlockD(isoFile *iso, u8 *dst, int lsn)
{
	int ret;
	int i;

//	printf("_isoReadBlockD %d, blocksize=%d, blockofs=%d\n", lsn, iso->blocksize, iso->blockofs);
	memset(dst, 0, iso->blockofs);
	for (i = 0; i < iso->dtablesize;i++)
	{
		if (iso->dtable[i] != lsn) continue;

		_seekfile(iso->handle, 16 + i*(iso->blocksize + 4) + 4, SEEK_SET);
		ret = _readfile(iso->handle, dst, iso->blocksize);
		if (ret < iso->blocksize) return -1;

		return 0;
	}
	printf("block %d not found in dump\n", lsn);

	return -1;
}

int _isoReadBlockM(isoFile *iso, u8 *dst, int lsn)
{
	u64 ofs;
	int ret;
	int i;

	for (i = 0; i < 8; i++)
	{
		if (lsn >= iso->multih[i].slsn &&
		        lsn <= iso->multih[i].elsn)
		{
			break;
		}
	}
	if (i == 8) return -1;

	ofs = (u64)(lsn - iso->multih[i].slsn) * iso->blocksize + iso->offset;
//	printf("_isoReadBlock %d, blocksize=%d, blockofs=%d\n", lsn, iso->blocksize, iso->blockofs);
	memset(dst, 0, iso->blockofs);
	_seekfile(iso->multih[i].handle, ofs, SEEK_SET);
	ret = _readfile(iso->multih[i].handle, dst, iso->blocksize);
	
	if (ret < iso->blocksize)
	{
		printf("read error %d\n", ret);
		return -1;
	}

	return 0;
}

int isoReadBlock(isoFile *iso, u8 *dst, int lsn)
{
	int ret;

	if (lsn > iso->blocks)
	{
		printf("isoReadBlock: %d > %d\n", lsn, iso->blocks);
		return -1;
	}
	
	if (iso->flags & ISOFLAGS_BLOCKDUMP)
		ret = _isoReadBlockD(iso, dst, lsn);
	else if (iso->flags & ISOFLAGS_MULTI)
		ret = _isoReadBlockM(iso, dst, lsn);
	else
		ret = _isoReadBlock(iso, dst, lsn);
	
	if (ret == -1) return ret;

	if (iso->type == ISOTYPE_CD)
	{
		LSNtoMSF(dst + 12, lsn);
		dst[15] = 2;
	}

	return 0;
}


int _isoWriteBlock(isoFile *iso, u8 *src, int lsn)
{
	u64 ofs = (u64)lsn * iso->blocksize + iso->offset;
	int ret;

	_seekfile(iso->handle, ofs, SEEK_SET);
	ret = _writefile(iso->handle, src, iso->blocksize);
	if (ret < iso->blocksize) return -1;

	return 0;
}

int _isoWriteBlockD(isoFile *iso, u8 *src, int lsn)
{
	int ret;

//	printf("_isoWriteBlock %d (ofs=%d)\n", iso->blocksize, ofs);
	ret = _writefile(iso->handle, &lsn, 4);
	if (ret < 4) return -1;
	ret = _writefile(iso->handle, src, iso->blocksize);
//	printf("_isoWriteBlock %d\n", ret);
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
	
	if (ret == -1) return ret;
	return 0;
}

void isoClose(isoFile *iso)
{
	if (iso->handle) _closefile(iso->handle);
	if (iso->htable) _closefile(iso->htable);
	if (iso->buffer) free(iso->buffer);
	
	free(iso);
}

