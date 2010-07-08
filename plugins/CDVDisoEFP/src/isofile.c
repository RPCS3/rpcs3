/*  isofile.c
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

#ifndef __LINUX__
#ifdef __linux__
#define __LINUX__
#endif /* __linux__ */
#endif /* No __LINUX__ */
#define CDVDdefs
#include "PS2Edefs.h"

#include "logfile.h"
#include "multifile.h"
#include "isocompress.h"
#include "actualfile.h"
#include "imagetype.h"
#include "toc.h"
#include "ecma119.h"
#include "isofile.h"

const char *isofileext[] =
{
	".iso",
	".bin",
	".img",
	NULL
};
const char *cdname = "CD-XA001\0";
const char *playstationid = "PLAYSTATION\0";
// const char ps1/2name?
// Internal functions
void IsoNameStripExt(struct IsoFile *isofile)
{
	int tempext;
	int tempnamepos;
	int tempextpos;

	tempext = 0;
	while (isofileext[tempext] != NULL)
	{
		tempextpos = 0;
		while (*(isofileext[tempext] + tempextpos) != 0)  tempextpos++;
		tempnamepos = isofile->namepos;
		while ((tempnamepos > 0) && (tempextpos > 0) &&
		        (isofile->name[tempnamepos - 1] == *(isofileext[tempext] + tempextpos - 1)))
		{
			tempnamepos--;
			tempextpos--;
		} // ENDWHILE- Comparing one extension to the end of the file name
		if (tempextpos == 0)
		{
			isofile->namepos = tempnamepos; // Move name pointer in front of ext.
			tempext = 0; // ... and test the list all over again.
		}
		else
		{
			tempext++; // Next ext in the list to test...
		} // ENDIF- Did we find a match?
	} // ENDWHILE- looking through extension list
} // END IsoNameStripExt()

// External functions
struct IsoFile *IsoFileOpenForRead(const char *filename)
{
	struct IsoFile *newfile;
	int retval;
	int i;
	char tempblock[2448];
	struct tocTN toctn;
	struct tocTD toctd;
//   union {
//     struct ECMA119PrimaryVolume vol;
//     char ch[sizeof(struct ECMA119PrimaryVolume)];
//   } *volcheck;
	union
	{
		struct ECMA119PrimaryVolume *vol;
		char *ch;
	} volcheck;
	newfile = NULL;
	if (filename == NULL)  return(NULL);
#ifdef VERBOSE_FUNCTION_ISOFILE
	PrintLog("CDVD isofile: IsoFileOpenForRead(%s)", filename);
#endif /* VERBOSE_FUNCTION_ISOFILE */
	newfile = (struct IsoFile *) malloc(sizeof(struct IsoFile));
	if (newfile == NULL)  return(NULL);

	newfile->sectorpos = 0;
	newfile->openforread = 1; // Read-only ISO
	newfile->filebytepos = 0;
	newfile->filesectorpos = 0;
	newfile->blocksize = 0; // Flags as non-detected yet (Compress vs. Image)
	newfile->tabledata = NULL;
	newfile->namepos = 0;
	while ((newfile->namepos < 255) &&
	        (*(filename + newfile->namepos) != 0))
	{
		newfile->name[newfile->namepos] = *(filename + newfile->namepos);
		newfile->namepos++;
	} // ENDWHILE- copying the file name in...
	newfile->name[newfile->namepos] = 0; // And 0-terminate.
	IsoNameStripExt(newfile); // Ex: -I00.Z[.bin]
	// File Compression name detection
	newfile->compress = IsoNameStripCompress(newfile); // Ex: -I00.bin[.Z]
	newfile->compresspos = newfile->namepos;
	// Test File name compression
	retval = -1;
	if (newfile->compress > 0)
	{
		retval = CompressOpenForRead(newfile);
		if (retval == -1)  CompressClose(newfile);
	} // ENDIF- Have a compression type hint? Test it out

