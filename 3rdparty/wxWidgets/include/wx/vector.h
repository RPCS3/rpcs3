///////////////////////////////////////////////////////////////////////////////
// Name:        wx/vector.h
// Purpose:     STL vector clone
// Author:      Lindsay Mathieson
// Modified by:
// Created:     30.07.2001
// Copyright:   (c) 2001 Lindsay Mathieson <lindsay@mathieson.org>
// Licence:     wxWindows licence
///////////////////////////////////////////////////////////////////////////////

#ifndef _WX_VECTOR_H_
#define _WX_VECTOR_H_

#include "wx/defs.h"

class WXDLLIMPEXP_BASE wxVectorBase
{
public:
    typedef size_t size_type;
private:
    size_type m_allocsize;
    size_type m_size,
              m_capacity;
    void **m_objects;

protected:
    bool Alloc(size_type sz)
    {
        // work in multiples of m_allocsize;
        sz = (sz / m_allocsize + 1) * m_allocsize;
        if (sz <= m_capacity)
            return true;

        // try to realloc
        void *mem = realloc(m_objects, sizeof(void *) * sz);
        if (! mem)
            return false; // failed
        // success
        m_objects = (void **) mem;
        m_capacity = sz;
        return true;
    }

    // untyped destructor of elements - must be overriden
    virtual void Free(void *) = 0;
    // untyped copy constructor of elements - must be overriden
    virtual void *Copy(const void *) const = 0;

    const void *GetItem(size_type idx) const
    {
        wxASSERT(idx < m_size);
        return m_objects[idx];
    }

    void Append(void *obj)
    {
        wxASSERT(m_size < m_capacity);
        m_objects[m_size] = obj;
        m_size++;
    }

    void RemoveAt(size_type idx)
    {
        wxASSERT(idx < m_size);
        Free(m_objects[idx]);
        if (idx < m_size - 1)
            memcpy(
                m_objects + idx,
                m_objects + idx + 1,
                ( m_size - idx - 1 ) * sizeof(void*) );
        m_size--;
    }

    bool copy(const wxVectorBase& vb)
    {
        clear();
        if (! Alloc(vb.size()))
            return false;

        for (size_type i = 0; i < vb.size(); i++)
        {
            void *o = vb.Copy(vb.GetItem(i));
            if (! o)
                return false;
            Append(o);
        }

        return true;
    }

public:
    wxVectorBase() : m_allocsize(16), m_size(0), m_capacity(0), m_objects(0) {}
    virtual ~wxVectorBase() {} // calm down GCC

    void clear()
    {
        for (size_type i = 0; i < size(); i++)
            Free(m_objects[i]);
        free(m_objects);
        m_objects = 0;
        m_size = m_capacity = 0;
    }

    void reserve(size_type n)
    {
        if ( !Alloc(n) )
        {
            wxFAIL_MSG( _T("out of memory in wxVector::reserve()") );
        }
    }

    size_type size() const
    {
        return m_size;
    }

    size_type capacity() const
    {
        return m_capacity;
    }

    bool empty() const
    {
        return size() == 0;
    }

    wxVectorBase& operator = (const wxVectorBase& vb)
    {
        wxCHECK(copy(vb), *this);
        return *this;
    }
};

#define WX_DECLARE_VECTORBASE(obj, cls)\
protected:\
    virtual void Free(void *o)\
    {\
        delete (obj *) o;\
    }\
    virtual void *Copy(const void *o) const\
    {\
        return new obj(*(obj *) o);\
    }\
public:\
    cls() {}\
    cls(const cls& c) : wxVectorBase()\
    {\
        wxCHECK2(copy(c), return);\
    }\
    ~cls()\
    {\
        clear();\
    }

#define _WX_DECLARE_VECTOR(obj, cls, exp)\
class exp cls : public wxVectorBase\
{\
    WX_DECLARE_VECTORBASE(obj, cls)\
public:\
    void push_back(const obj& o)\
    {\
        wxCHECK2(Alloc(size() + 1), return);\
        Append(new obj(o));\
    }\
    void pop_back()\
    {\
        RemoveAt(size() - 1);\
    }\
    const obj& at(size_type idx) const\
    {\
        return *(obj *) GetItem(idx);\
    }\
    obj& at(size_type idx)\
    {\
        return *(obj *) GetItem(idx);\
    }\
    const obj& operator[](size_type idx) const\
    {\
        return at(idx);\
    }\
    obj& operator[](size_type idx)\
    {\
        return at(idx);\
    }\
    const obj& front() const\
    {\
        return at(0);\
    }\
    obj& front()\
    {\
        return at(0);\
    }\
    const obj& back() const\
    {\
        return at(size() - 1);\
    }\
    obj& back()\
    {\
        return at(size() - 1);\
    }\
    size_type erase(size_type idx)\
    {\
        RemoveAt(idx);\
        return idx;\
    }\
}

#define WX_DECLARE_VECTOR(obj, cls) \
  _WX_DECLARE_VECTOR(obj, cls, WXDLLEXPORT)

#endif // _WX_VECTOR_H_

