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


#include "PrecompiledHeader.h"
#include "IopCommon.h"
#include "IsoFileFormats.h"

#include <errno.h>

static const char* nameFromType(int type)
{
    switch(type)
    {
        case ISOTYPE_CD:		return "CD";
        case ISOTYPE_DVD:		return "DVD";
        case ISOTYPE_AUDIO:		return "Audio CD";
        case ISOTYPE_DVDDL:		return "DVD9 (dual-layer)";
        case ISOTYPE_ILLEGAL:	return "Illegal media";
        default:				return "Unknown or corrupt";
    }
}

int InputIsoFile::ReadSync(u8* dst, uint lsn)
{
	if (lsn > m_blocks)
	{
		FastFormatUnicode msg;
		msg.Write("isoFile error: Block index is past the end of file! (%u > %u).", lsn, m_blocks);

		pxAssertDev(false, msg);
		Console.Error(msg);
		return -1;
	}

	return m_reader->ReadSync(dst+m_blockofs, lsn, 1);
}

void InputIsoFile::BeginRead2(uint lsn)
{
	if (lsn > m_blocks)
	{
		FastFormatUnicode msg;
		msg.Write("isoFile error: Block index is past the end of file! (%u > %u).", lsn, m_blocks);

		pxAssertDev(false, msg);
		Console.Error(msg);

		// [TODO] : Throw exception?
		//  Typically an error like this is bad; indicating an invalid dump or corrupted
		//  iso file.

		m_current_lsn = -1;
		return;
	}
	
	m_current_lsn = lsn;

	if(lsn >= m_read_lsn && lsn < (m_read_lsn+m_read_count))
	{
		// Already buffered
		return;
	}

	m_read_lsn = lsn;
	m_read_count = 1;

	if(ReadUnit > 1)
	{
		//m_read_lsn   = lsn - (lsn % ReadUnit);

		m_read_count = min(ReadUnit, m_blocks - m_read_lsn);
	}

	m_reader->BeginRead(m_readbuffer, m_read_lsn, m_read_count);
	m_read_inprogress = true;
}

int InputIsoFile::FinishRead3(u8* dst, uint mode)
{
	int _offset, length;
	int ret = 0;

	if(m_current_lsn < 0)
		return -1;

	if(m_read_inprogress)
	{
		ret = m_reader->FinishRead();
		m_read_inprogress = false;

		if(ret < 0)
			return ret;
	}
		
	switch (mode)
	{
	case CDVD_MODE_2352:
		_offset = 0;
		length = 2352;
		break;
	case CDVD_MODE_2340:
		_offset = 12;
		length = 2340;
		break;
	case CDVD_MODE_2328:
		_offset = 24;
		length = 2328;
		break;
	case CDVD_MODE_2048:
		_offset = 24;
		length = 2048;
		break;
	}

	int end1 = m_blockofs + m_blocksize;
	int end2 = _offset + length;
	int end = min(end1, end2);

	int diff = m_blockofs - _offset;
	int ndiff = 0;
	if(diff > 0)
	{
		memset(dst, 0, diff);
		_offset = m_blockofs;
	}
	else 
	{
		ndiff = -diff;
		diff = 0;
	}

	length = end - _offset;

	uint read_offset = (m_current_lsn - m_read_lsn) * m_blocksize;
	memcpy_fast(dst + diff, m_readbuffer + ndiff + read_offset, length);
	
	if (m_type == ISOTYPE_CD && diff >= 12)
	{
		lsn_to_msf(dst + diff - 12, m_current_lsn);
		dst[diff - 9] = 2;
	}

	return 0;
}

InputIsoFile::InputIsoFile()
{
	_init();
}

InputIsoFile::~InputIsoFile() throw()
{
	Close();
}

void InputIsoFile::_init()
{
	m_type			= ISOTYPE_ILLEGAL;
	m_flags			= 0;

	m_offset		= 0;
	m_blockofs		= 0;
	m_blocksize		= 0;
	m_blocks		= 0;
	
	m_read_inprogress = false;
	m_read_count = 0;
	m_read_lsn = -1;
}

// Tests the specified filename to see if it is a supported ISO type.  This function typically
// executes faster than IsoFile::Open since it does not do the following:
//  * check for multi-part ISOs.  I tests for header info in the main/root ISO only.
//  * load blockdump indexes.
//
// Note that this is a member method, and that it will clobber any existing ISO state.
// (assertions are generated in debug mode if the object state is not already closed).
bool InputIsoFile::Test( const wxString& srcfile )
{
	Close();
	m_filename = srcfile;

	return Open(srcfile, true);
}

