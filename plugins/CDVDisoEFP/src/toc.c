/*  toc.c
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
#include <sys/types.h> // off64_t
#ifndef __LINUX__
#ifdef __linux__
#define __LINUX__
#endif /* __linux__ */
#endif /* No __LINUX__ */
#define CDVDdefs
#include "PS2Edefs.h"
#include "logfile.h"
#include "convert.h"
#include "isofile.h"
#include "actualfile.h"
#include "toc.h"
// PCSX2's .toc file format:
// 1 unsigned char - CDVD_TYPE_????
// 1 tocTN
// As many tocTDs as it takes.

extern void IsoInitTOC(struct IsoFile *isofile)
{
	int i;
	off64_t sectorsize;

	if (isofile == NULL)  return;

#ifdef VERBOSE_FUNCTION_TOC
	PrintLog("CDVDiso TOC: IsoInitTOC()");
#endif /* VERBOSE_FUNCTION_TOC */

	if (isofile->multi > 0)
	{
		sectorsize = isofile->multisectorend[isofile->multiend];
	}
	else
	{
		sectorsize = isofile->filesectorsize;
	} // ENDIF- Establish largest sector from multifile? (or single file?)

	for (i = 0; i < 2048; i++)  isofile->toc[i] = 0;
	switch (isofile->cdvdtype)
	{
		case CDVD_TYPE_DVDV:
		case CDVD_TYPE_PS2DVD:
			if ((isofile->filesectorsize > (2048*1024)) ||
			        (isofile->multi > 0))
			{
				isofile->toc[0] = 0x24; // Dual-Sided DVD (?)
				isofile->toc[4] = 0x41;
				isofile->toc[5] = 0x95;
			}
			else
			{
				isofile->toc[0] = 0x04; // Single-Sided DVD (?)
				isofile->toc[4] = 0x86;
				isofile->toc[5] = 0x72;
			} // ENDIF- Too many sectors for a single-layered disc?
			isofile->toc[1] = 0x02;
			isofile->toc[2] = 0xF2;
			isofile->toc[3] = 0x00;
			isofile->toc[16] = 0x00;
			isofile->toc[17] = 0x03;
			isofile->toc[18] = 0x00;
			isofile->toc[19] = 0x00;
			return; // DVD's don't have tracks. Might track multisession later...
			break;

		case CDVD_TYPE_PS2CD:
		case CDVD_TYPE_PSCD:
			isofile->toc[0] = 0x41;
			break;
		case CDVD_TYPE_CDDA:
			isofile->toc[0] = 0x01;
			break;
		default:
			break;
	} // ENDSWITCH isofile->cdvdtype - Which TOC for which type?
	// CD Details here... (tracks and stuff)
	isofile->toc[2] = 0xA0;
	isofile->toc[7] = 0x01; // Starting Track No.
	isofile->toc[12] = 0xA1;
	isofile->toc[17] = 0x01; // Ending Track No.
	isofile->toc[22] = 0xA2;
	LBAtoMSF(sectorsize, &isofile->toc[27]);
	isofile->toc[27] = HEXTOBCD(isofile->toc[27]);
	isofile->toc[28] = HEXTOBCD(isofile->toc[28]);
	isofile->toc[29] = HEXTOBCD(isofile->toc[29]);
	isofile->toc[40] = 0x02; // YellowBook? Data Mode?
	isofile->toc[42] = 0x01; // Track No.
	LBAtoMSF(0, &isofile->toc[47]);
	isofile->toc[47] = HEXTOBCD(isofile->toc[47]);
	isofile->toc[48] = HEXTOBCD(isofile->toc[48]);
	isofile->toc[49] = HEXTOBCD(isofile->toc[49]);
} // END IsoInitTOC()

extern void IsoAddTNToTOC(struct IsoFile *isofile, struct tocTN toctn)
{
	if (isofile == NULL) return;
#ifdef VERBOSE_FUNCTION_TOC
	PrintLog("CDVDiso TOC: IsoAddTNToTOC()");
#endif /* VERBOSE_FUNCTION_TOC */
	isofile->toc[7] = HEXTOBCD(toctn.strack);
	isofile->toc[17] = HEXTOBCD(toctn.etrack);
	return;
} // END IsoAddTNToTOC()