	if (retval == -1)
	{
		newfile->compress = 5;
		while ((newfile->compress > 0) && (retval == -1))
		{
			retval = CompressOpenForRead(newfile);
			if (retval == -1)
			{
				CompressClose(newfile);
				newfile->compress--;
			} // ENDIF- Failed to open? Close it... and try the next one.
		} // ENDWHILE- Trying to find a compression scheme that will work...

		if (newfile->compress == 0)
		{
			newfile->handle = ActualFileOpenForRead(newfile->name);
			if (newfile->handle == ACTUALHANDLENULL)
			{
				free(newfile);
				newfile = NULL;
				return(NULL);
			} // ENDIF- Failed to open? Abort.
			newfile->filebytesize = ActualFileSize(newfile->handle);
		} // ENDIF- No compression? Open it uncompressed.
	} // ENDIF- Temp- failed to open? Abort...
	// Compressed data file with no table? Return prematurely...
	// Condition detection: compress > 0, tablehandle == ACTUALHANDLENULL
	if (retval == -2)
	{
#ifdef VERBOSE_FUNCTION_ISOFILE
		PrintLog("CDVD isofile:   Data file with no table!");
#endif /* VERBOSE_FUNCTION_ISOFILE */
		return(newfile);
	} // ENDIF-

	newfile->imagetype = DetectImageType(newfile);

	if (newfile->compress == 0)
	{
		newfile->filesectorsize = newfile->filebytesize / newfile->blocksize;
	} // ENDIF- Now that blocksize is known, raw file sectors can be figured out

	IsoNameStripExt(newfile); // Ex: -I00[.bin].Z

	IsoNameStripMulti(newfile); // Ex: [-I00].bin.Z

#ifdef VERBOSE_DISC_INFO
	PrintLog("CDVD isofile:   Filename: %s", filename);
	if (newfile->multi > 0)  PrintLog("CDVD isofile:   Multiple <2GB files.");
	PrintLog("CDVD isofile:   Compression Method: %s",
	         compressdesc[newfile->compress]);
	PrintLog("CDVD isofile:   Image Type: %s",
	         newfile->imagename);
	PrintLog("CDVD isofile:   Block Size: %lli", newfile->blocksize);
	PrintLog("CDVD isofile:   Total Sectors (of first file): %lli",
	         newfile->filesectorsize);
#endif /* VERBOSE_DISC_INFO */

	// Load a TOC from a .toc file (is there is one)
	retval = IsoLoadTOC(newfile);
	if (retval == 0)  return(newfile);

	// Get the volume sector for disc type test
	retval = IsoFileSeek(newfile, 16);
	if (retval < 0)
	{
		newfile = IsoFileClose(newfile);
		return(NULL);
	} // ENDIF- Could not find the directory sector? Abort.
	retval = IsoFileRead(newfile, tempblock);
	if (retval < 0)
	{
		newfile = IsoFileClose(newfile);
		return(NULL);
	} // ENDIF- Could not read the directory sector? Abort.

	volcheck.ch = tempblock;
	volcheck.ch += newfile->blockoffset;
	if (ValidateECMA119PrimaryVolume(volcheck.vol) != 0)
	{
#ifdef VERBOSE_DISC_INFO
		PrintLog("CDVD isofile:   Not an ISO9660 disc! Music CD perhaps?");
#endif /* VERBOSE_DISC_INFO */
		newfile->cdvdtype = CDVD_TYPE_CDDA;

	}
	else
	{
		// Is this a playstation image?
		i = 0;
		while ((*(playstationid + i) != 0) &&
		        (*(playstationid + i) == tempblock[newfile->blockoffset + 8 + i]))  i++;
		if (*(playstationid + i) != 0)
		{
#ifdef VERBOSE_DISC_INFO
			PrintLog("CDVD isofile:   Not a Playstation Disc!");
#endif /* VERBOSE_DISC_INFO */
			newfile->cdvdtype = CDVD_TYPE_DVDV;
		}
		else
		{
			newfile->cdvdtype = CDVD_TYPE_PS2DVD;
		} // ENDIF- Is this not a Playstation 1 image?
		// Sidenote: if the emulator is just playing Playstation 2 images, we could
		// just invalidate the image file right here.
	} // ENDIF- Not an ISO9660 disc? Assume Music CD.
	if (newfile->cdvdtype == CDVD_TYPE_PS2DVD)
	{
		// Is this a Playstation CD image?
		i = 0;
		while ((*(cdname + i) != 0) &&
		        (*(cdname + i) == tempblock[newfile->blockoffset + 1024 + i]))  i++;
		if (*(cdname + i) == 0)
		{
			newfile->cdvdtype = CDVD_TYPE_PSCD;
#ifdef VERBOSE_DISC_INFO
			PrintLog("CDVD isofile:   Image is a Playstation 1 CD.");
#endif /* VERBOSE_DISC_INFO */
		}
		else
		{
			if (newfile->blocksize != 2048)
			{
				newfile->cdvdtype = CDVD_TYPE_PS2CD;
#ifdef VERBOSE_DISC_INFO
				PrintLog("CDVD isofile:   Image is a Playstation 2 CD.");
#endif /* VERBOSE_DISC_INFO */
			}
			else
			{
#ifdef VERBOSE_DISC_INFO
				PrintLog("CDVD isofile:   Image is a DVD.");
#endif /* VERBOSE_DISC_INFO */
			} // ENDIF- Is the blocksize not 2048? CD image then.
		} // ENDIF- Is this a PS1 CD image?
	} // ENDIF- Is this a Playstation image?
	volcheck.ch = NULL;

