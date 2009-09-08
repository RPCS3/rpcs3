/*  PCSX2 - PS2 Emulator for PCs
 *  Copyright (C) 2002-2009  PCSX2 Dev Team
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
/*
 *  Original code from libcdvd by Hiryu & Sjeep (C) 2002
 *  Modified by Florin for PCSX2 emu
 */

#ifndef __ISOFSDRV_H__
#define __ISOFSDRV_H__

#include "IsoFScdvd.h"

/* Filing-system exported functions */
void IsoFS_init();
int IsoFS_open(const char *name, int mode);
int IsoFS_lseek(int fd, int offset, int whence);
int IsoFS_read( int fd, char * buffer, int size );
int IsoFS_close( int fd);

#endif//__ISOFSDRV_H__
