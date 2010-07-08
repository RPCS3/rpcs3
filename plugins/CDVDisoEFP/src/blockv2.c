/*  blockv2.c
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
#include "blockv2.h"

#ifdef _WIN32
#pragma pack(1)
#endif /* _WIN32 */

struct BlockV2Header
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
#ifdef _WIN32
#pragma pack()
#endif /* _WIN32 */

int BlockV2OpenTableForRead(struct IsoFile *isofile)
{
	int i;
	int j;
	off64_t offset;
	int tableoffset;
	int retval;
	union TableMap tablemap;

#ifdef VERBOSE_FUNCTION_BLOCKV2
	PrintLog("CDVDiso BlockV2: OpenTableForRead()");
#endif /* VERBOSE_FUNCTION_BLOCKV2 */

	// We pre-read the WHOLE offset table.
	isofile->tabledata = (char *) malloc(isofile->filesectorsize * sizeof(struct TableData));
	if (isofile->tabledata == NULL)
	{
#ifdef VERBOSE_WARNING_BLOCKV2
		PrintLog("CDVDiso BlockV2:   Couldn't allocate internal table!");
#endif /* VERBOSE_WARNING_BLOCKV2 */
		return(-1);
	} // ENDIF- Could not get enough memory to hold table data
	offset = sizeof(struct BlockV2Header);
	tableoffset = 0;
	for (i = 0; i < isofile->filesectorsize; i++)
	{
		retval = BlockV2Seek(isofile, offset);
		if (retval != 0)
		{
#ifdef VERBOSE_WARNING_BLOCKV2
			PrintLog("CDVDiso BlockV2:   Failed to find sector %i!", i);
#endif /* VERBOSE_WARNING_BLOCKV2 */
			return(-1);
		} // ENDIF- Trouble finding a lsn id? Fail.

		retval = ActualFileRead(isofile->handle,
		                        sizeof(int),
		                        (char *) & j);
		if (retval != sizeof(int))
		{
#ifdef VERBOSE_WARNING_BLOCKV2
			PrintLog("CDVDiso BlockV2:   Failed to read in sector %i!", i);
#endif /* VERBOSE_WARNING_BLOCKV2 */
			return(-1);
		} // ENDIF- Trouble reading in a lsn id? Table damaged... fail.

		tablemap.table.offset = ConvertEndianUInt(j); // Actually, a lsn.
		for (j = 0; j < sizeof(struct TableData); j++)
			*(isofile->tabledata + tableoffset + j) = tablemap.ch[j];
		offset += isofile->blocksize + 4;
		tableoffset += sizeof(struct TableData);
	} // NEXT i- reading in the sizes, and making offset as I go.
	isofile->tablehandle = ACTUALHANDLENULL;
	return(0);
} // END BlockV2OpenTableForRead()

int BlockV2SeekTable(struct IsoFile *isofile, off64_t sector)
{
	int i;
	int tableoffset;
	union TableMap tablemap;
	off64_t filesectorstart;
#ifdef VERBOSE_FUNCTION_BLOCKV2
	PrintLog("CDVDiso BlockV2: SeekTable(%lli)", sector);
#endif /* VERBOSE_FUNCTION_BLOCKV2 */
	if ((isofile->filesectorpos >= 0) &&
	        (isofile->filesectorpos < isofile->filesectorsize))
	{
		tableoffset = isofile->filesectorpos * sizeof(struct TableData);
		for (i = 0; i < sizeof(struct TableData); i++)
			tablemap.ch[i] = *(isofile->tabledata + tableoffset + i);
		if (sector == tablemap.table.offset)
		{
			return(0);
		} // ENDIF- Are we already pointing at the right sector?

	}
	else
	{
		isofile->filesectorpos = 0;
		tablemap.table.offset = -1;
	} // ENDIF- Is the file sector pointer within table limits?
	filesectorstart = isofile->filesectorpos;
	isofile->filesectorpos++;
	if (isofile->filesectorpos >= isofile->filesectorsize)
		isofile->filesectorpos = 0;
	while ((isofile->filesectorpos != filesectorstart) &&
	        (tablemap.table.offset != sector))
	{
		tableoffset = isofile->filesectorpos * sizeof(struct TableData);
		for (i = 0; i < sizeof(struct TableData); i++)
			tablemap.ch[i] = *(isofile->tabledata + tableoffset + i);
		if (tablemap.table.offset != sector)
		{
			isofile->filesectorpos++;
			if (isofile->filesectorpos >= isofile->filesectorsize)
				isofile->filesectorpos = 0;
		} // ENDIF- Still didn't find it? move to next sector.
	} // ENDWHILE- Scanning through whole sector list (starting at current pos)

	if (isofile->filesectorpos == filesectorstart)
	{
		return(-1);
	} // ENDIF- Did we loop through the whole file... and not find this sector?

	return(0);
} // END BlockV2SeekTable()

int BlockV2ReadTable(struct IsoFile *isofile, struct TableData *table)
{
#ifdef VERBOSE_FUNCTION_BLOCKV2
	PrintLog("CDVDiso BlockV2: ReadTable()");
#endif /* VERBOSE_FUNCTION_BLOCKV2 */
	table->offset = sizeof(int) + isofile->blocksize;
	table->offset *= isofile->filesectorpos;
	table->offset += sizeof(struct BlockV2Header) + 4;
	table->size = isofile->blocksize;
	isofile->filesectorpos++;
	return(0);
} // END BlockV2ReadTable()