	if ((newfile->cdvdtype == CDVD_TYPE_DVDV) &&
	        (newfile->blocksize == 2352))  newfile->cdvdtype = CDVD_TYPE_CDDA;
	// Slap together a TOC based on the above guesswork.
	IsoInitTOC(newfile);
	if ((newfile->cdvdtype != CDVD_TYPE_PS2DVD) &&
	        (newfile->cdvdtype != CDVD_TYPE_DVDV))
	{
		toctn.strack = 1;
		toctn.etrack = 1;
		IsoAddTNToTOC(newfile, toctn);
		toctd.type = 0;
		toctd.lsn = newfile->filesectorsize;
		IsoAddTDToTOC(newfile, 0xAA, toctd);
		toctd.type = 0; // ?
		if (newfile->cdvdtype == CDVD_TYPE_CDDA)
		{
			toctd.type = CDVD_AUDIO_TRACK; // Music track assumed
		}
		else
		{
			toctd.type = CDVD_MODE1_TRACK; // Data track assumed
		} // ENDIF- Is this track a music or data track?
		toctd.lsn = 0;
		IsoAddTDToTOC(newfile, 1, toctd);
	} // ENDIF- Is this a CD? Single track for all sectors
	return(newfile);
} // END IsoFileOpenForRead()

int IsIsoFile(const char *filename)
{
	int retval;
	struct IsoFile *tempfile;
#ifdef VERBOSE_FUNCTION_ISOFILE
	PrintLog("CDVD isofile: IsIsoFile()");
#endif /* VERBOSE_FUNCTION_ISOFILE */
	retval = IsActualFile(filename);
	if (retval < 0)  return(retval); // Not a regular file? Report it.

	tempfile = NULL;
	tempfile = IsoFileOpenForRead(filename);
	if (tempfile == NULL)  return(-3); // Not an image file? Report it.

	retval = 0;
	if ((tempfile->compress > 0) &&
	        (tempfile->tablehandle == ACTUALHANDLENULL) &&
	        (tempfile->tabledata == NULL))  retval = -4;
	tempfile = IsoFileClose(tempfile);
	return(retval);
} // END IsIsoFile()

int IsoFileSeek(struct IsoFile *file, off64_t sector)
{
	int retval;
	if (file == NULL)  return(-1);
	if (sector < 0)  return(-1);

	if (sector == file->sectorpos)  return(0);

#ifdef VERBOSE_FUNCTION_ISOFILE
	PrintLog("CDVD isofile: IsoFileSeek(%llu)", sector);
#endif /* VERBOSE_FUNCTION_ISOFILE */

	if (file->multi > 0)
	{
		retval = MultiFileSeek(file, sector);
	}
	else if (file->compress > 0)
	{
		retval = CompressSeek(file, sector);
	}
	else
	{
		retval = ActualFileSeek(file->handle,
		                        (sector * file->blocksize) + file->imageheader);
		if (retval == 0)
		{
			file->filesectorpos = sector;
			file->filebytepos = (sector * file->blocksize) + file->imageheader;
		} // ENDIF- Succeeded? Adjust internal pointers
	} // ENDLONGIF- Seek right file? Or compressed block? Or Raw block?
	if (retval < 0)
	{
#ifdef VERBOSE_FUNCTION_ISOFILE
		PrintLog("CDVD isofile:   Trouble finding the sector!");
#endif /* VERBOSE_FUNCTION_ISOFILE */
		return(-1);
	} // ENDIF- Trouble reading the block? Say so!

	file->sectorpos = sector;
	return(0);
} // END IsoFileSeek()

