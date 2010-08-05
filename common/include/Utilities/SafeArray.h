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


//////////////////////////////////////////////////////////////////////////////////////////
// Safe deallocation macros -- checks pointer validity (non-null) when needed, and sets
// pointer to null after deallocation.

#define safe_delete( ptr ) \
	((void) (delete (ptr)), (ptr) = NULL)

#define safe_delete_array( ptr ) \
	((void) (delete[] (ptr)), (ptr) = NULL)

// No checks for NULL -- wxWidgets says it's safe to skip NULL checks and it runs on 
// just about every compiler and libc implementation of any recentness.
#define safe_free( ptr ) \
	( (void) (free( ptr ), !!0), (ptr) = NULL )
	//((void) (( ( (ptr) != NULL ) && (free( ptr ), !!0) ), (ptr) = NULL))

#define safe_fclose( ptr ) \
	((void) (( ( (ptr) != NULL ) && (fclose( ptr ), !!0) ), (ptr) = NULL))

// Implementation note: all known implementations of _aligned_free check the pointer for
// NULL status (our implementation under GCC, and microsoft's under MSVC), so no need to
// do it here.
#define safe_aligned_free( ptr ) \
	((void) ( _aligned_free( ptr ), (ptr) = NULL ))

#define SafeSysMunmap( ptr, size ) \
	((void) ( HostSys::Munmap( (uptr)(ptr), size ), (ptr) = NULL ))

// Microsoft Windows only macro, useful for freeing out COM objects:
#define safe_release( ptr ) \
	((void) (( ( (ptr) != NULL ) && ((ptr)->Release(), !!0) ), (ptr) = NULL))

// --------------------------------------------------------------------------------------
//  SafeArray
// --------------------------------------------------------------------------------------
// Handy little class for allocating a resizable memory block, complete with exception
// error handling and automatic cleanup.  A lightweight alternative to std::vector.
//
template< typename T >
class SafeArray
{
	DeclareNoncopyableObject(SafeArray);

public:
	static const int DefaultChunkSize = 0x1000 * sizeof(T);

public:
	wxString	Name;			// user-assigned block name
	int			ChunkSize;

protected:
	T*			m_ptr;
	int			m_size;			// size of the allocation of memory

protected:
	SafeArray( const wxChar* name, T* allocated_mem, int initSize );
	virtual T* _virtual_realloc( int newsize );

	// A safe array index fetcher.  Asserts if the index is out of bounds (dev and debug
	// builds only -- no bounds checking is done in release builds).
	T* _getPtr( uint i ) const
	{
		IndexBoundsCheckDev( Name.c_str(), i, m_size );
		return &m_ptr[i];
	}

public:
	virtual ~SafeArray() throw();

	explicit SafeArray( const wxChar* name=L"Unnamed" );
	explicit SafeArray( int initialSize, const wxChar* name=L"Unnamed" );
	
	void Dispose();
	void ExactAlloc( int newsize );
	void MakeRoomFor( int newsize )
	{
		if( newsize > m_size )
			ExactAlloc( newsize );
	}

	bool IsDisposed() const { return (m_ptr==NULL); }

	// Returns the size of the memory allocation, as according to the array type.
	int GetLength() const { return m_size; }
	// Returns the size of the memory allocation in bytes.
	int GetSizeInBytes() const { return m_size * sizeof(T); }

	// Extends the containment area of the array.  Extensions are performed
	// in chunks.
	void GrowBy( int items )
	{
		MakeRoomFor( m_size + ChunkSize + items + 1 );
	}

	// Gets a pointer to the requested allocation index.
	// DevBuilds : Generates assertion if the index is invalid.
	T *GetPtr( uint idx=0 ) { return _getPtr( idx ); }
	const T *GetPtr( uint idx=0 ) const { return _getPtr( idx ); }

