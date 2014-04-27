/*  multifile.h
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

#ifndef MULTIFILE_H
#define MULTIFILE_H

// #ifndef __LINUX__
// #ifdef __linux__
// #define __LINUX__
// #endif /* __linux__ */
// #endif /* No __LINUX__ */

// #define CDVDdefs
// #include "PS2Edefs.h"
#include "isofile.h"

// #define VERBOSE_FUNCTION_MULTIFILE
// #define VERBOSE_WARNING_MULTIFILE

extern char *multinames[];

extern void IsoNameStripMulti(struct IsoFile *isofile);
extern int MultiFileSeek(struct IsoFile *isofile, off64_t sector);
extern int MultiFileRead(struct IsoFile *isofile, char *block);

extern int MultiFileWrite(struct IsoFile *isofile, char *block);

#endif /* MULTIFILE_H */
