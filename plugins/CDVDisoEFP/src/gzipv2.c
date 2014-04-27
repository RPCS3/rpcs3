/*  gzipv2.c
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

#include "zlib/zlib.h"

#include "convert.h"
#include "logfile.h"
#include "isofile.h" // IsoFile
#include "isocompress.h" // TableData, TableMap
#include "actualfile.h"
#include "gzipv2.h"

#ifdef _WIN32
#pragma pack(1)
#endif /* _WIN32 */

struct GZipV2Header
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
struct GZipV2Table
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

int GZipV2OpenTableForRead(struct IsoFile *isofile)
{
	int i;
	int j;
	char tableext[] = ".table\0";
	off64_t offset;
	off64_t actual;
	int tableoffset;
	struct GZipV2Table table;
	int retval;
	union TableMap tablemap;
#ifdef VERBOSE_FUNCTION_GZIPV2
	PrintLog("CDVDiso GZipV2: OpenTableForRead()");
#endif /* VERBOSE_FUNCTION_GZIPV2 */
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
#ifdef VERBOSE_WARNING_GZIPV2
		PrintLog("CDVDiso GZipV2:   Couldn't open table!");
#endif /* VERBOSE_WARNING_GZIPV2 */
		return(-2);
	} // ENDIF- Couldn't open table file? Fail.

	offset = isofile->filesectorsize * sizeof(struct GZipV2Table);
	actual = ActualFileSize(isofile->tablehandle);
	if (offset != actual)
	{
#ifdef VERBOSE_WARNING_GZIPV2
		PrintLog("CDVDiso GZipV2:   Table not the correct size! (Should be %lli, is %lli)",
		         offset, actual);
#endif /* VERBOSE_WARNING_GZIPV2 */
		return(-2);
	} // ENDIF- Not the correct-sized table for the data file? Fail.

	// We pre-read the WHOLE offset table.
	isofile->tabledata = (char *) malloc(isofile->filesectorsize * sizeof(struct TableData));
	if (isofile->tabledata == NULL)
	{
#ifdef VERBOSE_WARNING_GZIPV2
		PrintLog("CDVDiso GZipV2:   Couldn't allocate internal table!");
#endif /* VERBOSE_WARNING_GZIPV2 */
		return(-2);
	} // ENDIF- Could not get enough memory to hold table data
	offset = sizeof(struct GZipV2Header);
	tableoffset = 0;
	for (i = 0; i < isofile->filesectorsize; i++)
	{
		retval = ActualFileRead(isofile->tablehandle,
		                        sizeof(struct GZipV2Table),
		                        (char *) & table);
		if (retval != sizeof(struct GZipV2Table))
		{
#ifdef VERBOSE_WARNING_GZIPV2
			PrintLog("CDVDiso GZipV2:   Failed to read in sector %i!", i);
#endif /* VERBOSE_WARNING_GZIPV2 */
			return(-2);
		} // ENDIF- Trouble reading in a size? Table damaged... fail.

		tablemap.table.offset = offset;
		tablemap.table.size = ConvertEndianUInt(table.size);
		for (j = 0; j < sizeof(struct TableData); j++)
			*(isofile->tabledata + tableoffset + j) = tablemap.ch[j];
		offset += table.size;
		tableoffset += sizeof(struct TableData);
	} // NEXT i- reading in the sizes, and making offset as I go.

	ActualFileClose(isofile->tablehandle);
	isofile->tablehandle = ACTUALHANDLENULL;
	return(0);
} // END GZipV2OpenTableForRead()

int GZipV2SeekTable(struct IsoFile *isofile, off64_t sector)
{
#ifdef VERBOSE_FUNCTION_GZIPV2
	PrintLog("CDVDiso GZipV2: SeekTable(%lli)", sector);
#endif /* VERBOSE_FUNCTION_GZIPV2 */
	isofile->filesectorpos = sector;
	return(0);
} // END GZipV2SeekTable()

