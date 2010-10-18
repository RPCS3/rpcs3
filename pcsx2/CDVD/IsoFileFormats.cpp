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

static const uint BlockDumpHeaderSize = 16;

bool isoFile::detect()
{
	u8 buf[2456];
	u8* pbuf;

	ReadBlock(buf, 16);

	pbuf = buf + 24;

	if (strncmp((char*)(pbuf+1), "CD001", 5)) return false; // Not ISO 9660 compliant

	if (*(u16*)(pbuf+166) == 2048)
		m_type = ISOTYPE_CD;
	else
		m_type = ISOTYPE_DVD;

	return true; // We can deal with this.
}

void isoFile::_ReadDtable()
{
	_IsoPart& headpart( m_parts[0] );

	wxFileOffset flen = headpart.handle->GetLength();
	m_dtablesize	= (flen - BlockDumpHeaderSize) / (m_blocksize + 4);
	m_dtable		= new u32[m_dtablesize];

	headpart.Seek(BlockDumpHeaderSize);

	for (int i=0; i < m_dtablesize; ++i)
	{
		headpart.Read(m_dtable[i]);
		headpart.Seek(m_blocksize, wxFromCurrent);
	}
}

bool isoFile::tryIsoType(u32 _size, s32 _offset, s32 _blockofs)
{
	m_blocksize	= _size;
	m_offset	= _offset;
	m_blockofs	= _blockofs;

	return detect();
}

// based on florin's CDVDbin detection code :)
// Parameter:
//
// 
// Returns true if the image is valid/known/supported, or false if not (type == ISOTYPE_ILLEGAL).
bool isoFile::Detect( bool readType )
{
	char buf[32];
	int len = m_filename.Length();

	m_type = ISOTYPE_ILLEGAL;

	_IsoPart& headpart( m_parts[0] );

	headpart.Seek( 0 );
	headpart.Read( buf, 4 );

	if (strncmp(buf, "BDV2", 4) == 0)
	{
		m_flags = ISOFLAGS_BLOCKDUMP_V2;
		headpart.Read(m_blocksize);
		headpart.Read(m_blocks);
		headpart.Read(m_blockofs);

		if (readType)
		{
			_ReadDtable();
			return detect();
		}
		return true;
	}

	// First sanity check: no sane CD image has less than 16 sectors, since that's what
	// we need simply to contain a TOC.  So if the file size is not large enough to
	// accommodate that, it is NOT a CD image --->

	wxFileOffset size = headpart.handle->GetLength();

	if (size < (2048 * 16)) return false;

	m_blocks = 16;

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

	return true;
}

// Generates format header information for blockdumps.
void isoFile::WriteFormat(int _blockofs, uint _blocksize, uint _blocks)
{
	m_blocksize	= _blocksize;
	m_blocks	= _blocks;
	m_blockofs	= _blockofs;

	Console.WriteLn("blockoffset = %d", m_blockofs);
	Console.WriteLn("blocksize   = %u", m_blocksize);
	Console.WriteLn("blocks	     = %u", m_blocks);

	if (m_flags & ISOFLAGS_BLOCKDUMP_V2)
	{
		outWrite("BDV2", 4);
		outWrite(m_blocksize);
		outWrite(m_blocks);
		outWrite(m_blockofs);
	}
}

void isoFile::_ReadBlockD(u8* dst, uint lsn)
{
	_IsoPart& headpart( m_parts[0] );

//	Console.WriteLn("_isoReadBlockD %u, blocksize=%u, blockofs=%u\n", lsn, iso->blocksize, iso->blockofs);

	memset(dst, 0, m_blockofs);
	for (int i = 0; i < m_dtablesize; ++i)
	{
		if (m_dtable[i] != lsn) continue;

		// We store the LSN (u32) along with each block inside of blockdumps, so the
		// seek position ends up being based on (m_blocksize + 4) instead of just m_blocksize.

#ifdef PCSX2_DEBUG
		u32 check_lsn;
		headpart.Seek( BlockDumpHeaderSize + (i * (m_blocksize + 4)) );
		m_parts[0].Read( check_lsn );
		pxAssert( check_lsn == lsn );
#else
		headpart.Seek( BlockDumpHeaderSize + (i * (m_blocksize + 4)) + 4 );
#endif

		m_parts[0].Read( dst + m_blockofs, m_blocksize );
	}

	Console.WriteLn("Block %u not found in dump", lsn);
}