bool InputIsoFile::Open( const wxString& srcfile, bool testOnly )
{
	Close();
	m_filename = srcfile;
	
	m_reader = new FlatFileReader();
	m_reader->Open(m_filename);

	bool isBlockdump, isCompressed = false;
	if(isBlockdump = BlockdumpFileReader::DetectBlockdump(m_reader))
	{
		delete m_reader;

		BlockdumpFileReader *bdr = new BlockdumpFileReader();;

		bdr->Open(m_filename);

		m_blockofs = bdr->GetBlockOffset();
		m_blocksize = bdr->GetBlockSize();

		m_reader = bdr;

		ReadUnit = 1;		
	} else if (isCompressed = CompressedFileReader::DetectCompressed(m_reader)) {
		delete m_reader;
		m_reader = CompressedFileReader::GetNewReader(m_filename);
		m_reader->Open(m_filename);
	}

	bool detected = Detect();
		
	if(testOnly)
		return detected;
		
	if (!detected)
		throw Exception::BadStream()
			.SetUserMsg(_("Unrecognized ISO image file format"))
			.SetDiagMsg(L"ISO mounting failed: PCSX2 is unable to identify the ISO image type.");

	if(!isBlockdump && !isCompressed)
	{
		ReadUnit = MaxReadUnit;
		
		m_reader->SetDataOffset(m_offset);
		m_reader->SetBlockSize(m_blocksize);
	
		// Returns the original reader if single-part or a Multipart reader otherwise
		m_reader =	MultipartFileReader::DetectMultipart(m_reader);
	}

	m_blocks = m_reader->GetBlockCount();

	Console.WriteLn(Color_StrongBlue, L"isoFile open ok: %s", m_filename.c_str());

	ConsoleIndentScope indent;

	Console.WriteLn("Image type  = %s", nameFromType(m_type)); 
	//Console.WriteLn("Fileparts   = %u", m_numparts); // Pointless print, it's 1 unless it says otherwise above
	DevCon.WriteLn ("blocks      = %u", m_blocks);
	DevCon.WriteLn ("offset      = %d", m_offset);
	DevCon.WriteLn ("blocksize   = %u", m_blocksize);
	DevCon.WriteLn ("blockoffset = %d", m_blockofs);

	return true;
}

void InputIsoFile::Close()
{
	delete m_reader;
	m_reader = NULL;
	
	_init();
}

bool InputIsoFile::IsOpened() const
{
	return m_reader != NULL;
}

bool InputIsoFile::tryIsoType(u32 _size, s32 _offset, s32 _blockofs)
{
	static u8 buf[2456];

	m_blocksize	= _size;
	m_offset	= _offset;
	m_blockofs	= _blockofs;
	
	m_reader->SetDataOffset(_offset);
	m_reader->SetBlockSize(_size);
	
	if(ReadSync(buf, 16) < 0)
		return false;

	if (strncmp((char*)(buf+25), "CD001", 5)) // Not ISO 9660 compliant
		return false;

	m_type = (*(u16*)(buf+190) == 2048) ? ISOTYPE_CD : ISOTYPE_DVD;

	return true; // We can deal with this.
}

// based on florin's CDVDbin detection code :)
// Parameter:
//
// 
// Returns true if the image is valid/known/supported, or false if not (type == ISOTYPE_ILLEGAL).
bool InputIsoFile::Detect( bool readType )
{
	m_type = ISOTYPE_ILLEGAL;

	AsyncFileReader* headpart = m_reader;
		
	// First sanity check: no sane CD image has less than 16 sectors, since that's what
	// we need simply to contain a TOC.  So if the file size is not large enough to
	// accommodate that, it is NOT a CD image --->

	int sectors = headpart->GetBlockCount();

	if (sectors < 17)
		return false;

	m_blocks = 17;

	if (tryIsoType(2048, 0, 24)) return true;				// ISO 2048
	if (tryIsoType(2336, 0, 16)) return true;				// RAW 2336
	if (tryIsoType(2352, 0, 0)) return true; 				// RAW 2352
	if (tryIsoType(2448, 0, 0)) return true; 				// RAWQ 2448

	if (tryIsoType(2048, 150 * 2048, 24)) return true;		// NERO ISO 2048
	if (tryIsoType(2352, 150 * 2048, 0)) return true;		// NERO RAW 2352
	if (tryIsoType(2448, 150 * 2048, 0)) return true;		// NERO RAWQ 2448

	if (tryIsoType(2048, -8, 24)) return true; 				// ISO 2048
	if (tryIsoType(2352, -8, 0)) return true;				// RAW 2352
	if (tryIsoType(2448, -8, 0)) return true;				// RAWQ 2448

	m_offset	= 0;
	m_blocksize	= CD_FRAMESIZE_RAW;
	m_blockofs	= 0;
	m_type		= ISOTYPE_AUDIO;
	
	m_reader->SetDataOffset(m_offset);
	m_reader->SetBlockSize(m_blocksize);

	//BUG: This also detects a memory-card-file as a valid Audio-CD ISO... -avih
	return true;
}
