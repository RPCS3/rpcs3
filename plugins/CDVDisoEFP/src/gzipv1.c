/*  gzipv1.c
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

#include "zlib/zlib.h"

#include "convert.h"
#include "logfile.h"
#include "isofile.h"
#include "isocompress.h" // TableData
#include "actualfile.h"
#include "gzipv1.h"

#ifdef _WIN32
#pragma pack(1)
#endif /* _WIN32 */

struct GZipV1Table
{
	unsigned int offset; // Data file position
	unsigned short size; // of Compressed data
#ifdef _WIN32
};
#else
} __attribute__((packed));
#endif /* _WIN32 */
#ifdef _WIN32
#pragma pack()
#endif /* _WIN32 */

int GZipV1OpenTableForRead(struct IsoFile *isofile)
{
	int i;
	int j;
	char tableext[] = ".table\0";
#ifdef VERBOSE_FUNCTION_GZIPV1
	PrintLog("CDVDiso GZipV1: OpenTableForRead()");
#endif /* VERBOSE_FUNCTION_GZIPV1 */
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
#ifdef VERBOSE_WARNING_GZIPV1
		PrintLog("CDVDiso GZipV1:   Couldn't open table!");
#endif /* VERBOSE_WARNING_GZIPV1 */
		return(-2);
	} // ENDIF- Couldn't open table file? Fail.

	isofile->filesectorsize = ActualFileSize(isofile->tablehandle)
	                          / sizeof(struct GZipV1Table);
	return(0);
} // END GZipV1OpenTableForRead()

int GZipV1SeekTable(struct IsoFile *isofile, off64_t sector)
{
	off64_t target;
	int retval;
#ifdef VERBOSE_FUNCTION_GZIPV1
	PrintLog("CDVDiso GZipV1: SeekTable(%lli)", sector);
#endif /* VERBOSE_FUNCTION_GZIPV1 */
	target = sector * sizeof(struct GZipV1Table);
	retval = ActualFileSeek(isofile->tablehandle, target);
	if (retval < 0)
	{
#ifdef VERBOSE_WARNING_GZIPV1
		PrintLog("CDVDiso GZipV1:   Couldn't find sector!");
#endif /* VERBOSE_WARNING_GZIPV1 */
		return(-2);
	} // ENDIF- Trouble finding the place? Fail.

	isofile->filesectorpos = sector;
	return(0);
} // END GZipV1SeekTable()

int GZipV1ReadTable(struct IsoFile *isofile, struct TableData *table)
{
	int retval;
	struct GZipV1Table temptable;
#ifdef VERBOSE_FUNCTION_GZIPV1
	PrintLog("CDVDiso GZipV1: ReadTable()");
#endif /* VERBOSE_FUNCTION_GZIPV1 */
	retval = ActualFileRead(isofile->tablehandle,
	                        sizeof(struct GZipV1Table),
	                        (char *) & temptable);
	if (retval != sizeof(struct GZipV1Table))
	{
#ifdef VERBOSE_WARNING_GZIPV1
		PrintLog("CDVDiso GZipV1:   Couldn't read table entry!");
#endif /* VERBOSE_WARNING_GZIPV1 */
		return(-2);
	} // ENDIF- Trouble reading table entry? Fail.
	table->offset = ConvertEndianUInt(temptable.offset);
	table->size = ConvertEndianUShort(temptable.size);
	isofile->filesectorpos++;
	return(0);
} // END GZipV1ReadTable()

int GZipV1OpenTableForWrite(struct IsoFile *isofile)
{
	int i;
	int j;
	char tableext[] = ".table\0";
#ifdef VERBOSE_FUNCTION_GZIPV1
	PrintLog("CDVDiso GZipV1: OpenTableForWrite()");
#endif /* VERBOSE_FUNCTION_GZIPV1 */
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
#ifdef VERBOSE_WARNING_GZIPV1
		PrintLog("CDVDiso GZipV1:   Couldn't open table!");
#endif /* VERBOSE_WARNING_GZIPV1 */
		return(-2);
	} // ENDIF- Couldn't open table file? Fail.

	isofile->filesectorsize = 0;
	return(0);
} // END GZipV1OpenTableForWrite()

int GZipV1WriteTable(struct IsoFile *isofile, struct TableData table)
{
	int retval;
	struct GZipV1Table temptable;
	unsigned int tempint;

#ifdef VERBOSE_FUNCTION_GZIPV1
	PrintLog("CDVDiso GZipV1: WriteTable(%lli, %i)", table.offset, table.size);
#endif /* VERBOSE_FUNCTION_GZIPV1 */

	tempint = table.offset;
	temptable.offset = ConvertEndianUInt(tempint);
	temptable.size = ConvertEndianUShort(table.size);
	retval = ActualFileWrite(isofile->tablehandle,
	                         sizeof(struct GZipV1Table),
	                         (char *) & temptable);
	if (retval != sizeof(struct GZipV1Table))
	{
#ifdef VERBOSE_WARNING_GZIPV1
		PrintLog("CDVDiso GZipV1:   Couldn't write table entry!");
#endif /* VERBOSE_WARNING_GZIPV1 */
		return(-2);
	} // ENDIF- Trouble reading table entry? Fail.
	return(0);
} // END GZipV1WriteTable()

