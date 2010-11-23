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

#include "MemcpyFast.h"
#include "SafeArray.h"

// Internal constructor for use by derived classes.  This allows a derived class to
// use its own memory allocation (with an aligned memory, for example).
// Throws:
//   Exception::OutOfMemory if the allocated_mem pointer is NULL.
template< typename T >
SafeArray<T>::SafeArray( const wxChar* name, T* allocated_mem, int initSize )
	: Name( name )
{
	ChunkSize	= DefaultChunkSize;
	m_ptr		= allocated_mem;
	m_size		= initSize;

	if( m_ptr == NULL )
		throw Exception::OutOfMemory(name)
			.SetDiagMsg(wxsFormat(L"Called from 'SafeArray::ctor' [size=%d]", initSize));
}

template< typename T >
T* SafeArray<T>::_virtual_realloc( int newsize )
{
	T* retval = (T*)((m_ptr == NULL) ?
		malloc( newsize * sizeof(T) ) :
		realloc( m_ptr, newsize * sizeof(T) )
	);
	
	if( IsDebugBuild )
	{
		// Zero everything out to 0xbaadf00d, so that its obviously uncleared
		// to a debuggee

		u32* fill = (u32*)&retval[m_size];
		const u32* end = (u32*)((((uptr)&retval[newsize-1])-3) & ~0x3);
		for( ; fill<end; ++fill ) *fill = 0xbaadf00d;
	}
	
	return retval;
}

template< typename T >
SafeArray<T>::~SafeArray() throw()
{
	safe_free( m_ptr );
}

template< typename T >
SafeArray<T>::SafeArray( const wxChar* name )
	: Name( name )
{
	ChunkSize	= DefaultChunkSize;
	m_ptr		= NULL;
	m_size		= 0;
}

template< typename T >
SafeArray<T>::SafeArray( int initialSize, const wxChar* name )
	: Name( name )
{
	ChunkSize	= DefaultChunkSize;
	m_ptr		= (initialSize==0) ? NULL : (T*)malloc( initialSize * sizeof(T) );
	m_size		= initialSize;

	if( (initialSize != 0) && (m_ptr == NULL) )
		throw Exception::OutOfMemory(name)
			.SetDiagMsg(wxsFormat(L"Called from 'SafeArray::ctor' [size=%d]", initialSize));
}

// Clears the contents of the array to zero, and frees all memory allocations.
template< typename T >
void SafeArray<T>::Dispose()
{
	m_size = 0;
	safe_free( m_ptr );
}

template< typename T >
T* SafeArray<T>::_getPtr( uint i ) const
{
	IndexBoundsAssumeDev( Name.c_str(), i, m_size );
	return &m_ptr[i];
}

// reallocates the array to the explicit size.  Can be used to shrink or grow an
// array, and bypasses the internal threshold growth indicators.
template< typename T >
void SafeArray<T>::ExactAlloc( int newsize )
{
	if( newsize == m_size ) return;

	m_ptr = _virtual_realloc( newsize );
	if( m_ptr == NULL )
		throw Exception::OutOfMemory(Name)
			.SetDiagMsg(wxsFormat(L"Called from 'SafeArray::ExactAlloc' [oldsize=%d] [newsize=%d]", m_size, newsize));

	m_size = newsize;
}

template< typename T >
SafeArray<T>* SafeArray<T>::Clone() const
{
	SafeArray<T>* retval = new SafeArray<T>( m_size );
	memcpy_fast( retval->GetPtr(), m_ptr, sizeof(T) * m_size );
	return retval;
}


// --------------------------------------------------------------------------------------
//  SafeAlignedArray<T>  (implementations)
// --------------------------------------------------------------------------------------

template< typename T, uint Alignment >
T* SafeAlignedArray<T,Alignment>::_virtual_realloc( int newsize )
{
	return (T*)( ( this->m_ptr == NULL ) ?
		_aligned_malloc( newsize * sizeof(T), Alignment ) :
		_aligned_realloc( this->m_ptr, newsize * sizeof(T), Alignment )
	);
}

// Appends "(align: xx)" to the name of the allocation in devel builds.
// Maybe useful,maybe not... no harm in attaching it. :D

template< typename T, uint Alignment >
SafeAlignedArray<T,Alignment>::~SafeAlignedArray() throw()
{
	safe_aligned_free( this->m_ptr );
	// mptr is set to null, so the parent class's destructor won't re-free it.
}

