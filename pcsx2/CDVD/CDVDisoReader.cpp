/*  PCSX2 - PS2 Emulator for PCs
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


/*
 *  Original code from libcdvd by Hiryu & Sjeep (C) 2002
 *  Modified by Florin for PCSX2 emu
 *  Fixed CdRead by linuzappz
 */

#include "PrecompiledHeader.h"

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>

#include "CDVDisoReader.h"

static u8 *pbuffer;
static u8 cdbuffer[CD_FRAMESIZE_RAW] = {0};
static isoFile iso;

static int psize, cdtype;

static s32 layer1start = -1;
static bool layer1searched = false;

void CALLBACK ISOclose()
{
	iso.Close();
}

s32 CALLBACK ISOopen(const char* pTitle)
{
	ISOclose();		// just in case

	if( (pTitle == NULL) || (pTitle[0] == 0) )
	{
		Console.Error( "CDVDiso Error: No filename specified." );
		return -1;
	}

	// The current plugin API doesn't expect exceptions to propagate out of the API
	// calls, so we need to catch them, log them, and return -1.

	try {
		iso.Open(fromUTF8(pTitle));
	}
	catch( BaseException& ex )
	{
		Console.Error( ex.FormatDiagnosticMessage() );
		return -1;
	}

	switch (iso.GetType())
	{
		case ISOTYPE_DVD:
			cdtype = CDVD_TYPE_PS2DVD;
			break;
		case ISOTYPE_AUDIO:
			cdtype = CDVD_TYPE_CDDA;
			break;
		default:
			cdtype = CDVD_TYPE_PS2CD;
			break;
	}

	layer1start = -1;
	layer1searched = false;
	
	return 0;
}

s32 CALLBACK ISOreadSubQ(u32 lsn, cdvdSubQ* subq)
{
	// fake it
	u8 min, sec, frm;
	subq->ctrl		= 4;
	subq->mode		= 1;
	subq->trackNum	= itob(1);
	subq->trackIndex = itob(1);

	lba_to_msf(lsn, &min, &sec, &frm);
	subq->trackM	= itob(min);
	subq->trackS	= itob(sec);
	subq->trackF	= itob(frm);

	subq->pad		= 0;

	lba_to_msf(lsn + (2*75), &min, &sec, &frm);
	subq->discM		= itob(min);
	subq->discS		= itob(sec);
	subq->discF		= itob(frm);

	return 0;
}

s32 CALLBACK ISOgetTN(cdvdTN *Buffer)
{
	Buffer->strack = 1;
	Buffer->etrack = 1;

	return 0;
}

s32 CALLBACK ISOgetTD(u8 Track, cdvdTD *Buffer)
{
	if (Track == 0)
	{
		Buffer->lsn = iso.GetBlockCount();
	}
	else
	{
		Buffer->type = CDVD_MODE1_TRACK;
		Buffer->lsn = 0;
	}

	return 0;
}

#include "gui/App.h"
#include "Utilities/HashMap.h"

static bool testForPartitionInfo( const u8 (&tempbuffer)[CD_FRAMESIZE_RAW] )
{
	const int off = iso.GetBlockOffset();

	// test for: CD001
	return (
		(tempbuffer[off+1] == 0x43) &&
		(tempbuffer[off+2] == 0x44) &&
		(tempbuffer[off+3] == 0x30) &&
		(tempbuffer[off+4] == 0x30) &&
		(tempbuffer[off+5] == 0x31)
	);
}