extern void IsoAddTDToTOC(struct IsoFile *isofile,
	                          unsigned char track,
	                          struct tocTD toctd)
{
	int temptrack;
	int position;
#ifdef VERBOSE_FUNCTION_TOC
	PrintLog("CDVDiso TOC: IsoAddTNToTOC(%u)", track);
#endif /* VERBOSE_FUNCTION_TOC */
	if (isofile == NULL)  return;
	temptrack = track;
	if (temptrack == 0xAA)  temptrack = 0;
	if (temptrack > 99)  return; // Only up to 99 tracks allowed.
	if (temptrack == 0)
	{
		LBAtoMSF(toctd.lsn, &isofile->toc[27]);
		isofile->toc[27] = HEXTOBCD(isofile->toc[27]);
		isofile->toc[28] = HEXTOBCD(isofile->toc[28]);
		isofile->toc[29] = HEXTOBCD(isofile->toc[29]);
	}
	else
	{
		position = temptrack * 10;
		position += 30;
		isofile->toc[position] = toctd.type;
		isofile->toc[position + 2] = HEXTOBCD(temptrack);
		LBAtoMSF(toctd.lsn, &isofile->toc[position + 7]);
		isofile->toc[position + 7] = HEXTOBCD(isofile->toc[position + 7]);
		isofile->toc[position + 8] = HEXTOBCD(isofile->toc[position + 8]);
		isofile->toc[position + 9] = HEXTOBCD(isofile->toc[position + 9]);
	} // ENDIF- Is this a lead-out? (or an actual track?)
} // END IsoAddTDToTOC()
extern int IsoLoadTOC(struct IsoFile *isofile)
{
	char tocext[] = ".toc\0";
	char tocheader[5];
	ACTUALHANDLE tochandle;
	char tocname[256];
	int i;
	int j;
	int retval;
	unsigned char cdvdtype;
	struct tocTN toctn;
	struct tocTD toctd;
	if (isofile == NULL)  return(-1);
	i = 0;
	while ((i < 256) && (isofile->name[i] != 0))
	{
		tocname[i] = isofile->name[i];
		i++;
	} // ENDWHILE- Copying the data name to the toc name
	j = 0;
	while ((i < 256) && (tocext[j] != 0))
	{
		tocname[i] = tocext[j];
		i++;
		j++;
	} // ENDWHILE- Append ".toc" to end of name
	tocname[i] = 0; // And 0-terminate
	tochandle = ActualFileOpenForRead(tocname);
	if (tochandle == ACTUALHANDLENULL)  return(-1);

	retval = ActualFileRead(tochandle, 4, tocheader);
	if (retval < 4)
	{
		ActualFileClose(tochandle);
		tochandle = ACTUALHANDLENULL;
		return(-1);
	} // ENDIF- Trouble reading the 'toc' file?
	if ((tocheader[0] != 'T') ||
	        (tocheader[1] != 'O') ||
	        (tocheader[2] != 'C') ||
	        (tocheader[3] != '1'))
	{
		ActualFileClose(tochandle);
		tochandle = ACTUALHANDLENULL;
		return(-1);
	} // ENDIF- Not a 'toc' file after all?
#ifdef VERBOSE_FUNCTION_TOC
	PrintLog("CDVDiso TOC: IsoLoadTOC(%s)", tocname);
#endif /* VERBOSE_FUNCTION_TOC */
	retval = ActualFileRead(tochandle, 1, (char *) & cdvdtype);
	if (retval < 1)
	{
		ActualFileClose(tochandle);
		tochandle = ACTUALHANDLENULL;
		return(-1);
	} // ENDIF- Trouble reading the 'toc' file?
	isofile->cdvdtype = cdvdtype;
	IsoInitTOC(isofile);

	if ((cdvdtype != CDVD_TYPE_PS2DVD) && (cdvdtype != CDVD_TYPE_DVDV))
	{
		retval = ActualFileRead(tochandle, sizeof(struct tocTN), (char *) & toctn);
		if (retval < sizeof(struct tocTN))
		{
			ActualFileClose(tochandle);
			tochandle = ACTUALHANDLENULL;
			return(-1);
		} // ENDIF- Trouble reading the 'toc' file?

		if ((toctn.strack > 99) || (toctn.etrack > 99))
		{
			ActualFileClose(tochandle);
			tochandle = ACTUALHANDLENULL;
			return(-1);
		} // ENDIF- Track numbers out of range?
#ifdef VERBOSE_FUNCTION_TOC
		PrintLog("CDVDiso TOC:   Start Track %u   End Track %u",
		         toctn.strack, toctn.etrack);
#endif /* VERBOSE_FUNCTION_TOC */
		IsoAddTNToTOC(isofile, toctn);
		retval = ActualFileRead(tochandle, sizeof(struct tocTD), (char *) & toctd);
		if (retval < sizeof(struct tocTD))
		{
			ActualFileClose(tochandle);
			tochandle = ACTUALHANDLENULL;
			return(-1);
		} // ENDIF- Trouble reading the 'toc' file?
		if (toctd.type != 0)
		{
			ActualFileClose(tochandle);
			tochandle = ACTUALHANDLENULL;
			return(-1);
		} // ENDIF- Track numbers out of range?
#ifdef VERBOSE_FUNCTION_TOC
		PrintLog("CDVDiso TOC:   Total Sectors: %lu", toctd.lsn);
#endif /* VERBOSE_FUNCTION_TOC */
		IsoAddTDToTOC(isofile, 0xAA, toctd);
		for (i = toctn.strack; i <= toctn.etrack; i++)
		{
			retval = ActualFileRead(tochandle, sizeof(struct tocTD), (char *) & toctd);
			if (retval < sizeof(struct tocTD))
			{
				ActualFileClose(tochandle);
				tochandle = ACTUALHANDLENULL;
				return(-1);
			} // ENDIF- Trouble reading the 'toc' file?
#ifdef VERBOSE_FUNCTION_TOC
			PrintLog("CDVDiso TOC:   Track %u  Type %u  Sector Start: %lu",
			         i, toctd.type, toctd.lsn);
#endif /* VERBOSE_FUNCTION_TOC */
			IsoAddTDToTOC(isofile, i, toctd);
		} // NEXT i- read in each track
	} // ENDIF- Not a DVD? (Then read in CD track data)
	ActualFileClose(tochandle);
	tochandle = ACTUALHANDLENULL;
	return(0);
} // END IsoLoadTOC()