void isoFile::_ReadBlock(u8* dst, uint lsn)
{
	pxAssumeMsg(lsn <= m_blocks,	"Invalid lsn passed into isoFile::_ReadBlock.");
	pxAssumeMsg(m_numparts,			"Invalid isoFile object state; an iso file needs at least one part!");

	uint i;
	for (i = 0; i < m_numparts-1; ++i)
	{
		// lsn indexes should always go in order; use an assertion just to be sure:
		pxAssume(lsn >= m_parts[i].slsn);
		if (lsn <= m_parts[i].elsn) break;
	}

	wxFileOffset ofs = (wxFileOffset)(lsn - m_parts[i].slsn) * m_blocksize + m_offset;

//	Console.WriteLn("_isoReadBlock %u, blocksize=%u, blockofs=%u\n", lsn, iso->blocksize, iso->blockofs);

	memset(dst, 0, m_blockofs);
	m_parts[i].Seek(ofs);
	m_parts[i].Read(dst + m_blockofs, m_blocksize);
}

void isoFile::ReadBlock(u8* dst, uint lsn)
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

		return;
	}

	if (m_flags == ISOFLAGS_BLOCKDUMP_V2)
		_ReadBlockD(dst, lsn);
	else
		_ReadBlock(dst, lsn);

	if (m_type == ISOTYPE_CD)
	{
		lsn_to_msf(dst + 12, lsn);
		dst[15] = 2;
	}
}

void isoFile::_WriteBlock(const u8* src, uint lsn)
{
	wxFileOffset ofs = (wxFileOffset)lsn * m_blocksize + m_offset;

	m_outstream->SeekO( ofs );
	outWrite( src + m_blockofs, m_blocksize );
}

void isoFile::_WriteBlockD(const u8* src, uint lsn)
{
	outWrite<u32>( lsn );
	outWrite( src + m_blockofs, m_blocksize );
}

void isoFile::WriteBlock(const u8* src, uint lsn)
{
	if (m_flags == ISOFLAGS_BLOCKDUMP_V2)
		_WriteBlockD(src, lsn);
	else
		_WriteBlock(src, lsn);
}

// --------------------------------------------------------------------------------------
//  IsoFile (implementations) : Init / Open / Create
// --------------------------------------------------------------------------------------

isoFile::isoFile()
{
	_init();
}

isoFile::~isoFile() throw()
{
	Close();
}

void isoFile::_init()
{
	m_type			= ISOTYPE_ILLEGAL;
	m_flags			= 0;

	m_offset		= 0;
	m_blockofs		= 0;
	m_blocksize		= 0;
	m_blocks		= 0;

	m_dtable		= 0;
	m_dtablesize	= 0;
}

// Tests for a filename extension in both upper and lower case, if the filesystem happens
// to be case-sensitive.
bool pxFileExists_WithExt( const wxFileName& filename, const wxString& ext )
{
	wxFileName part1 = filename;
	part1.SetExt( ext.Lower() );

	if (part1.FileExists()) return true;
	if (!wxFileName::IsCaseSensitive()) return false;

	part1.SetExt( ext.Upper() );
	return part1.FileExists();
}

void pxStream_OpenCheck( const wxStreamBase& stream, const wxString& fname, const wxString& mode )
{
	if (stream.IsOk()) return;

	ScopedExcept ex(Exception::FromErrno(fname, errno));
	ex->SetDiagMsg( pxsFmt(L"Unable to open the file for %s: %s", mode.c_str(), ex->DiagMsg().c_str()) );
	ex->Rethrow();
}

