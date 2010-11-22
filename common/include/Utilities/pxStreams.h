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

#pragma once

// --------------------------------------------------------------------------------------
//  pxStreamBase
// --------------------------------------------------------------------------------------
class pxStreamBase
{
	DeclareNoncopyableObject(pxStreamBase);

protected:
	// Filename of the stream, provided by the creator/caller.  This is typically used *only*
	// for generating comprehensive error messages when an error occurs (the stream name is
	// passed to the exception handlers).
	wxString	m_filename;

public:
	pxStreamBase(const wxString& filename);
	virtual ~pxStreamBase() throw() {}

	// Implementing classes should return the base wxStream object (usually either a wxInputStream
	// or wxOputStream derivative).
	virtual wxStreamBase* GetWxStreamBase() const=0;
	virtual void Close()=0;
	
	bool IsOk() const;
	wxString GetStreamName() const { return m_filename; }
};


// --------------------------------------------------------------------------------------
//  pxOutputStream
// --------------------------------------------------------------------------------------
class pxOutputStream : public pxStreamBase
{
	DeclareNoncopyableObject(pxOutputStream);

protected:
	ScopedPtr<wxOutputStream>	m_stream_out;

public:
	pxOutputStream(const wxString& filename, ScopedPtr<wxOutputStream>& output);
	pxOutputStream(const wxString& filename, wxOutputStream* output);

	virtual ~pxOutputStream() throw() {}
	virtual void Write( const void* data, size_t size );
	
	void SetStream( const wxString& filename, ScopedPtr<wxOutputStream>& stream );
	void SetStream( const wxString& filename, wxOutputStream* stream );

	void Close() { m_stream_out.Delete(); }

	virtual wxStreamBase* GetWxStreamBase() const;

	template< typename T >
	void Write( const T& data )
	{
		Write( &data, sizeof(data) );
	}
};

// --------------------------------------------------------------------------------------
//  pxInputStream
// --------------------------------------------------------------------------------------
class pxInputStream : public pxStreamBase
{
	DeclareNoncopyableObject(pxInputStream);

protected:
	ScopedPtr<wxInputStream>	m_stream_in;

public:
	pxInputStream(const wxString& filename, ScopedPtr<wxInputStream>& input);
	pxInputStream(const wxString& filename, wxInputStream* input);

	virtual ~pxInputStream() throw() {}
	virtual void Read( void* dest, size_t size );
	
	void SetStream( const wxString& filename, ScopedPtr<wxInputStream>& stream );
	void SetStream( const wxString& filename, wxInputStream* stream );

	void Close() { m_stream_in.Delete(); }

	virtual wxStreamBase* GetWxStreamBase() const;

	template< typename T >
	void Read( T& dest )
	{
		Read( &dest, sizeof(dest) );
	}
};
