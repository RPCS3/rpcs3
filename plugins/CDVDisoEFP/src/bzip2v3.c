/*  bzip2v3.c
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

#include <stddef.h> // NULL
#include <stdlib.h> // malloc()
#include <sys/types.h> // off64_t

#include "bzip2/bzlib.h"

#include "convert.h"
#include "logfile.h"
#include "isofile.h" // IsoFile
#include "isocompress.h" // TableData, TableMap
#include "actualfile.h"
#include "convert.h"
#include "bzip2v3.h"

#ifdef _WIN32
#pragma pack(1)
#endif /* _WIN32 */
struct BZip2V3Header
{
	char id[4];
	unsigned short blocksize;
	off64_t numblocks;
	unsigned short blockoffset;
#ifdef _WIN32
};
#else
} __attribute__((packed));
#endif /* _WIN32 */

struct BZip2V3Table
{
	off64_t offset;
#ifdef _WIN32
};
#else
} __attribute__((packed));
#endif /* _WIN32 */

#ifdef _WIN32
#pragma pack()
#endif /* _WIN32 */

int BZip2V3OpenTableForRead(struct IsoFile *isofile)
{
	int i;
	int j;
	char tableext[] = ".table\0";
	off64_t numentries;
	off64_t offset;
	off64_t actual;
#ifdef VERBOSE_FUNCTION_BZIP2V3
	PrintLog("CDVDiso BZip2V3: OpenTableForRead()");
#endif /* VERBOSE_FUNCTION_BZIP2V3 */
	i = 0;
	while ((i < 256) && (isofile->name[i] != 0))
	{
		isofile->tablename[i] = isofile->name[i];
		i++;
	} // ENDWHILE- Copying the data name to the table name
	j = 0;
	while ((i < 256) && (tableext[j] != 0))
	{
		isofile->tablename[i] = tableext[j];
		i++;
		j++;
	} // ENDWHILE- Adding the ".table" extension.
	isofile->tablename[i] = 0; // And 0-terminate.

	isofile->tablehandle = ActualFileOpenForRead(isofile->tablename);
	if (isofile->tablehandle == ACTUALHANDLENULL)
	{
#ifdef VERBOSE_WARNING_BZIP2V3
		PrintLog("CDVDiso BZip2V3:   Couldn't open table!");
#endif /* VERBOSE_WARNING_BZIP2V3 */
		return(-2);
	} // ENDIF- Couldn't open table file? Fail.

	numentries = isofile->filesectorsize / isofile->numsectors;
	if ((isofile->filesectorsize % isofile->numsectors) != 0) numentries++;
	offset = numentries * sizeof(struct BZip2V3Table);
	actual = ActualFileSize(isofile->tablehandle);
	if (offset != actual)
	{
#ifdef VERBOSE_WARNING_BZIP2V3
		PrintLog("CDVDiso BZip2V3:   Table not the correct size! (Should be %lli, is %lli)",
		         offset, actual);
#endif /* VERBOSE_WARNING_BZIP2V3 */
		return(-2);
	} // ENDIF- Not the correct-sized table for the data file? Fail.

	return(0);
} // END BZip2V3OpenTableForRead()

int BZip2V3SeekTable(struct IsoFile *isofile, off64_t sector)
{
	off64_t target;
	int retval;

#ifdef VERBOSE_FUNCTION_BZIP2V3
	PrintLog("CDVDiso BZip2V3: SeekTable(%lli)", sector);
#endif /* VERBOSE_FUNCTION_BZIP2V3 */

	target = sector / isofile->numsectors;
	target *= sizeof(struct BZip2V3Table);
	retval = ActualFileSeek(isofile->tablehandle, target);
	if (retval < 0)
	{
#ifdef VERBOSE_WARNING_BZIP2V3
		PrintLog("CDVDiso BZip2V3:   Couldn't find sector!");
#endif /* VERBOSE_WARNING_BZIP2V3 */
		return(-2);
	} // ENDIF- Trouble finding the place? Fail.

	isofile->filesectorpos = sector;
	isofile->compsector = isofile->filesectorsize + isofile->numsectors;
	return(0);
} // END BZip2V3SeekTable()

