/*  bzip2v2.c
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
#include <stdlib.h> // malloc()
#include <sys/types.h> // off64_t

#include "bzip2/bzlib.h"

#include "convert.h"
#include "logfile.h"
#include "isofile.h" // IsoFile
#include "isocompress.h" // TableData, TableMap
#include "actualfile.h"
// #include "convert.h"
#include "bzip2v2.h"

#ifdef _WIN32
#pragma pack(1)
#endif /* _WIN32 */
struct BZip2V2Header
{
	char id[4];
	unsigned int blocksize;
	unsigned int numblocks;
	unsigned int blockoffset;
#ifdef _WIN32
};
#else
} __attribute__((packed));
#endif /* _WIN32 */

struct BZip2V2Table
{
	unsigned int size;
#ifdef _WIN32
};
#else
} __attribute__((packed));
#endif /* _WIN32 */

#ifdef _WIN32
#pragma pack()
#endif /* _WIN32 */

int BZip2V2OpenTableForRead(struct IsoFile *isofile)
{
	int i;
	int j;
	char tableext[] = ".table\0";
	off64_t numentries;
	off64_t offset;
	off64_t actual;
	int tableoffset;
	struct BZip2V2Table table;
	int retval;
	union TableMap tablemap;
#ifdef VERBOSE_FUNCTION_BZIP2V2
	PrintLog("CDVDiso BZip2V2: OpenTableForRead()");
#endif /* VERBOSE_FUNCTION_BZIP2V2 */
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
#ifdef VERBOSE_WARNING_BZIP2V2
		PrintLog("CDVDiso BZip2V2:   Couldn't open table!");
#endif /* VERBOSE_WARNING_BZIP2V2 */
		return(-2);
	} // ENDIF- Couldn't open table file? Fail.

	numentries = isofile->filesectorsize / 16;
	if ((isofile->filesectorsize % 16) != 0) numentries++;
	offset = numentries * sizeof(struct BZip2V2Table);
	actual = ActualFileSize(isofile->tablehandle);
	if (offset != actual)
	{
#ifdef VERBOSE_WARNING_BZIP2V2
		PrintLog("CDVDiso BZip2V2:   Table not the correct size! (Should be %lli, is %lli)",
		         offset, actual);
#endif /* VERBOSE_WARNING_BZIP2V2 */
		return(-2);
	} // ENDIF- Not the correct-sized table for the data file? Fail.

	// We pre-read the WHOLE offset table.
	isofile->tabledata = (char *) malloc(numentries * sizeof(struct TableData));
	if (isofile->tabledata == NULL)
	{
#ifdef VERBOSE_WARNING_BZIP2V2
		PrintLog("CDVDiso BZip2V2:   Couldn't allocate internal table!");
#endif /* VERBOSE_WARNING_BZIP2V2 */
		return(-2);
	} // ENDIF- Could not get enough memory to hold table data
	offset = sizeof(struct BZip2V2Header);
	tableoffset = 0;
	for (i = 0; i < numentries; i++)
	{
		retval = ActualFileRead(isofile->tablehandle,
		                        sizeof(struct BZip2V2Table),
		                        (char *) & table);
		if (retval != sizeof(struct BZip2V2Table))
		{
#ifdef VERBOSE_WARNING_BZIP2V2
			PrintLog("CDVDiso BZip2V2:   Failed to read in entry %i!", i);
#endif /* VERBOSE_WARNING_BZIP2V2 */
			return(-2);
		} // ENDIF- Trouble reading in a size? Table damaged... fail.

		tablemap.table.offset = offset;
		tablemap.table.size = ConvertEndianUInt(table.size);
		for (j = 0; j < sizeof(struct TableData); j++)
			*(isofile->tabledata + tableoffset + j) = tablemap.ch[j];
		offset += tablemap.table.size;
		tableoffset += sizeof(struct TableData);
	} // NEXT i- reading in the sizes, and making offset as I go.

	ActualFileClose(isofile->tablehandle);
	isofile->tablehandle = ACTUALHANDLENULL;
	return(0);
} // END BZip2V2OpenTableForRead()

int BZip2V2SeekTable(struct IsoFile *isofile, off64_t sector)
{
#ifdef VERBOSE_FUNCTION_BZIP2V2
	PrintLog("CDVDiso BZip2V2: SeekTable(%lli)", sector);
#endif /* VERBOSE_FUNCTION_BZIP2V2 */
	isofile->filesectorpos = sector;
	isofile->compsector = isofile->filesectorsize + isofile->numsectors;
	return(0);
} // END BZip2V2SeekTable()

