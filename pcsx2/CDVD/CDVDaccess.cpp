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

#include "PrecompiledHeader.h"

#define ENABLE_TIMESTAMPS

#ifdef _WIN32
#include <windows.h>
#endif

#include <ctype.h>
#include <time.h>
#include <wx/datetime.h>

#include "IopCommon.h"
#include "IsoFStools.h"
#include "IsoFSdrv.h"
#include "CDVDisoReader.h"

// ----------------------------------------------------------------------------
// diskTypeCached
// Internal disc type cache, to reduce the overhead of disc type checks, which are
// performed quite liberally by many games (perhaps intended to keep the PS2 DVD
// from spinning down due to idle activity?).
// Cache is set to -1 for init and when the disc is removed/changed, which invokes
// a new DiskTypeCheck.  All subsequent checks use the non-negative value here.
//
static int diskTypeCached = -1;

// used to bridge the gap between the old getBuffer api and the new getBuffer2 api.
int lastReadSize;
int lastLSN;		// needed for block dumping

// Records last read block length for block dumping
static int plsn = 0;
static isoFile *blockDumpFile = NULL;

// Assertion check for CDVD != NULL (in devel and debug builds), because its handier than
// relying on DEP exceptions -- and a little more reliable too.
static void CheckNullCDVD()
{
	DevAssert( CDVD != NULL, "Invalid CDVD object state (null pointer exception)" );
}

//////////////////////////////////////////////////////////////////////////////////////////
// Disk Type detection stuff (from cdvdGigaherz)
//
int CheckDiskTypeFS(int baseType)
{
	int		f;
	char	buffer[256];//if a file is longer...it should be shorter :D
	char	*pos;
	static struct TocEntry tocEntry;

	IsoFS_init();
	
	f = IsoFS_open("SYSTEM.CNF;1", 1);
	
	// check if the file exists
	if (f >= 0)
	{
		int size = IsoFS_read(f, buffer, 256);
		IsoFS_close(f);

		buffer[size]='\0';

		pos = strstr(buffer, "BOOT2");
		if (pos == NULL)
		{
			pos = strstr(buffer, "BOOT");
			if (pos == NULL)  return CDVD_TYPE_ILLEGAL;
			return CDVD_TYPE_PSCD;
		}
		return (baseType==CDVD_TYPE_DETCTCD) ? CDVD_TYPE_PS2CD : CDVD_TYPE_PS2DVD;
	}

	if (IsoFS_findFile("PSX.EXE;1", &tocEntry) == TRUE)
	{
		return CDVD_TYPE_PSCD;
	}

	if (IsoFS_findFile("VIDEO_TS/VIDEO_TS.IFO;1", &tocEntry) == TRUE)
	{
		return CDVD_TYPE_DVDV;
	}

	return CDVD_TYPE_ILLEGAL; // << Only for discs which aren't ps2 at all.
}

static int FindDiskType(int mType)
{
	int dataTracks = 0;
	int audioTracks = 0;
	int iCDType = mType;
	cdvdTN tn;

	CDVD->getTN(&tn);

	if (tn.strack != tn.etrack) // multitrack == CD.
	{
		iCDType = CDVD_TYPE_DETCTCD;
	}
	else if (mType < 0)
	{
		static u8 bleh[CD_FRAMESIZE_RAW];
		cdvdTD td;

		CDVD->getTD(0,&td);
		if (td.lsn > 452849)
		{
			iCDType = CDVD_TYPE_DETCTDVDS;
		}
		else
		{
			if (DoCDVDreadSector(bleh, 16, CDVD_MODE_2048) == 0)
			{
				const cdVolDesc& volDesc = (cdVolDesc&)bleh;
				if(volDesc.rootToc.tocSize == 2048) 
					iCDType = CDVD_TYPE_DETCTCD;
				else
					iCDType = CDVD_TYPE_DETCTDVDS;
			}
		}
	}

	if (iCDType == CDVD_TYPE_DETCTDVDS)
	{
		s32 dlt = 0;
		u32 l1s = 0;

		if(CDVD->getDualInfo(&dlt,&l1s)==0)
		{
			if (dlt > 0) iCDType = CDVD_TYPE_DETCTDVDD;
		}
	}

	switch(iCDType)
	{
		case CDVD_TYPE_DETCTCD:
			Console::Status(" * CDVD Disk Open: CD, %d tracks (%d to %d):", params tn.etrack-tn.strack+1,tn.strack,tn.etrack);
			break;
		
		case CDVD_TYPE_DETCTDVDS:
			Console::Status(" * CDVD Disk Open: DVD, Single layer or unknown:");
			break;
		
		case CDVD_TYPE_DETCTDVDD:
			Console::Status(" * CDVD Disk Open: DVD, Double layer:");
			break;
	}

	audioTracks = dataTracks = 0;
	for(int i = tn.strack; i <= tn.etrack; i++)
	{
		cdvdTD td,td2;
		
		CDVD->getTD(i,&td);

		if (tn.etrack > i)
			CDVD->getTD(i+1,&td2);
		else
			CDVD->getTD(0,&td2);

		int tlength = td2.lsn - td.lsn;

		if (td.type == CDVD_AUDIO_TRACK) 
		{
			audioTracks++;
			Console::Status(" * * Track %d: Audio (%d sectors)", params i,tlength);
		}
		else 
		{
			dataTracks++;
			Console::Status(" * * Track %d: Data (Mode %d) (%d sectors)", params i,((td.type==CDVD_MODE1_TRACK)?1:2),tlength);
		}
	}

	if (dataTracks > 0)
	{
		iCDType=CheckDiskTypeFS(iCDType);
	}

	if (audioTracks > 0)
	{
		switch (iCDType)
		{
			case CDVD_TYPE_PS2CD:
				iCDType=CDVD_TYPE_PS2CDDA;
				break;
			case CDVD_TYPE_PSCD:
				iCDType=CDVD_TYPE_PSCDDA;
				break;
			default:
				iCDType=CDVD_TYPE_CDDA;
				break;
		}
	}

	return iCDType;
}