template< typename T, uint Alignment >
SafeAlignedArray<T,Alignment>::SafeAlignedArray( int initialSize, const wxChar* name ) :
	SafeArray<T>::SafeArray(
		name,
		(T*)_aligned_malloc( initialSize * sizeof(T), Alignment ),
		initialSize
	)
{
}

template< typename T, uint Alignment >
SafeAlignedArray<T,Alignment>* SafeAlignedArray<T,Alignment>::Clone() const
{
	SafeAlignedArray<T,Alignment>* retval = new SafeAlignedArray<T,Alignment>( this->m_size );
	memcpy_fast( retval->GetPtr(), this->m_ptr, sizeof(T) * this->m_size );
	return retval;
}

// --------------------------------------------------------------------------------------
//  SafeList<T>  (implementations)
// --------------------------------------------------------------------------------------

template< typename T >
T* SafeList<T>::_virtual_realloc( int newsize )
{
	return (T*)realloc( m_ptr, newsize * sizeof(T) );
}

template< typename T >
SafeList<T>::~SafeList() throw()
{
	safe_free( m_ptr );
}

template< typename T >
SafeList<T>::SafeList( const wxChar* name )
	: Name( name )
{
	ChunkSize	= DefaultChunkSize;
	m_ptr		= NULL;
	m_allocsize	= 0;
	m_length	= 0;
}

template< typename T >
SafeList<T>::SafeList( int initialSize, const wxChar* name )
	: Name( name )
{
	ChunkSize	= DefaultChunkSize;
	m_allocsize	= initialSize;
	m_length	= 0;
	m_ptr		= (T*)malloc( initialSize * sizeof(T) );

	if( m_ptr == NULL )
		throw Exception::OutOfMemory(Name)
			.SetDiagMsg(wxsFormat(L"called from 'SafeList::ctor' [length=%d]", m_length));

	for( int i=0; i<m_allocsize; ++i )
	{
		new (&m_ptr[i]) T();
	}

}

template< typename T >
T* SafeList<T>::_getPtr( uint i ) const
{
	IndexBoundsAssumeDev( Name.c_str(), i, m_length );
	return &m_ptr[i];
}

// Ensures that the allocation is large enough to fit data of the
// amount requested.  The memory allocation is not resized smaller.
template< typename T >
void SafeList<T>::MakeRoomFor( int blockSize )
{
	if( blockSize > m_allocsize )
	{
		const int newalloc = blockSize + ChunkSize;
		m_ptr = _virtual_realloc( newalloc );
		if( m_ptr == NULL )
			throw Exception::OutOfMemory(Name)
				.SetDiagMsg(wxsFormat(L"Called from 'SafeList::MakeRoomFor' [oldlen=%d] [newlen=%d]", m_length, blockSize));

		for( ; m_allocsize<newalloc; ++m_allocsize )
		{
			new (&m_ptr[m_allocsize]) T();
		}
	}
}

// Appends an item to the end of the list and returns a handle to it.
template< typename T >
T& SafeList<T>::New()
{
	_MakeRoomFor_threshold( m_length + 1 );
	return m_ptr[m_length++];
}

template< typename T >
int SafeList<T>::Add( const T& src )
{
	_MakeRoomFor_threshold( m_length + 1 );
	m_ptr[m_length] = src;
	return m_length++;
}

// Same as Add, but returns the handle of the new object instead of it's array index.
template< typename T >
T& SafeList<T>::AddNew( const T& src )
{
	_MakeRoomFor_threshold( m_length + 1 );
	m_ptr[m_length] = src;
	return m_ptr[m_length];
}

// Performs a standard array-copy removal of the given item.  All items past the
// given item are copied over.
// DevBuilds : Generates assertion if the index is invalid.
template< typename T >
void SafeList<T>::Remove( int index )
{
	IndexBoundsAssumeDev( Name.c_str(), index, m_length );

	int copylen = m_length - index;
	if( copylen > 0 )
		memcpy_fast( &m_ptr[index], &m_ptr[index+1], copylen );
}

template< typename T >
SafeList<T>* SafeList<T>::Clone() const
{
	SafeList<T>* retval = new SafeList<T>( m_length );
	memcpy_fast( retval->m_ptr, m_ptr, sizeof(T) * m_length );
	return retval;
}

template< typename T >
void SafeList<T>::_MakeRoomFor_threshold( int newsize )
{
	MakeRoomFor( newsize + ChunkSize );
}
