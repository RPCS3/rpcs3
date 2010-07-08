/*  bzip2v3.h
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

#ifndef BZIP2V3_H
#define BZIP2V3_H

#include <sys/types.h>

#include "isofile.h"
#include "isocompress.h"

// #define VERBOSE_FUNCTION_BZIP2V3
// #define VERBOSE_WARNING_BZIP2V3

extern int BZip2V3OpenTableForRead(struct IsoFile *isofile);
extern int BZip2V3SeekTable(struct IsoFile *isofile, off64_t sector);
extern int BZip2V3ReadTable(struct IsoFile *isofile, struct TableData *table);

extern int BZip2V3OpenTableForWrite(struct IsoFile *isofile);
extern int BZip2V3WriteTable(struct IsoFile *isofile, struct TableData table);
extern int BZip2V3OpenForRead(struct IsoFile *isofile);
extern int BZip2V3Seek(struct IsoFile *isofile, off64_t sector);
extern int BZip2V3Read(struct IsoFile *isofile, int bytes, char *buffer);
extern void BZip2V3Close(struct IsoFile *isofile);

extern int BZip2V3OpenForWrite(struct IsoFile *isofile);
extern int BZip2V3Write(struct IsoFile *isofile, char *buffer);

#endif /* BZIP2V3_H */
