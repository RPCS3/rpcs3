/*  tablerebuild.c
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
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *  PCSX2 members can be contacted through their website at www.pcsx2.net.
 */


#include <stddef.h> // NULL
#include <stdio.h> // sprintf()
#include <stdlib.h> // malloc()

#include "mainbox.h"
#include "progressbox.h"
#include "isofile.h"
#include "multifile.h"
#include "isocompress.h" // CompressClose()
#include "gzipv1.h"
#include "gzipv2.h"
#include "bzip2v2.h"
#include "bzip2v3.h"
#include "actualfile.h" // ACTUALHANDLENULL


void IsoTableRebuild(const char *filename)
{
	struct IsoFile *datafile;
	struct IsoFile *tablefile;
	int retval;
	char tempblock[65536];
	int stop;

	struct TableData table;

	datafile = IsoFileOpenForRead(filename);

	// Note: This is the start of the "Multifile" process. It's commented
	//   out so at least we can rebuild 1 part of a multifile at a time.
	// IsoNameStripExt(datafile);
	// IsoNameStripMulti(datafile);

	// Prep tablefile to hold ONLY a table (no data)
	tablefile = (struct IsoFile *) malloc(sizeof(struct IsoFile));
	if (tablefile == NULL)
	{
		datafile = IsoFileClose(datafile);
		return;
	} // ENDIF- Failed to allocate? Abort.

	tablefile->sectorpos = 0;
	tablefile->openforread = 0;
	tablefile->filebytepos = 0;
	tablefile->filebytesize = 0;
	tablefile->filesectorpos = 0;
	tablefile->filesectorsize = 0;
	tablefile->handle = ACTUALHANDLENULL;

	tablefile->namepos = 0;
	while ((tablefile->namepos < 255) &&
	        (*(filename + tablefile->namepos) != 0))
	{
		tablefile->name[tablefile->namepos] = *(filename + tablefile->namepos);
		tablefile->namepos++;
	} // ENDWHILE- Copying file name into tablefile
	tablefile->name[tablefile->namepos] = 0; // And 0-terminate.

	tablefile->imageheader = datafile->imageheader;
	tablefile->blocksize = datafile->blocksize;
	tablefile->blockoffset = datafile->blockoffset;
	tablefile->cdvdtype = 0; // Not important right now.

	tablefile->compress = datafile->compress;
	tablefile->compresspos = datafile->compresspos;
	tablefile->numsectors = datafile->numsectors;
	tablefile->tabledata = NULL;

	switch (tablefile->compress)
	{
		case 1:
			retval = GZipV1OpenTableForWrite(tablefile);
			break;
		case 2:
			retval = -1;
			break;
		case 3:
			retval = GZipV2OpenTableForWrite(tablefile);
			break;
		case 4:
			retval = BZip2V2OpenTableForWrite(tablefile);
			break;
		case 5:
			retval = BZip2V3OpenTableForWrite(tablefile);
			break;
		default:
			retval = -1;
			break;
	} // ENDSWITCH compress- Which table are we writing out?
	if (retval < 0)
	{
		datafile = IsoFileClose(datafile);
		return;
	} // ENDIF- Failed to open table file? Abort

	sprintf(tempblock, "Rebuilding table for %s", datafile->name);
	ProgressBoxStart(tempblock, datafile->filebytesize);

	stop = 0;
	mainboxstop = 0;
	progressboxstop = 0;
	while ((stop == 0) && (datafile->filebytepos < datafile->filebytesize))
	{
		switch (datafile->compress)
		{
			case 1:
				retval = GZipV1Read(datafile, 0, tempblock);
				break;
			case 2:
				retval = -1;
				break;
			case 3:
				retval = GZipV2Read(datafile, 0, tempblock);
				break;
			case 4:
				retval = BZip2V2Read(datafile, 0, tempblock);
				break;
			case 5:
				retval = BZip2V3Read(datafile, 0, tempblock);
				break;
			default:
				retval = -1;
				break;
		} // ENDSWITCH compress- Scanning for the next complete compressed block

		if (retval <= 0)
		{
#ifdef FUNCTION_WARNING_TABLEREBUILD
			PrintLog("CDVDiso rebuild:   failed to decompress - data corrupt");
#endif /* FUNCTION_WARNING_TABLEREBUILD */
			stop = 1;
		}
		else
		{
			table.offset = datafile->filebytepos - retval;
			table.size = retval;
			switch (tablefile->compress)
			{
				case 1:
					retval = GZipV1WriteTable(tablefile, table);
					break;
				case 2:
					retval = -1;
					break;
				case 3:
					retval = GZipV2WriteTable(tablefile, table);
					break;
				case 4:
					retval = BZip2V2WriteTable(tablefile, table);
					break;
				case 5:
					retval = BZip2V3WriteTable(tablefile, table);
					break;
				default:
					retval = -1;
					break;
			} // ENDSWITCH compress- Writing out the relavent table facts
			if (retval < 0)  stop = 1;
		} // ENDIF- Do we have a valid record to write an entry for?

		ProgressBoxTick(datafile->filebytepos);

		if (mainboxstop != 0)  stop = 2;
		if (progressboxstop != 0)  stop = 2;
	} // ENDWHILE- Read in the data file and writing a table, 1 block at a time

	ProgressBoxStop();

	CompressClose(tablefile); // Guarentee the table is flushed and closed.
	if (stop != 0)
	{
		ActualFileDelete(tablefile->tablename);
	} // ENDIF- Aborted or trouble? Delete the table file
	tablefile = IsoFileClose(tablefile);
	datafile = IsoFileClose(datafile);

	return;
} // END IsoTableRebuild()