extern int IsoSaveTOC(struct IsoFile *isofile)
{
	char tocext[] = ".toc\0";
	char tocheader[] = "TOC1\0";
	ACTUALHANDLE tochandle;
	char tocname[256];
	int i;
	int j;
	int retval;
	unsigned char cdvdtype;
	struct tocTN toctn;
	struct tocTD toctd;
	char temptime[3];
	if (isofile == NULL)  return(-1);
	i = 0;
	while ((i < 256) && (isofile->name[i] != 0))
	{
		tocname[i] = isofile->name[i];
		i++;
	} // ENDWHILE- Copying the data name to the toc name
	j = 0;
	while ((i < 256) && (tocext[j] != 0))
	{
		tocname[i] = tocext[j];
		i++;
		j++;
	} // ENDWHILE- Append ".toc" to end of name
	tocname[i] = 0; // And 0-terminate

	ActualFileDelete(tocname);
	tochandle = ActualFileOpenForWrite(tocname);
	if (tochandle == ACTUALHANDLENULL)  return(-1);

	retval = ActualFileWrite(tochandle, 4, tocheader);
	if (retval < 4)
	{
		ActualFileClose(tochandle);
		tochandle = ACTUALHANDLENULL;
		ActualFileDelete(tocname);
		return(-1);
	} // ENDIF- Trouble writing to the 'toc' file?

	cdvdtype = isofile->cdvdtype;
	ActualFileWrite(tochandle, 1, (char *) &cdvdtype);
	if ((cdvdtype != CDVD_TYPE_PS2DVD) && (cdvdtype != CDVD_TYPE_DVDV))
	{
		toctn.strack = BCDTOHEX(isofile->toc[7]);
		toctn.etrack = BCDTOHEX(isofile->toc[17]);
		ActualFileWrite(tochandle, sizeof(struct tocTN), (char *) &toctn);
		// Leadout Data
		toctd.type = 0;
		temptime[0] = BCDTOHEX(isofile->toc[27]);
		temptime[1] = BCDTOHEX(isofile->toc[28]);
		temptime[2] = BCDTOHEX(isofile->toc[29]);
		toctd.lsn = MSFtoLBA(temptime);
		ActualFileWrite(tochandle, sizeof(struct tocTD), (char *) &toctd);
		for (i = toctn.strack; i <= toctn.etrack; i++)
		{
			j = i * 10 + 30;
			toctd.type = isofile->toc[j];
			temptime[0] = BCDTOHEX(isofile->toc[j + 7]);
			temptime[1] = BCDTOHEX(isofile->toc[j + 8]);
			temptime[2] = BCDTOHEX(isofile->toc[j + 9]);
			toctd.lsn = MSFtoLBA(temptime);
			ActualFileWrite(tochandle, sizeof(struct tocTD), (char *) &toctd);
		} // NEXT i- write out each track
	} // ENDIF- Not a DVD? (Then output CD track data)
	ActualFileClose(tochandle);
	tochandle = ACTUALHANDLENULL;
	return(0);
} // END IsoSaveTOC()