int BZip2V2ReadTable(struct IsoFile *isofile, struct TableData *table)
{
	off64_t target;
	union TableMap tablemap;
	off64_t i;

#ifdef VERBOSE_FUNCTION_BZIP2V2
	PrintLog("CDVDiso BZip2V2: ReadTable()");
#endif /* VERBOSE_FUNCTION_BZIP2V2 */

	target = (isofile->filesectorpos / isofile->numsectors)
	         * sizeof(struct TableData);
	for (i = 0; i < sizeof(struct TableData); i++)
		tablemap.ch[i] = *(isofile->tabledata + target + i);
	table->offset = tablemap.table.offset;
	table->size = tablemap.table.size;
	return(0);
} // END BZip2V2ReadTable()

int BZip2V2OpenTableForWrite(struct IsoFile *isofile)
{
	int i;
	int j;
	char tableext[] = ".table\0";

#ifdef VERBOSE_FUNCTION_BZIP2V2
	PrintLog("CDVDiso BZip2V2: OpenTableForWrite()");
#endif /* VERBOSE_FUNCTION_BZIP2V2 */

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
#ifdef VERBOSE_WARNING_BZIP2V2
		PrintLog("CDVDiso BZip2V2:   Couldn't open table!");
#endif /* VERBOSE_WARNING_BZIP2V2 */
		return(-2);
	} // ENDIF- Couldn't open table file? Fail.
	// isofile->filesectorsize = 0;
	return(0);
} // END BZip2V2OpenTableForWrite()

int BZip2V2WriteTable(struct IsoFile *isofile, struct TableData table)
{
	int retval;
	struct BZip2V2Table bv2table;
	unsigned int tempint;
#ifdef VERBOSE_FUNCTION_BZIP2V2
	PrintLog("CDVDiso BZip2V2: WriteTable(%lli, %i)", table.offset, table.size);
#endif /* VERBOSE_FUNCTION_BZIP2V2 */
	tempint = table.size;
	bv2table.size = ConvertEndianUInt(tempint);
	retval = ActualFileWrite(isofile->tablehandle,
	                         sizeof(struct BZip2V2Table),
	                         (char *) & bv2table);
	if (retval != sizeof(struct BZip2V2Table))
	{
#ifdef VERBOSE_WARNING_BZIP2V2
		PrintLog("CDVDiso BZip2V2:   Couldn't write table entry!");
#endif /* VERBOSE_WARNING_BZIP2V2 */
		return(-2);
	} // ENDIF- Trouble reading table entry? Fail.
	return(0);
} // END BZip2V2WriteTable()

int BZip2V2OpenForRead(struct IsoFile *isofile)
{
	int retval;
	struct BZip2V2Header header;
#ifdef VERBOSE_FUNCTION_BZIP2V2
	PrintLog("CDVDiso BZip2V2: OpenForRead()");
#endif /* VERBOSE_FUNCTION_BZIP2V2 */
	isofile->handle = ActualFileOpenForRead(isofile->name);
	if (isofile->handle == ACTUALHANDLENULL)
	{
		return(-1);
	} // ENDIF- Couldn't open data file? Fail.

	isofile->filebytesize = ActualFileSize(isofile->handle);
	isofile->filebytepos = 0;
	isofile->filesectorpos = 0;

	isofile->imageheader = 0;
	isofile->numsectors = 16; // Sectors per block
	retval = ActualFileRead(isofile->handle,
	                        sizeof(struct BZip2V2Header),
	                        (char *) & header);
	if (retval != sizeof(struct BZip2V2Header))
	{
#ifdef VERBOSE_WARNING_BZIP2V2
		PrintLog("CDVDiso BZip2V2:   Couldn't read header!");
#endif /* VERBOSE_WARNING_BZIP2V2 */
		ActualFileClose(isofile->handle);
		isofile->handle = ACTUALHANDLENULL;
		return(-1);
	} // ENDIF Could not read the first sector? Fail.
	isofile->filebytepos += retval;

	if ((header.id[0] != 'B') ||
	        (header.id[1] != 'Z') ||
	        (header.id[2] != 'V') ||
	        (header.id[3] != '2'))
	{
#ifdef VERBOSE_WARNING_BZIP2V2
		PrintLog("CDVDiso BZip2V2:   Not a bzip2 v2 compression header!");
#endif /* VERBOSE_WARNING_BZIP2V2 */
		ActualFileClose(isofile->handle);
		isofile->handle = ACTUALHANDLENULL;
		return(-1);
	} // ENDIF- ID for this compression type doesn't match?

	isofile->blocksize = ConvertEndianUInt(header.blocksize);
	isofile->filesectorsize = ConvertEndianUInt(header.numblocks);
	isofile->blockoffset = ConvertEndianUInt(header.blockoffset);
	isofile->filesectorpos = 0;
	isofile->compsector = header.numblocks + isofile->numsectors;
	return(0);
} // END BZip2V2OpenForRead()

