#pragma once

#include <limits>
#include <memory>

extern void  _createHeap_wxString();
extern void  _destroyHeap_wxString();
extern void* _allocHeap_wxString( size_t size );
extern void* _reallocHeap_wxString( void* original, size_t size );
extern void  _freeHeap_wxString( void* ptr );
extern char* _mswHeap_Strdup( const char* src );
extern wchar_t* _mswHeap_Strdup( const wchar_t* src );


extern void  _createHeap_wxObject();
extern void  _destroyHeap_wxObject();
extern void* _allocHeap_wxObject( size_t size );
extern void* _reallocHeap_wxObject( void* original, size_t size );
extern void  _freeHeap_wxObject( void* ptr );

// _pxDestroy_ uses microsoft's internal definition for _Destroy(), found in xmemory;
// which suppresses a warning that we otherwise get.  The warning appears to be specific
// to MSVC, so a basic _MSC_VER define will hopefully do the trick to allow support to
// non-Micrsoft compilers.
template<class T> inline
void _pxDestroy_(T* _Ptr)
{
#ifdef _MSC_VER
	std::_Destroy( _Ptr );
#else
	(_Ptr)->~T();
#endif
}

// --------------------------------------------------------------------------------------
//  wxStringAllocator
// --------------------------------------------------------------------------------------
template<typename T>
class wxStringAllocator
{
public :
	//    typedefs

	typedef T					value_type;
	typedef value_type*			pointer;
	typedef const value_type*	const_pointer;
	typedef value_type&			reference;
	typedef const value_type&	const_reference;
	typedef std::size_t			size_type;
	typedef std::ptrdiff_t		difference_type;

public :
	//    convert an allocator<T> to allocator<U>

	template<typename U>
	struct rebind {
		typedef wxStringAllocator<U> other;
	};

public :
	wxStringAllocator()
	{
		_createHeap_wxString();
	}

	~wxStringAllocator()
	{
		_destroyHeap_wxString();
	}

	wxStringAllocator(wxStringAllocator const&) {}

	template<typename U>
	explicit wxStringAllocator(wxStringAllocator<U> const&) {}

	//    address

	pointer address(reference r) { return &r; }
	const_pointer address(const_reference r) { return &r; }

	//    memory allocation

	pointer allocate(size_type cnt,
		typename std::allocator<void>::const_pointer = 0)
	{
		return reinterpret_cast<pointer>( _allocHeap_wxString(cnt * sizeof(T)) );
	}

	void deallocate(pointer p, size_type)
	{
		_freeHeap_wxString( p );
	}

	//    size

	size_type max_size() const {
		return std::numeric_limits<size_type>::max() / sizeof(T);
	}

	//    construction/destruction

	// standard placement-new syntax to initialize the object:
	void construct(pointer p, const T& t) { new(p) T(t); }
	// standard placement destructor:
	void destroy(pointer p) { _pxDestroy_(p); }

	bool operator==(wxStringAllocator const&) { return true; }
	bool operator!=(wxStringAllocator const& a) { return !operator==(a); }
};    //    end of class Allocator

// --------------------------------------------------------------------------------------
//  wxObjectAllocator
// --------------------------------------------------------------------------------------
template<typename T>
class wxObjectAllocator
{
public :
	//    typedefs

	typedef T					value_type;
	typedef value_type*			pointer;
	typedef const value_type*	const_pointer;
	typedef value_type&			reference;
	typedef const value_type&	const_reference;
	typedef std::size_t			size_type;
	typedef std::ptrdiff_t		difference_type;

public :
	//    convert an allocator<T> to allocator<U>

	template<typename U>
	struct rebind {
		typedef wxObjectAllocator<U> other;
	};

public :
	wxObjectAllocator()
	{
		_createHeap_wxObject();
	}

	~wxObjectAllocator()
	{
		_destroyHeap_wxObject();
	}

	wxObjectAllocator(wxObjectAllocator const&) {}

	template<typename U>
	explicit wxObjectAllocator(wxObjectAllocator<U> const&) {}

	//    address

	pointer address(reference r) { return &r; }
	const_pointer address(const_reference r) { return &r; }

	//    memory allocation

	pointer allocate(size_type cnt,
		typename std::allocator<void>::const_pointer = 0)
	{
		return reinterpret_cast<pointer>( _allocHeap_wxObject(cnt * sizeof(T)) );
	}

	void deallocate(pointer p, size_type)
	{
		_freeHeap_wxObject( p );
	}

	//    size

	size_type max_size() const {
		return std::numeric_limits<size_type>::max() / sizeof(T);
	}

	//    construction/destruction

	// standard placement-new syntax to initialize the object:
	void construct(pointer p, const T& t) { new(p) T(t); }

	// standard placement destructor:
	void destroy(pointer p) { _pxDestroy_(p); }

	bool operator==(wxObjectAllocator const&) { return true; }
	bool operator!=(wxObjectAllocator const& a) { return !operator==(a); }
};    //    end of class Allocator