int BZip2V3ReadTable(struct IsoFile *isofile, struct TableData *table)
{
	int retval;
	struct BZip2V3Table temptable;

#ifdef VERBOSE_FUNCTION_BZIP2V3
	PrintLog("CDVDiso BZip2V3: ReadTable()");
#endif /* VERBOSE_FUNCTION_BZIP2V3 */

	retval = ActualFileRead(isofile->tablehandle,
	                        sizeof(struct BZip2V3Table),
	                        (char *) & temptable);
	if (retval != sizeof(struct BZip2V3Table))
	{
#ifdef VERBOSE_WARNING_BZIP2V3
		PrintLog("CDVDiso BZip2V3:   Couldn't read table entry!");
#endif /* VERBOSE_WARNING_BZIP2V3 */
		return(-2);
	} // ENDIF- Trouble reading table entry? Fail.

	table->offset = ConvertEndianOffset(temptable.offset);
	table->size = 0;
	return(0);
} // END BZip2V3ReadTable()

int BZip2V3OpenTableForWrite(struct IsoFile *isofile)
{
	int i;
	int j;
	char tableext[] = ".table\0";
#ifdef VERBOSE_FUNCTION_BZIP2V3
	PrintLog("CDVDiso BZip2V3: OpenTableForWrite()");
#endif /* VERBOSE_FUNCTION_BZIP2V3 */
	i = 0;
	while ((i < 256) && (isofile->name[i] != 0))
	{
		isofile->tablename[i] = isofile->name[i];
		i++;
	} // ENDWHILE- Copying the data name to the table name
	j = 0;
	while ((i < 256) && (tableext[j] != 0))
	{
		isofile->tablename[i] = tableext[j];
		i++;
		j++;
	} // ENDWHILE- Adding the ".table" extension.
	isofile->tablename[i] = 0; // And 0-terminate.

	isofile->tablehandle = ActualFileOpenForWrite(isofile->tablename);
	if (isofile->tablehandle == ACTUALHANDLENULL)
	{
#ifdef VERBOSE_WARNING_BZIP2V3
		PrintLog("CDVDiso BZip2V3:   Couldn't open table!");
#endif /* VERBOSE_WARNING_BZIP2V3 */
		return(-2);
	} // ENDIF- Couldn't open table file? Fail.

	// isofile->filesectorsize = 0;
	return(0);
} // END BZip2V3OpenTableForWrite()

int BZip2V3WriteTable(struct IsoFile *isofile, struct TableData table)
{
	int retval;
	struct BZip2V3Table bv3table;
#ifdef VERBOSE_FUNCTION_BZIP2V3
	PrintLog("CDVDiso BZip2V3: WriteTable(%lli, %i)", table.offset, table.size);
#endif /* VERBOSE_FUNCTION_BZIP2V3 */
	bv3table.offset = ConvertEndianOffset(table.offset);
	retval = ActualFileWrite(isofile->tablehandle,
	                         sizeof(struct BZip2V3Table),
	                         (char *) & bv3table);
	if (retval != sizeof(struct BZip2V3Table))
	{
#ifdef VERBOSE_WARNING_BZIP2V3
		PrintLog("CDVDiso BZip2V3:   Couldn't write table entry!");
#endif /* VERBOSE_WARNING_BZIP2V3 */
		return(-2);
	} // ENDIF- Trouble reading table entry? Fail.

	return(0);
} // END BZip2V3WriteTable()