int IsoFileRead(struct IsoFile *file, char *block)
{
	int retval;

	if (file == NULL)  return(-1);
	if (block == NULL)  return(-1);
	if (file->openforread == 0)  return(-1);

#ifdef VERBOSE_FUNCTION_ISOFILE
	PrintLog("CDVD isofile: IsoFileRead(%i)", file->blocksize);
#endif /* VERBOSE_FUNCTION_ISOFILE */

	if (file->multi > 0)
	{
		retval = MultiFileRead(file, block);
	}
	else if (file->compress > 0)
	{
		retval = CompressRead(file, block);
	}
	else
	{
		if (file->sectorpos >= file->filesectorsize)  return(-1);
		retval = ActualFileRead(file->handle,
		                        file->blocksize,
		                        block);
		if (retval > 0)  file->filebytepos += retval;
		if (retval == file->blocksize)  file->filesectorpos++;
	} // ENDLONGIF- Read right file? Or compressed block? Or Raw block?
	if (retval < 0)
	{
#ifdef VERBOSE_FUNCTION_ISOFILE
		PrintLog("CDVD isofile:   Trouble reading the sector!");
#endif /* VERBOSE_FUNCTION_ISOFILE */
		return(-1);
	} // ENDIF- Trouble reading the block? Say so!

	if (retval < file->blocksize)
	{
#ifdef VERBOSE_FUNCTION_ISOFILE
		PrintLog("CDVD isofile:   Short block! Got %i out of %i bytes",
		         retval, file->blocksize);
#endif /* VERBOSE_FUNCTION_ISOFILE */
		return(-1);
	} // ENDIF- Didn't get enough bytes? Say so!

	file->sectorpos++;
	return(0);
} // END IsoFileRead()

struct IsoFile *IsoFileClose(struct IsoFile *file)
{
	if (file == NULL)  return(NULL);

	if (file->handle != ACTUALHANDLENULL)
	{
#ifdef VERBOSE_FUNCTION_ISOFILE
		PrintLog("CDVD isofile: IsoFileClose()");
#endif /* VERBOSE_FUNCTION_ISOFILE */
		if (file->compress > 0)
		{
			CompressClose(file);
		}
		else
		{
			ActualFileClose(file->handle);
			file->handle = ACTUALHANDLENULL;
		} // ENDIF- Compressed File? Close (and flush) compression too.
	} // ENDIF- Open Handle? Close the file
	free(file);
	return(NULL);
} // END IsoFileClose()