	// Gets an element of this memory allocation much as if it were an array.
	// DevBuilds : Generates assertion if the index is invalid.
	T& operator[]( int idx ) { return *_getPtr( (uint)idx ); }
	const T& operator[]( int idx ) const { return *_getPtr( (uint)idx ); }

	virtual SafeArray<T>* Clone() const;
};

//////////////////////////////////////////////////////////////////////////////////////////
// SafeList - Simple growable container without all the mess or hassle of std containers.
//
// This container is intended for reasonably simple class types only.  Things which this
// container does not handle with desired robustness:
//
//  * Classes with non-trivial constructors (such that construction creates much overhead)
//  * Classes with copy constructors (copying is done using performance memcpy)
//  * Classes with destructors (they're not called, sorry!)
//
template< typename T >
class SafeList
{
	DeclareNoncopyableObject(SafeList);

public:
	static const int DefaultChunkSize = 0x80 * sizeof(T);

public:
	wxString	Name;			// user-assigned block name
	int			ChunkSize;		// assigned DefaultChunkSize on init, reconfigurable at any time.

protected:
	T*		m_ptr;
	int		m_allocsize;		// size of the allocation of memory
	uint	m_length;			// length of the array (active items, not buffer allocation)

protected:
	virtual T* _virtual_realloc( int newsize );
	void _MakeRoomFor_threshold( int newsize );

	T* _getPtr( uint i ) const
	{
		IndexBoundsCheckDev( Name.c_str(), i, m_length );
		return &m_ptr[i];
	}

public:
	virtual ~SafeList() throw();
	explicit SafeList( const wxChar* name=L"Unnamed" );
	explicit SafeList( int initialSize, const wxChar* name=L"Unnamed" );
	virtual SafeList<T>* Clone() const;

	void Remove( int index );
	void MakeRoomFor( int blockSize );

	T& New();
	int Add( const T& src );
	T& AddNew( const T& src );

	// Returns the size of the list, as according to the array type.  This includes
	// mapped items only.  The actual size of the allocation may differ.
	int GetLength() const { return m_length; }

	// Returns the size of the list, in bytes.  This includes mapped items only.
	// The actual size of the allocation may differ.
	int GetSizeInBytes() const { return m_length * sizeof(T); }

	void MatchLengthToAllocatedSize()
	{
		m_length = m_allocsize;
	}

	void GrowBy( int items )
	{
		MakeRoomFor( m_length + ChunkSize + items + 1 );
	}

	// Sets the item length to zero.  Does not free memory allocations.
	void Clear()
	{
		m_length = 0;
	}

	// Gets an element of this memory allocation much as if it were an array.
	// DevBuilds : Generates assertion if the index is invalid.
	T& operator[]( int idx )				{ return *_getPtr( (uint)idx ); }
	const T& operator[]( int idx ) const	{ return *_getPtr( (uint)idx ); }

	T* GetPtr()				{ return m_ptr; }
	const T* GetPtr() const	{ return m_ptr; }

	T& GetLast()			{ return m_ptr[m_length-1]; }
	const T& GetLast() const{ return m_ptr[m_length-1]; }
};

// --------------------------------------------------------------------------------------
//  SafeAlignedArray<T>
// --------------------------------------------------------------------------------------
// Handy little class for allocating a resizable memory block, complete with
// exception-based error handling and automatic cleanup.
// This one supports aligned data allocations too!

template< typename T, uint Alignment >
class SafeAlignedArray : public SafeArray<T>
{
	typedef SafeArray<T> _parent;

protected:
	T* _virtual_realloc( int newsize );

public:
	using _parent::operator[];

	virtual ~SafeAlignedArray();

	explicit SafeAlignedArray( const wxChar* name=L"Unnamed" ) :
		SafeArray<T>::SafeArray( name )
	{
	}

	explicit SafeAlignedArray( int initialSize, const wxChar* name=L"Unnamed" );
	virtual SafeAlignedArray<T,Alignment>* Clone() const;
};