// multi-part ISO support is provided for FAT32 compatibility; so that large 4GB+ isos
// can be split into multiple smaller files.
//
// Returns TRUE if multiple parts for the ISO are found.  Returns FALSE if only one
// part is found.
void isoFile::FindParts()
{
	wxFileName nameparts( m_filename );
	wxString curext( nameparts.GetExt() );
	wxChar prefixch = wxTolower(curext[0]);

	// Multi-part rules!
	//  * The first part can either be the proper extension (ISO, MDF, etc) or the numerical
	//    extension (I00, I01, M00, M01, etc).
	//  * Numerical extensions MUST begin at 00 (I00 etc), regardless of if the first part
	//    is proper or numerical.

	uint i = 0;

	if ((curext.Length() == 3) && (curext[1] == L'0') && (curext[2] == L'0'))
	{
		// First file is an OO, so skip 0 in the loop below:
		i = 1;
	}

	FastFormatUnicode extbuf;

	extbuf.Write( L"%c%02u", prefixch, i );
	nameparts.SetExt( extbuf );
	if (!pxFileExists_WithExt(nameparts, extbuf)) return;

	DevCon.WriteLn( Color_Blue, "isoFile: multi-part %s detected...", curext.Upper().c_str() );
	ConsoleIndentScope indent;

	for (; i < MaxSplits; ++i)
	{
		extbuf.Clear();
		extbuf.Write( L"%c%02u", prefixch, i );
		if (!pxFileExists_WithExt(nameparts, extbuf)) break;

		_IsoPart& thispart( m_parts[m_numparts] );

		thispart.handle = new wxFileInputStream( nameparts.GetFullPath() );
		pxStream_OpenCheck( *thispart.handle, nameparts.GetFullPath(), L"reading" );

		m_blocks += thispart.CalculateBlocks( m_blocks, m_blocksize );

		DevCon.WriteLn( Color_Blue, L"\tblocks %u - %u in: %s",
			thispart.slsn, thispart.elsn,
			nameparts.GetFullPath().c_str()
		);
		++m_numparts;
	}

	//Console.WriteLn( Color_Blue, "isoFile: multi-part ISO loaded (%u parts found)", m_numparts );
}

// Tests the specified filename to see if it is a supported ISO type.  This function typically
// executes faster than isoFile::Open since it does not do the following:
//  * check for multi-part ISOs.  I tests for header info in the main/root ISO only.
//  * load blockdump indexes.
//
// Note that this is a member method, and that it will clobber any existing ISO state.
// (assertions are generated in debug mode if the object state is not already closed).
bool isoFile::Test( const wxString& srcfile )
{
	pxAssertMsg( !m_parts[0].handle, "Warning!  isoFile::Test is about to clobber whatever existing iso bound to this isoFile object!" );

	Close();
	m_filename = srcfile;

	m_parts[0].handle = new wxFileInputStream( m_filename );
	pxStream_OpenCheck( *m_parts[0].handle, m_filename, L"reading" );

	m_numparts		= 1;
	m_parts[0].slsn = 0;

	// elsn is unknown at this time, but is also unused when m_numparts == 1.
	// (and if numparts is incremented, elsn will get assigned accordingly)

	return Detect( false );
}