struct IsoFile *IsoFileOpenForWrite(const char *filename,
			                                    int imagetype,
			                                    int multi,
			                                    int compress)
{
	struct IsoFile *newfile;

	newfile = NULL;

	if (filename == NULL)  return(NULL);
	if ((imagetype < 0) || (imagetype > 11))  return(NULL);
	if ((compress < 0) || (compress > 5))  return(NULL);

#ifdef VERBOSE_FUNCTION_ISOFILE
	PrintLog("CDVD isofile: IsoFileOpenForWrite()");
#endif /* VERBOSE_FUNCTION_ISOFILE */

	newfile = (struct IsoFile *) malloc(sizeof(struct IsoFile));
	if (newfile == NULL)  return(NULL);
	newfile->sectorpos = 0;
	newfile->openforread = 0; // Write-only file
	newfile->filebytesize = 0;
	newfile->filebytepos = 0;
	newfile->filesectorsize = 0;
	newfile->filesectorpos = 0;

	// if(toc != NULL) {
	//   for(i = 0; i < 2048; i++)  newfile->toc[i] = *(toc + i);
	// } else {
	//   for(i = 0; i < 2048; i++)  newfile->toc[i] = 0;
	// } // ENDIF- Do we have a PS2 Table of Contents to save out as well?

	newfile->namepos = 0;
	while ((newfile->namepos < 255) &&
	        (*(filename + newfile->namepos) != 0))
	{
		newfile->name[newfile->namepos] = *(filename + newfile->namepos);
		newfile->namepos++;
	} // ENDWHILE- copying the file name in...
	newfile->name[newfile->namepos] = 0; // And 0-terminate.

	IsoNameStripExt(newfile);
	IsoNameStripCompress(newfile);
	IsoNameStripExt(newfile);
	IsoNameStripMulti(newfile);
	newfile->name[newfile->namepos] = 0; // And 0-terminate.

	newfile->imagetype = imagetype;
	GetImageType(newfile, imagetype);
	newfile->cdvdtype = CDVD_TYPE_PS2DVD; // Does it matter here? Nope.

	newfile->multi = multi;
	if (newfile->multi > 0)
	{
		newfile->name[newfile->namepos + 0] = '-';
		newfile->name[newfile->namepos + 1] = 'I';
		newfile->name[newfile->namepos + 2] = '0';
		newfile->name[newfile->namepos + 3] = '0';
		newfile->name[newfile->namepos + 4] = 0;
		newfile->multipos = newfile->namepos + 3;
		newfile->namepos += 4;
		newfile->multistart = 0;
		newfile->multiend = 0;
		newfile->multinow = 0;
		newfile->multioffset = 0;
		newfile->multisectorend[0] = 0;
	} // ENDIF- Are we creating a multi-file?

	newfile->compress = compress;
	switch (newfile->compress)
	{
		case 1:
		case 3:
			newfile->name[newfile->namepos + 0] = '.';
			newfile->name[newfile->namepos + 1] = 'Z';
			newfile->name[newfile->namepos + 2] = 0;
			newfile->namepos += 2;
			break;

		case 2:
			newfile->name[newfile->namepos + 0] = '.';
			newfile->name[newfile->namepos + 1] = 'd';
			newfile->name[newfile->namepos + 2] = 'u';
			newfile->name[newfile->namepos + 3] = 'm';
			newfile->name[newfile->namepos + 4] = 'p';
			newfile->name[newfile->namepos + 5] = 0;
			newfile->namepos += 5;
			break;

		case 4:
			newfile->name[newfile->namepos + 0] = '.';
			newfile->name[newfile->namepos + 1] = 'B';
			newfile->name[newfile->namepos + 2] = 'Z';
			newfile->name[newfile->namepos + 3] = '2';
			newfile->name[newfile->namepos + 4] = 0;
			newfile->namepos += 4;
			break;
		case 5:
			newfile->name[newfile->namepos + 0] = '.';
			newfile->name[newfile->namepos + 1] = 'b';
			newfile->name[newfile->namepos + 2] = 'z';
			newfile->name[newfile->namepos + 3] = '2';
			newfile->name[newfile->namepos + 4] = 0;
			newfile->namepos += 4;
			break;

		case 0:
		default:
			break;
	} // ENDSWITCH compress- which compression extension should we add on?
	newfile->name[newfile->namepos + 0] = '.';
	newfile->name[newfile->namepos + 4] = 0;
	if (newfile->blocksize == 2048)
	{
		newfile->name[newfile->namepos + 1] = 'i';
		newfile->name[newfile->namepos + 2] = 's';
		newfile->name[newfile->namepos + 3] = 'o';
	}
	else
	{
		newfile->name[newfile->namepos + 1] = 'b';
		newfile->name[newfile->namepos + 2] = 'i';
		newfile->name[newfile->namepos + 3] = 'n';
	} // ENDIF- Is this a true ISO (or just a raw BIN file?)
	newfile->namepos += 4;

	if (IsActualFile(newfile->name) == 0)
	{
		free(newfile);
		newfile = NULL;
		return(NULL);
	} // ENDIF- Does the destination file already exist?

	if (newfile->compress > 0)
	{
		CompressOpenForWrite(newfile);
		if ((newfile->handle != ACTUALHANDLENULL) &&
		        (newfile->tablehandle == ACTUALHANDLENULL))
		{
			ActualFileClose(newfile->handle);
			newfile->handle = ACTUALHANDLENULL;
		} // ENDIF Data file created, but table file stopped? Close and remove data
	}
	else
	{
		newfile->handle = ActualFileOpenForWrite(newfile->name);
	} // ENDIF- Writing out a compressed file?
	if (newfile->handle == ACTUALHANDLENULL)
	{
		free(newfile);
		newfile = NULL;
		return(NULL);
	} // ENDIF- Couldn't create file? Abort
	return(newfile);
} // END IsoFileOpenForWrite()

