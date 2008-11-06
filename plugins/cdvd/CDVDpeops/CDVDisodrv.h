/*
 *  Original code from libcdvd by Hiryu & Sjeep (C) 2002
 *  Modified by Florin for PCSX2 emu
 */

#ifndef __CDVDISODRV_H__
#define __CDVDISODRV_H__

//#include "Common.h"
#include "CDVDlib.h"

extern CdRMode cdReadMode;

/* Filing-system exported functions */
void CDVDFS_init();
int CDVDFS_open(char *name, int mode);
int CDVDFS_lseek(int fd, int offset, int whence);
int CDVDFS_read( int fd, char * buffer, int size );
int CDVDFS_write( int fd, char * buffer, int size );
int CDVDFS_close( int fd);

#endif//__CDVDISODRV_H__
