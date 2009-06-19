/* 
 *	Copyright (C) 2007 Gabest
 *	http://www.gabest.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *   
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *   
 *  You should have received a copy of the GNU General Public License
 *  along with GNU Make; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA. 
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#pragma once

struct cdvdSubQ
{
	uint8 ctrl:4;		// control and mode bits
	uint8 mode:4;		// control and mode bits
	uint8 trackNum;		// current track number (1 to 99)
	uint8 trackIndex;	// current index within track (0 to 99)
	uint8 trackM;		// current minute location on the disc (BCD encoded)
	uint8 trackS;		// current sector location on the disc (BCD encoded)
	uint8 trackF;		// current frame location on the disc (BCD encoded)
	uint8 pad;			// unused
	uint8 discM;			// current minute offset from first track (BCD encoded)
	uint8 discS;			// current sector offset from first track (BCD encoded)
	uint8 discF;			// current frame offset from first track (BCD encoded)
};

struct cdvdTD // NOT bcd coded
{
	uint32 lsn;
	uint8 type;
};

struct cdvdTN
{
	uint8 strack;	// number of the first track (usually 1)
	uint8 etrack;	// number of the last track
};

// CDVDreadTrack mode values:

#define CDVD_MODE_2352	0	// full 2352 bytes
#define CDVD_MODE_2340	1	// skip sync (12) bytes
#define CDVD_MODE_2328	2	// skip sync+head+sub (24) bytes
#define CDVD_MODE_2048	3	// skip sync+head+sub (24) bytes
#define CDVD_MODE_2368	4	// full 2352 bytes + 16 subq

// CDVDgetDiskType returns:

#define CDVD_TYPE_ILLEGAL	0xff	// Illegal Disc
#define CDVD_TYPE_DVDV		0xfe	// DVD Video
#define CDVD_TYPE_CDDA		0xfd	// Audio CD
#define CDVD_TYPE_PS2DVD	0x14	// PS2 DVD
#define CDVD_TYPE_PS2CDDA	0x13	// PS2 CD (with audio)
#define CDVD_TYPE_PS2CD		0x12	// PS2 CD
#define CDVD_TYPE_PSCDDA 	0x11	// PS CD (with audio)
#define CDVD_TYPE_PSCD		0x10	// PS CD
#define CDVD_TYPE_UNKNOWN	0x05	// Unknown
#define CDVD_TYPE_DETCTDVDD	0x04	// Detecting Dvd Dual Sided
#define CDVD_TYPE_DETCTDVDS	0x03	// Detecting Dvd Single Sided
#define CDVD_TYPE_DETCTCD	0x02	// Detecting Cd
#define CDVD_TYPE_DETCT		0x01	// Detecting
#define CDVD_TYPE_NODISC 	0x00	// No Disc

// CDVDgetTrayStatus returns:

#define CDVD_TRAY_CLOSE		0x00
#define CDVD_TRAY_OPEN		0x01

// cdvdTD.type (track types for cds)

#define CDVD_AUDIO_TRACK	0x01
#define CDVD_MODE1_TRACK	0x41
#define CDVD_MODE2_TRACK	0x61

#define CDVD_AUDIO_MASK		0x00
#define CDVD_DATA_MASK		0x40
//	CDROM_DATA_TRACK	0x04	// do not enable this! (from linux kernel)

//

#define CACHE_BLOCK_COUNT 16

class CDVDolioApp
{
	static const char* m_ini;
	static const char* m_section;

public:
	CDVDolioApp();

	HMODULE GetModuleHandle();

	string GetConfig(const char* entry, const char* value);
	void SetConfig(const char* entry, const char* value);
	int GetConfig(const char* entry, int value);
	void SetConfig(const char* entry, int value);
};

extern CDVDolioApp theApp;

//

class CDVD
{
	HANDLE m_hFile;
	string m_label;
	OVERLAPPED m_overlapped;
	struct {int count, size, offset;} m_block;
	struct {uint8 buff[2048 * CACHE_BLOCK_COUNT]; bool pending; int start, count;} m_cache;
	uint8 m_buff[2352];

	LARGE_INTEGER MakeOffset(int lsn);
	bool SyncRead(int lsn);

public:
	CDVD();
	virtual ~CDVD();

	bool Open(const char* path);
	void Close();
	const char* GetLabel();
	bool Read(int lsn, int mode = CDVD_MODE_2048);
	uint8* GetBuffer();
	uint32 GetTN(cdvdTN* buff);
	uint32 GetTD(uint8 track, cdvdTD* buff);
};

