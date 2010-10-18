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
#include "wxBaseTools.h"
#include "pxStreams.h"

#include <wx/stream.h>


// --------------------------------------------------------------------------------------
//  pxStreamReader  (implementations)
// --------------------------------------------------------------------------------------
// Interface for reading data from a gzip stream.
//

pxStreamReader::pxStreamReader(const wxString& filename, ScopedPtr<wxInputStream>& input)
	: m_filename( filename )
	, m_stream( input.DetachPtr() )
{
}

pxStreamReader::pxStreamReader(const wxString& filename, wxInputStream* input)
	: m_filename( filename )
	, m_stream( input )
{
}

void pxStreamReader::SetStream( const wxString& filename, ScopedPtr<wxInputStream>& stream )
{
	m_filename = filename;
	m_stream = stream.DetachPtr();
}

void pxStreamReader::SetStream( const wxString& filename, wxInputStream* stream )
{
	m_filename = filename;
	m_stream = stream;
}

void pxStreamReader::Read( void* dest, size_t size )
{
	m_stream->Read(dest, size);
	if (m_stream->GetLastError() == wxSTREAM_READ_ERROR)
	{
		int err = errno;
		if (!err)
			throw Exception::BadStream(m_filename).SetDiagMsg(L"Cannot read from file (bad file handle?)");

		ScopedExcept ex(Exception::FromErrno(m_filename, err));
		ex->SetDiagMsg( L"cannot read from file: " + ex->DiagMsg() );
		ex->Rethrow();
	}

	// IMPORTANT!  The underlying file/source Eof() stuff is not really reliable, so we
	// must always use the explicit check against the number of bytes read to determine
	// end-of-stream conditions.

	if ((size_t)m_stream->LastRead() < size)
		throw Exception::EndOfStream( m_filename );
}

// --------------------------------------------------------------------------------------
//  pxStreamWriter
// --------------------------------------------------------------------------------------
pxStreamWriter::pxStreamWriter(const wxString& filename, ScopedPtr<wxOutputStream>& output)
	: m_filename( filename )
	, m_outstream( output.DetachPtr() )
{
	
}

pxStreamWriter::pxStreamWriter(const wxString& filename, wxOutputStream* output)
	: m_filename( filename )
	, m_outstream( output )
{
}

void pxStreamWriter::SetStream( const wxString& filename, ScopedPtr<wxOutputStream>& stream )
{
	m_filename = filename;
	m_outstream = stream.DetachPtr();
}

void pxStreamWriter::SetStream( const wxString& filename, wxOutputStream* stream )
{
	m_filename = filename;
	m_outstream = stream;
}


void pxStreamWriter::Write( const void* src, size_t size )
{
	m_outstream->Write(src, size);
	if(m_outstream->GetLastError() == wxSTREAM_WRITE_ERROR)
	{
		int err = errno;
		if (!err)
			throw Exception::BadStream(m_filename).SetDiagMsg(L"Cannot write to file/stream.");

		ScopedExcept ex(Exception::FromErrno(m_filename, err));
		ex->SetDiagMsg( L"Cannot write to file: " + ex->DiagMsg() );
		ex->Rethrow();
	}
}

// --------------------------------------------------------------------------------------
//  pxTextStream
// --------------------------------------------------------------------------------------

// Returns TRUE if the source is UTF8, or FALSE if it's just ASCII crap.
bool pxReadLine( wxInputStream& input, std::string& dest )
{
	dest.clear();
	bool isUTF8 = false;
	while ( true )
	{
		char c;
		input.Read(&c, sizeof(c));
		if( c == 0 )			break;
		if( input.Eof() )		break;
		if( c == '\n' )			break; // eat on UNIX
		if( c == '\r' )
		{
			input.Read(&c, sizeof(c));
			if( c == 0 )			break;
			if( input.Eof() )		break;
			if( c == '\n' )			break;

			input.Ungetch(c);
			break;
		}
		dest += c;
		if( c & 0x80 ) isUTF8 = true;
	}
	
	return isUTF8;
}

void pxReadLine( wxInputStream& input, wxString& dest, std::string& intermed )
{
	dest.clear();
	if( pxReadLine( input, intermed ) )
		dest = fromUTF8(intermed.c_str());
	else
	{
		// Optimized ToAscii conversion.
		// wx3.0 : NOT COMPATIBLE!!  (on linux anyway)
		const char* ascii = intermed.c_str();
		while( *ascii != 0 ) dest += (wchar_t)(unsigned char)*ascii++;
	}
}

void pxReadLine( wxInputStream& input, wxString& dest )
{
	std::string line;
	pxReadLine( input, dest, line );
}

wxString pxReadLine( wxInputStream& input )
{
	wxString result;
	pxReadLine( input, result );
	return result;
}

void pxWriteLine( wxOutputStream& output )
{
	output.Write( "\n", 1 );
}

void pxWriteLine( wxOutputStream& output, const wxString& text )
{
	if( !text.IsEmpty() )
	{
		pxToUTF8 utf8(text);
		output.Write(utf8, utf8.Length());
	}
	pxWriteLine( output );
}

void pxWriteMultiline( wxOutputStream& output, const wxString& src )
{
	if( src.IsEmpty() ) return;

	wxString result( src );
	result.Replace( L"\r\n", L"\n" );
	result.Replace( L"\r", L"\n" );
	
	pxToUTF8 utf8(result);
	output.Write(utf8, utf8.Length());
}
