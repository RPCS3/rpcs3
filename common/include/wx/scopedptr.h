/////////////////////////////////////////////////////////////////////////////
// Name:        wx/scopedptr.h
// Purpose:     scoped smart pointer class
// Author:      Jesse Lovelace <jllovela@eos.ncsu.edu>
// Created:     06/01/02
// RCS-ID:      $Id: scopedptr.h 60411 2009-04-27 13:59:08Z CE $
// Copyright:   (c) Jesse Lovelace and original Boost authors (see below)
//              (c) 2009 Vadim Zeitlin
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#pragma once
#include "wx/defs.h"

//  This class closely follows the implementation of the boost
//  library scoped_ptr and is an adaption for c++ macro's in
//  the wxWidgets project. The original authors of the boost
//  scoped_ptr are given below with their respective copyrights.

//  (C) Copyright Greg Colvin and Beman Dawes 1998, 1999.
//  Copyright (c) 2001, 2002 Peter Dimov
//
//  Permission to copy, use, modify, sell and distribute this software
//  is granted provided this copyright notice appears in all copies.
//  This software is provided "as is" without express or implied
//  warranty, and with no claim as to its suitability for any purpose.
//
//  See http://www.boost.org/libs/smart_ptr/scoped_ptr.htm for documentation.
//

// ----------------------------------------------------------------------------
// wxScopedPtr: A scoped pointer
// ----------------------------------------------------------------------------

template <class T>
class wxScopedPtr
{
public:
    typedef T element_type;

    wxEXPLICIT wxScopedPtr(T * ptr = NULL) : m_ptr(ptr) { }

    ~wxScopedPtr() { delete m_ptr; }

    // test for pointer validity: defining conversion to unspecified_bool_type
    // and not more obvious bool to avoid implicit conversions to integer types
    typedef T *(wxScopedPtr<T>::*unspecified_bool_type)() const;

    operator unspecified_bool_type() const
    {
        return m_ptr ? &wxScopedPtr<T>::get : NULL;
    }

    void reset(T * ptr = NULL)
    {
        if ( ptr != m_ptr )
        {
            delete m_ptr;
            m_ptr = ptr;
        }
    }

    T *release()
    {
        T *ptr = m_ptr;
        m_ptr = NULL;
        return ptr;
    }

    T & operator*() const
    {
        wxASSERT(m_ptr != NULL);
        return *m_ptr;
    }

    T * operator->() const
    {
        wxASSERT(m_ptr != NULL);
        return m_ptr;
    }

    T * get() const
    {
        return m_ptr;
    }

    void swap(wxScopedPtr& other)
    {
        T * const tmp = other.m_ptr;
        other.m_ptr = m_ptr;
        m_ptr = tmp;
    }

private:
    T * m_ptr;

	wxScopedPtr(const wxScopedPtr<T>&);
	wxScopedPtr& operator=(const wxScopedPtr<T>&);
};
