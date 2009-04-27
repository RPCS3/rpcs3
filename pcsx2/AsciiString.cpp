/*  Pcsx2 - Pc Ps2 Emulator
 *  Copyright (C) 2002-2009  Pcsx2 Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include "PrecompiledHeader.h"

#include "SafeArray.h"
#include "AsciiString.h"

using namespace Threading;

namespace AsciiStringAllocator
{
	struct PrivateHandle
	{
		AsciiStringAllocatorHandle* PublicHandle;
	};

	static const uint UnitLength = 256;
	static const uint UnitLengthMask = UnitLength-1;

	static const uint AllocationCleanupThreshold = 256;
	static const uint InitialBufferLength = 0x100000;		// 1MB!
	static const AsciiStringAllocatorHandle* PublicHandleAllocated = (AsciiStringAllocatorHandle*)1;

	static uint m_release_counter( 0 );

	static SafeArray<PrivateHandle> m_PrivateHandles( InitialBufferLength / UnitLength );
	static SafeArray<char> m_StringBuffer( InitialBufferLength );

	static int m_NextPrivateHandle;
	static int m_NextBufferIndex;
	
	static MutexLock m_Lock;

	static char* GetPtr( const AsciiStringAllocatorHandle& handle )
	{
		return &m_StringBuffer[ m_PrivateHandles[ handle.Index ] ];
	}
	
	static void DoCompact()
	{
		if( m_release_counter < AllocationCleanupThreshold ) return;

		ScopedLock locker( m_Lock );
		int handlecount = m_PrivateHandles.GetLength();
		int writepos = 0;
		for( int readpos=0; readpos<handlecount; readpos++ )
		{
			PrivateHandle& curhandle = m_PrivateHandles[readpos];
			
			if( curhandle.PublicHandle != NULL )
			{
				int cwpos = writepos;
				writepos++;
				if( cwpos == readpos ) continue;

				if( curhandle.PublicHandle != PublicHandleAllocated )
					curhandle.PublicHandle->Index = cwpos;

				// todo: replace this with a hardcoded XMM inline copy of 256 bytes. :)

				memcpy_fast(
					m_StringBuffer.GetPtr(cwpos*UnitLength),
					m_StringBuffer.GetPtr(readpos*UnitLength),
					UnitLength
				);
			}
		}
	}

	void New( AsciiStringAllocatorHandle& dest, int length )
	{
		int numblocks = (length / UnitLength)+1;
		length = numblocks * UnitLength;

		ScopedLock locker( m_Lock );
		AsciiStringAllocatorHandle retval( m_NextPrivateHandle, length );
		m_PrivateHandles[m_NextPrivateHandle++].PublicHandle = &dest;
		for( int p=numblocks-1; p; --p, ++m_NextPrivateHandle )
			m_PrivateHandles[m_NextPrivateHandle].PublicHandle = PublicHandleAllocated;

		m_StringBuffer.MakeRoomFor( m_NextPrivateHandle * UnitLength );

		return retval;
	}
	
	bool Grow( AsciiStringAllocatorHandle& dest )
	{
		ScopedLock locker( m_lock );

		if( m_PrivateHandles[m_NextPrivateHandle].PublicHandle == NULL )
		{
			m_PrivateHandles[m_NextPrivateHandle].PublicHandle = PublicHandleAllocated;
			return true;
		}
		return false;
	}

	// releases the block without clearing the handle structure information
	// and without doing a block compact check.
	static int _release( AsciiStringAllocatorHandle& handle )
	{
		const int numblocks = handle.Length / UnitLength;
		const int endblock = handle.Index + numblocks;

		ScopedLock locker( m_Lock );
		for( int i=handle.Index; i<endblock; i++ )
			m_PrivateHandles[i].PublicHandle = NULL;

		// Rewind the NextPrivateHandle if we haven't allocated anything else
		// since this allocation was made.
		if( endblock == m_NextPrivateHandle ) 
			m_NextPrivateHandle = handle.Index;
			
		return numblocks;
	}

	void Release( AsciiStringAllocatorHandle& handle )
	{
		handle.Index = -1;
		handle.Length = 0;
		m_release_counter += _release( handle );
		
		if( m_release_counter >= AllocationCleanupThreshold )
			DoCleanup();
	}
	
	// Allocates a new handle and copies the old string contents to the new reserve.
	void Reallocate( AsciiStringAllocatorHandle& handle, int newsize )
	{
		int newblocks = (newsize / UnitLength)+1;
		newsize = newblocks * UnitLength;

		ScopedLock locker( m_Lock );
		_release( handle );

		m_StringBuffer.MakeRoomFor( m_NextPrivateHandle + newblocks );
		if( m_NextPrivateHandle != handle.Index )
		{
			memcpy_fast(
				m_StringBuffer.GetPtr( m_NextPrivateHandle ),
				m_StringBuffer(handle.Index),
				handle.Length
			);
			handle.Index = m_NextPrivateHandle;
		}
		handle.Length = newsize;
	}
};

AsciiStringAllocatorHandle::GetPtr() const
{
	return AsciiStringAllocator::GetPtr( *this );
}

//////////////////////////////////////////////////////////////////////////////////////////
//

AsciiString::AsciiString( int length_reserve )
{
	AsciiStringAllocator::New( m_MemoryHandle, len )
}

const wxCharBuffer AsciiString::mb_str() const
{
	// use wxCharBuffer here for sake of safety, since parallel string operations could
	// result in the pointer to this string becoming invalid.
	ScopedLock locker( AsciiStringAllocator::m_Lock );
	return wxCharBuffer( AsciiStringAllocator::GetPtr( m_MemoryHandle ) );
}

AsciiStringLock::operator char*()
{
	return m_buffer;
}

char& AsciiStringLock::operator[]( int idx )
{
	return m_buffer[idx];
}

char AsciiStringLock::GetCharAt( int idx ) const
{
	return m_buffer[idx];
}

char* AsciiStringLock::GetPtr()
{
	return m_buffer;
}

AsciiStringLock::AsciiStringLock( AsciiString& str ) :
	m_string( str )
{
	m_string.Lock();
}

AsciiStringLock::~AsciiStringLock()
{
	m_string.Unlock();
}

AsciiStringLock::operator char*()
{
	AsciiStringAllocator::GetPtr( m_handle );
}

AsciiString AsciiString::operator+( const AsciiString& right )
{
	int len = GetLength() + right.GetLength();

	AsciiString dest( len+1 );

	char* lockptr = m_MemoryHandle.GetPtr();
	memcpy_fast( lockptr, GetBufferPtr(), GetLength() );
	memcpy_fast( lockptr+GetLength(), right.GetBufferPtr(), right.GetLength() );
	lockptr[dest.GetLength()] = 0;
}

AsciiString& AsciiString::Append( const AsciiString& src )
{
	int needlen = src.GetLength() + GetLength()+1;
	if( needlen >= m_MemoryHandle.Length )
	{
		// The new string is too large -- looks like we're going to need to allocate
		// a larger block.  We try and use Grow first, if the appending string is very
		// short (it sometimes saves the need to copy the block to a new location)
		
		if( src.GetLength() >= AsciiStringAllocator::UnitLength || !AsciiStringAllocator::Grow( m_MemoryHandle ) )
			AsciiStringAllocator::Reallocate( m_MemoryHandle, needlen );
	}
	
	char* lockptr = m_MemoryHandle.GetPtr();
	memcpy_fast( lockptr+GetLength(), src.GetBufferPtr(), src.GetLength() );
	lockptr[GetLength()] = 0;
}
