/*  Pcsx2 - Pc Ps2 Emulator
 *  Copyright (C) 2002-2008  Pcsx2 Team
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

#ifndef __SAFEARRAY_H__
#define __SAFEARRAY_H__

#include "MemcpyFast.h"

extern void* __fastcall pcsx2_aligned_malloc(size_t size, size_t align);
extern void* __fastcall pcsx2_aligned_realloc(void* handle, size_t size, size_t align);
extern void pcsx2_aligned_free(void* pmem);

// aligned_malloc: Implement/declare linux equivalents here!
#if !defined(_MSC_VER) && !defined(HAVE_ALIGNED_MALLOC)
#	define _aligned_malloc pcsx2_aligned_malloc
#	define _aligned_free pcsx2_aligned_free
# 	define _aligned_realloc pcsx2_aligned_realloc
#endif

//////////////////////////////////////////////////////////////
// Safe deallocation macros -- always check pointer validity (non-null)
// and set pointer to null on deallocation.

#define safe_delete( ptr ) \
	if( ptr != NULL ) { \
		delete ptr; \
		ptr = NULL; \
	}

#define safe_delete_array( ptr ) \
	if( ptr != NULL ) { \
		delete[] ptr; \
		ptr = NULL; \
	}

#define safe_free( ptr ) \
	if( ptr != NULL ) { \
		free( ptr ); \
		ptr = NULL; \
	}

#define safe_aligned_free( ptr ) \
	if( ptr != NULL ) { \
		_aligned_free( ptr ); \
		ptr = NULL; \
	}

#define SafeSysMunmap( ptr, size ) \
	if( ptr != NULL ) { \
		SysMunmap( (uptr)ptr, size ); \
		ptr = NULL; \
	}


//////////////////////////////////////////////////////////////////
// Handy little class for allocating a resizable memory block, complete with
// exception-based error handling and automatic cleanup.

template< typename T >
class MemoryAlloc : public NoncopyableObject
{
public:
	static const int DefaultChunkSize = 0x1000 * sizeof(T);

public: 
	const std::string Name;		// user-assigned block name
	int ChunkSize;

protected:
	T* m_ptr;
	int m_size;	// size of the allocation of memory

	const static std::string m_str_Unnamed;
protected:
	// Internal contructor for use by derrived classes.  This allws a derrived class to
	// use its own memory allocation (with an aligned memory, for example).
	// Throws:
	//   Exception::OutOfMemory if the allocated_mem pointr is NULL.
	explicit MemoryAlloc( const std::string& name, T* allocated_mem, int initSize ) : 
	  Name( name )
	, ChunkSize( DefaultChunkSize )
	, m_ptr( allocated_mem )
	, m_size( initSize )
	{
		if( m_ptr == NULL )
			throw Exception::OutOfMemory();
	}

	virtual T* _virtual_realloc( int newsize )
	{
		return (T*)realloc( m_ptr, newsize * sizeof(T) );
	}

public:
	virtual ~MemoryAlloc()
	{
		safe_free( m_ptr );
	}

	explicit MemoryAlloc( const std::string& name="Unnamed" ) : 
	  Name( name )
	, ChunkSize( DefaultChunkSize )
	, m_ptr( NULL )
	, m_size( 0 )
	{
	}

	explicit MemoryAlloc( int initialSize, const std::string& name="Unnamed" ) : 
	  Name( name )
	, ChunkSize( DefaultChunkSize )
	, m_ptr( (T*)malloc( initialSize * sizeof(T) ) )
	, m_size( initialSize )
	{
		if( m_ptr == NULL )
			throw Exception::OutOfMemory();
	}

	// Returns the size of the memory allocation, as according to the array type.
	int GetLength() const { return m_size; }
	// Returns the size of the memory allocation in bytes.
	int GetSizeInBytes() const { return m_size * sizeof(T); }

	// Ensures that the allocation is large enough to fit data of the
	// amount requested.  The memory allocation is not resized smaller.
	void MakeRoomFor( int blockSize )
	{
		std::string temp;
		
		if( blockSize > m_size )
		{
			const uint newalloc = blockSize + ChunkSize;
			m_ptr = _virtual_realloc( newalloc );
			if( m_ptr == NULL )
			{
				throw Exception::OutOfMemory(
					"Out-of-memory on block re-allocation. "
					"Old size: " + to_string( m_size ) + " bytes, "
					"New size: " + to_string( newalloc ) + " bytes"
				);
			}
			m_size = newalloc;
		}
	}

	// Gets a pointer to the requested allocation index.
	// DevBuilds : Throws std::out_of_range() if the index is invalid.
	T *GetPtr( uint idx=0 ) { return _getPtr( idx ); }
	const T *GetPtr( uint idx=0 ) const { return _getPtr( idx ); }

	// Gets an element of this memory allocation much as if it were an array.
	// DevBuilds : Throws std::out_of_range() if the index is invalid.
	T& operator[]( int idx ) { return *_getPtr( (uint)idx ); }
	const T& operator[]( int idx ) const { return *_getPtr( (uint)idx ); }

	virtual MemoryAlloc<T>* Clone() const
	{
		MemoryAlloc<T>* retval = new MemoryAlloc<T>( m_size );
		memcpy_fast( retval->GetPtr(), m_ptr, sizeof(T) * m_size );
		return retval;
	}

protected:
	// A safe array index fetcher.  Throws an exception if the array index
	// is outside the bounds of the array.
	// Performance Considerations: This function adds quite a bit of overhead
	// to array indexing and thus should be done infrequently if used in
	// time-critical situations.  Indead of using it from inside loops, cache
	// the pointer into a local variable and use stad (unsafe) C indexes.
	T* _getPtr( uint i ) const
	{
#ifdef PCSX2_DEVBUILD
		if( i >= (uint)m_size )
		{
			throw Exception::IndexBoundsFault(
				"Index out of bounds on MemoryAlloc: " + Name + 
				" (index=" + to_string(i) + 
				", size=" + to_string(m_size) + ")"
			);
		}
#endif
		return &m_ptr[i];
	}

};

//////////////////////////////////////////////////////////////////
// Handy little class for allocating a resizable memory block, complete with
// exception-based error handling and automatic cleanup.
// This one supports aligned data allocations too!

template< typename T, uint Alignment >
class SafeAlignedArray : public MemoryAlloc<T>
{
protected:
	T* _virtual_realloc( int newsize )
	{
		return (T*)_aligned_realloc( this->m_ptr, newsize * sizeof(T), Alignment );
	}

	// Appends "(align: xx)" to the name of the allocation in devel builds.
	// Maybe useful,maybe not... no harm in atatching it. :D
	string _getName( const string& src )
	{
#ifdef PCSX2_DEVBUILD
		return src + "(align:" + to_string(Alignment) + ")";
#endif
		return src;
	}

public:
	virtual ~SafeAlignedArray()
	{
		safe_aligned_free( this->m_ptr );
		// mptr is set to null, so the parent class's destructor won't re-free it.
	}

	explicit SafeAlignedArray( const std::string& name="Unnamed" ) : 
		MemoryAlloc<T>::MemoryAlloc( name )
	{
	}

	explicit SafeAlignedArray( int initialSize, const std::string& name="Unnamed" ) : 
		MemoryAlloc<T>::MemoryAlloc(
			_getName(name),
			(T*)_aligned_malloc( initialSize * sizeof(T), Alignment ),
			initialSize 
		)
	{
	}

	virtual SafeAlignedArray<T,Alignment>* Clone() const
	{
		SafeAlignedArray<T,Alignment>* retval = new SafeAlignedArray<T,Alignment>( this->m_size );
		memcpy_fast( retval->GetPtr(), this->m_ptr, sizeof(T) * this->m_size );
		return retval;
	}
};

#endif