static void DetectDiskType()
{
	if (CDVD->getTrayStatus() == CDVD_TRAY_OPEN)
	{
		diskTypeCached = CDVD_TYPE_NODISC;
		return;
	}

	int baseMediaType = CDVD->getDiskType();
	int mType = -1;

	// Paranoid mode: do not trust the plugin's detection system to work correctly.
	// (.. and there's no reason plugins should be doing their own detection anyway).

	switch(baseMediaType)
	{
		case CDVD_TYPE_CDDA:
		case CDVD_TYPE_PSCD:
		case CDVD_TYPE_PS2CD:
		case CDVD_TYPE_PSCDDA:
		case CDVD_TYPE_PS2CDDA:
			mType = CDVD_TYPE_DETCTCD;
			break;
		
		case CDVD_TYPE_DVDV:
		case CDVD_TYPE_PS2DVD:
			mType = CDVD_TYPE_DETCTDVDS;
			break;
		
		case CDVD_TYPE_DETCTDVDS:
		case CDVD_TYPE_DETCTDVDD:
		case CDVD_TYPE_DETCTCD:
			mType = baseMediaType;
			break;
		
		case CDVD_TYPE_NODISC:
			diskTypeCached = CDVD_TYPE_NODISC;
			return;
	}

	diskTypeCached = FindDiskType(mType);
}

// ----------------------------------------------------------------------------
// CDVDsys_ChangeSource
//
void CDVDsys_ChangeSource( CDVD_SourceType type )
{
	if( g_plugins != NULL )
		g_plugins->Close( PluginId_CDVD );
		
	switch( type )
	{
		case CDVDsrc_Iso:
			CDVD = &CDVDapi_Iso;
		break;
		
		case CDVDsrc_NoDisc:
			CDVD = &CDVDapi_NoDisc;
		break;

		case CDVDsrc_Plugin:
			CDVD = &CDVDapi_Plugin;
		break;

		jNO_DEFAULT;
	}
}

s32 DoCDVDopen(const char* pTitleFilename)
{
	CheckNullCDVD();

	int ret = CDVD->open(pTitleFilename);
	int cdtype = DoCDVDdetectDiskType();

	if (EmuConfig.CdvdDumpBlocks && (cdtype != CDVD_TYPE_NODISC))
	{
		// TODO: Add a blockdumps configurable folder, and use that instead of CWD().

		// TODO: "Untitled" should use pnach/slus name resolution, slus if no patch,
		// and finally an "Untitled-[ElfCRC]" if no slus.

		wxString temp( Path::Combine( wxGetCwd(), CDVD->getUniqueFilename() ) );

#ifdef ENABLE_TIMESTAMPS
		wxDateTime curtime( wxDateTime::GetTimeNow() );

		temp += wxsFormat( L" (%04d-%02d-%02d %02d-%02d-%02d)",
			curtime.GetYear(), curtime.GetMonth(), curtime.GetDay(),
			curtime.GetHour(), curtime.GetMinute(), curtime.GetSecond()
		);
#endif
		temp += L".dump";

		cdvdTD td;
		CDVD->getTD(0, &td);

		blockDumpFile = isoCreate(temp.ToAscii().data(), ISOFLAGS_BLOCKDUMP_V3);

		if( blockDumpFile != NULL )
		{
			int blockofs = 0, blocksize = CD_FRAMESIZE_RAW, blocks = td.lsn;

			// hack: Because of limitations of the current cdvd design, we can't query the blocksize
			// of the underlying media.  So lets make a best guess:

			switch(cdtype)
			{
				case CDVD_TYPE_PS2DVD:
				case CDVD_TYPE_DVDV:
				case CDVD_TYPE_DETCTDVDS:
				case CDVD_TYPE_DETCTDVDD:
					blocksize = 2048;
				break;
			}
			isoSetFormat(blockDumpFile, blockofs, blocksize, blocks);
		}
	}
	else
	{
		blockDumpFile = NULL;
	}

	return ret;
}