int IsoFileWrite(struct IsoFile *file, char *block)
{
	int byteswritten;

	if (file == NULL)  return(-1);
	if (block == NULL)  return(-1);
	if (file->openforread == 1)  return(-1);

#ifdef VERBOSE_FUNCTION_ISOFILE
	PrintLog("CDVD isofile: IsoFileWrite()");
#endif /* VERBOSE_FUNCTION_ISOFILE */

	byteswritten = 0;
	if (file->multi > 0)
	{
		byteswritten = MultiFileWrite(file, block);
	}
	else if (file->compress > 0)
	{
		byteswritten = CompressWrite(file, block);
	}
	else
	{
		byteswritten = ActualFileWrite(file->handle,
		                               file->blocksize,
		                               block);
		if (byteswritten > 0)  file->filebytepos += byteswritten;
		if (byteswritten == file->blocksize)  file->filesectorpos++;
	} // ENDLONGIF- Write to different file? Compressed block? or Raw?
	if (byteswritten < 0)
	{
#ifdef VERBOSE_FUNCTION_ISOFILE
		PrintLog("CDVD isofile:   Trouble writing the sector!");
#endif /* VERBOSE_FUNCTION_ISOFILE */
		return(-1);
	} // ENDIF- Trouble reading the block? Say so!
	if (file->filebytepos > file->filebytesize)
		file->filebytesize = file->filebytepos;
	if (byteswritten < file->blocksize)
	{
#ifdef VERBOSE_FUNCTION_ISOFILE
		PrintLog("CDVD isofile:   Short block! Wrote %i out of %i bytes",
		         byteswritten, file->blocksize);
#endif /* VERBOSE_FUNCTION_ISOFILE */
		return(-1);
	} // ENDIF- Didn't write enough bytes? Say so!
	if (file->filesectorpos > file->filesectorsize)
		file->filesectorsize = file->filesectorpos;
	file->sectorpos++;
	return(0);
} // END IsoFileWrite()

struct IsoFile *IsoFileCloseAndDelete(struct IsoFile *file)
{
	int i;
	if (file == NULL)  return(NULL);
	if (file->handle != ACTUALHANDLENULL)
	{
#ifdef VERBOSE_FUNCTION_ISOFILE
		PrintLog("CDVD isofile: IsoFileCloseAndDelete()");
#endif /* VERBOSE_FUNCTION_ISOFILE */

		if (file->compress > 0)
		{
			CompressClose(file);
		}
		else
		{
			ActualFileClose(file->handle);
			file->handle = ACTUALHANDLENULL;
		} // ENDIF- Compressed File? Close (and flush) compression too.
	} // ENDIF- Open Handle? Close the file

	if (file->multi == 1)
	{
		for (i = file->multistart; i <= file->multiend; i++)
		{
			file->name[file->multipos] = '0' + i;
			ActualFileDelete(file->name);
			if (file->compress > 0)
			{
				file->tablename[file->multipos] = '0' + i;
				ActualFileDelete(file->tablename);
			} // ENDIF- Get the table file too?
		} // NEXT i- iterate through each multi-file name, removing it.
	}
	else
	{
		ActualFileDelete(file->name);
		if (file->compress > 0)
		{
			ActualFileDelete(file->tablename);
		} // ENDIF- Get the table file too?
	} // ENDIF- Do we have to remove multiple files?

	free(file);
	return(NULL);
} // END IsoFileCloseAndDelete()
