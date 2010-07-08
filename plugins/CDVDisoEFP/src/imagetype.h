/*  imagetype.h
 *  Copyright (C) 2002-2005  PCSX2 Team
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
 *
 *  PCSX2 members can be contacted through their website at www.pcsx2.net.
 */

#ifndef IMAGETYPE_H
#define IMAGETYPE_H

// #ifndef __LINUX__
// #ifdef __linux__
// #define __LINUX__
// #endif /* __linux__ */
// #endif /* No __LINUX__ */

// #define CDVDdefs
// #include "PS2Edefs.h"
#include "isofile.h"

struct ImageTypes
{
	char *name;
	off64_t blocksize;
	off64_t fileoffset;
	int dataoffset;
	int conversiontype; // For conversionbox to write a new file as.
};

// Note: Worked around since a failure occurred with MSVCRT.DLL. It couldn't
//   printf a char string inside an array of structures. Don't know why.
// extern struct ImageTypes imagedata[];

extern void GetImageType(struct IsoFile *isofile, int imagetype);
extern int GetImageTypeConvertTo(int imagetype);
extern int DetectImageType(struct IsoFile *isofile);

#endif /* IMAGETYPE_H */
