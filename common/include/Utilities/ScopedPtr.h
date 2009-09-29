#pragma once

#include <wx/scopedarray.h>

// --------------------------------------------------------------------------------------
//  ScopedPtr
// --------------------------------------------------------------------------------------

template< typename T >
class ScopedPtr
{
	DeclareNoncopyableObject(ScopedPtr);
	
protected:
	T* m_ptr;

public:
    typedef T element_type;

    wxEXPLICIT ScopedPtr(T * ptr = NULL) : m_ptr(ptr) { }

    ~ScopedPtr()
		{ Delete(); }

	ScopedPtr& Reassign(T * ptr = NULL)
	{
		if ( ptr != m_ptr )
		{
			Delete();
			m_ptr = ptr;
		}
		return *this;
	}
	
	ScopedPtr& Delete()
	{
		// Thread-safe deletion: Set the pointer to NULL first, and then issue
		// the deletion.  This allows pending Application messages that might be
		// dependent on the current object to nullify their actions.

		T* deleteme = m_ptr;
		m_ptr = NULL;
		delete deleteme;

		return *this;
	}

	// Removes the pointer from scoped management, but does not delete!
    T *DetachPtr()
    {
        T *ptr = m_ptr;
        m_ptr = NULL;
        return ptr;
    }

	// Returns the managed pointer.  Can return NULL as a valid result if the ScopedPtr
	// has no object in management.
	T* GetPtr() const
	{
		return m_ptr;
	}

	void SwapPtr(ScopedPtr& other)
	{
		T * const tmp = other.m_ptr;
		other.m_ptr = m_ptr;
		m_ptr = tmp;
	}
	
	// ----------------------------------------------------------------------------
	//  ScopedPtr Operators
	// ----------------------------------------------------------------------------
	// I've decided to use the ATL's approach to pointer validity tests, opposed to
	// the wx/boost approach (which uses some bizarre member method pointer crap, and can't
	// allow the T* implicit casting.
	
	bool operator!() const throw()
	{
		return m_ptr == NULL;
	}
	
	operator T*() const
	{
		return m_ptr;
	}

	// Equality
	bool operator==(T* pT) const throw()
	{
		return m_ptr == pT;
	}

	// Inequality
	bool operator!=(T* pT) const  throw()
	{
		return !operator==(pT);
	}

	// Convenient assignment operator.  ScopedPtr = NULL will issue an automatic deletion
	// of the managed pointer.
	ScopedPtr& operator=( T* src )
	{
		return Reassign( src );
	}

	// Dereference operator, returns a handle to the managed pointer.
	// Generates a debug assertion if the object is NULL!
    T& operator*() const
    {
        wxASSERT(m_ptr != NULL);
        return *m_ptr;
    }

    T* operator->() const
    {
        wxASSERT(m_ptr != NULL);
        return m_ptr;
    }
};


// --------------------------------------------------------------------------------------
//  pxObjPtr   --  fancified version of wxScopedPtr
// --------------------------------------------------------------------------------------
// This class is a non-null scoped pointer container.  What that means is that the object
// always resets itself to a valid "placebo" function rather than NULL, such that methods
// can be invoked safely without fear of NULL pointer exceptions.  This system is useful
// for objects where most or all public methods can fail silently, and still allow program
// execution flow to continue.
//
// It also implements basic scoped pointer behavior: when the pxObjPtr class is deleted,
// it will automatically delete the pointer in its posession, if the pointer is valid.
//
// Notes:
//  * This class intentionally does not implement the "release" API, because it doesn't
//    really make sense within the context of a non-nullable pointer specification.
//
template< typename T, T& DefaultStaticInst >
class pxObjPtr
{
	DeclareNoncopyableObject(pxObjPtr);

protected:
    T * m_ptr;

public:
    typedef T element_type;

    explicit pxObjPtr(T * ptr = &DefaultStaticInst) : m_ptr(ptr) { }

	bool IsEmpty() const
	{
		return m_ptr != &DefaultStaticInst;
	}

    ~pxObjPtr()
    {
		if( !IsEmpty() ) delete m_ptr;
		m_ptr = NULL;
	}

    // test for pointer validity: defining conversion to unspecified_bool_type
    // and not more obvious bool to avoid implicit conversions to integer types
    typedef T *(pxObjPtr<T,DefaultStaticInst>::*unspecified_bool_type)() const;

    operator unspecified_bool_type() const
    {
        return ( !IsEmpty() ) ? &pxScopedPtr<T>::get : NULL;
    }

    void reset(T * ptr = &DefaultStaticInst)
    {
        if ( ptr != m_ptr )
        {
			if( !IsEmpty() )
				delete m_ptr;
            m_ptr = ptr;
        }
    }

    T& operator*() const
    {
        wxASSERT(m_ptr != NULL);
        return *m_ptr;
    }

    T* operator->() const
    {
        wxASSERT(m_ptr != NULL);
        return m_ptr;
    }

    T* get() const
    {
        wxASSERT(m_ptr != NULL);
        return m_ptr;
    }

    void swap(pxObjPtr& other)
    {
		// Neither pointer in either container should ever be NULL...
		wxASSERT(m_ptr != NULL);
		wxASSERT(other.m_ptr != NULL);

        T * const tmp = other.m_ptr;
        other.m_ptr = m_ptr;
        m_ptr = tmp;
    }
};