int GZipV2ReadTable(struct IsoFile *isofile, struct TableData *table)
{
	off64_t target;
	union TableMap tablemap;
	off64_t i;
#ifdef VERBOSE_FUNCTION_GZIPV2
	PrintLog("CDVDiso GZipV2: ReadTable()");
#endif /* VERBOSE_FUNCTION_GZIPV2 */
	target = isofile->filesectorpos * sizeof(struct TableData);
	for (i = 0; i < sizeof(struct TableData); i++)
		tablemap.ch[i] = *(isofile->tabledata + target + i);
	table->offset = tablemap.table.offset;
	table->size = tablemap.table.size;
	isofile->filesectorpos++;
	return(0);
} // END GZipV2ReadTable()

int GZipV2OpenTableForWrite(struct IsoFile *isofile)
{
	int i;
	int j;
	char tableext[] = ".table\0";
#ifdef VERBOSE_FUNCTION_GZIPV2
	PrintLog("CDVDiso GZipV2: OpenTableForWrite()");
#endif /* VERBOSE_FUNCTION_GZIPV2 */
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
#ifdef VERBOSE_WARNING_GZIPV2
		PrintLog("CDVDiso GZipV2:   Couldn't open table!");
#endif /* VERBOSE_WARNING_GZIPV2 */
		return(-2);
	} // ENDIF- Couldn't open table file? Fail.

	isofile->filesectorsize = 0;
	return(0);
} // END GZipV2OpenTableForWrite()

int GZipV2WriteTable(struct IsoFile *isofile, struct TableData table)
{
	int retval;
	struct GZipV2Table gv2table;
	unsigned int tempint;

#ifdef VERBOSE_FUNCTION_GZIPV2
	PrintLog("CDVDiso GZipV2: WriteTable(%lli, %i)", table.offset, table.size);
#endif /* VERBOSE_FUNCTION_GZIPV2 */

	tempint = table.size;
	gv2table.size = ConvertEndianUInt(tempint);
	retval = ActualFileWrite(isofile->tablehandle,
	                         sizeof(struct GZipV2Table),
	                         (char *) & gv2table);
	if (retval != sizeof(unsigned int))
	{
#ifdef VERBOSE_WARNING_GZIPV2
		PrintLog("CDVDiso GZipV2:   Couldn't write table entry!");
#endif /* VERBOSE_WARNING_GZIPV2 */
		return(-2);
	} // ENDIF- Trouble reading table entry? Fail.

	return(0);
} // END GZipV2WriteTable()

int GZipV2OpenForRead(struct IsoFile *isofile)
{
	int retval;
	struct GZipV2Header header;

#ifdef VERBOSE_FUNCTION_GZIPV2
	PrintLog("CDVDiso GZipV2: OpenForRead()");
#endif /* VERBOSE_FUNCTION_GZIPV2 */

	isofile->handle = ActualFileOpenForRead(isofile->name);
	if (isofile->handle == ACTUALHANDLENULL)
	{
		return(-1);
	} // ENDIF- Couldn't open data file? Fail.
	isofile->filebytesize = ActualFileSize(isofile->handle);
	isofile->filebytepos = 0;
	isofile->filesectorpos = 0;
	isofile->imageheader = 0;
	isofile->numsectors = 1; // Sectors per block

	retval = ActualFileRead(isofile->handle,
	                        sizeof(struct GZipV2Header),
	                        (char *) & header);
	if (retval != sizeof(struct GZipV2Header))
	{
#ifdef VERBOSE_WARNING_GZIPV2
		PrintLog("CDVDiso GZipV2:   Couldn't read header!");
#endif /* VERBOSE_WARNING_GZIPV2 */
		ActualFileClose(isofile->handle);
		isofile->handle = ACTUALHANDLENULL;
		return(-1);
	} // ENDIF Could not read the first sector? Fail.
	isofile->filebytepos += retval;
	if ((header.id[0] != 'Z') ||
	        (header.id[1] != ' ') ||
	        (header.id[2] != 'V') ||
	        (header.id[3] != '2'))
	{
#ifdef VERBOSE_WARNING_GZIPV2
		PrintLog("CDVDiso GZipV2:   Not a gzip v2 compression header!");
#endif /* VERBOSE_WARNING_GZIPV2 */
		ActualFileClose(isofile->handle);
		isofile->handle = ACTUALHANDLENULL;
		return(-1);
	} // ENDIF- ID for this compression type doesn't match?
	isofile->blocksize = ConvertEndianUInt(header.blocksize);
	isofile->filesectorsize = ConvertEndianUInt(header.numblocks);
	isofile->blockoffset = ConvertEndianUInt(header.blockoffset);
	isofile->filesectorpos = 0;
	return(0);
} // END GZipV2OpenForRead()

