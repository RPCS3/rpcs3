/*  CDVDnull
 *  Copyright (C) 2002-2010  PCSX2 Dev Team
 *
 *  PCSX2 is free software: you can redistribute it and/or modify it under the terms
 *  of the GNU Lesser General Public License as published by the Free Software Found-
 *  ation, either version 3 of the License, or (at your option) any later version.
 *
 *  PCSX2 is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 *  without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 *  PURPOSE.  See the GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along with PCSX2.
 *  If not, see <http://www.gnu.org/licenses/>.
 */


#include <stdio.h>
#include "CDVD.h"
#include "svnrev.h"

#ifdef _MSC_VER
#define snprintf sprintf_s
#endif
static char libraryName[256];

const unsigned char version = PS2E_CDVD_VERSION;
const unsigned char revision = 0;
const unsigned char build = 6;

EXPORT_C_(char*) PS2EgetLibName()
{
	snprintf( libraryName, 255, "CDVDnull Driver %lld%s",SVN_REV,	SVN_MODS ? "m" : "");
	return libraryName;	
}

EXPORT_C_(u32) PS2EgetLibType()
{
	return PS2E_LT_CDVD;
}

EXPORT_C_(u32) CALLBACK PS2EgetLibVersion2(u32 type)
{
	return (version << 16) | (revision << 8) | build;
}

EXPORT_C_(s32) CDVDinit()
{
	return 0;
}

EXPORT_C_(s32) CDVDopen(const char* pTitle)
{
	return 0;
}

EXPORT_C_(void) CDVDclose()
{
}

EXPORT_C_(void) CDVDshutdown()
{
}

EXPORT_C_(s32) CDVDreadTrack(u32 lsn, int mode)
{
	return -1;
}

// return can be NULL (for async modes)
EXPORT_C_(u8*) CDVDgetBuffer()
{
	return NULL;
}

EXPORT_C_(s32) CDVDreadSubQ(u32 lsn, cdvdSubQ* subq)
{
	return -1;
}

EXPORT_C_(s32) CDVDgetTN(cdvdTN *Buffer)
{
	return -1;
}

EXPORT_C_(s32) CDVDgetTD(u8 Track, cdvdTD *Buffer)
{
	return -1;
}

EXPORT_C_(s32) CDVDgetTOC(void* toc)
{
	return -1;
}

EXPORT_C_(s32) CDVDgetDiskType()
{
	return CDVD_TYPE_NODISC;
}

EXPORT_C_(s32) CDVDgetTrayStatus()
{
	return CDVD_TRAY_CLOSE;
}

EXPORT_C_(s32) CDVDctrlTrayOpen()
{
	return 0;
}

EXPORT_C_(s32) CDVDctrlTrayClose()
{
	return 0;
}

EXPORT_C_(void) CDVDconfigure()
{
	SysMessage("Nothing to Configure");
}

EXPORT_C_(void) CDVDabout()
{
	SysMessage("%s %d.%d", "CDVDnull Driver", revision, build);
}

EXPORT_C_(s32) CDVDtest()
{
	return 0;
}
