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

// --------------------------------------------------------------------------------------
//  pxStreamWriter
// --------------------------------------------------------------------------------------
class pxStreamWriter
{
	DeclareNoncopyableObject(pxStreamWriter);

protected:
	wxString					m_filename;
	ScopedPtr<wxOutputStream>	m_outstream;

public:
	pxStreamWriter(const wxString& filename, ScopedPtr<wxOutputStream>& output);
	pxStreamWriter(const wxString& filename, wxOutputStream* output);

	virtual ~pxStreamWriter() throw() {}
	virtual void Write( const void* data, size_t size );
	
	void Close() { m_outstream.Delete(); }
	wxOutputStream* GetBaseStream() const { return m_outstream; }

	void SetStream( const wxString& filename, ScopedPtr<wxOutputStream>& stream );
	void SetStream( const wxString& filename, wxOutputStream* stream );
	
	wxString GetStreamName() const { return m_filename; }

	template< typename T >
	void Write( const T& data )
	{
		Write( &data, sizeof(data) );
	}
};

// --------------------------------------------------------------------------------------
//  pxStreamReader
// --------------------------------------------------------------------------------------
class pxStreamReader
{
	DeclareNoncopyableObject(pxStreamReader);

protected:
	wxString					m_filename;
	ScopedPtr<wxInputStream>	m_stream;

public:
	pxStreamReader(const wxString& filename, ScopedPtr<wxInputStream>& input);
	pxStreamReader(const wxString& filename, wxInputStream* input);

	virtual ~pxStreamReader() throw() {}
	virtual void Read( void* dest, size_t size );
	
	void SetStream( const wxString& filename, ScopedPtr<wxInputStream>& stream );
	void SetStream( const wxString& filename, wxInputStream* stream );

	wxInputStream* GetBaseStream() const { return m_stream; }
	void Close() { m_stream.Delete(); }

	wxString GetStreamName() const { return m_filename; }

	template< typename T >
	void Read( T& dest )
	{
		Read( &dest, sizeof(dest) );
	}
};