int GZipV2Seek(struct IsoFile *isofile, off64_t position)
{
	int retval;

#ifdef VERBOSE_FUNCTION_GZIPV2
	PrintLog("CDVDiso GZipV2: Seek(%lli)", position);
#endif /* VERBOSE_FUNCTION_GZIPV2 */

	retval = ActualFileSeek(isofile->handle, position);
	if (retval < 0)
	{
#ifdef VERBOSE_WARNING_GZIPV2
		PrintLog("CDVDiso GZipV2:   Couldn't find the start of the compressed block!");
#endif /* VERBOSE_WARNING_GZIPV2 */
		return(-1);
	} // ENDIF- Couldn't find the data entry? Fail.
	isofile->filebytepos = position;
	return(0);

	return(-1); // Fail. (Due to lack of ambition?)
} // END GZipV2Seek()

int GZipV2Read(struct IsoFile *isofile, int bytes, char *buffer)
{
	int retval;
	unsigned long blocklen;
	z_stream strm;
	unsigned long tempin;
	char tempblock[2800];
	unsigned long tempout;

#ifdef VERBOSE_FUNCTION_GZIPV2
	PrintLog("CDVDiso GZipV2: Read(%i)", bytes);
#endif /* VERBOSE_FUNCTION_GZIPV2 */

	if (bytes > 0)
	{
		retval = ActualFileRead(isofile->handle, bytes, tempblock);
		if (retval > 0)  isofile->filebytepos += retval;
		if (retval != bytes)
		{
#ifdef VERBOSE_WARNING_GZIPV2
			PrintLog("CDVDiso GZipV2:   Cannot read bytes! Returned: (%i)", retval);
#endif /* VERBOSE_WARNING_GZIPV2 */
			return(-1);
		} // ENDIF- Trouble reading compressed sector? Abort.

		blocklen = isofile->blocksize;
		retval = uncompress(buffer, &blocklen, tempblock, bytes);
		if (retval != Z_OK)
		{
#ifdef VERBOSE_WARNING_GZIPV2
			PrintLog("CDVDiso GZipV2:   Cannot decode block! Returned: (%i)", retval);
#endif /* VERBOSE_WARNING_GZIPV2 */
			return(-1);
		} // ENDIF- Trouble decoding the sector? Abort.
		return(0);
	} // ENDIF- Do we know how many compressed bytes to get for this record?

	// Hmm. Don't know the compressed size? Well, we'll just have to find it.

	tempin = 0;
	tempout = 0;
	retval = Z_OK;
	while ((tempin < 2800) && (tempout < isofile->blocksize * isofile->numsectors))
	{
		strm.zalloc = (alloc_func)0;
		strm.zfree = (free_func)0;

		strm.next_in = tempblock;
		strm.next_out = buffer;
		strm.avail_in = tempin;
		strm.avail_out = 2800;

		retval = inflateInit(&strm);
		if (retval != Z_OK) return(-1);
		while ((tempin < 2800) && (retval == Z_OK))
		{
			retval = ActualFileRead(isofile->handle, 1, &tempblock[tempin]);
			if (retval != 1)
			{
#ifdef VERBOSE_FUNCTION_GZIPV2
				PrintLog("CDVDiso GZipV2:   Cannot read a byte! Returned: (%i)", retval);
#endif /* VERBOSE_FUNCTION_GZIPV2 */
				return(-1);
			} // ENDIF- Trouble reading compressed sector? Abort.
			tempin++;
			strm.avail_in++;

			strm.next_in = &tempblock[tempin - strm.avail_in];
			retval = inflate(&strm, Z_NO_FLUSH);
		} // ENDWHILE- trying to uncompress an increasingly filled buffer
		tempout = 2800 - strm.avail_out;
		inflateEnd(&strm);

	} // ENDWHILE- trying to uncompress a whole buffer
	if (retval != Z_STREAM_END)  return(-1);
	if (tempin == 2800)  return(-1);
	isofile->filebytepos += tempin;
	return(tempin); // Send out # of compressed bytes (to record in table)
} // END GZipV2Read()

