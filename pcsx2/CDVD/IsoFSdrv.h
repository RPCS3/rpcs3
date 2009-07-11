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

#ifndef __ISOFSDRV_H__
#define __ISOFSDRV_H__

#include "IsoFScdvd.h"

extern CdRMode cdReadMode;

/* Filing-system exported functions */
void CDVDFS_init();
int CDVDFS_open(const char *name, int mode);
int CDVDFS_lseek(int fd, int offset, int whence);
int CDVDFS_read( int fd, char * buffer, int size );
int CDVDFS_write( int fd, char * buffer, int size );
int CDVDFS_close( int fd);

#endif//__ISOFSDRV_H__
