/*  Pcsx2 - Pc Ps2 Emulator
 *  Copyright (C) 2002-2008  Pcsx2 Team
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

#include <string.h>

#include "CDVDiso.h"
#include "CDVDisodrv.h"

CdRMode cdReadMode;

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
//	called by 80000592 sceCdInit()
//////////////////////////////////////////////////////////////////////
void CDVDFS_init(){

	if (inited)	return;//might change in the future as a param; forceInit/Reset

#ifdef RPC_LOG
	RPC_LOG("[CDVDisodrv:init] CDVD Filesystem v1.00\n");
	RPC_LOG("[CDVDisodrv     ] \tby A.Lee (aka Hiryu) & Nicholas Van Veen (aka Sjeep)\n");
	RPC_LOG("[CDVDisodrv     ] Initializing '%s' file driver.\n", "cdfs");
#endif

	//CdInit(0);	already called by plugin loading system ;)
	
	cdReadMode.trycount = 0;
	cdReadMode.spindlctrl = CdSpinStm;
	cdReadMode.datapattern = CdSecS2048;	//isofs driver only needs
											//2KB sectors

	memset(fd_table, 0, sizeof(fd_table));
	memset(fd_used, 0, 16*sizeof(int));

	inited = TRUE;

	return;
}

//////////////////////////////////////////////////////////////////////
//						CDVDFS_open
//	called by 80000001 fileio_open for devices: "cdrom:", "cdrom0:"
//////////////////////////////////////////////////////////////////////
int CDVDFS_open(char *name, int mode){
	register int j;
	static struct TocEntry tocEntry;

	// check if the file exists
	if (CDVD_findfile(name, &tocEntry) != TRUE)
		return -1;

	if(mode != 1) return -2;	//SCE_RDONLY

	// set up a new file descriptor
	for(j=0; j < 16; j++) if(fd_used[j] == 0) break;
	if(j >= 16) return -3;

	fd_used[j] = 1;
	files_open++;

#ifdef RPC_LOG
	RPC_LOG("[CDVDisodrv:open] internal fd=%d\n", j);
#endif

	fd_table[j].fileSize = tocEntry.fileSize;
	fd_table[j].LBA = tocEntry.fileLBA;
	fd_table[j].filePos = 0;

#ifdef RPC_LOG
	RPC_LOG("[CDVDisodrv     ] tocEntry.fileSize = %d\n",tocEntry.fileSize);
#endif

   	return j;
}

//////////////////////////////////////////////////////////////////////
//						CDVDFS_lseek
//	called by 80000001 fileio_lseek for devices: "cdrom:", "cdrom0:"
//////////////////////////////////////////////////////////////////////
int CDVDFS_lseek(int fd, int offset, int whence){

	if ((fd >= 16) || (fd_used[fd]==0)){
#ifdef RPC_LOG
		RPC_LOG("[CDVDisodrv:lseek] ERROR: File does not appear to be open!\n");
#endif
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
//	called by 80000001 fileio_read for devices: "cdrom:", "cdrom0:", "cdfs:"
//////////////////////////////////////////////////////////////////////
int CDVDFS_read( int fd, char *buffer, int size ){
//	int start_sector;
	int off_sector;
//	int num_sectors;

	//static char local_buffer[2024*2048];	//4MB
	static char lb[2048];					//2KB
	//Start, Aligned, End
	int  ssector, asector, esector;
	int  ssize=0, asize,   esize;

	if ((fd >= 16) || (fd_used[fd]==0)){
#ifdef RPC_LOG
		RPC_LOG("[CDVDisodrv:read] ERROR: File does not appear to be open!\n");
#endif
		return -1;
	}

		// A few sanity checks
	if (fd_table[fd].filePos > fd_table[fd].fileSize){
		// We cant start reading from past the beginning of the file
		return 0;	// File exists but we couldnt read anything from it
	}

	if ((fd_table[fd].filePos + size) > fd_table[fd].fileSize)
		size = fd_table[fd].fileSize - fd_table[fd].filePos;

	// Now work out where we want to start reading from
	asector = ssector = fd_table[fd].LBA + (fd_table[fd].filePos >> 11);
	off_sector = (fd_table[fd].filePos & 0x7FF);
	if (off_sector){
		ssize   = min(2048 - off_sector, size);
		size   -= ssize;
		asector++;
	}
	asize = size & 0xFFFFF800;
	esize = size & 0x000007FF;
	esector=asector + (asize >> 11);
	size += ssize;

#ifdef RPC_LOG
	RPC_LOG("[CDVDisodrv:read] read sectors 0x%08X to 0x%08X\n", ssector, esector-(esize==0));
#endif

	if (ssize){	if (CdRead(ssector, 1, lb, &cdReadMode) != TRUE){
#ifdef RPC_LOG
			RPC_LOG("[CDVDisodrv:    ] Couldn't Read from file for some reason\n");
#endif
			return 0;
		}
		FreezeMMXRegs(1);
		memcpy_fast(buffer, lb + off_sector, ssize);
		FreezeMMXRegs(0);
	}
	if (asize)	if (CdRead(asector, asize >> 11, buffer+ssize, &cdReadMode) != TRUE){
#ifdef RPC_LOG
		RPC_LOG("[CDVDisodrv:    ] Couldn't Read from file for some reason\n");
#endif
		return 0;
	}
	if (esize){	if (CdRead(esector, 1, lb, &cdReadMode) != TRUE){
#ifdef RPC_LOG
			RPC_LOG("[CDVDisodrv:    ] Couldn't Read from file for some reason\n");
#endif
			return 0;
		}
		FreezeMMXRegs(1);
		memcpy_fast(buffer+ssize+asize, lb, esize);
		FreezeMMXRegs(0);
	}
/***********************
	// Now work out where we want to start reading from
	start_sector = fd_table[fd].LBA + (fd_table[fd].filePos >> 11);
	off_sector = (fd_table[fd].filePos & 0x7FF);
	num_sectors = ((off_sector + size) >> 11) + 1;

#ifdef RPC_LOG
	RPC_LOG("[CDVDisodrv:read] read sectors 0x%08X to 0x%08X\n",start_sector,start_sector+num_sectors);
#endif

	// Read the data (we only ever get 16KB max request at once)
	if (CdRead(start_sector, num_sectors, local_buffer, &cdReadMode) != TRUE){
#ifdef RPC_LOG
		//RPC_LOG("sector = %d, start sector = %d\n",sector,start_sector);
		RPC_LOG("[CDVDisodrv:    ] Couldn't Read from file for some reason\n");
#endif
		return 0;
	}
	//CdSync(0);	hm, a wait function maybe...

	memcpy_fast(buffer,local_buffer+off_sector,size);
**************************/
	fd_table[fd].filePos += size;

	return (size);
}

//////////////////////////////////////////////////////////////////////
//						CDVDFS_write
//	called by 80000001 fileio_write for devices: "cdrom:", "cdrom0:"
//	hehe, this ain't a CD writing option :D
//////////////////////////////////////////////////////////////////////
int CDVDFS_write( int fd, char * buffer, int size ){
   if(size == 0) return 0;
   else 		 return -1;
}

//////////////////////////////////////////////////////////////////////
//						CDVDFS_close
//	called by 80000001 fileio_close for devices: "cdrom:", "cdrom0:"
//////////////////////////////////////////////////////////////////////
int CDVDFS_close( int fd){

	if ((fd >= 16) || (fd_used[fd]==0)){
#ifdef RPC_LOG
		RPC_LOG("[CDVDisodrv:close] ERROR: File does not appear to be open!\n");
#endif
		return -1;
	}

#ifdef RPC_LOG
	RPC_LOG("[CDVDisodrv:close] internal fd %d\n", fd);
#endif

	fd_used[fd] = 0;
	files_open--;

    return 0;
}