int BlockV2OpenTableForWrite(struct IsoFile *isofile)
{
	return(-1);
} // END BlockV2OpenTableForWrite()

int BlockV2WriteTable(struct IsoFile *isofile, struct TableData table)
{
	return(-1);
} // END BlockV2WriteTable()

int BlockV2OpenForRead(struct IsoFile *isofile)
{
	int retval;
	struct BlockV2Header header;

#ifdef VERBOSE_FUNCTION_BLOCKV2
	PrintLog("CDVDiso BlockV2: OpenForRead()");
#endif /* VERBOSE_FUNCTION_BLOCKV2 */

	isofile->handle = ActualFileOpenForRead(isofile->name);
	if (isofile->handle == ACTUALHANDLENULL)
	{
		return(-1);
	} // ENDIF- Couldn't open data file? Fail.
	isofile->filebytesize = ActualFileSize(isofile->handle);
	isofile->filebytepos = 0;

	isofile->imageheader = 0;
	isofile->numsectors = 1; // Sectors per block
	retval = ActualFileRead(isofile->handle,
	                        sizeof(struct BlockV2Header),
	                        (char *) & header);
	if (retval != sizeof(struct BlockV2Header))
	{
#ifdef VERBOSE_WARNING_BLOCKV2
		PrintLog("CDVDiso BlockV2:   Couldn't read header!");
#endif /* VERBOSE_WARNING_BLOCKV2 */
		ActualFileClose(isofile->handle);
		isofile->handle = ACTUALHANDLENULL;
		return(-1);
	} // ENDIF Could not read the first sector? Fail.
	isofile->filebytepos += retval;

	if ((header.id[0] != 'B') ||
	        (header.id[1] != 'D') ||
	        (header.id[2] != 'V') ||
	        (header.id[3] != '2'))
	{
#ifdef VERBOSE_WARNING_BLOCKV2
		PrintLog("CDVDiso BlockV2:   Not a block dump v2 header!");
#endif /* VERBOSE_WARNING_BLOCKV2 */
		ActualFileClose(isofile->handle);
		isofile->handle = ACTUALHANDLENULL;
		return(-1);
	} // ENDIF- ID for this compression type doesn't match?

	isofile->blocksize = ConvertEndianUInt(header.blocksize);
	// isofile->filesectorsize = ConvertEndianUInt(header.numblocks);
	isofile->blockoffset = ConvertEndianUInt(header.blockoffset);
	isofile->filesectorsize = isofile->filebytesize;
	isofile->filesectorsize -= 16;
	isofile->filesectorsize /= (isofile->blocksize + sizeof(int));
	isofile->filesectorpos = 0;
	return(0);
} // END BlockV2OpenForRead()

int BlockV2Seek(struct IsoFile *isofile, off64_t position)
{
	int retval;

#ifdef VERBOSE_FUNCTION_BLOCKV2
	PrintLog("CDVDiso BlockV2: Seek(%lli)", position);
#endif /* VERBOSE_FUNCTION_BLOCKV2 */

	retval = ActualFileSeek(isofile->handle, position);
	if (retval < 0)
	{
#ifdef VERBOSE_WARNING_BLOCKV2
		PrintLog("CDVDiso BlockV2:   Couldn't find the start of the block!");
#endif /* VERBOSE_WARNING_BLOCKV2 */
		return(-1);
	} // ENDIF- Couldn't find the data entry? Fail.
	isofile->filebytepos = position;
	return(0);

	return(-1); // Fail. (Due to lack of ambition?)
} // END BlockV2Seek()

int BlockV2Read(struct IsoFile *isofile, int bytes, char *buffer)
{
	int retval;
#ifdef VERBOSE_FUNCTION_BLOCKV2
	PrintLog("CDVDiso BlockV2: Read(%i)", bytes);
#endif /* VERBOSE_FUNCTION_BLOCKV2 */
	retval = ActualFileRead(isofile->handle, isofile->blocksize, buffer);
	if (retval > 0)  isofile->filebytepos += retval;
	if (retval != isofile->blocksize)
	{
#ifdef VERBOSE_WARNING_BLOCKV2
		PrintLog("CDVDiso BlockV2:   Cannot read bytes! Returned: (%i)", retval);
#endif /* VERBOSE_WARNING_BLOCKV2 */
		return(-1);
	} // ENDIF- Trouble reading compressed sector? Abort.

	return(isofile->blocksize);
} // END BlockV2Read()

int BlockV2OpenForWrite(struct IsoFile *isofile)
{
	return(-1);
} // END BlockV2OpenForWrite()

int BlockV2Write(struct IsoFile *isofile, char *buffer)
{
	return(-1);
} // END BlockV2Write()

void BlockV2Close(struct IsoFile *isofile)
{
#ifdef VERBOSE_FUNCTION_BLOCKV2
	PrintLog("CDVDiso BlockV2: Close()");
#endif /* VERBOSE_FUNCTION_BLOCKV2 */
	if (isofile->handle != ACTUALHANDLENULL)
	{
		ActualFileClose(isofile->handle);
		isofile->handle = ACTUALHANDLENULL;
	} // ENDIF- Is there a data file open? Close it.

	if (isofile->tabledata != NULL)
	{
		free(isofile->tabledata);
		isofile->tabledata = NULL;
	} // ENDIF- Do we have a read-in table to clear out?
	return;
} // END BlockV2Close()
