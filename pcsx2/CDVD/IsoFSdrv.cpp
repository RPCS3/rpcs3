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
 
/*
 *  Original code from libcdvd by Hiryu & Sjeep (C) 2002
 *  Modified by Florin for PCSX2 emu
 */

#include "PrecompiledHeader.h"

#include "IsoFStools.h"
#include "IsoFSdrv.h"

struct fdtable{
//int fd;
	int fileSize;
	int LBA;
	int filePos;
};

static struct fdtable fd_table[16];
static int fd_used[16];
static int files_open=0;
static int inited=FALSE;

/*************************************************************
* The functions below are the normal file-system operations, *
*     used to provide a standard filesystem interface		 *
*************************************************************/

//////////////////////////////////////////////////////////////////////
//						CDVDFS_init
//////////////////////////////////////////////////////////////////////
void IsoFS_init()
{
	if (inited)	return; //might change in the future as a param; forceInit/Reset

	ISOFS_LOG("[IsoFSdrv:init] Initializing '%s' file driver.", "IsoFS");

	memzero_obj( fd_table );
	memzero_obj( fd_used );

	inited = TRUE;

	return;
}

//////////////////////////////////////////////////////////////////////
//						CDVDFS_open
//////////////////////////////////////////////////////////////////////
int IsoFS_open(const char *name, int mode){
	int j;
	static struct TocEntry tocEntry;

	// check if the file exists
	if (IsoFS_findFile(name, &tocEntry) != TRUE)
		return -1;

	if(mode != 1) return -2;	//SCE_RDONLY

	// set up a new file descriptor
	for(j=0; j < 16; j++) if(fd_used[j] == 0) break;
	if(j >= 16) return -3;

	fd_used[j] = 1;
	files_open++;

	ISOFS_LOG("[IsoFSdrv:open] internal fd=%d", j);

	fd_table[j].fileSize = tocEntry.fileSize;
	fd_table[j].LBA = tocEntry.fileLBA;
	fd_table[j].filePos = 0;

	ISOFS_LOG("[IsoFSdrv:open] tocEntry.fileSize = %d",tocEntry.fileSize);
   	return j;
}

//////////////////////////////////////////////////////////////////////
//						CDVDFS_lseek
//////////////////////////////////////////////////////////////////////
int IsoFS_lseek(int fd, int offset, int whence){

	if ((fd >= 16) || (fd_used[fd]==0)){
		ISOFS_LOG("[IsoFSdrv:lseek] ERROR: File does not appear to be open!");
		return -1;
	}

	switch(whence){
	case SEEK_SET:
		fd_table[fd].filePos = offset;
		break;

	case SEEK_CUR:
		fd_table[fd].filePos += offset;
		break;

	case SEEK_END:
		fd_table[fd].filePos = fd_table[fd].fileSize + offset;
		break;

	default:
		return -1;
	}

	if (fd_table[fd].filePos < 0)
		fd_table[fd].filePos = 0;

	if (fd_table[fd].filePos > fd_table[fd].fileSize)
		fd_table[fd].filePos = fd_table[fd].fileSize;

	return fd_table[fd].filePos;
}

//////////////////////////////////////////////////////////////////////
//						CDVDFS_read
//////////////////////////////////////////////////////////////////////
int IsoFS_read( int fd, char *buffer, int size )
{
	int off_sector;
	static char lb[2048];					//2KB

	//Start, Aligned, End
	int  ssector, asector, esector;
	int  ssize=0, asize,   esize;

	if ((fd >= 16) || (fd_used[fd]==0))
	{
		ISOFS_LOG("[IsoFSdrv:read] ERROR: File does not appear to be open!");
		return -1;
	}

		// A few sanity checks
	if (fd_table[fd].filePos > fd_table[fd].fileSize)
	{
		// We cant start reading from past the beginning of the file
		return 0;	// File exists but we couldn't read anything from it
	}

	if ((fd_table[fd].filePos + size) > fd_table[fd].fileSize)
		size = fd_table[fd].fileSize - fd_table[fd].filePos;

	// Now work out where we want to start reading from
	asector = ssector = fd_table[fd].LBA + (fd_table[fd].filePos >> 11);
	off_sector = (fd_table[fd].filePos & 0x7FF);
	if (off_sector)
	{
		ssize   = std::min(2048 - off_sector, size);
		size   -= ssize;
		asector++;
	}
	asize = size & 0xFFFFF800;
	esize = size & 0x000007FF;
	esector=asector + (asize >> 11);
	size += ssize;

	ISOFS_LOG("[IsoFSdrv:read] read sectors 0x%08X to 0x%08X", ssector, esector-(esize==0));

	if (ssize)
	{	
		if (IsoFS_readSectors(ssector, 1, lb) != TRUE)
		{
			ISOFS_LOG("[IsoFSdrv:read] Couldn't Read from file for some reason");
			return 0;
		}
		memcpy_fast(buffer, lb + off_sector, ssize);
	}
	if (asize)	if (IsoFS_readSectors(asector, asize >> 11, buffer+ssize) != TRUE)
	{
		ISOFS_LOG("[IsoFSdrv:read] Couldn't Read from file for some reason");
		return 0;
	}
	if (esize)
	{
		if (IsoFS_readSectors(esector, 1, lb) != TRUE)
		{
			ISOFS_LOG("[IsoFSdrv:read] Couldn't Read from file for some reason");
			return 0;
		}
		memcpy_fast(buffer+ssize+asize, lb, esize);
	}
/***********************
	// Now work out where we want to start reading from
	start_sector = fd_table[fd].LBA + (fd_table[fd].filePos >> 11);
	off_sector = (fd_table[fd].filePos & 0x7FF);
	num_sectors = ((off_sector + size) >> 11) + 1;

	RPC_LOG("[IsoFSdrv:read] read sectors 0x%08X to 0x%08X",start_sector,start_sector+num_sectors);

	// Read the data (we only ever get 16KB max request at once)
	if (CdRead(start_sector, num_sectors, local_buffer, &cdReadMode) != TRUE){

		//RPC_LOG("sector = %d, start sector = %d",sector,start_sector);
		RPC_LOG("[IsoFSdrv:    ] Couldn't Read from file for some reason");
		return 0;
	}
	//CdSync(0);	hm, a wait function maybe...

	memcpy_fast(buffer,local_buffer+off_sector,size);
**************************/
	fd_table[fd].filePos += size;

	return (size);
}

//////////////////////////////////////////////////////////////////////
//						CDVDFS_close
//////////////////////////////////////////////////////////////////////
int IsoFS_close( int fd)
{

	if ((fd >= 16) || (fd_used[fd]==0))
	{
		ISOFS_LOG("[IsoFSdrv:close] ERROR: File does not appear to be open!");
		return -1;
	}
	ISOFS_LOG("[IsoFSdrv:close] internal fd %d", fd);
	fd_used[fd] = 0;
	files_open--;

    return 0;
}

