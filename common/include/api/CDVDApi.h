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
 *	Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */


#ifndef __CDVDAPI_H__
#define __CDVDAPI_H__

// Note; this header is experimental, and will be a shifting target. Only use this if you are willing to repeatedly fix breakage.

/*
 *  Based on PS2E Definitions by
	   linuzappz@hotmail.com,
 *          shadowpcsx2@yahoo.gr,
 *          and florinsasu@hotmail.com
 */

#include "Pcsx2Api.h"

/* CDVD plugin API */

// Basic functions.

EXPORT_C_(s32)   CDVDinit(char *configpath);
EXPORT_C_(s32)   CDVDopen(void *pDisplay, const char* pTitleFilename);
EXPORT_C_(void)  CDVDclose();
EXPORT_C_(void)  CDVDshutdown();
EXPORT_C_(s32)   CDVDreadTrack(u32 lsn, int mode);

// return can be NULL (for async modes)
EXPORT_C_(u8*)   CDVDgetBuffer();

EXPORT_C_(s32)   CDVDreadSubQ(u32 lsn, cdvdSubQ* subq);//read subq from disc (only cds have subq data)
EXPORT_C_(s32)   CDVDgetTN(cdvdTN *Buffer);			//disk information
EXPORT_C_(s32)   CDVDgetTD(u8 Track, cdvdTD *Buffer);	//track info: min,sec,frame,type
EXPORT_C_(s32)   CDVDgetTOC(void* toc);				//gets ps2 style toc from disc
EXPORT_C_(s32)   CDVDgetDiskType();					//CDVD_TYPE_xxxx
EXPORT_C_(s32)   CDVDgetTrayStatus();					//CDVD_TRAY_xxxx
EXPORT_C_(s32)   CDVDctrlTrayOpen();					//open disc tray
EXPORT_C_(s32)   CDVDctrlTrayClose();					//close disc tray

// Extended functions

EXPORT_C_(void)  CDVDconfigure();
EXPORT_C_(void)  CDVDabout();
EXPORT_C_(s32)   CDVDtest();
EXPORT_C_(void)  CDVDnewDiskCB(void (*callback)());

typedef struct _cdvdSubQ {
	u8 ctrl:4;		// control and mode bits
	u8 mode:4;		// control and mode bits
	u8 trackNum;	// current track number (1 to 99)
	u8 trackIndex;	// current index within track (0 to 99)
	u8 trackM;		// current minute location on the disc (BCD encoded)
	u8 trackS;		// current sector location on the disc (BCD encoded)
	u8 trackF;		// current frame location on the disc (BCD encoded)
	u8 pad;			// unused
	u8 discM;		// current minute offset from first track (BCD encoded)
	u8 discS;		// current sector offset from first track (BCD encoded)
	u8 discF;		// current frame offset from first track (BCD encoded)
} cdvdSubQ;

typedef struct _cdvdTD { // NOT bcd coded
	u32 lsn;
	u8 type;
} cdvdTD;

typedef struct _cdvdTN {
	u8 strack;	//number of the first track (usually 1)
	u8 etrack;	//number of the last track
} cdvdTN;

// CDVDreadTrack mode values:
enum {
CDVD_MODE_2352	0,	// full 2352 bytes
CDVD_MODE_2340	1,	// skip sync (12) bytes
CDVD_MODE_2328	2,	// skip sync+head+sub (24) bytes
CDVD_MODE_2048	3,	// skip sync+head+sub (24) bytes
CDVD_MODE_2368	4	// full 2352 bytes + 16 subq
} TrackModes

// CDVDgetDiskType returns:
enum {
CDVD_TYPE_ILLEGAL		= 0xff,	// Illegal Disc
CDVD_TYPE_DVDV			= 0xfe,	// DVD Video
CDVD_TYPE_CDDA			= 0xfd,	// Audio CD
CDVD_TYPE_PS2DVD		= 0x14,	// PS2 DVD
CDVD_TYPE_PS2CDDA		= 0x13,	// PS2 CD (with audio)
CDVD_TYPE_PS2CD			= 0x12,	// PS2 CD
CDVD_TYPE_PSCDDA 		= 0x11,	// PS CD (with audio)
CDVD_TYPE_PSCD			= 0x10,	// PS CD
CDVD_TYPE_UNKNOWN 		= 0x05,	// Unknown
CDVD_TYPE_DETCTDVDD 	= 0x04,	// Detecting Dvd Dual Sided
CDVD_TYPE_DETCTDVDS 	= 0x03,	// Detecting Dvd Single Sided
CDVD_TYPE_DETCTCD 		= 0x02,	// Detecting Cd
CDVD_TYPE_DETCT			= 0x01,	// Detecting
CDVD_TYPE_NODISC 		= 0x00	// No Disc
} DiskType;

// CDVDgetTrayStatus returns:
enum {
CDVD_TRAY_CLOSE	= 0x00,
CDVD_TRAY_OPEN   = 0x01
} TrayStatus;

// cdvdTD.type (track types for cds)
enum {
CDVD_AUDIO_TRACK = 0x01,
CDVD_MODE1_TRACK = 0x41,
CDVD_MODE2_TRACK = 0x61
} CDVDTDType;

enum {
CDVD_AUDIO_MASK = 0x00,
CDVD_DATA_MASK = 0x40
//	CDROM_DATA_TRACK	0x04	//do not enable this! (from linux kernel)
} CDVD_Masks;

#endif // __CDVDAPI_H__