int BZip2V2Seek(struct IsoFile *isofile, off64_t position)
{
	int retval;

#ifdef VERBOSE_FUNCTION_BZIP2V2
	PrintLog("CDVDiso BZip2V2: Seek(%lli)", position);
#endif /* VERBOSE_FUNCTION_BZIP2V2 */

	retval = ActualFileSeek(isofile->handle, position);
	if (retval < 0)
	{
#ifdef VERBOSE_WARNING_BZIP2V2
		PrintLog("CDVDiso BZip2V2:   Couldn't find the start of the compressed block!");
#endif /* VERBOSE_WARNING_BZIP2V2 */
		return(-1);
	} // ENDIF- Couldn't find the data entry? Fail.
	isofile->filebytepos = position;
	return(0);

	return(-1); // Fail. (Due to lack of ambition?)
} // END BZip2V2Seek()

int BZip2V2Read(struct IsoFile *isofile, int bytes, char *buffer)
{
	int retval;
	unsigned int blocklen;
	bz_stream strm;
	unsigned int tempin;
	char tempblock[65536];
	unsigned int tempout;

#ifdef VERBOSE_FUNCTION_BZIP2V2
	PrintLog("CDVDiso BZip2V2: Read(%i)", bytes);
#endif /* VERBOSE_FUNCTION_BZIP2V2 */

	if (bytes > 0)
	{
		retval = ActualFileRead(isofile->handle, bytes, tempblock);
		if (retval > 0)  isofile->filebytepos += retval;
		if (retval != bytes)
		{
#ifdef VERBOSE_WARNING_BZIP2V2
			PrintLog("CDVDiso BZip2V2:   Cannot read bytes! Returned: (%i)", retval);
#endif /* VERBOSE_WARNING_BZIP2V2 */
			return(-1);
		} // ENDIF- Trouble reading compressed sector? Abort.

		blocklen = 65536;
		retval = BZ2_bzBuffToBuffDecompress(buffer, &blocklen, tempblock, bytes, 0, 0);
		if (retval != BZ_OK)
		{
#ifdef VERBOSE_WARNING_BZIP2V2
			PrintLog("CDVDiso BZip2V2:   Cannot decode block! Returned: (%i)", retval);
#endif /* VERBOSE_WARNING_BZIP2V2 */
			return(-1);
		} // ENDIF- Trouble decoding the sector? Abort.
		return(0);
	} // ENDIF- Do we know how many compressed bytes to get for this record?

	// Hmm. Don't know the compressed size? Well, we'll just have to find it.

	tempin = 0;
	tempout = 0;
	retval = BZ_OK;
	while ((tempin < 65536) && (tempout < isofile->blocksize * isofile->numsectors))
	{
		strm.bzalloc = NULL;
		strm.bzfree = NULL;
		strm.opaque = NULL;
		retval = BZ2_bzDecompressInit(&strm, 0, 0);
		if (retval != BZ_OK) return(-1);

		strm.next_out = buffer;

		strm.avail_in = tempin;
		strm.avail_out = 65536;
		retval = BZ_OK;
		while ((tempin < 65536) && (retval == BZ_OK))
		{
			retval = ActualFileRead(isofile->handle, 1, &tempblock[tempin]);
			if (retval != 1)
			{
#ifdef VERBOSE_WARNING_BZIP2V2
				PrintLog("CDVDiso BZip2V2:   Cannot read a byte! Returned: (%i)", retval);
#endif /* VERBOSE_WARNING_BZIP2V2 */
				BZ2_bzDecompressEnd(&strm);
				return(-1);
			} // ENDIF- Trouble reading a piece of compressed sector? Abort.
			tempin++;
			strm.avail_in++;

			strm.next_in = &tempblock[tempin - strm.avail_in];
			retval = BZ2_bzDecompress(&strm);
		} // ENDWHILE- trying to uncompress an increasingly filled buffer
		tempout = 65536 - strm.avail_out;
		BZ2_bzDecompressEnd(&strm);

	} // ENDWHILE- trying to uncompress a whole buffer
	if (retval != BZ_STREAM_END)
	{
#ifdef VERBOSE_WARNING_BZIP2V2
		PrintLog("CDVDiso BZip2V2:   Failed to decode block! (Returned %i", retval);
#endif /* VERBOSE_WARNING_BZIP2V2 */
		return(-1);
	} // ENDIF- Not a clean cutoff of a buffer? Say so.

	if (tempin == 65536)  return(-1);
	isofile->filebytepos += tempin;
	return(tempin); // Send out # of compressed bytes (to record in table)
} // END BZip2V2Read()

