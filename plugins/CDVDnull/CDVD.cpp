/*  CDVDnull
 *  Copyright (C) 2002-2009  Pcsx2 Team
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
 */
 
#include <stdio.h>

#include "CDVD.h"

const char *LibName = "CDVDnull Driver";

const unsigned char version = PS2E_CDVD_VERSION;
const unsigned char revision = 0;
const unsigned char build = 6;

EXPORT_C_(char*) PS2EgetLibName()
{
	return (char *)LibName;
}

EXPORT_C_(u32) PS2EgetLibType()
{
	return PS2E_LT_CDVD;
}

EXPORT_C_(u32) CALLBACK PS2EgetLibVersion2(u32 type)
{
	return (version << 16) | (revision << 8) | build;
}

#ifdef _WIN32
void SysMessage(const char *fmt, ...)
{
	va_list list;
	char tmp[512];

	va_start(list, fmt);
	vsprintf(tmp, fmt, list);
	va_end(list);

	MessageBox(0, tmp, "CDVDnull Msg", 0);
}
#endif

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
	SysMessage("%s %d.%d", LibName, revision, build);
}

EXPORT_C_(s32) CDVDtest()
{
	return 0;
}