int BZip2V3OpenForRead(struct IsoFile *isofile)
{
	int retval;
	struct BZip2V3Header header;

#ifdef VERBOSE_FUNCTION_BZIP2V3
	PrintLog("CDVDiso BZip2V3: OpenForRead()");
#endif /* VERBOSE_FUNCTION_BZIP2V3 */

	isofile->handle = ActualFileOpenForRead(isofile->name);
	if (isofile->handle == ACTUALHANDLENULL)
	{
		return(-1);
	} // ENDIF- Couldn't open data file? Fail.
	isofile->filebytesize = ActualFileSize(isofile->handle);
	isofile->filebytepos = 0;
	isofile->filesectorpos = 0;
	isofile->imageheader = 0;
	retval = ActualFileRead(isofile->handle,
	                        sizeof(struct BZip2V3Header),
	                        (char *) & header);
	if (retval != sizeof(struct BZip2V3Header))
	{
#ifdef VERBOSE_WARNING_BZIP2V3
		PrintLog("CDVDiso BZip2V3:   Couldn't read header!");
#endif /* VERBOSE_WARNING_BZIP2V3 */
		ActualFileClose(isofile->handle);
		isofile->handle = ACTUALHANDLENULL;
		return(-1);
	} // ENDIF Could not read the first sector? Fail.
	isofile->filebytepos += retval;

	if ((header.id[0] != 'B') ||
	        (header.id[1] != 'Z') ||
	        (header.id[2] != 'V') ||
	        (header.id[3] != '3'))
	{
#ifdef VERBOSE_WARNING_BZIP2V3
		PrintLog("CDVDiso BZip2V3:   Not a bzip2 v3 compression header!");
#endif /* VERBOSE_WARNING_BZIP2V3 */
		ActualFileClose(isofile->handle);
		isofile->handle = ACTUALHANDLENULL;
		return(-1);
	} // ENDIF- ID for this compression type doesn't match?

	isofile->blocksize = ConvertEndianUShort(header.blocksize);
	isofile->filesectorsize = ConvertEndianOffset(header.numblocks);
	isofile->blockoffset = ConvertEndianUShort(header.blockoffset);
	isofile->numsectors = (65536 / isofile->blocksize) - 1;
	isofile->filesectorpos = 0;
	isofile->compsector = header.numblocks + isofile->numsectors;
	return(0);
} // END BZip2V3OpenForRead()

int BZip2V3Seek(struct IsoFile *isofile, off64_t position)
{
	int retval;
#ifdef VERBOSE_FUNCTION_BZIP2V3
	PrintLog("CDVDiso BZip2V3: Seek(%lli)", position);
#endif /* VERBOSE_FUNCTION_BZIP2V3 */
	retval = ActualFileSeek(isofile->handle, position);
	if (retval < 0)
	{
#ifdef VERBOSE_WARNING_BZIP2V3
		PrintLog("CDVDiso BZip2V3:   Couldn't find the start of the compressed block!");
#endif /* VERBOSE_WARNING_BZIP2V3 */
		return(-1);
	} // ENDIF- Couldn't find the data entry? Fail.
	isofile->filebytepos = position;
	return(0);
	return(-1); // Fail. (Due to lack of ambition?)
} // END BZip2V3Seek()