int GZipV1OpenForRead(struct IsoFile *isofile)
{
	int retval;
	char tempblock[2448];
#ifdef VERBOSE_FUNCTION_GZIPV1
	PrintLog("CDVDiso GZipV1: OpenForRead()");
#endif /* VERBOSE_FUNCTION_GZIPV1 */
	isofile->handle = ActualFileOpenForRead(isofile->name);
	if (isofile->handle == ACTUALHANDLENULL)
	{
		return(-1);
	} // ENDIF- Couldn't open data file? Fail.

	isofile->filebytesize = ActualFileSize(isofile->handle);
	isofile->filebytepos = 0;
	isofile->filesectorpos = 0;

	isofile->imageheader = 0;
	isofile->blocksize = 2352;
	isofile->blockoffset = 0; // Don't panic. "imagetype.c" will test later.
	isofile->numsectors = 1; // Sectors per block
	retval = GZipV1Read(isofile, 2448, tempblock);
	if (retval != 0)
	{
#ifdef VERBOSE_WARNING_GZIPV1
		PrintLog("CDVDiso GZipV1:   Couldn't decode the first block. Not compressed?");
#endif /* VERBOSE_WARNING_GZIPV1 */
		ActualFileClose(isofile->handle);
		isofile->handle = ACTUALHANDLENULL;
		return(-1);
	} // ENDIF Could not read the first sector? Fail.
	ActualFileSeek(isofile->handle, 0); // Restart at top of file
	isofile->filebytepos = 0;
	isofile->filesectorpos = 0;
	return(0);
} // END GZipV1OpenForRead()

int GZipV1Seek(struct IsoFile *isofile, off64_t position)
{
	int retval;
#ifdef VERBOSE_FUNCTION_GZIPV1
	PrintLog("CDVDiso GZipV1: Seek(%lli)", position);
#endif /* VERBOSE_FUNCTION_GZIPV1 */
	retval = ActualFileSeek(isofile->handle, position);
	if (retval < 0)
	{
#ifdef VERBOSE_WARNING_GZIPV1
		PrintLog("CDVDiso GZipV1:   Couldn't find the start of the compressed block!");
#endif /* VERBOSE_WARNING_GZIPV1 */
		return(-1);
	} // ENDIF- Couldn't find the data entry? Fail.
	isofile->filebytepos = position;
	return(0);
	return(-1); // Fail. (Due to lack of ambition?)
} // END GZipV1Seek()

int GZipV1Read(struct IsoFile *isofile, int bytes, char *buffer)
{
	int retval;
	unsigned long blocklen;
	z_stream strm;
	unsigned long tempin;
	char tempblock[2800];
	unsigned long tempout;
#ifdef VERBOSE_FUNCTION_GZIPV1
	PrintLog("CDVDiso GZipV1: Read(%i)", bytes);
#endif /* VERBOSE_FUNCTION_GZIPV1 */
	if (bytes > 0)
	{
		retval = ActualFileRead(isofile->handle, bytes, tempblock);
		if (retval > 0)  isofile->filebytepos += retval;
		if (retval != bytes)
		{
#ifdef VERBOSE_WARNING_GZIPV1
			PrintLog("CDVDiso GZipV1:   Cannot read bytes! Returned: (%i)", retval);
#endif /* VERBOSE_WARNING_GZIPV1 */
			return(-1);
		} // ENDIF- Trouble reading compressed sector? Abort.
		blocklen = isofile->blocksize;
		retval = uncompress(buffer, &blocklen, tempblock, bytes);
		if (retval != Z_OK)
		{
#ifdef VERBOSE_WARNING_GZIPV1
			PrintLog("CDVDiso GZipV1:   Cannot decode block! Returned: (%i)", retval);
#endif /* VERBOSE_WARNING_GZIPV1 */
			return(-1);
		} // ENDIF- Trouble decoding the sector? Abort.

		return(0);
	} // ENDIF- Do we know how many compressed bytes to get for this record?
	// Hmm. Don't know the compressed size? Well, we'll just have to find it.
	tempin = 0;
	tempout = 0;
	retval = Z_OK;
	while ((tempin < 2800) && (tempout < 2352))
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
#ifdef VERBOSE_WARNING_GZIPV1
				PrintLog("CDVDiso GZipV1:   Cannot read a byte! Returned: (%i)", retval);
#endif /* VERBOSE_WARNING_GZIPV1 */
				return(-1);
			} // ENDIF- Trouble reading compressed sector? Abort.
			tempin++;
			strm.avail_in++;
			strm.next_in = &tempblock[tempin - strm.avail_in];
			retval = inflate(&strm, Z_NO_FLUSH);
		} // ENDWHILE- trying to uncompress an increasingly filled buffer
		tempout = strm.total_out;
		inflateEnd(&strm);
