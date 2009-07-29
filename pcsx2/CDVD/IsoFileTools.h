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

 #ifndef __ISO_FILE_TOOLS_H__
#define __ISO_FILE_TOOLS_H__

#ifndef _LARGEFILE_SOURCE
#define _LARGEFILE_SOURCE 
#endif

#ifndef _LARGEFILE64_SOURCE
#define _LARGEFILE64_SOURCE
#endif

#define __USE_FILE_OFFSET64
#define _FILE_OFFSET_BITS 64

#ifdef _MSC_VER
#pragma warning(disable:4018)
#endif

#include "IopCommon.h"

#include <stdio.h>
#include <fcntl.h>
#include <errno.h>

void *_openfile(const char *filename, int flags);
u64 _tellfile(void *handle);
int _seekfile(void *handle, u64 offset, int whence);
int _readfile(void *handle, void *dst, int size);
int _writefile(void *handle, void *src, int size);
void _closefile(void *handle);

#endif