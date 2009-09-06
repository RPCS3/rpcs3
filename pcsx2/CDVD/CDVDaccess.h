/*  Pcsx2 - Pc Ps2 Emulator
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
*  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
*/

#pragma once

#include "Plugins.h"

enum CDVD_SourceType
{
	CDVDsrc_Iso = 0,	// use built in ISO api
	CDVDsrc_Plugin,		// use external plugin
	CDVDsrc_NoDisc,		// use built in CDVDnull
};

struct CDVD_API
{
	void (CALLBACK *close)();

	// Don't need init or shutdown.  iso/nodisc have no init/shutdown and plugin's
	// is handled by the PluginManager.

	// Don't need plugin specific things like freeze, test, or other stuff here.
	// Those are handled by the plugin manager specifically.

	_CDVDopen          open;
	_CDVDreadTrack     readTrack;
	_CDVDgetBuffer     getBuffer;
	_CDVDreadSubQ      readSubQ;
	_CDVDgetTN         getTN;
	_CDVDgetTD         getTD;
	_CDVDgetTOC        getTOC;
	_CDVDgetDiskType   getDiskType;
	_CDVDgetTrayStatus getTrayStatus;
	_CDVDctrlTrayOpen  ctrlTrayOpen;
	_CDVDctrlTrayClose ctrlTrayClose;
	_CDVDnewDiskCB     newDiskCB;

	// special functions, not in external interface yet
	_CDVDreadSector    readSector;
	_CDVDgetBuffer2    getBuffer2;
	_CDVDgetDualInfo   getDualInfo;
};

// ----------------------------------------------------------------------------
//   Multiple interface system for CDVD, used to provide internal CDVDiso and NoDisc,
//   and external plugin interfaces.  Do* functions are meant as replacements for
//   direct CDVD plugin invocation, and add universal block dumping features.
// ----------------------------------------------------------------------------

extern CDVD_API* CDVD;		// currently active CDVD access mode api (either Iso, NoDisc, or Plugin)

extern CDVD_API CDVDapi_Plugin;
extern CDVD_API CDVDapi_Iso;
extern CDVD_API CDVDapi_NoDisc;

extern const wxChar* CDVD_SourceLabels[];

extern void CDVDsys_ChangeSource( CDVD_SourceType type );
extern void CDVDsys_SetFile( CDVD_SourceType srctype, const wxString& newfile );

extern bool DoCDVDopen();
extern void DoCDVDclose();
extern s32  DoCDVDreadSector(u8* buffer, u32 lsn, int mode);
extern s32  DoCDVDreadTrack(u32 lsn, int mode);
extern s32  DoCDVDgetBuffer(u8* buffer);
//extern s32  DoCDVDreadSubQ(u32 lsn, cdvdSubQ* subq);
extern void DoCDVDnewDiskCB(void (*callback)());
extern s32  DoCDVDdetectDiskType();
extern void DoCDVDresetDiskTypeCache();

