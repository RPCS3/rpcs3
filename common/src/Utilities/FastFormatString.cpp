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
#include "Threading.h"

#include "TlsVariable.inl"
#include "SafeArray.inl"

using namespace Threading;

// Implement some very commonly used SafeArray types here
// (done here for lack of a better place)

template class SafeArray<char>;
template class SafeArray<wchar_t>;
template class SafeArray<u8>;

template class SafeAlignedArray<char,16>;
template class SafeAlignedArray<wchar_t,16>;
template class SafeAlignedArray<u8,16>;

// Sanity check: truncate strings if they exceed 512k in length.  Anything like that
// is either a bug or really horrible code that needs to be stopped before it causes
// system deadlock.
static const int MaxFormattedStringLength = 0x80000;

typedef ScopedAlignedAlloc<char,16> CharBufferType;

// --------------------------------------------------------------------------------------
//  FastFormatBuffers
// --------------------------------------------------------------------------------------
// This class provides a series of pre-allocated thread-local buffers for use by string
// formatting tools.  These buffers are handed out in round-robin style and require *no*
// thread sync objects and avoid multi-thread contention completely -- allowing multiple
// threads to format complicated strings concurrently with maximum efficiency.
//
class FastFormatBuffers
{
	DeclareNoncopyableObject(FastFormatBuffers);

protected:
	typedef char CharType;
	typedef CharBufferType BufferType;

	static const uint BufferCount = 6;

	BufferType		m_buffers[BufferCount];
	uint			m_curslot;

public:
	FastFormatBuffers()
	{
		// This protects against potential recursive calls to our formatter, by forcing those
		// calls to use a dynamic buffer for formatting.
		m_curslot = BufferCount;

		for (uint i=0; i<BufferCount; ++i)
		{
			m_buffers[i].Alloc(512);
		}

		m_curslot = 0;
	}

	virtual ~FastFormatBuffers() throw()
	{
		pxAssumeDev(m_curslot==0,
			wxsFormat(L"Dangling %s formatting buffer detected!",
				(sizeof(CharType)==1) ? L"UTF8/Ascii" : L"Wide-char"
			)
		);
	}

	bool HasFreeBuffer() const
	{
		return m_curslot < BufferCount-1;
	}

	BufferType& GrabBuffer()
	{
		++m_curslot;
		pxAssume(m_curslot < BufferCount);
		return m_buffers[m_curslot];
	}

	void ReleaseBuffer()
	{
		--m_curslot;
		pxAssume(m_curslot < BufferCount);
	}

	BufferType& operator[](uint i)
	{
		IndexBoundsAssume( ((sizeof(CharType)==1) ? L"Ascii Formatting Buffer" : L"Unicode Formatting Buffer"), i, BufferCount );
		return m_buffers[i];
	}
};

// --------------------------------------------------------------------------------------
//  GlobalBufferManager
// --------------------------------------------------------------------------------------
// This local-scope class is needed in order to safely deal with C++ initializing and destroying
// global objects in arbitrary order.  The initbit is updated by the object when constructed and
// destroyed; code using this class provides its own statically-initialized boolean (which MUST
// default to false!) and then sets the boolean to true to indicate the object is ready for use.
//
template< typename T >
class GlobalBufferManager
{
public:
	bool&		initbit;
	T			instance;

	GlobalBufferManager( bool& globalBoolean )
		: initbit( globalBoolean )
	{
		initbit = true;
	}
	
	~GlobalBufferManager() throw()
	{
		initbit = false;
		instance.Dispose();
	}

	T& Get()
	{
		return instance;
	}

	operator T&()
	{
		return instance;
	}
};

static bool buffer_is_avail = false;
static GlobalBufferManager< BaseTlsVariable< FastFormatBuffers > > m_buffer_tls(buffer_is_avail);

//static __ri void format_that_ascii_mess( SafeArray<char>& buffer, uint writepos, const char* fmt, va_list argptr )
static __ri void format_that_ascii_mess( CharBufferType& buffer, uint writepos, const char* fmt, va_list argptr )
{
	while( true )
	{
		int size = buffer.GetLength();
		int len = vsnprintf(buffer.GetPtr(writepos), size-writepos, fmt, argptr);

		// some implementations of vsnprintf() don't NUL terminate
		// the string if there is not enough space for it so
		// always do it manually
		buffer[size-1] = '\0';

		if (size >= MaxFormattedStringLength) break;

		// vsnprintf() may return either -1 (traditional Unix behavior) or the
		// total number of characters which would have been written if the
		// buffer were large enough (newer standards such as Unix98)

		if (len < 0)
			len = size + (size/4);

		len += writepos;
		if (len < size) break;
		buffer.Resize( len + 128 );
	};

	// performing an assertion or log of a truncated string is unsafe, so let's not; even
	// though it'd be kinda nice if we did.
}

// returns the length of the formatted string, in characters (wxChars).
static __ri uint format_that_unicode_mess( CharBufferType& buffer, uint writepos, const wxChar* fmt, va_list argptr)
{
	while( true )
	{
		int size = buffer.GetLength() / sizeof(wxChar);
		int len = wxVsnprintf((wxChar*)buffer.GetPtr(writepos*sizeof(wxChar)), size-writepos, fmt, argptr);

		// some implementations of vsnprintf() don't NUL terminate
		// the string if there is not enough space for it so
		// always do it manually
		((wxChar*)buffer.GetPtr())[size-1] = L'\0';

		if( size >= MaxFormattedStringLength ) return size-1;

		// vsnprintf() may return either -1 (traditional Unix behavior) or the
		// total number of characters which would have been written if the
		// buffer were large enough (newer standards such as Unix98)

		if (len < 0)
			len = size + (size/4);

		len += writepos;
		if (len < size) return len;
		buffer.Resize( (len + 128) * sizeof(wxChar) );
	};

	// performing an assertion or log of a truncated string is unsafe, so let's not; even
	// though it'd be kinda nice if we did.

	pxAssume( false );
	return 0;		// unreachable.
}

