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

using namespace Threading;

// Sanity check: truncate strings if they exceed 512k in length.  Anything like that
// is either a bug or really horrible code that needs to be stopped before it causes
// system deadlock.
static const int MaxFormattedStringLength = 0x80000;

// --------------------------------------------------------------------------------------
//  FormatBuffer
// --------------------------------------------------------------------------------------
// This helper class provides a "safe" way for us to check if the global handles for the
// string format buffer have been initialized or destructed by the C++ global heap manager.
// (C++ has no way to enforce init/destruct order on complex globals, thus the convoluted
// use of booleans to check the status of the type initializers).
//
template< typename CharType >
class FormatBuffer : public Mutex
{
public:
	bool&				clearbit;
	SafeArray<CharType>	buffer;
	wxMBConvUTF8		ConvUTF8;

	FormatBuffer( bool& bit_to_clear_on_destruction )
		: clearbit( bit_to_clear_on_destruction )
		, buffer( 4096, wxsFormat( L"%s Format Buffer", (sizeof(CharType)==1) ? "Ascii" : "Unicode" ) )
	{
		bit_to_clear_on_destruction = false;
	}

	virtual ~FormatBuffer() throw()
	{
		clearbit = true;
		Wait();		// lock the mutex, just in case.
	}
};

// Assume the buffer is 'deleted' -- technically it's never existed on startup,
// but "deleted" is well enough.

static bool ascii_buffer_is_deleted = true;
static bool unicode_buffer_is_deleted = true;

static FormatBuffer<char>	ascii_buffer( ascii_buffer_is_deleted );
static FormatBuffer<wxChar>	unicode_buffer( unicode_buffer_is_deleted );

static void format_that_ascii_mess( SafeArray<char>& buffer, const char* fmt, va_list argptr )
{
	while( true )
	{
		int size = buffer.GetLength();
		int len = vsnprintf(buffer.GetPtr(), size, fmt, argptr);

		// some implementations of vsnprintf() don't NUL terminate
		// the string if there is not enough space for it so
		// always do it manually
		buffer[size-1] = '\0';

		if( size >= MaxFormattedStringLength ) break;

		// vsnprintf() may return either -1 (traditional Unix behavior) or the
		// total number of characters which would have been written if the
		// buffer were large enough (newer standards such as Unix98)

		if ( len < 0 )
			len = size + (size/4);

		if ( len < size ) break;
		buffer.ExactAlloc( len + 1 );
	};

	// performing an assertion or log of a truncated string is unsafe, so let's not; even
	// though it'd be kinda nice if we did.
}

static void format_that_unicode_mess( SafeArray<wxChar>& buffer, const wxChar* fmt, va_list argptr)
{
	while( true )
	{
		int size = buffer.GetLength();
		int len = wxVsnprintf(buffer.GetPtr(), size, fmt, argptr);

		// some implementations of vsnprintf() don't NUL terminate
		// the string if there is not enough space for it so
		// always do it manually
		buffer[size-1] = L'\0';

		if( size >= MaxFormattedStringLength ) break;

		// vsnprintf() may return either -1 (traditional Unix behavior) or the
		// total number of characters which would have been written if the
		// buffer were large enough (newer standards such as Unix98)

		if ( len < 0 )
			len = size + (size/4);

		if ( len < size ) break;
		buffer.ExactAlloc( len + 1 );
	};

	// performing an assertion or log of a truncated string is unsafe, so let's not; even
	// though it'd be kinda nice if we did.
}

// returns the length of the string (not including the 0)
int FastFormatString_AsciiRaw(wxCharBuffer& dest, const char* fmt, va_list argptr)
{
	if( ascii_buffer_is_deleted )
	{
		// This means that the program is shutting down and the C++ destructors are
		// running, randomly deallocating static variables from existence.  We handle it
		// as gracefully as possible by allocating local vars to do our bidding (slow, but
		// ultimately necessary!)

		SafeArray<char>	localbuf( 4096, L"Temporary Ascii Formatting Buffer" );
		format_that_ascii_mess( localbuf, fmt, argptr );
		dest = localbuf.GetPtr();
		return strlen(dest);
	}
	else
	{
		// This is normal operation.  The static buffers are available for use, and we use
		// them for sake of efficiency (fewer heap allocs, for sure!)

		ScopedLock locker( ascii_buffer );
		format_that_ascii_mess( ascii_buffer.buffer, fmt, argptr );
		dest = ascii_buffer.buffer.GetPtr();
		return strlen(dest);
	}
	
}

wxString FastFormatString_Ascii(const char* fmt, va_list argptr)
{
	if( ascii_buffer_is_deleted )
	{
		// This means that the program is shutting down and the C++ destructors are
		// running, randomly deallocating static variables from existence.  We handle it
		// as gracefully as possible by allocating local vars to do our bidding (slow, but
		// ultimately necessary!)
	
		SafeArray<char>	localbuf( 4096, L"Temporary Ascii Formatting Buffer" );
		format_that_ascii_mess( localbuf, fmt, argptr );
		return fromUTF8( localbuf.GetPtr() );
	}
	else
	{
		// This is normal operation.  The static buffers are available for use, and we use
		// them for sake of efficiency (fewer heap allocs, for sure!)

		ScopedLock locker( ascii_buffer );
		format_that_ascii_mess( ascii_buffer.buffer, fmt, argptr );
		return fromUTF8( ascii_buffer.buffer.GetPtr() );
	}
}

wxString FastFormatString_Unicode(const wxChar* fmt, va_list argptr)
{
	// See above for the explanation on the _is_deleted flags.
	
	if( unicode_buffer_is_deleted )
	{
		SafeArray<wxChar> localbuf( 4096, L"Temporary Unicode Formatting Buffer" );
		format_that_unicode_mess( localbuf, fmt, argptr );
		return localbuf.GetPtr();
	}
	else
	{
		ScopedLock locker( unicode_buffer );
		format_that_unicode_mess( unicode_buffer.buffer, fmt, argptr );
		return unicode_buffer.buffer.GetPtr();
	}
}
