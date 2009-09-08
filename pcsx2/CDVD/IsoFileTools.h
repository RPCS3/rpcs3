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

#pragma once

#ifndef _LARGEFILE_SOURCE
#define _LARGEFILE_SOURCE
#endif

#ifndef _LARGEFILE64_SOURCE
#define _LARGEFILE64_SOURCE
#endif

#ifndef __USE_FILE_OFFSET64
#define __USE_FILE_OFFSET64
#endif

#define _FILE_OFFSET_BITS 64

#ifdef _MSC_VER
#	pragma warning(disable:4018)	// disable signed/unsigned mismatch error
#endif

#include "IopCommon.h"

#include <stdio.h>
#include <fcntl.h>
#include <errno.h>

extern void *_openfile(const char *filename, int flags);
extern u64  _tellfile(void *handle);
extern int  _seekfile(void *handle, u64 offset, int whence);
extern int  _readfile(void *handle, void *dst, int size);
extern int  _writefile(void *handle, const void *src, int size);
extern void _closefile(void *handle);

