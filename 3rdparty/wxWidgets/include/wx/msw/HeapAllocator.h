#pragma once

#include <limits>

extern void  _allocateHeap_wxString();
extern void  _destroyHeap_wxString();
extern void* _allocHeap_wxString( size_t size );
extern void  _freeHeap_wxString( void* ptr );

extern void  _allocateHeap_wxObject();
extern void  _destroyHeap_wxObject();
extern void* _allocHeap_wxObject( size_t size );
extern void  _freeHeap_wxObject( void* ptr );

template<typename T>
class wxStringAllocator
{
public :
	//    typedefs

	typedef T value_type;
	typedef value_type* pointer;
	typedef const value_type* const_pointer;
	typedef value_type& reference;
	typedef const value_type& const_reference;
	typedef std::size_t size_type;
	typedef std::ptrdiff_t difference_type;

public :
	//    convert an allocator<T> to allocator<U>

	template<typename U>
	struct rebind {
		typedef wxStringAllocator<U> other;
	};

public :
	wxStringAllocator()
	{
		_allocateHeap_wxString();
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
	void destroy(pointer p) { p->~T(); }

	bool operator==(wxStringAllocator const&) { return true; }
	bool operator!=(wxStringAllocator const& a) { return !operator==(a); }
};    //    end of class Allocator


// --------------------------------------------------------------------------------------
//
// --------------------------------------------------------------------------------------
template<typename T>
class wxObjectAllocator
{
public :
	//    typedefs

	typedef T value_type;
	typedef value_type* pointer;
	typedef const value_type* const_pointer;
	typedef value_type& reference;
	typedef const value_type& const_reference;
	typedef std::size_t size_type;
	typedef std::ptrdiff_t difference_type;

public :
	//    convert an allocator<T> to allocator<U>

	template<typename U>
	struct rebind {
		typedef wxObjectAllocator<U> other;
	};

public :
	wxObjectAllocator()
	{
		_allocateHeap_wxObject();
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
	void destroy(pointer p) { p->~T(); }

	bool operator==(wxObjectAllocator const&) { return true; }
	bool operator!=(wxObjectAllocator const& a) { return !operator==(a); }
};    //    end of class Allocator

