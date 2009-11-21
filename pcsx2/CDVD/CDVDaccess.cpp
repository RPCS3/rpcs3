/*  PCSX2 - PS2 Emulator for PCs
 *  Copyright (C) 2002-2009  PCSX2 Dev Team
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


#include "PrecompiledHeader.h"
#include "IopCommon.h"

#define ENABLE_TIMESTAMPS

#ifdef _WIN32
#	include <wx/msw/wrapwin.h>
#endif

#include <ctype.h>
#include <time.h>
#include <wx/datetime.h>
#include <exception>

#include "IsoFS/IsoFS.h"
#include "IsoFS/IsoFSCDVD.h"
#include "CDVDisoReader.h"
#include "Utilities/ScopedPtr.h"

const wxChar* CDVD_SourceLabels[] =
{
	L"Iso",
	L"Plugin",
	L"NoDisc",
	NULL
};

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
//static int plsn = 0;
static isoFile *blockDumpFile = NULL;

// Assertion check for CDVD != NULL (in devel and debug builds), because its handier than
// relying on DEP exceptions -- and a little more reliable too.
static void CheckNullCDVD()
{
	pxAssertDev( CDVD != NULL, "Invalid CDVD object state (null pointer exception)" );
}

//////////////////////////////////////////////////////////////////////////////////////////
// Disk Type detection stuff (from cdvdGigaherz)
//
static int CheckDiskTypeFS(int baseType)
{
	IsoFSCDVD isofs;
	IsoDirectory rootdir(isofs);
	try {
		IsoFile file( rootdir, L"SYSTEM.CNF;1");

		int size = file.getLength();

		ScopedArray<char> buffer((int)file.getLength()+1);
		file.read((u8*)(buffer.GetPtr()),size);
		buffer[size]='\0';

		char* pos = strstr(buffer.GetPtr(), "BOOT2");
		if (pos == NULL)
		{
			pos = strstr(buffer.GetPtr(), "BOOT");
			if (pos == NULL)  return CDVD_TYPE_ILLEGAL;
			return CDVD_TYPE_PSCD;
		}

		return (baseType==CDVD_TYPE_DETCTCD) ? CDVD_TYPE_PS2CD : CDVD_TYPE_PS2DVD;
	}
	catch( Exception::FileNotFound& )
	{
	}

	if( rootdir.IsFile(L"PSX.EXE;1") )
		return CDVD_TYPE_PSCD;

	if( rootdir.IsFile(L"VIDEO_TS/VIDEO_TS.IFO;1") )
		return CDVD_TYPE_DVDV;

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
				//const cdVolDesc& volDesc = (cdVolDesc&)bleh;
				//if(volDesc.rootToc.tocSize == 2048)
				if(*(u16*)(bleh+166) == 2048)
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
			Console.WriteLn(" * CDVD Disk Open: CD, %d tracks (%d to %d):", tn.etrack-tn.strack+1,tn.strack,tn.etrack);
			break;

		case CDVD_TYPE_DETCTDVDS:
			Console.WriteLn(" * CDVD Disk Open: DVD, Single layer or unknown:");
			break;

		case CDVD_TYPE_DETCTDVDD:
			Console.WriteLn(" * CDVD Disk Open: DVD, Double layer:");
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
			Console.WriteLn(" * * Track %d: Audio (%d sectors)", i,tlength);
		}
		else
		{
			dataTracks++;
			Console.WriteLn(" * * Track %d: Data (Mode %d) (%d sectors)", i,((td.type==CDVD_MODE1_TRACK)?1:2),tlength);
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

static wxString			m_SourceFilename[3];
static CDVD_SourceType	m_CurrentSourceType = CDVDsrc_NoDisc;

void CDVDsys_SetFile( CDVD_SourceType srctype, const wxString& newfile )
{
	m_SourceFilename[srctype] = newfile;
}

CDVD_SourceType CDVDsys_GetSourceType()
{
	return m_CurrentSourceType;
}

void CDVDsys_ChangeSource( CDVD_SourceType type )
{
	GetPluginManager().Close( PluginId_CDVD );

	switch( m_CurrentSourceType = type )
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

bool DoCDVDopen()
{
	CheckNullCDVD();

	// the new disk callback is set on Init also, but just in case the plugin clears it for
	// some reason on close, we re-send here:
	CDVD->newDiskCB( cdvdNewDiskCB );

	// Win32 Fail: the old CDVD api expects MBCS on Win32 platforms, but generating a MBCS
	// from unicode is problematic since we need to know the codepage of the text being
	// converted (which isn't really practical knowledge).  A 'best guess' would be the
	// default codepage of the user's Windows install, but even that will fail and return
	// question marks if the filename is another language.
	// Likely Fix: Force new versions of CDVD plugins to expect UTF8 instead.

	int ret = CDVD->open( !m_SourceFilename[m_CurrentSourceType].IsEmpty() ?
		m_SourceFilename[m_CurrentSourceType].ToUTF8() : (char*)NULL
	);

	if( ret == -1 ) return false;

	int cdtype = DoCDVDdetectDiskType();

	if (EmuConfig.CdvdDumpBlocks && (cdtype != CDVD_TYPE_NODISC))
	{
		// TODO: Add a blockdumps configurable folder, and use that instead of CWD().

		// TODO: "Untitled" should use pnach/slus name resolution, slus if no patch,
		// and finally an "Untitled-[ElfCRC]" if no slus.

		wxString somepick( Path::GetFilenameWithoutExt( m_SourceFilename[m_CurrentSourceType] ) );
		if( somepick.IsEmpty() )
			somepick = L"Untitled";

		wxString temp( Path::Combine( wxGetCwd(), somepick ) );

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

		blockDumpFile = isoCreate(temp.ToUTF8(), ISOFLAGS_BLOCKDUMP_V3);

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

	return true;
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

	//DevCon.Warning("CDVD readTrack(lsn=%d,mode=%d)",params lsn, lastReadSize);
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

void CALLBACK NODISCnewDiskCB(void (* /* callback */)())
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
};