int BZip2V3Read(struct IsoFile *isofile, int bytes, char *buffer)
{
	int retval;
	unsigned short tempsize;
	char tempblock[65536];
	unsigned int blocklen;
#ifdef VERBOSE_FUNCTION_BZIP2V3
	PrintLog("CDVDiso BZip2V3: Read()");
#endif /* VERBOSE_FUNCTION_BZIP2V3 */
	retval = ActualFileRead(isofile->handle,
	                        sizeof(unsigned short),
	                        (char *) & tempsize);
	if (retval > 0)  isofile->filebytepos += retval;
	if (retval != sizeof(unsigned short))
	{
#ifdef VERBOSE_WARNING_BZIP2V3
		PrintLog("CDVDiso BZip2V3:   Cannot read bytes! Returned: (%i)", retval);
#endif /* VERBOSE_WARNING_BZIP2V3 */
		return(-1);
	} // ENDIF- Trouble reading compressed sector? Abort.
	tempsize = ConvertEndianUShort(tempsize);
	retval = ActualFileRead(isofile->handle, tempsize, tempblock);
	if (retval > 0)  isofile->filebytepos += retval;
	if (retval != tempsize)
	{
#ifdef VERBOSE_WARNING_BZIP2V3
		PrintLog("CDVDiso BZip2V3:   Cannot read bytes! Returned: (%i)", retval);
#endif /* VERBOSE_WARNING_BZIP2V3 */
		return(-1);
	} // ENDIF- Trouble reading compressed sector? Abort.

	blocklen = 65536;
	retval = BZ2_bzBuffToBuffDecompress(buffer, &blocklen, tempblock, tempsize, 0, 0);
	if (retval != BZ_OK)
	{
#ifdef VERBOSE_WARNING_BZIP2V3
		PrintLog("CDVDiso BZip2V3:   Cannot decode block! Returned: (%i)", retval);
#endif /* VERBOSE_WARNING_BZIP2V3 */
		return(-1);
	} // ENDIF- Trouble decoding the sector? Abort.
	return(tempsize + sizeof(unsigned short));
} // END BZip2V3Read()

int BZip2V3OpenForWrite(struct IsoFile *isofile)
{
	char garbage[sizeof(struct BZip2V3Header)];
	int i;
	if (isofile == NULL)  return(-1);
#ifdef VERBOSE_FUNCTION_BZIP2V3
	PrintLog("CDVDiso BZip2V3: OpenForWrite()");
#endif /* VERBOSE_FUNCTION_BZIP2V3 */
	isofile->handle = ActualFileOpenForWrite(isofile->name);
	if (isofile->handle == ACTUALHANDLENULL)
	{
		return(-1);
	} // ENDIF- Couldn't open data file? Fail.
	for (i = 0; i < sizeof(struct BZip2V3Header); i++)  garbage[i] = 0;
	ActualFileWrite(isofile->handle, sizeof(struct BZip2V3Header), garbage);

	isofile->filebytesize = 0;
	isofile->filebytepos = sizeof(struct BZip2V3Header);
	isofile->numsectors = (65536 / isofile->blocksize) - 1;
	isofile->filesectorsize = 0;
	isofile->filesectorpos = 0;
	isofile->compsector = 0;
	return(0);
} // END BZip2V3OpenForWrite()

int BZip2V3Write(struct IsoFile *isofile, char *buffer)
{
	int retval;
	unsigned int blocklen;
	char tempblock[65536];
	unsigned short tempsize;

#ifdef VERBOSE_FUNCTION_BZIP2V3
	PrintLog("CDVDiso BZip2V3: Write()");
#endif /* VERBOSE_FUNCTION_BZIP2V3 */

	blocklen = 65536;
	retval = BZ2_bzBuffToBuffCompress(tempblock,
	                                  &blocklen,
	                                  buffer,
	                                  isofile->blocksize * isofile->numsectors,
	                                  9, 0, 250);
	if (retval != BZ_OK)
	{
#ifdef VERBOSE_FUNCTION_BZIP2V3
		PrintLog("CDVDiso BZip2V3:   Cannot encode block! Returned: (%i)", retval);
#endif /* VERBOSE_FUNCTION_BZIP2V3 */
		return(-1);
	} // ENDIF- Trouble compressing a block? Abort.
	tempsize = blocklen;
	tempsize = ConvertEndianUShort(tempsize);
	retval = ActualFileWrite(isofile->handle,
	                         sizeof(unsigned short),
	                         (char *) & tempsize);
	if (retval > 0)  isofile->filebytepos += retval;
	if (retval < sizeof(unsigned short))
	{
#ifdef VERBOSE_FUNCTION_BZIP2V3
		PrintLog("CDVDiso BZip2V3:   Cannot write bytes! Returned: (%i out of %llu)",
		         retval, sizeof(unsigned short));
#endif /* VERBOSE_FUNCTION_BZIP2V3 */
		return(-1);
	} // ENDIF- Trouble writing out the compressed block? Abort.

	retval = ActualFileWrite(isofile->handle, blocklen, tempblock);
	if (retval > 0)  isofile->filebytepos += retval;
	if (retval < blocklen)
	{
#ifdef VERBOSE_FUNCTION_BZIP2V3
		PrintLog("CDVDiso BZip2V3:   Cannot write bytes! Returned: (%i out of %llu)",
		         retval, blocklen);
#endif /* VERBOSE_FUNCTION_BZIP2V3 */
		return(-1);
	} // ENDIF- Trouble writing out the compressed block? Abort.

	return(blocklen + sizeof(unsigned short)); // Not in list? Fail.
} // END BZip2V3Write()

