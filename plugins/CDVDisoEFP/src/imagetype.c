/*  imagetype.c
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
#include <sys/types.h> // off64_t
// #ifndef __LINUX__
// #ifdef __linux__
// #define __LINUX__
// #endif /* __linux__ */
// #endif /* No __LINUX__ */
// #define CDVDdefs
// #include "PS2Edefs.h"

#include "isofile.h"
#include "actualfile.h"
#include "imagetype.h"

// Based (mostly) off of florin's CDVDbin detection code, twice removed
//  with some additions from the <linux/cdrom.h> header.
struct ImageTypes imagedata[] =
{
	{ "ISO 2048",             2048,        0,  0,  0 },
	{ "YellowBook 2064",      2064,        0, 16,  1 },
	{ "RAW 2064",             2064,        0,  0,  2 },
	{ "GreenBook 2072",       2072,        0, 24,  3 },
	{ "RAW 2072",             2072,        0,  0,  4 },
	{ "RAW 2324",             2324,        0,  0,  5 },
	{ "RAW 2328",             2328,        0,  0,  6 },
	{ "RAW 2336",             2336,        0,  0,  7 },
	{ "GreenBook 2352",       2352,        0, 24,  8 },
	{ "YellowBook 2352",      2352,        0, 16,  9 },
	{ "RedBook 2352",         2352,        0,  0, 10 },
	{ "RAWQ 2448",            2448,        0,  0, 11 },
	{ "NERO ISO 2048",        2048, 150*2048,  0,  0 },
	{ "NERO GreenBook 2352",  2352, 150*2352, 24,  8 },
	{ "NERO YellowBook 2352", 2352, 150*2352, 16,  9 },
	{ "NERO RedBook 2352",    2352, 150*2352,  0, 10 },
	{ "NERO RAWQ 2448",       2448, 150*2448,  0, 11 },
	{ "Alt ISO 2048",         2048,        8,  0,  0 },
	{ "Alt RAW 2336",         2336,        8,  0,  7 },
	{ "Alt GreenBook 2352",   2352,        8, 24,  8 },
	{ "Alt YellowBook 2352",  2352,        8, 16,  9 },
	{ "Alt RedBook 2352",     2352,        8,  0, 10 },
	{ "Alt RAWQ 2448",        2448,        8,  0, 11 },
	{ NULL, 0, 0, 0, 0 }
};

#define REDBOOK2352 10

void GetImageType(struct IsoFile *isofile, int imagetype)
{
	int temptype;
	int i;

	temptype = imagetype;
	if ((temptype < 0) || (temptype > 22))  temptype = REDBOOK2352;
	i = 0;
	while ((i < 40) && (*(imagedata[temptype].name + i) != 0))
	{
		isofile->imagename[i] = *(imagedata[temptype].name + i);
		i++;
	} // ENDWHILE- filling in the image name
	isofile->imagename[i] = 0; // And 0-terminate.

	isofile->blocksize = imagedata[temptype].blocksize;
	isofile->imageheader = imagedata[temptype].fileoffset;
	isofile->blockoffset = imagedata[temptype].dataoffset;
} // END GetImageType()

int GetImageTypeConvertTo(int imagetype)
{
	return(imagedata[imagetype].conversiontype);
} // END GetImageTypeConvertTo()

int DetectImageType(struct IsoFile *isofile)
{
	char comparestr[] = "CD001";
	int newtype;
	off64_t targetpos;
	char teststr[2448];
	int dataoffset;
	int i;
	int retval;

	newtype = 0;
	if (isofile->compress > 0)
	{
		IsoFileSeek(isofile, 16);
		IsoFileRead(isofile, teststr);
		while (imagedata[newtype].name != NULL)
		{
			if ((isofile->blocksize == imagedata[newtype].blocksize) &&
			        (isofile->imageheader == imagedata[newtype].fileoffset))
			{
				dataoffset = imagedata[newtype].dataoffset + 1;
				i = 0;
				while ((i < 5) && (teststr[dataoffset + i] == comparestr[i])) i++;
				if (i == 5)
				{
					GetImageType(isofile, newtype);
					return(newtype);
				} // ENDIF- Did we find a match?
			} // ENDIF- Do these pieces match the compression storage pieces?
			newtype++;
		} // ENDWHILE- looking for the image type that fits the stats
	}
	else
	{
		while (imagedata[newtype].name != NULL)
		{
			targetpos = (16 * imagedata[newtype].blocksize)
			            + imagedata[newtype].fileoffset
			            + imagedata[newtype].dataoffset
			            + 1; // Moves to start of string
			retval = ActualFileSeek(isofile->handle, targetpos);
			if (retval == 0)
			{
				retval = ActualFileRead(isofile->handle, 5, teststr);
				if (retval == 5)
				{
					i = 0;
					while ((i < 5) && (teststr[i] == comparestr[i]))  i++;
					if (i == 5)
					{
						ActualFileSeek(isofile->handle, isofile->imageheader);
						GetImageType(isofile, newtype);
						return(newtype);
					} // ENDIF- Did we find a match?
				} // ENDIF- Could we read in the test string? Cool! Test it.
			} // ENDIF- Could actually get to this point?
			newtype++;
		} // ENDWHILE- looking for the directory header string "CD001"
		ActualFileSeek(isofile->handle, isofile->imageheader);
	} // ENDIF- Do we match type to compression stats? (Or search against raw data?)
	GetImageType(isofile, REDBOOK2352);
	return(REDBOOK2352); // Couldn't find it? Guess it's RAW 2352, then. (Audio CD?)
} // END ImageDetect()