int GZipV2OpenForWrite(struct IsoFile *isofile)
{
	char garbage[sizeof(struct GZipV2Header)];
	int i;
	if (isofile == NULL)  return(-1);
#ifdef VERBOSE_FUNCTION_GZIPV2
	PrintLog("CDVDiso GZipV2: OpenForWrite()");
#endif /* VERBOSE_FUNCTION_GZIPV2 */
	isofile->handle = ActualFileOpenForWrite(isofile->name);
	if (isofile->handle == ACTUALHANDLENULL)
	{
		return(-1);
	} // ENDIF- Couldn't open data file? Fail.
	for (i = 0; i < sizeof(struct GZipV2Header); i++)  garbage[i] = 0;
	ActualFileWrite(isofile->handle, sizeof(struct GZipV2Header), garbage);

	isofile->filebytesize = 0;
	isofile->filebytepos = sizeof(struct GZipV2Header);
	isofile->filesectorpos = 0;
	isofile->filesectorsize = 0;
	return(0);
} // END GZipV2OpenForWrite()

int GZipV2Write(struct IsoFile *isofile, char *buffer)
{
	int retval;
	unsigned long blocklen;
	char tempblock[2800];
#ifdef VERBOSE_FUNCTION_GZIPV2
	PrintLog("CDVDiso GZipV2: Write()");
#endif /* VERBOSE_FUNCTION_GZIPV2 */
	blocklen = 2800;
	retval = compress2(tempblock, &blocklen,
	                   buffer, isofile->blocksize,
	                   Z_BEST_COMPRESSION);
	if (retval != Z_OK)
	{
#ifdef VERBOSE_FUNCTION_GZIPV2
		PrintLog("CDVDiso GZipV2:   Cannot encode block! Returned: (%i)", retval);
#endif /* VERBOSE_FUNCTION_GZIPV2 */
		return(-1);
	} // ENDIF- Trouble compressing a block? Abort.

	retval = ActualFileWrite(isofile->handle, blocklen, tempblock);
	if (retval > 0)  isofile->filebytepos += retval;
	if (retval < blocklen)
	{
#ifdef VERBOSE_FUNCTION_GZIPV2
		PrintLog("CDVDiso GZipV2:   Cannot write bytes! Returned: (%i out of %llu)",
		         retval, blocklen);
#endif /* VERBOSE_FUNCTION_GZIPV2 */
		return(-1);
	} // ENDIF- Trouble writing out the compressed block? Abort.
	isofile->filesectorpos++;
	return(blocklen); // Not in list? Fail.
} // END GZipV2Write()

void GZipV2Close(struct IsoFile *isofile)
{
	struct GZipV2Header header;
	unsigned int tempint;
#ifdef VERBOSE_FUNCTION_GZIPV2
	PrintLog("CDVDiso GZipV2: Close()");
#endif /* VERBOSE_FUNCTION_GZIPV2 */
	if (isofile->tablehandle != ACTUALHANDLENULL)
	{
		ActualFileClose(isofile->tablehandle);
		isofile->tablehandle = ACTUALHANDLENULL;
	} // ENDIF- Is there a table file open? Close it.

	if (isofile->handle != ACTUALHANDLENULL)
	{
		if (isofile->openforread == 0)
		{
			header.id[0] = 'Z';
			header.id[1] = ' ';
			header.id[2] = 'V';
			header.id[3] = '2';
			tempint = isofile->blocksize;
			header.blocksize = ConvertEndianUInt(tempint);
			tempint = isofile->filesectorsize;
			header.numblocks = ConvertEndianUInt(tempint);
			tempint = isofile->blockoffset;
			header.blockoffset = ConvertEndianUInt(tempint);
			ActualFileSeek(isofile->handle, 0);
			ActualFileWrite(isofile->handle,
			                sizeof(struct GZipV2Header),
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
} // END GZipV2Close()