void BZip2V3Close(struct IsoFile *isofile)
{
	struct BZip2V3Header header;
	struct TableData table;
	int compptr;
	int i;
	int retval;
	unsigned short tempsize;

#ifdef VERBOSE_FUNCTION_BZIP2V3
	PrintLog("CDVDiso BZip2V3: Close()");
#endif /* VERBOSE_FUNCTION_BZIP2V3 */

	if ((isofile->tablehandle != ACTUALHANDLENULL) &&
	        (isofile->handle != ACTUALHANDLENULL))
	{
		if (isofile->openforread == 0)
		{
			if (isofile->compsector != isofile->filesectorpos)
			{
				compptr = isofile->filesectorpos - isofile->compsector;
				compptr *= isofile->blocksize;
				for (i = compptr; i < 65536; i++)  isofile->compblock[i] = 0;
				retval = BZip2V3Write(isofile, isofile->compblock);
				table.offset = isofile->filebytepos - retval;
				table.size = retval;
				BZip2V3WriteTable(isofile, table);
				isofile->compsector = isofile->filesectorpos;
			} // ENDIF - still have buffers to write?
		} // ENDIF- Opened for write? Don't forget to flush the file buffer!
	} // ENDIF- Both data file and table file are open?

	if (isofile->tablehandle != ACTUALHANDLENULL)
	{
		ActualFileClose(isofile->tablehandle);
		isofile->tablehandle = ACTUALHANDLENULL;
	} // ENDIF- Is there a table file open? Close it.
	if (isofile->handle != ACTUALHANDLENULL)
	{
		if (isofile->openforread == 0)
		{
			if (isofile->compsector != isofile->filesectorpos)
			{
				compptr = isofile->filesectorpos - isofile->compsector;
				compptr *= isofile->blocksize;
				for (i = compptr; i < 65536; i++)  isofile->compblock[i] = 0;
				BZip2V3Write(isofile, isofile->compblock);
			} // ENDIF - still have buffers to write?
			header.id[0] = 'B';
			header.id[1] = 'Z';
			header.id[2] = 'V';
			header.id[3] = '3';
			tempsize = isofile->blocksize;
			header.blocksize = ConvertEndianUShort(tempsize);
			header.numblocks = ConvertEndianOffset(isofile->filesectorsize);
			tempsize = isofile->blockoffset;
			header.blockoffset = ConvertEndianUShort(tempsize);
			ActualFileSeek(isofile->handle, 0);
			ActualFileWrite(isofile->handle,
			                sizeof(struct BZip2V3Header),
			                (char *) &header);
		} // ENDIF- Opened for write? Don't forget to update the header block!

		ActualFileClose(isofile->handle);
		isofile->handle = ACTUALHANDLENULL;
	} // ENDIF- Is there a data file open? Close it.

	if (isofile->tabledata != NULL)
	{
		free(isofile->tabledata);
		isofile->tabledata = NULL;
	} // ENDIF- Do we have a read-in table to clear out?
	return;
} // END BZip2V3Close()