void isoFile::Open( const wxString& srcfile )
{
	Close();
	m_filename = srcfile;

	m_parts[0].handle = new wxFileInputStream( m_filename );
	pxStream_OpenCheck( *m_parts[0].handle, m_filename, L"reading" );

	m_numparts		= 1;
	m_parts[0].slsn = 0;
	
	// elsn is unknown at this time, but is also unused when m_numparts == 1.
	// (and if numparts is incremented, elsn will get assigned accordingly)
	
	if (!Detect())
		throw Exception::BadStream().SetUserMsg(wxLt("Unrecognized ISO file format."));

	m_blocks = m_parts[0].CalculateBlocks( 0, m_blocksize );

	FindParts();
	if (m_numparts > 1)
	{
		Console.WriteLn( Color_Blue, "isoFile: multi-part ISO detected.  %u parts found." );
	}

	const char* isotypename = NULL;
    switch(m_type)
    {
        case ISOTYPE_CD:	isotypename = "CD";			break;
        case ISOTYPE_DVD:	isotypename = "DVD";		break;
        case ISOTYPE_AUDIO:	isotypename = "Audio CD";	break;

        case ISOTYPE_DVDDL:
			isotypename = "DVD9 (dual-layer)";
		break;

        case ISOTYPE_ILLEGAL:
        default:
			isotypename = "illegal media";
		break;
    }

	Console.WriteLn(Color_StrongBlue, L"isoFile open ok: %s", m_filename.c_str());

	ConsoleIndentScope indent;
	Console.WriteLn("Image type  = %s", isotypename); 
	Console.WriteLn("Fileparts   = %u", m_numparts);
	DevCon.WriteLn ("blocks      = %u", m_blocks);
	DevCon.WriteLn ("offset      = %d", m_offset);
	DevCon.WriteLn ("blocksize   = %u", m_blocksize);
	DevCon.WriteLn ("blockoffset = %d", m_blockofs);
}

void isoFile::Create(const wxString& filename, int flags)
{
	Close();
	m_filename = filename;

	m_flags		= flags;
	m_offset	= 0;
	m_blockofs	= 24;
	m_blocksize	= 2048;

	m_outstream = new wxFileOutputStream( m_filename );
	pxStream_OpenCheck( *m_outstream, m_filename, L"writing" );

	Console.WriteLn("isoFile create ok: %s ", m_filename.c_str());
}

void isoFile::Close()
{
	for (uint i=0; i<MaxSplits; ++i)
		m_parts[i].handle.Delete();

	m_dtable.Delete();

	_init();
}

bool isoFile::IsOpened() const
{
	return m_parts[0].handle && m_parts[0].handle->IsOk();
}

void isoFile::outWrite( const void* src, size_t size )
{
	m_outstream->Write(src, size);
	if(m_outstream->GetLastError() == wxSTREAM_WRITE_ERROR)
	{
		int err = errno;
		if (!err)
			throw Exception::BadStream(m_filename).SetDiagMsg(pxsFmt(L"An error occurred while writing %u bytes to file", size));

		ScopedExcept ex(Exception::FromErrno(m_filename, err));
		ex->SetDiagMsg( pxsFmt(L"An error occurred while writing %u bytes to file: %s", size, ex->DiagMsg().c_str()) );
		ex->Rethrow();
	}
}

// --------------------------------------------------------------------------------------
//  _IsoPart
// --------------------------------------------------------------------------------------

_IsoPart::~_IsoPart() throw()
{
}

void _IsoPart::Read( void* dest, size_t size )
{
	handle->Read(dest, size);
	if (handle->GetLastError() == wxSTREAM_READ_ERROR)
	{
		int err = errno;
		if (!err)
			throw Exception::BadStream(filename).SetDiagMsg(L"Cannot read from file (bad file handle?)");

		ScopedExcept ex(Exception::FromErrno(filename, err));
		ex->SetDiagMsg( L"cannot read from file: " + ex->DiagMsg() );
		ex->Rethrow();
	}

	// IMPORTANT!  The underlying file/source Eof() stuff is not really reliable, so we
	// must always use the explicit check against the number of bytes read to determine
	// end-of-stream conditions.

	if ((size_t)handle->LastRead() < size)
		throw Exception::EndOfStream( filename );
}

void _IsoPart::Seek(wxFileOffset pos, wxSeekMode mode)
{
	handle->SeekI(pos, mode);
}

void _IsoPart::SeekEnd(wxFileOffset pos)
{
	handle->SeekI(pos, wxFromEnd);
}

wxFileOffset _IsoPart::Tell() const
{
	return handle->TellI();
}

// returns the number of blocks contained in this part of the iso image.
uint _IsoPart::CalculateBlocks( uint startBlock, uint blocksize )
{
	wxFileOffset partsize = handle->GetLength();

	slsn = startBlock;
	uint numBlocks = partsize / blocksize;
	elsn = startBlock + numBlocks - 1;
	return numBlocks;
}
