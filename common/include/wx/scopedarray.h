/////////////////////////////////////////////////////////////////////////////
// Name:        wx/scopedarray.h
// Purpose:     scoped smart pointer class
// Author:      Vadim Zeitlin
// Created:     2009-02-03
// RCS-ID:      $Id: scopedarray.h 58634 2009-02-03 12:01:46Z VZ $
// Copyright:   (c) Jesse Lovelace and original Boost authors (see below)
//              (c) 2009 Vadim Zeitlin
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#pragma once
#include "wx/defs.h"

// ----------------------------------------------------------------------------
// wxScopedArray: A scoped array, same as a wxScopedPtr but uses delete[]
// instead of delete.
// ----------------------------------------------------------------------------

template <class T>
class wxScopedArray
{
public:
	typedef T element_type;

	wxEXPLICIT wxScopedArray(T * array = NULL) : m_array(array) { }

	~wxScopedArray() { delete [] m_array; }

	// test for pointer validity: defining conversion to unspecified_bool_type
	// and not more obvious bool to avoid implicit conversions to integer types
	typedef T *(wxScopedArray<T>::*unspecified_bool_type)() const;
	operator unspecified_bool_type() const
	{
		return m_array ? &wxScopedArray<T>::get : NULL;
	}

	void reset(T *array = NULL)
	{
		if ( array != m_array )
		{
			delete [] m_array;
			m_array = array;
		}
	}

	T& operator[](size_t n) const { return m_array[n]; }

	T *get() const { return m_array; }

	void swap(wxScopedArray &other)
	{
		T * const tmp = other.m_array;
		other.m_array = m_array;
		m_array = tmp;
	}

private:
	T *m_array;

	wxScopedArray(const wxScopedArray<T>&);
	wxScopedArray& operator=(const wxScopedArray<T>&);
};