CharBufferType* GetFormatBuffer( bool& deleteDest )
{
	deleteDest = false;
	if (buffer_is_avail)
	{
		if (m_buffer_tls.Get()->HasFreeBuffer())
			return &m_buffer_tls.Get()->GrabBuffer();
	}

	deleteDest = true;

	return new CharBufferType(2048);
}

// --------------------------------------------------------------------------------------
//  FastFormatUnicode  (implementations)
// --------------------------------------------------------------------------------------
// [TODO] This class should actually be renamed to FastFormatNative or FastFormatString, and
// adopted to properly support 1-byte wxChar types (mostly requiring some changes to the
// WriteV functions).  The current implementation is fine for wx2.8, which always defaults
// to wide-varieties of wxChar -- but wx3.0 will use UTF8 for linux distros, which will break
// this class nicely in its current state. --air

FastFormatUnicode::FastFormatUnicode()
{
	m_dest = GetFormatBuffer(m_deleteDest);
	Clear();
}

FastFormatUnicode::~FastFormatUnicode() throw()
{
	if (m_deleteDest)
		delete m_dest;
	else
		m_buffer_tls.Get()->ReleaseBuffer();
}

void FastFormatUnicode::Clear()
{
	m_Length = 0;
	((wxChar*)m_dest->GetPtr())[0] = 0;
}

FastFormatUnicode& FastFormatUnicode::WriteV( const char* fmt, va_list argptr )
{
	wxString converted( fromUTF8(FastFormatAscii().WriteV( fmt, argptr )) );

	const uint inspos = m_Length;
	const uint convLen = converted.Length();
	m_dest->MakeRoomFor((inspos + convLen + 64) * sizeof(wxChar));
	memcpy_fast( &((wxChar*)m_dest->GetPtr())[inspos], converted, (convLen+1)*sizeof(wxChar) );
	m_Length += convLen;

	return *this;
}

FastFormatUnicode& FastFormatUnicode::WriteV( const wxChar* fmt, va_list argptr )
{
	m_Length = format_that_unicode_mess( *m_dest, m_Length, fmt, argptr );
	return *this;
}

FastFormatUnicode& FastFormatUnicode::Write( const char* fmt, ... )
{
	va_list list;
	va_start(list, fmt);
	WriteV(fmt,list);
	va_end(list);
	return *this;
}

FastFormatUnicode& FastFormatUnicode::Write( const wxChar* fmt, ... )
{
	va_list list;
	va_start(list, fmt);
	WriteV(fmt,list);
	va_end(list);
	return *this;
}

bool FastFormatUnicode::IsEmpty() const
{
	return ((wxChar&)(*m_dest)[0]) == 0;
}

FastFormatUnicode& FastFormatUnicode::ToUpper()
{
	wxChar* ch = (wxChar*)m_dest->GetPtr();
	for ( uint i=0; i<m_Length; ++i, ++ch )
		*ch = (wxChar)wxToupper(*ch);

	return *this;
}

FastFormatUnicode& FastFormatUnicode::ToLower()
{
	wxChar* ch = (wxChar*)m_dest->GetPtr();
	for ( uint i=0; i<m_Length; ++i, ++ch )
		*ch = (wxChar)wxTolower(*ch);

	return *this;
}

FastFormatUnicode& FastFormatUnicode::operator+=(const char* psz )
{
	Write( L"%s", fromUTF8(psz).c_str() );
	return *this;
}

wxString& operator+=(wxString& str1, const FastFormatUnicode& str2)
{
	str1.Append(str2.c_str(), str2.Length());
	return str1;
}

wxString operator+(const wxString& str1, const FastFormatUnicode& str2)
{
	wxString s = str1;
	s += str2;
	return s;
}

wxString operator+(const wxChar* str1, const FastFormatUnicode& str2)
{
	wxString s = str1;
	s += str2;
	return s;
}

wxString operator+(const FastFormatUnicode& str1, const wxString& str2)
{
	wxString s = str2;
	s += str1;
	return s;
}

wxString operator+(const FastFormatUnicode& str1, const wxChar* str2)
{
	wxString s = str2;
	s += str1;
	return s;
}


// --------------------------------------------------------------------------------------
//  FastFormatAscii  (implementations)
// --------------------------------------------------------------------------------------
FastFormatAscii::FastFormatAscii()
{
	m_dest = GetFormatBuffer(m_deleteDest);
	Clear();
}

FastFormatAscii::~FastFormatAscii() throw()
{
	if (m_deleteDest)
		delete m_dest;
	else
		m_buffer_tls.Get()->ReleaseBuffer();
}

void FastFormatAscii::Clear()
{
	m_dest->GetPtr()[0] = 0;
}

const wxString FastFormatAscii::GetString() const
{
	return fromAscii(m_dest->GetPtr());
}

FastFormatAscii& FastFormatAscii::WriteV( const char* fmt, va_list argptr )
{
	format_that_ascii_mess( *m_dest, strlen(m_dest->GetPtr()), fmt, argptr );
	return *this;
}

FastFormatAscii& FastFormatAscii::Write( const char* fmt, ... )
{
	va_list list;
	va_start(list, fmt);
	WriteV(fmt,list);
	va_end(list);
	return *this;
}


bool FastFormatAscii::IsEmpty() const
{
	return (*m_dest)[0] == 0;
}
