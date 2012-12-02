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

void pxStream_OpenCheck( const wxStreamBase& stream, const wxString& fname, const wxString& mode )
{
	if (stream.IsOk()) return;

	ScopedExcept ex(Exception::FromErrno(fname, errno));
	ex->SetDiagMsg( pxsFmt(L"Unable to open the file for %s: %s", mode.c_str(), ex->DiagMsg().c_str()) );
	ex->Rethrow();
}

OutputIsoFile::OutputIsoFile()
{
	_init();
}

OutputIsoFile::~OutputIsoFile() throw()
{
	Close();
}

void OutputIsoFile::_init()
{
	m_version		= 0;

	m_offset		= 0;
	m_blockofs		= 0;
	m_blocksize		= 0;
	m_blocks		= 0;

	m_dtable		= 0;
	m_dtablesize	= 0;
}

void OutputIsoFile::Create(const wxString& filename, int version)
{
	Close();
	m_filename = filename;

	m_version	= version;
	m_offset	= 0;
	m_blockofs	= 24;
	m_blocksize	= 2048;

	m_outstream = new wxFileOutputStream( m_filename );
	pxStream_OpenCheck( *m_outstream, m_filename, L"writing" );

	Console.WriteLn("isoFile create ok: %s ", m_filename.c_str());
}

// Generates format header information for blockdumps.
void OutputIsoFile::WriteFormat(int _blockofs, uint _blocksize, uint _blocks)
{
	m_blocksize	= _blocksize;
	m_blocks	= _blocks;
	m_blockofs	= _blockofs;

	Console.WriteLn("blockoffset = %d", m_blockofs);
	Console.WriteLn("blocksize   = %u", m_blocksize);
	Console.WriteLn("blocks	     = %u", m_blocks);

	if (m_version == 2)
	{
		outWrite("BDV2", 4);
		outWrite(m_blocksize);
		outWrite(m_blocks);
		outWrite(m_blockofs);
	}
}

void OutputIsoFile::_WriteBlock(const u8* src, uint lsn)
{
	wxFileOffset ofs = (wxFileOffset)lsn * m_blocksize + m_offset;

	m_outstream->SeekO( ofs );
	outWrite( src + m_blockofs, m_blocksize );
}

void OutputIsoFile::_WriteBlockD(const u8* src, uint lsn)
{
	// Find and ignore blocks that have already been dumped:
	for (int i=0; i<m_dtablesize; ++i)
	{
		if (m_dtable[i] == lsn) return;
	}

	m_dtable[m_dtablesize++] = lsn;
	outWrite<u32>( lsn );
	outWrite( src + m_blockofs, m_blocksize );
}

void OutputIsoFile::WriteBlock(const u8* src, uint lsn)
{
	if (m_version == 2)
		_WriteBlockD(src, lsn);
	else
		_WriteBlock(src, lsn);
}

void OutputIsoFile::Close()
{
	m_dtable.Delete();

	_init();
}

void OutputIsoFile::outWrite( const void* src, size_t size )
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

bool OutputIsoFile::IsOpened() const
{
	return m_outstream && m_outstream->IsOk();
}