static void FindLayer1Start()
{
	if (iso.GetBlockCount() < 0x230540) return;
	if (layer1start != -1) return;
	if (layer1searched) return;

	Console.WriteLn("isoFile: searching for layer1...");
	layer1searched = true;

	int blockresult = -1;

	// Check the ini file cache first:
	// Cache is stored in LayerBreakCache.ini, and is associated by hex-encoded hash key of the
	// complete filename/path of the iso file. :)

	wxString layerCacheFile( Path::Combine(GetSettingsFolder().ToString(), L"LayerBreakCache.ini") );
	wxFileConfig layerCacheIni( wxEmptyString, wxEmptyString, layerCacheFile, wxEmptyString, wxCONFIG_USE_RELATIVE_PATH );

	FastFormatUnicode cacheKey;
	cacheKey.Write( L"%X", HashTools::Hash( (s8*)iso.GetFilename().c_str(), iso.GetFilename().Length() * sizeof(wxChar) ) );

	blockresult = layerCacheIni.Read( cacheKey, -1 );
	if( blockresult != -1 )
	{
		u8 tempbuffer[CD_FRAMESIZE_RAW];
		iso.ReadBlock(tempbuffer, blockresult);

		if( testForPartitionInfo( tempbuffer ) )
		{
			Console.WriteLn( "isoFile: loaded second layer from settings cache, sector=0x%08x", blockresult );
			layer1start = blockresult;
		}
		else
		{
			Console.Warning( "isoFile: second layer info in the settings cache appears to be obsolete or invalid.  Ignoring..." );
		}
	}
	else
	{
		DevCon.WriteLn( "isoFile: no cached info for second layer found." );
	}

	if( layer1start == -1 )
	{

		// Layer sizes are arbitrary, and either layer could be the smaller (GoW and Rogue Galaxy
		// both have Layer1 larger than Layer0 for example), so we have to brute-force the search
		// from some arbitrary start position.
		//
		// Method: Inside->out.  We start at the middle of the image and work our way out toward
		// both the beginning and end of the image at the same time.  Most images have the layer
		// break quite close to the middle of the image, so this should be pretty fast in most cases.

		// [TODO] Layer searching can be slow, especially for compressed disc images, so it would
		// be quite courteous to pop up a status dialog bar that lets the user know that it's
		// thinking.  Since we're not on the GUI thread, we'll need to establish some messages
		// to create the window and pass progress increments back to it.


		uint midsector = (iso.GetBlockCount() / 2) & ~0xf;
		uint deviation = 0;

		while( (layer1start == -1) && (deviation < midsector-16) )
		{
			u8 tempbuffer[CD_FRAMESIZE_RAW];
			iso.ReadBlock(tempbuffer, midsector-deviation);

			if(testForPartitionInfo( tempbuffer ))
				layer1start = midsector-deviation;
			else
			{
				iso.ReadBlock(tempbuffer, midsector+deviation);
				if( testForPartitionInfo( tempbuffer ) )
					layer1start = midsector+deviation;
			}

			if( layer1start != -1 )
			{
				const int blockofs = iso.GetBlockOffset();
				if( !pxAssertDev( tempbuffer[blockofs] == 0x01, "Layer1-Detect: CD001 tag found, but the partition type is invalid." ) )
				{
					Console.Error( "isoFile: Invalid partition type on layer 1!? (type=0x%x)", tempbuffer[blockofs] );
				}
			}
			deviation += 16;
		}

		if( layer1start == -1 )
		{
			Console.Error("isoFile: Couldn't find layer1... iso image is probably corrupt or incomplete.");
		}
		else
		{
			Console.WriteLn( Color_Blue, "isoFile: second layer found at sector 0x%08x", layer1start);

			// Save layer information to configuration:

			layerCacheIni.Write( cacheKey, layer1start );
		}
	}
}

// Should return 0 if no error occurred, or -1 if layer detection FAILED.
s32 CALLBACK ISOgetDualInfo(s32* dualType, u32* _layer1start)
{
	FindLayer1Start();

	if (layer1start < 0)
	{
		*dualType = 0;
		*_layer1start = iso.GetBlockCount();
	}
	else
	{
		*dualType = 1;
		*_layer1start = layer1start;
	}
	return 0;
}

s32 CALLBACK ISOgetDiskType()
{
	return cdtype;
}

s32 CALLBACK ISOgetTOC(void* toc)
{
	u8 type = ISOgetDiskType();
	u8* tocBuff = (u8*)toc;

	//CDVD_LOG("CDVDgetTOC\n");

	if (type == CDVD_TYPE_DVDV || type == CDVD_TYPE_PS2DVD)
	{
		// get dvd structure format
		// scsi command 0x43
		memset(tocBuff, 0, 2048);

		FindLayer1Start();

		if (layer1start < 0)
		{
			// fake it
			tocBuff[ 0] = 0x04;
			tocBuff[ 1] = 0x02;
			tocBuff[ 2] = 0xF2;
			tocBuff[ 3] = 0x00;
			tocBuff[ 4] = 0x86;
			tocBuff[ 5] = 0x72;

			tocBuff[16] = 0x00;
			tocBuff[17] = 0x03;
			tocBuff[18] = 0x00;
			tocBuff[19] = 0x00;
			return 0;
		}
		else
		{
			// dual sided
			tocBuff[ 0] = 0x24;
			tocBuff[ 1] = 0x02;
			tocBuff[ 2] = 0xF2;
			tocBuff[ 3] = 0x00;
			tocBuff[ 4] = 0x41;
			tocBuff[ 5] = 0x95;

			tocBuff[14] = 0x60; // dual sided, ptp

			tocBuff[16] = 0x00;
			tocBuff[17] = 0x03;
			tocBuff[18] = 0x00;
			tocBuff[19] = 0x00;

			s32 l1s = layer1start + 0x30000 - 1;
			tocBuff[20] = (l1s >> 24);
			tocBuff[21] = (l1s >> 16) & 0xff;
			tocBuff[22] = (l1s >> 8) & 0xff;
			tocBuff[23] = (l1s >> 0) & 0xff;
		}
	}
	else if ((type == CDVD_TYPE_CDDA) || (type == CDVD_TYPE_PS2CDDA) ||
		(type == CDVD_TYPE_PS2CD) || (type == CDVD_TYPE_PSCDDA) || (type == CDVD_TYPE_PSCD))
	{
		// cd toc
		// (could be replaced by 1 command that reads the full toc)
		u8 min, sec, frm;
		s32 i, err;
		cdvdTN diskInfo;
		cdvdTD trackInfo;
		memset(tocBuff, 0, 1024);
		if (ISOgetTN(&diskInfo) == -1)
		{
			diskInfo.etrack = 0;
			diskInfo.strack = 1;
		}
		if (ISOgetTD(0, &trackInfo) == -1) trackInfo.lsn = 0;

		tocBuff[0] = 0x41;
		tocBuff[1] = 0x00;

		//Number of FirstTrack
		tocBuff[2] = 0xA0;
		tocBuff[7] = itob(diskInfo.strack);

		//Number of LastTrack
		tocBuff[12] = 0xA1;
		tocBuff[17] = itob(diskInfo.etrack);

		//DiskLength
		lba_to_msf(trackInfo.lsn, &min, &sec, &frm);
		tocBuff[22] = 0xA2;
		tocBuff[27] = itob(min);
		tocBuff[28] = itob(sec);

		for (i = diskInfo.strack; i <= diskInfo.etrack; i++)
		{
			err = ISOgetTD(i, &trackInfo);
			lba_to_msf(trackInfo.lsn, &min, &sec, &frm);
			tocBuff[i*10+30] = trackInfo.type;
			tocBuff[i*10+32] = err == -1 ? 0 : itob(i);	  //number
			tocBuff[i*10+37] = itob(min);
			tocBuff[i*10+38] = itob(sec);
			tocBuff[i*10+39] = itob(frm);
		}
	}
	else
		return -1;

	return 0;
}

