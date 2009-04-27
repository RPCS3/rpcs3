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
 
#pragma once

class AsciiStringAllocatorHandle
{
public:
	const int Index;
	const int Length;
	
public:
	AsciiStringAllocatorHandle( int handle, int len ) :
		Index( handle ),
		Length( len )
	{
	}
	
	char* GetPtr() const;
	void Lock();
	void Unlock();
};

class AsciiString
{
protected:
	AsciiStringAllocatorHandle m_MemoryHandle;
	int m_Length;

public:
	AsciiString();
	AsciiString( AsciiString& copycat );
	AsciiString( const char* src );
	AsciiString( const char* src, int length );
	AsciiString( const char* src, int startpos, int length );
	AsciiString( int length_reserve );

	const char* mb_str() const;
	char mb_str_unsafe();

	int GetLength() const
	{
		return m_Length;
	}
	
	AsciiString operator+( const AsciiString& right );
	AsciiString& Append( const AsciiString& src );
	
	void Lock();
	void Unlock();
};


class AsciiStringLock
{
protected:
	AsciiString& m_string;
	char* m_buffer;

public:
	AsciiStringLock( const AsciiStringAllocatorHandle& handle );
	~AsciiStringLock();

	char* GetPtr();
	char GetCharAt( int idx ) const;
	operator char*();
	char& operator[]( int idx );
	const char& operator[]( int idx ) const { return GetCharAt( idx ); }
};