#ifdef VERBOSE_FUNCTION_GZIPV1
		PrintLog("CDVDiso GZipV1:   tempin=%lu   tempout=%lu   retval=%i",
		         tempin, tempout, retval);
#endif /* VERBOSE_FUNCTION_GZIPV1 */
	} // ENDWHILE- trying to uncompress a whole buffer
	if (retval != Z_STREAM_END)
	{
#ifdef VERBOSE_WARNING_GZIPV1
		PrintLog("CDVDiso GZipV1:   Failed to decode block! Returned: (%i)", retval);
#endif /* VERBOSE_WARNING_GZIPV1 */
		return(-1);
	} // ENDIF- Trouble reading compressed sector? Abort.
	if (tempin == 2800)
	{
#ifdef VERBOSE_WARNING_GZIPV1
		PrintLog("CDVDiso GZipV1:   Overfilled input buffer for only %llu bytes!", tempout);
#endif /* VERBOSE_WARNING_GZIPV1 */
		return(-1);
	} // ENDIF- Trouble reading compressed sector? Abort.
	isofile->filebytepos += tempin;
	return(tempin); // Send out # of compressed bytes (to record in table)
} // END GZipV1Read()

int GZipV1OpenForWrite(struct IsoFile *isofile)
{
	if (isofile == NULL)  return(-1);
#ifdef VERBOSE_FUNCTION_GZIPV1
	PrintLog("CDVDiso GZipV1: OpenForWrite()");
#endif /* VERBOSE_FUNCTION_GZIPV1 */
	isofile->handle = ActualFileOpenForWrite(isofile->name);
	if (isofile->handle == ACTUALHANDLENULL)
	{
		return(-1);
	} // ENDIF- Couldn't open data file? Fail.

	isofile->filebytesize = 0;
	isofile->filebytepos = 0;
	return(0);
} // END GZipV1OpenForWrite()

int GZipV1Write(struct IsoFile *isofile, char *buffer)
{
	int retval;
	unsigned long blocklen;
	char tempblock[2800];
#ifdef VERBOSE_FUNCTION_GZIPV1
	PrintLog("CDVDiso GZipV1: Write()");
#endif /* VERBOSE_FUNCTION_GZIPV1 */
	blocklen = 2800;
	retval = compress2(tempblock, &blocklen,
	                   buffer, 2352,
	                   Z_BEST_COMPRESSION);
	if (retval != Z_OK)
	{
#ifdef VERBOSE_FUNCTION_GZIPV1
		PrintLog("CDVDiso GZipV1:   Cannot encode block! Returned: (%i)", retval);
#endif /* VERBOSE_FUNCTION_GZIPV1 */
		return(-1);
	} // ENDIF- Trouble compressing a block? Abort.

	retval = ActualFileWrite(isofile->handle, blocklen, tempblock);
	if (retval > 0)  isofile->filebytepos += retval;
	if (retval < blocklen)
	{
#ifdef VERBOSE_FUNCTION_GZIPV1
		PrintLog("CDVDiso GZipV1:   Cannot write bytes! Returned: (%i out of %llu)",
		         retval, blocklen);
#endif /* VERBOSE_FUNCTION_GZIPV1 */
		return(-1);
	} // ENDIF- Trouble writing out the compressed block? Abort.
	isofile->filesectorpos++;
	return(blocklen);
} // END GZipV1Write()

void GZipV1Close(struct IsoFile *isofile)
{
#ifdef VERBOSE_FUNCTION_GZIPV1
	PrintLog("CDVDiso GZipV1: Close()");
#endif /* VERBOSE_FUNCTION_GZIPV1 */

	// Flush Write data... if any was held in the compression block area.
	// In this case, though... nothing's held there.
	if (isofile->tablehandle != ACTUALHANDLENULL)
	{
		ActualFileClose(isofile->tablehandle);
		isofile->tablehandle = ACTUALHANDLENULL;
	} // ENDIF- Is there a table file open? Close it.

	if (isofile->handle != ACTUALHANDLENULL)
	{
		ActualFileClose(isofile->handle);
		isofile->handle = ACTUALHANDLENULL;
	} // ENDIF- Is there a data file open? Close it.
	return;
} // END GZipV1Close()