s32 CALLBACK ISOreadSector(u8* tempbuffer, u32 lsn, int mode)
{
	int _lsn = lsn;

	if (_lsn < 0) lsn = iso.GetBlockCount() + _lsn;
	if (lsn > iso.GetBlockCount()) return -1;

	if(mode == CDVD_MODE_2352)
	{
		iso.ReadBlock(tempbuffer, lsn);
		return 0;
	}

	iso.ReadBlock(cdbuffer, lsn);

	pbuffer = cdbuffer;

	switch (mode)
	{
		case CDVD_MODE_2352:
			// Unreachable due to shortcut above.
			pxAssume(false);
			break;

		case CDVD_MODE_2340:
			pbuffer += 12;
			psize = 2340;
			break;
		case CDVD_MODE_2328:
			pbuffer += 24;
			psize = 2328;
			break;
		case CDVD_MODE_2048:
			pbuffer += 24;
			psize = 2048;
			break;
		
		jNO_DEFAULT
	}

	memcpy_fast(tempbuffer, pbuffer, psize);

	return 0;
}

s32 CALLBACK ISOreadTrack(u32 lsn, int mode)
{
	int _lsn = lsn;

	if (_lsn < 0) lsn = iso.GetBlockCount() + _lsn;
	if (lsn > iso.GetBlockCount()) return -1;

	iso.ReadBlock(cdbuffer, lsn);
	pbuffer = cdbuffer;

	switch (mode)
	{
	case CDVD_MODE_2352:
		psize = 2352;
		break;
	case CDVD_MODE_2340:
		pbuffer += 12;
		psize = 2340;
		break;
	case CDVD_MODE_2328:
		pbuffer += 24;
		psize = 2328;
		break;
	case CDVD_MODE_2048:
		pbuffer += 24;
		psize = 2048;
		break;
	}

	return 0;
}

s32 CALLBACK ISOgetBuffer2(u8* buffer)
{
	memcpy_fast(buffer, pbuffer, psize);
	return 0;
}

u8* CALLBACK ISOgetBuffer()
{
	return pbuffer;
}

s32 CALLBACK ISOgetTrayStatus()
{
	return CDVD_TRAY_CLOSE;
}

s32 CALLBACK ISOctrlTrayOpen()
{
	return 0;
}
s32 CALLBACK ISOctrlTrayClose()
{
	return 0;
}

s32 CALLBACK ISOdummyS32()
{
	return 0;
}

void CALLBACK ISOnewDiskCB(void(* /* callback */)())
{
}

CDVD_API CDVDapi_Iso =
{
	ISOclose,

	ISOopen,
	ISOreadTrack,
	ISOgetBuffer, // emu shouldn't use this one.
	ISOreadSubQ,
	ISOgetTN,
	ISOgetTD,
	ISOgetTOC,
	ISOgetDiskType,
	ISOdummyS32,	// trayStatus
	ISOdummyS32,	// trayOpen
	ISOdummyS32,	// trayClose

	ISOnewDiskCB,

	ISOreadSector,
	ISOgetBuffer2,
	ISOgetDualInfo,
};