void DoCDVDclose()
{
	CheckNullCDVD();
	if(blockDumpFile) isoClose(blockDumpFile);
	if( CDVD->close != NULL )
		CDVD->close();

	DoCDVDresetDiskTypeCache();
}

s32 DoCDVDreadSector(u8* buffer, u32 lsn, int mode)
{
	CheckNullCDVD();
	int ret = CDVD->readSector(buffer,lsn,mode);

	if(ret == 0 && blockDumpFile != NULL )
	{
		isoWriteBlock(blockDumpFile, buffer, lsn);
	}

	return ret;
}

s32 DoCDVDreadTrack(u32 lsn, int mode)
{
	CheckNullCDVD();

	// TEMP: until all the plugins use the new CDVDgetBuffer style
	switch (mode)
	{
	case CDVD_MODE_2352:
		lastReadSize = 2352;
		break;
	case CDVD_MODE_2340:
		lastReadSize = 2340;
		break;
	case CDVD_MODE_2328:
		lastReadSize = 2328;
		break;
	case CDVD_MODE_2048:
		lastReadSize = 2048;
		break;
	}

	//DevCon::Notice("CDVD readTrack(lsn=%d,mode=%d)",params lsn, lastReadSize);
	lastLSN = lsn;
	return CDVD->readTrack(lsn,mode);
}

s32 DoCDVDgetBuffer(u8* buffer)
{
	CheckNullCDVD();
	int ret = CDVD->getBuffer2(buffer);

	if (ret == 0 && blockDumpFile != NULL)
	{
		isoWriteBlock(blockDumpFile, buffer, lastLSN);
	}

	return ret;
}

s32 DoCDVDdetectDiskType()
{
	CheckNullCDVD();
	if(diskTypeCached < 0) DetectDiskType();
	return diskTypeCached;
}

void DoCDVDresetDiskTypeCache()
{
	diskTypeCached = -1;
}

////////////////////////////////////////////////////////
//
// CDVD null interface for Run BIOS menu



s32 CALLBACK NODISCopen(const char* pTitle)
{
	return 0;
}

void CALLBACK NODISCclose()
{
}

s32 CALLBACK NODISCreadTrack(u32 lsn, int mode)
{
	return -1;
}

// return can be NULL (for async modes)
u8* CALLBACK NODISCgetBuffer()
{
	return NULL;
}

s32 CALLBACK NODISCreadSubQ(u32 lsn, cdvdSubQ* subq)
{
	return -1;
}

s32 CALLBACK NODISCgetTN(cdvdTN *Buffer)
{
	return -1;
}

s32 CALLBACK NODISCgetTD(u8 Track, cdvdTD *Buffer)
{
	return -1;
}

s32 CALLBACK NODISCgetTOC(void* toc)
{
	return -1;
}

s32 CALLBACK NODISCgetDiskType()
{
	return CDVD_TYPE_NODISC;
}

s32 CALLBACK NODISCgetTrayStatus()
{
	return CDVD_TRAY_CLOSE;
}

s32 CALLBACK NODISCdummyS32()
{
	return 0;
}

void CALLBACK NODISCnewDiskCB(__unused void (*callback)())
{
}

s32 CALLBACK NODISCreadSector(u8* tempbuffer, u32 lsn, int mode)
{
	return -1;
}

s32 CALLBACK NODISCgetBuffer2(u8* buffer)
{
	return -1;
}

s32 CALLBACK NODISCgetDualInfo(s32* dualType, u32* _layer1start)
{
	return -1;
}

wxString NODISCgetUniqueFilename()
{
	DevAssert( false, "NODISC is an invalid CDVD object for block dumping.. >_<" );
	return L"epicfail";
}

CDVD_API CDVDapi_NoDisc =
{
	NODISCclose,
	NODISCopen,
	NODISCreadTrack,
	NODISCgetBuffer,
	NODISCreadSubQ,
	NODISCgetTN,
	NODISCgetTD,
	NODISCgetTOC,
	NODISCgetDiskType,
	NODISCgetTrayStatus,
	NODISCdummyS32,
	NODISCdummyS32,

	NODISCnewDiskCB,

	NODISCreadSector,
	NODISCgetBuffer2,
	NODISCgetDualInfo,

	NODISCgetUniqueFilename
};
