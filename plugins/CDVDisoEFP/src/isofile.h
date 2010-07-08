/*  isofile.h
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

#ifndef ISOFILE_H
#define ISOFILE_H

#include <sys/types.h> // off64_t

#include "actualfile.h"

// #define VERBOSE_FUNCTION_ISOFILE
#define VERBOSE_DISC_INFO

struct IsoFile
{
	// *** Primary Data area
	char name[256];
	int namepos; // Used for detection of components within name
	ACTUALHANDLE handle;
	off64_t sectorpos; // Overall.
	// blocks (char [2352]) provided by users
	// *** Derived Stats from Primary Data
	int openforread; // 1 = Yes, 0 = No
	off64_t filebytesize;
	off64_t filebytepos;
	off64_t filesectorsize;
	off64_t filesectorpos;
	int cdvdtype; // for GetDiskType call
	char toc[2048]; // for GetTOC call

	// From imagetype.h
	int imagetype;
	char imagename[40];
	off64_t imageheader; // Header bytes in every file...
	off64_t blocksize; // sized to add quickly to other off64_t counters
	int blockoffset; // Where to place data in block
	// from isocompress.h
	int compress; // Compression Method used (0 = none, 1...)
	int compresspos; // Start pos of ".Z", ".BZ", etc...
	char tablename[256];
	ACTUALHANDLE tablehandle;
	char compblock[65536]; // Temporary storage of uncompressed sectors.
	off64_t compsector; // First sector of the compblock[]
	off64_t numsectors; // Number of sectors in a compression block
	char *tabledata; // Table holding area
	// From multifile.h
	int multi; // 0 = Single File, 1 = Multiple Files
	int multipos; // Position of Multi # ('0'-'9')
	off64_t multisectorend[10]; // Ending sector of each file...
	off64_t multioffset; // To help with seek calls. Sector offset.
	int multistart; // Starting file number (0-1)
	int multiend; // Ending file number (?-9)
	int multinow; // Current open file number

	// *** (Specific) Compression Data area
};

// Read-only section
// IsoFiles opened for read are treated as random-access files
extern int IsIsoFile(const char *filename);
// Returns an image type (positive or zero) or an error (negative)

extern struct IsoFile *IsoFileOpenForRead(const char *filename);
// Will seek blocksize and compression method on it's own.
extern int IsoFileSeek(struct IsoFile *file, off64_t sector);
// Sector, not byte.

extern int IsoFileRead(struct IsoFile *file, char *block);
// Buffers should be at least of "blocksize" size. 2352 bytes, please.

// Write-only section
// IsoFiles opened for write are treated as sequential files (still written
//  out a block at a time, though, to be read in later as random-access)
// No plans are made to make writes random-access at this time.
extern struct IsoFile *IsoFileOpenForWrite(const char *filename,
				        int imagetype,
				        int multi,
				        int compress);

extern int IsoFileWrite(struct IsoFile *file, char *block);
// Uncompressed buffers, please.
// Will compress with this call (if it's necessary.)

// Calls used for all Isofiles
extern struct IsoFile *IsoFileClose(struct IsoFile *file);
// Use return variable to NULL the file pointer.
// Ex: lastfile = IsoFileClose(lastfile);
extern struct IsoFile *IsoFileCloseAndDelete(struct IsoFile *file);
// For failure to finish writing out a file(set).

#endif /* ISOFILE_H */