int BZip2V2OpenForWrite(struct IsoFile *isofile)
{
	char garbage[sizeof(struct BZip2V2Header)];
	int i;

	if (isofile == NULL)  return(-1);

#ifdef VERBOSE_FUNCTION_BZIP2V2
	PrintLog("CDVDiso BZip2V2: OpenForWrite()");
#endif /* VERBOSE_FUNCTION_BZIP2V2 */

	isofile->handle = ActualFileOpenForWrite(isofile->name);
	if (isofile->handle == ACTUALHANDLENULL)
	{
		return(-1);
	} // ENDIF- Couldn't open data file? Fail.
	for (i = 0; i < sizeof(struct BZip2V2Header); i++)  garbage[i] = 0;
	ActualFileWrite(isofile->handle, sizeof(struct BZip2V2Header), garbage);
	isofile->filebytesize = 0;
	isofile->filebytepos = sizeof(struct BZip2V2Header);
	isofile->numsectors = 16;
	isofile->filesectorsize = 0;
	isofile->filesectorpos = 0;
	isofile->compsector = 0;
	return(0);
} // END BZip2V2OpenForWrite()

int BZip2V2Write(struct IsoFile *isofile, char *buffer)
{
	int retval;
	unsigned int blocklen;
	char tempblock[65536];

#ifdef VERBOSE_FUNCTION_BZIP2V2
	PrintLog("CDVDiso BZip2V2: Write()");
#endif /* VERBOSE_FUNCTION_BZIP2V2 */

	blocklen = 65536;
	retval = BZ2_bzBuffToBuffCompress(tempblock,
	                                  &blocklen,
	                                  buffer,
	                                  isofile->blocksize * isofile->numsectors,
	                                  9, 0, 250);
	if (retval != BZ_OK)
	{
#ifdef VERBOSE_FUNCTION_BZIP2V2
		PrintLog("CDVDiso BZip2V2:   Cannot encode block! Returned: (%i)", retval);
#endif /* VERBOSE_FUNCTION_BZIP2V2 */
		return(-1);
	} // ENDIF- Trouble compressing a block? Abort.
	retval = ActualFileWrite(isofile->handle, blocklen, tempblock);
	if (retval > 0)  isofile->filebytepos += retval;
	if (retval < blocklen)
	{
#ifdef VERBOSE_FUNCTION_BZIP2V2
		PrintLog("CDVDiso BZip2V2:   Cannot write bytes! Returned: (%i out of %llu)",
		         retval, blocklen);
#endif /* VERBOSE_FUNCTION_BZIP2V2 */
		return(-1);
	} // ENDIF- Trouble writing out the compressed block? Abort.
	return(blocklen); // Not in list? Fail.
} // END BZip2V2Write()

void BZip2V2Close(struct IsoFile *isofile)
{
	struct BZip2V2Header header;
	struct TableData table;
	int compptr;
	int i;
	int retval;

#ifdef VERBOSE_FUNCTION_BZIP2V2
	PrintLog("CDVDiso BZip2V2: Close()");
#endif /* VERBOSE_FUNCTION_BZIP2V2 */

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
				retval = BZip2V2Write(isofile, isofile->compblock);
				table.offset = isofile->filebytepos - retval;
				table.size = retval;
				BZip2V2WriteTable(isofile, table);
				isofile->compsector = isofile->filesectorpos;
			} // ENDIF - still have buffers to write?
		} // ENDIF- Opened for write? Don't forget to flush the file buffer!
	} // ENDIF- Are both the data file and table files open?

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
				BZip2V2Write(isofile, isofile->compblock);
			} // ENDIF - still have buffers to write?
			header.id[0] = 'B';
			header.id[1] = 'Z';
			header.id[2] = 'V';
			header.id[3] = '2';
			header.blocksize = ConvertEndianUInt(isofile->blocksize);
			header.numblocks = ConvertEndianUInt(isofile->filesectorsize);
			header.blockoffset = ConvertEndianUInt(isofile->blockoffset);
			ActualFileSeek(isofile->handle, 0);
			ActualFileWrite(isofile->handle,
			                sizeof(struct BZip2V2Header),
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
} // END BZip2V2Close()
