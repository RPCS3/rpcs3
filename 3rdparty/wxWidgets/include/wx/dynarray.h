///////////////////////////////////////////////////////////////////////////////
// Name:        wx/dynarray.h
// Purpose:     auto-resizable (i.e. dynamic) array support
// Author:      Vadim Zeitlin
// Modified by:
// Created:     12.09.97
// RCS-ID:      $Id: dynarray.h 45498 2007-04-16 13:03:05Z VZ $
// Copyright:   (c) 1998 Vadim Zeitlin <zeitlin@dptmaths.ens-cachan.fr>
// Licence:     wxWindows licence
///////////////////////////////////////////////////////////////////////////////

#ifndef   _DYNARRAY_H
#define   _DYNARRAY_H

#include "wx/defs.h"

#if wxUSE_STL
    #include "wx/beforestd.h"
    #include <vector>
    #include <algorithm>
    #include "wx/afterstd.h"
#endif

/*
  This header defines the dynamic arrays and object arrays (i.e. arrays which
  own their elements). Dynamic means that the arrays grow automatically as
  needed.

  These macros are ugly (especially if you look in the sources ;-), but they
  allow us to define "template" classes without actually using templates and so
  this works with all compilers (and may be also much faster to compile even
  with a compiler which does support templates). The arrays defined with these
  macros are type-safe.

  Range checking is performed in debug build for both arrays and objarrays but
  not in release build - so using an invalid index will just lead to a crash
  then.

  Note about memory usage: arrays never shrink automatically (although you may
  use Shrink() function explicitly), they only grow, so loading 10 millions in
  an array only to delete them 2 lines below might be a bad idea if the array
  object is not going to be destroyed soon. However, as it does free memory
  when destroyed, it is ok if the array is a local variable.
 */

// ----------------------------------------------------------------------------
// constants
// ----------------------------------------------------------------------------

/*
   The initial size by which an array grows when an element is added default
   value avoids allocate one or two bytes when the array is created which is
   rather inefficient
*/
#define WX_ARRAY_DEFAULT_INITIAL_SIZE    (16)

// ----------------------------------------------------------------------------
// types
// ----------------------------------------------------------------------------

/*
    Callback compare function for quick sort.

    It must return negative value, 0 or positive value if the first item is
    less than, equal to or greater than the second one.
 */
extern "C"
{
typedef int (wxCMPFUNC_CONV *CMPFUNC)(const void* pItem1, const void* pItem2);
}

// ----------------------------------------------------------------------------
// Base class managing data having size of type 'long' (not used directly)
//
// NB: for efficiency this often used class has no virtual functions (hence no
//     virtual table), even dtor is *not* virtual. If used as expected it
//     won't create any problems because ARRAYs from DEFINE_ARRAY have no dtor
//     at all, so it's not too important if it's not called (this happens when
//     you cast "SomeArray *" as "BaseArray *" and then delete it)
// ----------------------------------------------------------------------------

#if wxUSE_STL

template<class T>
class WXDLLIMPEXP_BASE wxArray_SortFunction
{
public:
    typedef int (wxCMPFUNC_CONV *CMPFUNC)(T* pItem1, T* pItem2);

    wxArray_SortFunction(CMPFUNC f) : m_f(f) { }
    bool operator()(const T& i1, const T& i2)
      { return m_f((T*)&i1, (T*)&i2) < 0; }
private:
    CMPFUNC m_f;
};

template<class T, typename F>
class WXDLLIMPEXP_BASE wxSortedArray_SortFunction
{
public:
    typedef F CMPFUNC;

    wxSortedArray_SortFunction(CMPFUNC f) : m_f(f) { }
    bool operator()(const T& i1, const T& i2)
      { return m_f(i1, i2) < 0; }
private:
    CMPFUNC m_f;
};

#define  _WX_DECLARE_BASEARRAY(T, name, classexp)                   \
   typedef int (wxCMPFUNC_CONV *CMPFUN##name)(T pItem1, T pItem2);  \
   typedef wxSortedArray_SortFunction<T, CMPFUN##name> name##_Predicate; \
   _WX_DECLARE_BASEARRAY_2(T, name, name##_Predicate, classexp)

#define  _WX_DECLARE_BASEARRAY_2(T, name, predicate, classexp)      \
classexp name : public std::vector<T>                               \
{                                                                   \
  typedef predicate Predicate;                                      \
  typedef predicate::CMPFUNC SCMPFUNC;                              \
public:                                                             \
  typedef wxArray_SortFunction<T>::CMPFUNC CMPFUNC;                 \
public:                                                             \
  void Empty() { clear(); }                                         \
  void Clear() { clear(); }                                         \
  void Alloc(size_t uiSize) { reserve(uiSize); }                    \
  void Shrink();                                                    \
                                                                    \
  size_t GetCount() const { return size(); }                        \
  void SetCount(size_t n, T v = T()) { resize(n, v); }              \
  bool IsEmpty() const { return empty(); }                          \
  size_t Count() const { return size(); }                           \
                                                                    \
  typedef T base_type;                                              \
                                                                    \
protected:                                                          \
  T& Item(size_t uiIndex) const                                     \
    { wxASSERT( uiIndex < size() ); return (T&)operator[](uiIndex); }   \
                                                                    \
  int Index(T e, bool bFromEnd = false) const;                      \
  int Index(T lItem, CMPFUNC fnCompare) const;                      \
  size_t IndexForInsert(T lItem, CMPFUNC fnCompare) const;          \
  void Add(T lItem, size_t nInsert = 1)                             \
    { insert(end(), nInsert, lItem); }                              \
  size_t Add(T lItem, CMPFUNC fnCompare);                           \
  void Insert(T lItem, size_t uiIndex, size_t nInsert = 1)          \
    { insert(begin() + uiIndex, nInsert, lItem); }                  \
  void Remove(T lItem);                                             \
  void RemoveAt(size_t uiIndex, size_t nRemove = 1)                 \
    { erase(begin() + uiIndex, begin() + uiIndex + nRemove); }      \
                                                                    \
  void Sort(CMPFUNC fCmp)                                           \
  {                                                                 \
    wxArray_SortFunction<T> p(fCmp);                                \
    std::sort(begin(), end(), p);                                   \
  }                                                                 \
}

#else // if !wxUSE_STL

#define  _WX_DECLARE_BASEARRAY(T, name, classexp)                   \
classexp name                                                       \
{                                                                   \
  typedef CMPFUNC SCMPFUNC; /* for compatibility wuth wxUSE_STL */  \
public:                                                             \
  name();                                                           \
  name(const name& array);                                          \
  name& operator=(const name& src);                                 \
  ~name();                                                          \
                                                                    \
  void Empty() { m_nCount = 0; }                                    \
  void Clear();                                                     \
  void Alloc(size_t n) { if ( n > m_nSize ) Realloc(n); }           \
  void Shrink();                                                    \
                                                                    \
  size_t GetCount() const { return m_nCount; }                      \
  void SetCount(size_t n, T defval = T());                          \
  bool IsEmpty() const { return m_nCount == 0; }                    \
  size_t Count() const { return m_nCount; }                         \
                                                                    \
  typedef T base_type;                                              \
                                                                    \
protected:                                                          \
  T& Item(size_t uiIndex) const                                     \
    { wxASSERT( uiIndex < m_nCount ); return m_pItems[uiIndex]; }   \
  T& operator[](size_t uiIndex) const { return Item(uiIndex); }     \
                                                                    \
  int Index(T lItem, bool bFromEnd = false) const;                  \
  int Index(T lItem, CMPFUNC fnCompare) const;                      \
  size_t IndexForInsert(T lItem, CMPFUNC fnCompare) const;          \
  void Add(T lItem, size_t nInsert = 1);                            \
  size_t Add(T lItem, CMPFUNC fnCompare);                           \
  void Insert(T lItem, size_t uiIndex, size_t nInsert = 1);         \
  void Remove(T lItem);                                             \
  void RemoveAt(size_t uiIndex, size_t nRemove = 1);                \
                                                                    \
  void Sort(CMPFUNC fnCompare);                                     \
                                                                    \
  /* *minimal* STL-ish interface, for derived classes */            \
  typedef T value_type;                                             \
  typedef value_type* iterator;                                     \
  typedef const value_type* const_iterator;                         \
  typedef value_type& reference;                                    \
  typedef const value_type& const_reference;                        \
  typedef int difference_type;                                      \
  typedef size_t size_type;                                         \
                                                                    \
  void assign(const_iterator first, const_iterator last);           \
  void assign(size_type n, const_reference v);                      \
  size_type capacity() const { return m_nSize; }                    \
  iterator erase(iterator first, iterator last)                     \
  {                                                                 \
    size_type idx = first - begin();                                \
    RemoveAt(idx, last - first);                                    \
    return begin() + idx;                                           \
  }                                                                 \
  iterator erase(iterator it) { return erase(it, it + 1); }         \
  void insert(iterator it, size_type n, const value_type& v)        \
    { Insert(v, it - begin(), n); }                                 \
  iterator insert(iterator it, const value_type& v = value_type())  \
  {                                                                 \
    size_type idx = it - begin();                                   \
    Insert(v, idx);                                                 \
    return begin() + idx;                                           \
  }                                                                 \
  void insert(iterator it, const_iterator first, const_iterator last);\
  void pop_back() { RemoveAt(size() - 1); }                         \
  void push_back(const value_type& v) { Add(v); }                   \
  void reserve(size_type n) { Alloc(n); }                           \
  void resize(size_type n, value_type v = value_type())             \
    { SetCount(n, v); }                                             \
                                                                    \
  iterator begin() { return m_pItems; }                             \
  iterator end() { return m_pItems + m_nCount; }                    \
  const_iterator begin() const { return m_pItems; }                 \
  const_iterator end() const { return m_pItems + m_nCount; }        \
                                                                    \
  /* the following functions may be made directly public because */ \
  /* they don't use the type of the elements at all */              \
public:                                                             \
  void clear() { Clear(); }                                         \
  bool empty() const { return IsEmpty(); }                          \
  size_type max_size() const { return INT_MAX; }                    \
  size_type size() const { return GetCount(); }                     \
                                                                    \
private:                                                            \
  void Grow(size_t nIncrement = 0);                                 \
  bool Realloc(size_t nSize);                                       \
                                                                    \
  size_t  m_nSize,                                                  \
          m_nCount;                                                 \
                                                                    \
  T      *m_pItems;                                                 \
}

#endif // !wxUSE_STL

// ============================================================================
// The private helper macros containing the core of the array classes
// ============================================================================

// Implementation notes:
//
// JACS: Salford C++ doesn't like 'var->operator=' syntax, as in:
//          { ((wxBaseArray *)this)->operator=((const wxBaseArray&)src);
//       so using a temporary variable instead.
//
// The classes need a (even trivial) ~name() to link under Mac X
//
// _WX_ERROR_REMOVE is needed to resolve the name conflict between the wxT()
// macro and T typedef: we can't use wxT() inside WX_DEFINE_ARRAY!

#define _WX_ERROR_REMOVE wxT("removing inexisting element in wxArray::Remove")

// ----------------------------------------------------------------------------
// _WX_DEFINE_TYPEARRAY: array for simple types
// ----------------------------------------------------------------------------

#if wxUSE_STL

#define  _WX_DEFINE_TYPEARRAY(T, name, base, classexp)                \
typedef int (CMPFUNC_CONV *CMPFUNC##T)(T *pItem1, T *pItem2);         \
classexp name : public base                                           \
{                                                                     \
public:                                                               \
  T& operator[](size_t uiIndex) const                                 \
    { return (T&)(base::operator[](uiIndex)); }                       \
  T& Item(size_t uiIndex) const                                       \
    { return (T&)/*const cast*/base::operator[](uiIndex); }           \
  T& Last() const                                                     \
    { return Item(Count() - 1); }                                     \
                                                                      \
  int Index(T e, bool bFromEnd = false) const                         \
    { return base::Index(e, bFromEnd); }                              \
                                                                      \
  void Add(T lItem, size_t nInsert = 1)                               \
    { insert(end(), nInsert, lItem); }                                \
  void Insert(T lItem, size_t uiIndex, size_t nInsert = 1)            \
    { insert(begin() + uiIndex, nInsert, lItem); }                    \
                                                                      \
  void RemoveAt(size_t uiIndex, size_t nRemove = 1)                   \
    { base::RemoveAt(uiIndex, nRemove); }                             \
  void Remove(T lItem)                                                \
    { int iIndex = Index(lItem);                                      \
      wxCHECK2_MSG( iIndex != wxNOT_FOUND, return,                    \
         _WX_ERROR_REMOVE);                                           \
      RemoveAt((size_t)iIndex); }                                     \
                                                                      \
  void Sort(CMPFUNC##T fCmp) { base::Sort((CMPFUNC)fCmp); }           \
}

#define  _WX_DEFINE_TYPEARRAY_PTR(T, name, base, classexp)         \
         _WX_DEFINE_TYPEARRAY(T, name, base, classexp)

#else // if !wxUSE_STL

// common declaration used by both _WX_DEFINE_TYPEARRAY and
// _WX_DEFINE_TYPEARRAY_PTR
#define  _WX_DEFINE_TYPEARRAY_HELPER(T, name, base, classexp, ptrop)  \
wxCOMPILE_TIME_ASSERT2(sizeof(T) <= sizeof(base::base_type),          \
                       TypeTooBigToBeStoredIn##base,                  \
                       name);                                         \
typedef int (CMPFUNC_CONV *CMPFUNC##T)(T *pItem1, T *pItem2);         \
classexp name : public base                                           \
{                                                                     \
public:                                                               \
  name() { }                                                          \
  ~name() { }                                                         \
                                                                      \
  name& operator=(const name& src)                                    \
    { base* temp = (base*) this;                                      \
      (*temp) = ((const base&)src);                                   \
      return *this; }                                                 \
                                                                      \
  T& operator[](size_t uiIndex) const                                 \
    { return (T&)(base::operator[](uiIndex)); }                       \
  T& Item(size_t uiIndex) const                                       \
    { return (T&)(base::operator[](uiIndex)); }                       \
  T& Last() const                                                     \
    { return (T&)(base::operator[](Count() - 1)); }                   \
                                                                      \
  int Index(T lItem, bool bFromEnd = false) const                     \
    { return base::Index((base_type)lItem, bFromEnd); }               \
                                                                      \
  void Add(T lItem, size_t nInsert = 1)                               \
    { base::Add((base_type)lItem, nInsert); }                         \
  void Insert(T lItem, size_t uiIndex, size_t nInsert = 1)            \
    { base::Insert((base_type)lItem, uiIndex, nInsert) ; }            \
                                                                      \
  void RemoveAt(size_t uiIndex, size_t nRemove = 1)                   \
    { base::RemoveAt(uiIndex, nRemove); }                             \
  void Remove(T lItem)                                                \
    { int iIndex = Index(lItem);                                      \
      wxCHECK2_MSG( iIndex != wxNOT_FOUND, return,                    \
         _WX_ERROR_REMOVE);                                           \
      base::RemoveAt((size_t)iIndex); }                               \
                                                                      \
  void Sort(CMPFUNC##T fCmp) { base::Sort((CMPFUNC)fCmp); }           \
                                                                      \
  /* STL-like interface */                                            \
private:                                                              \
  typedef base::iterator biterator;                                   \
  typedef base::const_iterator bconst_iterator;                       \
  typedef base::value_type bvalue_type;                               \
  typedef base::const_reference bconst_reference;                     \
public:                                                               \
  typedef T value_type;                                               \
  typedef value_type* pointer;                                        \
  typedef const value_type* const_pointer;                            \
  typedef value_type* iterator;                                       \
  typedef const value_type* const_iterator;                           \
  typedef value_type& reference;                                      \
  typedef const value_type& const_reference;                          \
  typedef base::difference_type difference_type;                      \
  typedef base::size_type size_type;                                  \
                                                                      \
  class reverse_iterator                                              \
  {                                                                   \
    typedef T value_type;                                             \
    typedef value_type& reference;                                    \
    typedef value_type* pointer;                                      \
    typedef reverse_iterator itor;                                    \
    friend inline itor operator+(int o, const itor& it)               \
        { return it.m_ptr - o; }                                      \
    friend inline itor operator+(const itor& it, int o)               \
        { return it.m_ptr - o; }                                      \
    friend inline itor operator-(const itor& it, int o)               \
        { return it.m_ptr + o; }                                      \
    friend inline difference_type operator-(const itor& i1,           \
                                            const itor& i2)           \
        { return i1.m_ptr - i2.m_ptr; }                               \
                                                                      \
  public:                                                             \
    pointer m_ptr;                                                    \
    reverse_iterator() : m_ptr(NULL) { }                              \
    reverse_iterator(pointer ptr) : m_ptr(ptr) { }                    \
    reverse_iterator(const itor& it) : m_ptr(it.m_ptr) { }            \
    reference operator*() const { return *m_ptr; }                    \
    ptrop                                                             \
    itor& operator++() { --m_ptr; return *this; }                     \
    const itor operator++(int)                                        \
      { reverse_iterator tmp = *this; --m_ptr; return tmp; }          \
    itor& operator--() { ++m_ptr; return *this; }                     \
    const itor operator--(int) { itor tmp = *this; ++m_ptr; return tmp; }\
    bool operator ==(const itor& it) const { return m_ptr == it.m_ptr; }\
    bool operator !=(const itor& it) const { return m_ptr != it.m_ptr; }\
  };                                                                  \
                                                                      \
  class const_reverse_iterator                                        \
  {                                                                   \
    typedef T value_type;                                             \
    typedef const value_type& reference;                              \
    typedef const value_type* pointer;                                \
    typedef const_reverse_iterator itor;                              \
    friend inline itor operator+(int o, const itor& it)               \
        { return it.m_ptr - o; }                                      \
    friend inline itor operator+(const itor& it, int o)               \
        { return it.m_ptr - o; }                                      \
    friend inline itor operator-(const itor& it, int o)               \
        { return it.m_ptr + o; }                                      \
    friend inline difference_type operator-(const itor& i1,           \
                                            const itor& i2)           \
        { return i1.m_ptr - i2.m_ptr; }                               \
                                                                      \
  public:                                                             \
    pointer m_ptr;                                                    \
    const_reverse_iterator() : m_ptr(NULL) { }                        \
    const_reverse_iterator(pointer ptr) : m_ptr(ptr) { }              \
    const_reverse_iterator(const itor& it) : m_ptr(it.m_ptr) { }      \
    const_reverse_iterator(const reverse_iterator& it) : m_ptr(it.m_ptr) { }\
    reference operator*() const { return *m_ptr; }                    \
    ptrop                                                             \
    itor& operator++() { --m_ptr; return *this; }                     \
    const itor operator++(int)                                        \
      { itor tmp = *this; --m_ptr; return tmp; }                      \
    itor& operator--() { ++m_ptr; return *this; }                     \
    const itor operator--(int) { itor tmp = *this; ++m_ptr; return tmp; }\
    bool operator ==(const itor& it) const { return m_ptr == it.m_ptr; }\
    bool operator !=(const itor& it) const { return m_ptr != it.m_ptr; }\
  };                                                                  \
                                                                      \
  name(size_type n, const_reference v) { assign(n, v); }              \
  name(const_iterator first, const_iterator last)                     \
    { assign(first, last); }                                          \
  void assign(const_iterator first, const_iterator last)              \
    { base::assign((bconst_iterator)first, (bconst_iterator)last); }  \
  void assign(size_type n, const_reference v)                         \
    { base::assign(n, (bconst_reference)v); }                         \
  reference back() { return *(end() - 1); }                           \
  const_reference back() const { return *(end() - 1); }               \
  iterator begin() { return (iterator)base::begin(); }                \
  const_iterator begin() const { return (const_iterator)base::begin(); }\
  size_type capacity() const { return base::capacity(); }             \
  iterator end() { return (iterator)base::end(); }                    \
  const_iterator end() const { return (const_iterator)base::end(); }  \
  iterator erase(iterator first, iterator last)                       \
    { return (iterator)base::erase((biterator)first, (biterator)last); }\
  iterator erase(iterator it)                                         \
    { return (iterator)base::erase((biterator)it); }                  \
  reference front() { return *begin(); }                              \
  const_reference front() const { return *begin(); }                  \
  void insert(iterator it, size_type n, const_reference v)            \
    { base::insert((biterator)it, n, (bconst_reference)v); }          \
  iterator insert(iterator it, const_reference v = value_type())      \
    { return (iterator)base::insert((biterator)it, (bconst_reference)v); }\
  void insert(iterator it, const_iterator first, const_iterator last) \
    { base::insert((biterator)it, (bconst_iterator)first,             \
                   (bconst_iterator)last); }                          \
  void pop_back() { base::pop_back(); }                               \
  void push_back(const_reference v)                                   \
    { base::push_back((bconst_reference)v); }                         \
  reverse_iterator rbegin() { return reverse_iterator(end() - 1); }   \
  const_reverse_iterator rbegin() const;                              \
  reverse_iterator rend() { return reverse_iterator(begin() - 1); }   \
  const_reverse_iterator rend() const;                                \
  void reserve(size_type n) { base::reserve(n); }                     \
  void resize(size_type n, value_type v = value_type())               \
    { base::resize(n, v); }                                           \
}

#define _WX_PTROP pointer operator->() const { return m_ptr; }
#define _WX_PTROP_NONE

#define _WX_DEFINE_TYPEARRAY(T, name, base, classexp)                 \
    _WX_DEFINE_TYPEARRAY_HELPER(T, name, base, classexp, _WX_PTROP)
#define _WX_DEFINE_TYPEARRAY_PTR(T, name, base, classexp)          \
    _WX_DEFINE_TYPEARRAY_HELPER(T, name, base, classexp, _WX_PTROP_NONE)

#endif // !wxUSE_STL

// ----------------------------------------------------------------------------
// _WX_DEFINE_SORTED_TYPEARRAY: sorted array for simple data types
//    cannot handle types with size greater than pointer because of sorting
// ----------------------------------------------------------------------------

#define _WX_DEFINE_SORTED_TYPEARRAY_2(T, name, base, defcomp, classexp, comptype)\
wxCOMPILE_TIME_ASSERT2(sizeof(T) <= sizeof(base::base_type),          \
                       TypeTooBigToBeStoredInSorted##base,            \
                       name);                                         \
classexp name : public base                                           \
{                                                                     \
  typedef comptype SCMPFUNC;                                          \
public:                                                               \
  name(comptype fn defcomp) { m_fnCompare = fn; }                     \
                                                                      \
  name& operator=(const name& src)                                    \
    { base* temp = (base*) this;                                      \
      (*temp) = ((const base&)src);                                   \
      m_fnCompare = src.m_fnCompare;                                  \
      return *this; }                                                 \
                                                                      \
  T& operator[](size_t uiIndex) const                                 \
    { return (T&)(base::operator[](uiIndex)); }                       \
  T& Item(size_t uiIndex) const                                       \
    { return (T&)(base::operator[](uiIndex)); }                       \
  T& Last() const                                                     \
    { return (T&)(base::operator[](size() - 1)); }                    \
                                                                      \
  int Index(T lItem) const                                            \
    { return base::Index(lItem, (CMPFUNC)m_fnCompare); }              \
                                                                      \
  size_t IndexForInsert(T lItem) const                                \
    { return base::IndexForInsert(lItem, (CMPFUNC)m_fnCompare); }     \
                                                                      \
  void AddAt(T item, size_t index)                                    \
    { base::insert(begin() + index, item); }                          \
                                                                      \
  size_t Add(T lItem)                                                 \
    { return base::Add(lItem, (CMPFUNC)m_fnCompare); }                \
                                                                      \
  void RemoveAt(size_t uiIndex, size_t nRemove = 1)                   \
    { base::erase(begin() + uiIndex, begin() + uiIndex + nRemove); }  \
  void Remove(T lItem)                                                \
    { int iIndex = Index(lItem);                                      \
      wxCHECK2_MSG( iIndex != wxNOT_FOUND, return,                    \
        _WX_ERROR_REMOVE );                                           \
      base::erase(begin() + iIndex); }                                \
                                                                      \
private:                                                              \
  comptype m_fnCompare;                                               \
}


// ----------------------------------------------------------------------------
// _WX_DECLARE_OBJARRAY: an array for pointers to type T with owning semantics
// ----------------------------------------------------------------------------

#define _WX_DECLARE_OBJARRAY(T, name, base, classexp)                    \
typedef int (CMPFUNC_CONV *CMPFUNC##T)(T **pItem1, T **pItem2);          \
classexp name : protected base                                           \
{                                                                        \
typedef int (CMPFUNC_CONV *CMPFUNC##base)(void **pItem1, void **pItem2); \
typedef base base_array;                                                 \
public:                                                                  \
  name() { }                                                             \
  name(const name& src);                                                 \
  name& operator=(const name& src);                                      \
                                                                         \
  ~name();                                                               \
                                                                         \
  void Alloc(size_t count) { reserve(count); }                           \
  size_t GetCount() const { return base_array::size(); }                 \
  size_t size() const { return base_array::size(); }                     \
  bool IsEmpty() const { return base_array::empty(); }                   \
  bool empty() const { return base_array::empty(); }                     \
  size_t Count() const { return base_array::size(); }                    \
  void Shrink() { base::Shrink(); }                                      \
                                                                         \
  T& operator[](size_t uiIndex) const                                    \
    { return *(T*)base::operator[](uiIndex); }                           \
  T& Item(size_t uiIndex) const                                          \
    { return *(T*)base::operator[](uiIndex); }                           \
  T& Last() const                                                        \
    { return *(T*)(base::operator[](size() - 1)); }                      \
                                                                         \
  int Index(const T& lItem, bool bFromEnd = false) const;                \
                                                                         \
  void Add(const T& lItem, size_t nInsert = 1);                          \
  void Add(const T* pItem)                                               \
    { base::push_back((T*)pItem); }                                      \
  void push_back(const T* pItem)                                         \
    { base::push_back((T*)pItem); }                                      \
  void push_back(const T& lItem)                                         \
    { Add(lItem); }                                                      \
                                                                         \
  void Insert(const T& lItem,  size_t uiIndex, size_t nInsert = 1);      \
  void Insert(const T* pItem, size_t uiIndex)                            \
    { base::insert(begin() + uiIndex, (T*)pItem); }                      \
                                                                         \
  void Empty() { DoEmpty(); base::clear(); }                             \
  void Clear() { DoEmpty(); base::clear(); }                             \
                                                                         \
  T* Detach(size_t uiIndex)                                              \
    { T* p = (T*)base::operator[](uiIndex);                              \
      base::erase(begin() + uiIndex); return p; }                        \
  void RemoveAt(size_t uiIndex, size_t nRemove = 1);                     \
                                                                         \
  void Sort(CMPFUNC##T fCmp) { base::Sort((CMPFUNC##base)fCmp); }        \
                                                                         \
private:                                                                 \
  void DoEmpty();                                                        \
  void DoCopy(const name& src);                                          \
}

// ============================================================================
// The public macros for declaration and definition of the dynamic arrays
// ============================================================================

// Please note that for each macro WX_FOO_ARRAY we also have
// WX_FOO_EXPORTED_ARRAY and WX_FOO_USER_EXPORTED_ARRAY which are exactly the
// same except that they use an additional __declspec(dllexport) or equivalent
// under Windows if needed.
//
// The first (just EXPORTED) macros do it if wxWidgets was compiled as a DLL
// and so must be used used inside the library. The second kind (USER_EXPORTED)
// allow the user code to do it when it wants. This is needed if you have a dll
// that wants to export a wxArray daubed with your own import/export goo.
//
// Finally, you can define the macro below as something special to modify the
// arrays defined by a simple WX_FOO_ARRAY as well. By default is is empty.
#define wxARRAY_DEFAULT_EXPORT

// ----------------------------------------------------------------------------
// WX_DECLARE_BASEARRAY(T, name) declare an array class named "name" containing
// the elements of type T
// ----------------------------------------------------------------------------

#define WX_DECLARE_BASEARRAY(T, name)                             \
    WX_DECLARE_USER_EXPORTED_BASEARRAY(T, name, wxARRAY_DEFAULT_EXPORT)

#define WX_DECLARE_EXPORTED_BASEARRAY(T, name)                    \
    WX_DECLARE_USER_EXPORTED_BASEARRAY(T, name, WXDLLEXPORT)

#define WX_DECLARE_USER_EXPORTED_BASEARRAY(T, name, expmode)      \
    typedef T _wxArray##name;                                     \
    _WX_DECLARE_BASEARRAY(_wxArray##name, name, class expmode)

// ----------------------------------------------------------------------------
// WX_DEFINE_TYPEARRAY(T, name, base) define an array class named "name" deriving
// from class "base" containing the elements of type T
//
// Note that the class defined has only inline function and doesn't take any
// space at all so there is no size penalty for defining multiple array classes
// ----------------------------------------------------------------------------

#define WX_DEFINE_TYPEARRAY(T, name, base)                        \
    WX_DEFINE_TYPEARRAY_WITH_DECL(T, name, base, class wxARRAY_DEFAULT_EXPORT)

#define WX_DEFINE_TYPEARRAY_PTR(T, name, base)                        \
    WX_DEFINE_TYPEARRAY_WITH_DECL_PTR(T, name, base, class wxARRAY_DEFAULT_EXPORT)

#define WX_DEFINE_EXPORTED_TYPEARRAY(T, name, base)               \
    WX_DEFINE_TYPEARRAY_WITH_DECL(T, name, base, class WXDLLEXPORT)

#define WX_DEFINE_EXPORTED_TYPEARRAY_PTR(T, name, base)               \
    WX_DEFINE_TYPEARRAY_WITH_DECL_PTR(T, name, base, class WXDLLEXPORT)

#define WX_DEFINE_USER_EXPORTED_TYPEARRAY(T, name, base, expdecl) \
    WX_DEFINE_TYPEARRAY_WITH_DECL(T, name, base, class expdecl)

#define WX_DEFINE_USER_EXPORTED_TYPEARRAY_PTR(T, name, base, expdecl) \
    WX_DEFINE_TYPEARRAY_WITH_DECL_PTR(T, name, base, class expdecl)

#define WX_DEFINE_TYPEARRAY_WITH_DECL(T, name, base, classdecl) \
    typedef T _wxArray##name;                                   \
    _WX_DEFINE_TYPEARRAY(_wxArray##name, name, base, classdecl)

#define WX_DEFINE_TYPEARRAY_WITH_DECL_PTR(T, name, base, classdecl) \
    typedef T _wxArray##name;                                          \
    _WX_DEFINE_TYPEARRAY_PTR(_wxArray##name, name, base, classdecl)

// ----------------------------------------------------------------------------
// WX_DEFINE_SORTED_TYPEARRAY: this is the same as the previous macro, but it
// defines a sorted array.
//
// Differences:
//  1) it must be given a COMPARE function in ctor which takes 2 items of type
//     T* and should return -1, 0 or +1 if the first one is less/greater
//     than/equal to the second one.
//  2) the Add() method inserts the item in such was that the array is always
//     sorted (it uses the COMPARE function)
//  3) it has no Sort() method because it's always sorted
//  4) Index() method is much faster (the sorted arrays use binary search
//     instead of linear one), but Add() is slower.
//  5) there is no Insert() method because you can't insert an item into the
//     given position in a sorted array but there is IndexForInsert()/AddAt()
//     pair which may be used to optimize a common operation of "insert only if
//     not found"
//
// Note that you have to specify the comparison function when creating the
// objects of this array type. If, as in 99% of cases, the comparison function
// is the same for all objects of a class, WX_DEFINE_SORTED_TYPEARRAY_CMP below
// is more convenient.
//
// Summary: use this class when the speed of Index() function is important, use
// the normal arrays otherwise.
// ----------------------------------------------------------------------------

// we need a macro which expands to nothing to pass correct number of
// parameters to a nested macro invocation even when we don't have anything to
// pass it
#define wxARRAY_EMPTY

#define WX_DEFINE_SORTED_TYPEARRAY(T, name, base)                         \
    WX_DEFINE_SORTED_USER_EXPORTED_TYPEARRAY(T, name, base,               \
                                             wxARRAY_DEFAULT_EXPORT)

#define WX_DEFINE_SORTED_EXPORTED_TYPEARRAY(T, name, base)                \
    WX_DEFINE_SORTED_USER_EXPORTED_TYPEARRAY(T, name, base, WXDLLEXPORT)

#define WX_DEFINE_SORTED_USER_EXPORTED_TYPEARRAY(T, name, base, expmode)  \
    typedef T _wxArray##name;                                             \
    typedef int (CMPFUNC_CONV *SCMPFUNC##name)(T pItem1, T pItem2);       \
    _WX_DEFINE_SORTED_TYPEARRAY_2(_wxArray##name, name, base,             \
                                wxARRAY_EMPTY, class expmode, SCMPFUNC##name)

// ----------------------------------------------------------------------------
// WX_DEFINE_SORTED_TYPEARRAY_CMP: exactly the same as above but the comparison
// function is provided by this macro and the objects of this class have a
// default constructor which just uses it.
//
// The arguments are: the element type, the comparison function and the array
// name
//
// NB: this is, of course, how WX_DEFINE_SORTED_TYPEARRAY() should have worked
//     from the very beginning - unfortunately I didn't think about this earlier
// ----------------------------------------------------------------------------

#define WX_DEFINE_SORTED_TYPEARRAY_CMP(T, cmpfunc, name, base)               \
    WX_DEFINE_SORTED_USER_EXPORTED_TYPEARRAY_CMP(T, cmpfunc, name, base,     \
                                                 wxARRAY_DEFAULT_EXPORT)

#define WX_DEFINE_SORTED_EXPORTED_TYPEARRAY_CMP(T, cmpfunc, name, base)      \
    WX_DEFINE_SORTED_USER_EXPORTED_TYPEARRAY_CMP(T, cmpfunc, name, base,     \
                                                 WXDLLEXPORT)

#define WX_DEFINE_SORTED_USER_EXPORTED_TYPEARRAY_CMP(T, cmpfunc, name, base, \
                                                     expmode)                \
    typedef T _wxArray##name;                                                \
    typedef int (CMPFUNC_CONV *SCMPFUNC##name)(T pItem1, T pItem2);          \
    _WX_DEFINE_SORTED_TYPEARRAY_2(_wxArray##name, name, base, = cmpfunc,     \
                                class expmode, SCMPFUNC##name)

// ----------------------------------------------------------------------------
// WX_DECLARE_OBJARRAY(T, name): this macro generates a new array class
// named "name" which owns the objects of type T it contains, i.e. it will
// delete them when it is destroyed.
//
// An element is of type T*, but arguments of type T& are taken (see below!)
// and T& is returned.
//
// Don't use this for simple types such as "int" or "long"!
//
// Note on Add/Insert functions:
//  1) function(T*) gives the object to the array, i.e. it will delete the
//     object when it's removed or in the array's dtor
//  2) function(T&) will create a copy of the object and work with it
//
// Also:
//  1) Remove() will delete the object after removing it from the array
//  2) Detach() just removes the object from the array (returning pointer to it)
//
// NB1: Base type T should have an accessible copy ctor if Add(T&) is used
// NB2: Never ever cast a array to it's base type: as dtor is not virtual
//      and so you risk having at least the memory leaks and probably worse
//
// Some functions of this class are not inline, so it takes some space to
// define new class from this template even if you don't use it - which is not
// the case for the simple (non-object) array classes
//
// To use an objarray class you must
//      #include "dynarray.h"
//      WX_DECLARE_OBJARRAY(element_type, list_class_name)
//      #include "arrimpl.cpp"
//      WX_DEFINE_OBJARRAY(list_class_name) // name must be the same as above!
//
// This is necessary because at the moment of DEFINE_OBJARRAY class parsing the
// element_type must be fully defined (i.e. forward declaration is not
// enough), while WX_DECLARE_OBJARRAY may be done anywhere. The separation of
// two allows to break cicrcular dependencies with classes which have member
// variables of objarray type.
// ----------------------------------------------------------------------------

#define WX_DECLARE_OBJARRAY(T, name)                        \
    WX_DECLARE_USER_EXPORTED_OBJARRAY(T, name, wxARRAY_DEFAULT_EXPORT)

#define WX_DECLARE_EXPORTED_OBJARRAY(T, name)               \
    WX_DECLARE_USER_EXPORTED_OBJARRAY(T, name, WXDLLEXPORT)

#define WX_DECLARE_OBJARRAY_WITH_DECL(T, name, decl) \
    typedef T _wxObjArray##name;                            \
    _WX_DECLARE_OBJARRAY(_wxObjArray##name, name, wxArrayPtrVoid, decl)

#define WX_DECLARE_USER_EXPORTED_OBJARRAY(T, name, expmode) \
    WX_DECLARE_OBJARRAY_WITH_DECL(T, name, class expmode)

// WX_DEFINE_OBJARRAY is going to be redefined when arrimpl.cpp is included,
// try to provoke a human-understandable error if it used incorrectly.
//
// there is no real need for 3 different macros in the DEFINE case but do it
// anyhow for consistency
#define WX_DEFINE_OBJARRAY(name) DidYouIncludeArrimplCpp
#define WX_DEFINE_EXPORTED_OBJARRAY(name)   WX_DEFINE_OBJARRAY(name)
#define WX_DEFINE_USER_EXPORTED_OBJARRAY(name)   WX_DEFINE_OBJARRAY(name)

// ----------------------------------------------------------------------------
// Some commonly used predefined base arrays
// ----------------------------------------------------------------------------

WX_DECLARE_USER_EXPORTED_BASEARRAY(const void *, wxBaseArrayPtrVoid,
                                   WXDLLIMPEXP_BASE);
WX_DECLARE_USER_EXPORTED_BASEARRAY(char, wxBaseArrayChar, WXDLLIMPEXP_BASE);
WX_DECLARE_USER_EXPORTED_BASEARRAY(short, wxBaseArrayShort, WXDLLIMPEXP_BASE);
WX_DECLARE_USER_EXPORTED_BASEARRAY(int, wxBaseArrayInt, WXDLLIMPEXP_BASE);
WX_DECLARE_USER_EXPORTED_BASEARRAY(long, wxBaseArrayLong, WXDLLIMPEXP_BASE);
WX_DECLARE_USER_EXPORTED_BASEARRAY(size_t, wxBaseArraySizeT, WXDLLIMPEXP_BASE);
WX_DECLARE_USER_EXPORTED_BASEARRAY(double, wxBaseArrayDouble, WXDLLIMPEXP_BASE);

// ----------------------------------------------------------------------------
// Convenience macros to define arrays from base arrays
// ----------------------------------------------------------------------------

#define WX_DEFINE_ARRAY(T, name)                                       \
    WX_DEFINE_TYPEARRAY(T, name, wxBaseArrayPtrVoid)
#define WX_DEFINE_ARRAY_PTR(T, name)                                \
    WX_DEFINE_TYPEARRAY_PTR(T, name, wxBaseArrayPtrVoid)
#define WX_DEFINE_EXPORTED_ARRAY(T, name)                              \
    WX_DEFINE_EXPORTED_TYPEARRAY(T, name, wxBaseArrayPtrVoid)
#define WX_DEFINE_EXPORTED_ARRAY_PTR(T, name)                       \
    WX_DEFINE_EXPORTED_TYPEARRAY_PTR(T, name, wxBaseArrayPtrVoid)
#define WX_DEFINE_ARRAY_WITH_DECL_PTR(T, name, decl)                \
    WX_DEFINE_TYPEARRAY_WITH_DECL_PTR(T, name, wxBaseArrayPtrVoid, decl)
#define WX_DEFINE_USER_EXPORTED_ARRAY(T, name, expmode)                \
    WX_DEFINE_TYPEARRAY_WITH_DECL(T, name, wxBaseArrayPtrVoid, wxARRAY_EMPTY expmode)
#define WX_DEFINE_USER_EXPORTED_ARRAY_PTR(T, name, expmode)         \
    WX_DEFINE_TYPEARRAY_WITH_DECL_PTR(T, name, wxBaseArrayPtrVoid, wxARRAY_EMPTY expmode)

#define WX_DEFINE_ARRAY_CHAR(T, name)                                 \
    WX_DEFINE_TYPEARRAY_PTR(T, name, wxBaseArrayChar)
#define WX_DEFINE_EXPORTED_ARRAY_CHAR(T, name)                        \
    WX_DEFINE_EXPORTED_TYPEARRAY_PTR(T, name, wxBaseArrayChar)
#define WX_DEFINE_USER_EXPORTED_ARRAY_CHAR(T, name, expmode)          \
    WX_DEFINE_TYPEARRAY_WITH_DECL_PTR(T, name, wxBaseArrayChar, wxARRAY_EMPTY expmode)

#define WX_DEFINE_ARRAY_SHORT(T, name)                                 \
    WX_DEFINE_TYPEARRAY_PTR(T, name, wxBaseArrayShort)
#define WX_DEFINE_EXPORTED_ARRAY_SHORT(T, name)                        \
    WX_DEFINE_EXPORTED_TYPEARRAY_PTR(T, name, wxBaseArrayShort)
#define WX_DEFINE_USER_EXPORTED_ARRAY_SHORT(T, name, expmode)          \
    WX_DEFINE_TYPEARRAY_WITH_DECL_PTR(T, name, wxBaseArrayShort, wxARRAY_EMPTY expmode)

#define WX_DEFINE_ARRAY_INT(T, name)                                   \
    WX_DEFINE_TYPEARRAY_PTR(T, name, wxBaseArrayInt)
#define WX_DEFINE_EXPORTED_ARRAY_INT(T, name)                          \
    WX_DEFINE_EXPORTED_TYPEARRAY_PTR(T, name, wxBaseArrayInt)
#define WX_DEFINE_USER_EXPORTED_ARRAY_INT(T, name, expmode)            \
    WX_DEFINE_TYPEARRAY_WITH_DECL_PTR(T, name, wxBaseArrayInt, wxARRAY_EMPTY expmode)

#define WX_DEFINE_ARRAY_LONG(T, name)                                  \
    WX_DEFINE_TYPEARRAY_PTR(T, name, wxBaseArrayLong)
#define WX_DEFINE_EXPORTED_ARRAY_LONG(T, name)                         \
    WX_DEFINE_EXPORTED_TYPEARRAY_PTR(T, name, wxBaseArrayLong)
#define WX_DEFINE_USER_EXPORTED_ARRAY_LONG(T, name, expmode)           \
    WX_DEFINE_TYPEARRAY_WITH_DECL_PTR(T, name, wxBaseArrayLong, wxARRAY_EMPTY expmode)

#define WX_DEFINE_ARRAY_SIZE_T(T, name)                                  \
    WX_DEFINE_TYPEARRAY_PTR(T, name, wxBaseArraySizeT)
#define WX_DEFINE_EXPORTED_ARRAY_SIZE_T(T, name)                         \
    WX_DEFINE_EXPORTED_TYPEARRAY_PTR(T, name, wxBaseArraySizeT)
#define WX_DEFINE_USER_EXPORTED_ARRAY_SIZE_T(T, name, expmode)           \
    WX_DEFINE_TYPEARRAY_WITH_DECL_PTR(T, name, wxBaseArraySizeT, wxARRAY_EMPTY expmode)

#define WX_DEFINE_ARRAY_DOUBLE(T, name)                                \
    WX_DEFINE_TYPEARRAY_PTR(T, name, wxBaseArrayDouble)
#define WX_DEFINE_EXPORTED_ARRAY_DOUBLE(T, name)                       \
    WX_DEFINE_EXPORTED_TYPEARRAY_PTR(T, name, wxBaseArrayDouble)
#define WX_DEFINE_USER_EXPORTED_ARRAY_DOUBLE(T, name, expmode)         \
    WX_DEFINE_TYPEARRAY_WITH_DECL_PTR(T, name, wxBaseArrayDouble, wxARRAY_EMPTY expmode)

// ----------------------------------------------------------------------------
// Convenience macros to define sorted arrays from base arrays
// ----------------------------------------------------------------------------

#define WX_DEFINE_SORTED_ARRAY(T, name)                                \
    WX_DEFINE_SORTED_TYPEARRAY(T, name, wxBaseArrayPtrVoid)
#define WX_DEFINE_SORTED_EXPORTED_ARRAY(T, name)                       \
    WX_DEFINE_SORTED_EXPORTED_TYPEARRAY(T, name, wxBaseArrayPtrVoid)
#define WX_DEFINE_SORTED_USER_EXPORTED_ARRAY(T, name, expmode)         \
    WX_DEFINE_SORTED_USER_EXPORTED_TYPEARRAY(T, name, wxBaseArrayPtrVoid, wxARRAY_EMPTY expmode)

#define WX_DEFINE_SORTED_ARRAY_CHAR(T, name)                          \
    WX_DEFINE_SORTED_TYPEARRAY(T, name, wxBaseArrayChar)
#define WX_DEFINE_SORTED_EXPORTED_ARRAY_CHAR(T, name)                 \
    WX_DEFINE_SORTED_EXPORTED_TYPEARRAY(T, name, wxBaseArrayChar)
#define WX_DEFINE_SORTED_USER_EXPORTED_ARRAY_CHAR(T, name, expmode)   \
    WX_DEFINE_SORTED_USER_EXPORTED_TYPEARRAY(T, name, wxBaseArrayChar, wxARRAY_EMPTY expmode)

#define WX_DEFINE_SORTED_ARRAY_SHORT(T, name)                          \
    WX_DEFINE_SORTED_TYPEARRAY(T, name, wxBaseArrayShort)
#define WX_DEFINE_SORTED_EXPORTED_ARRAY_SHORT(T, name)                 \
    WX_DEFINE_SORTED_EXPORTED_TYPEARRAY(T, name, wxBaseArrayShort)
#define WX_DEFINE_SORTED_USER_EXPORTED_ARRAY_SHORT(T, name, expmode)   \
    WX_DEFINE_SORTED_USER_EXPORTED_TYPEARRAY(T, name, wxBaseArrayShort, wxARRAY_EMPTY expmode)

#define WX_DEFINE_SORTED_ARRAY_INT(T, name)                            \
    WX_DEFINE_SORTED_TYPEARRAY(T, name, wxBaseArrayInt)
#define WX_DEFINE_SORTED_EXPORTED_ARRAY_INT(T, name)                   \
    WX_DEFINE_SORTED_EXPORTED_TYPEARRAY(T, name, wxBaseArrayInt)
#define WX_DEFINE_SORTED_USER_EXPORTED_ARRAY_INT(T, name, expmode)     \
    WX_DEFINE_SORTED_USER_EXPORTED_TYPEARRAY(T, name, wxBaseArrayInt, expmode)

#define WX_DEFINE_SORTED_ARRAY_LONG(T, name)                           \
    WX_DEFINE_SORTED_TYPEARRAY(T, name, wxBaseArrayLong)
#define WX_DEFINE_SORTED_EXPORTED_ARRAY_LONG(T, name)                  \
    WX_DEFINE_SORTED_EXPORTED_TYPEARRAY(T, name, wxBaseArrayLong)
#define WX_DEFINE_SORTED_USER_EXPORTED_ARRAY_LONG(T, name, expmode)    \
    WX_DEFINE_SORTED_USER_EXPORTED_TYPEARRAY(T, name, wxBaseArrayLong, expmode)

#define WX_DEFINE_SORTED_ARRAY_SIZE_T(T, name)                           \
    WX_DEFINE_SORTED_TYPEARRAY(T, name, wxBaseArraySizeT)
#define WX_DEFINE_SORTED_EXPORTED_ARRAY_SIZE_T(T, name)                  \
    WX_DEFINE_SORTED_EXPORTED_TYPEARRAY(T, name, wxBaseArraySizeT)
#define WX_DEFINE_SORTED_USER_EXPORTED_ARRAY_SIZE_T(T, name, expmode)    \
    WX_DEFINE_SORTED_USER_EXPORTED_TYPEARRAY(T, name, wxBaseArraySizeT, wxARRAY_EMPTY expmode)

// ----------------------------------------------------------------------------
// Convenience macros to define sorted arrays from base arrays
// ----------------------------------------------------------------------------

#define WX_DEFINE_SORTED_ARRAY_CMP(T, cmpfunc, name)                   \
    WX_DEFINE_SORTED_TYPEARRAY_CMP(T, cmpfunc, name, wxBaseArrayPtrVoid)
#define WX_DEFINE_SORTED_EXPORTED_ARRAY_CMP(T, cmpfunc, name)          \
    WX_DEFINE_SORTED_EXPORTED_TYPEARRAY_CMP(T, cmpfunc, name, wxBaseArrayPtrVoid)
#define WX_DEFINE_SORTED_USER_EXPORTED_ARRAY_CMP(T, cmpfunc,           \
                                                     name, expmode)    \
    WX_DEFINE_SORTED_USER_EXPORTED_TYPEARRAY_CMP(T, cmpfunc, name,     \
                                                 wxBaseArrayPtrVoid,   \
                                                 wxARRAY_EMPTY expmode)

#define WX_DEFINE_SORTED_ARRAY_CMP_CHAR(T, cmpfunc, name)             \
    WX_DEFINE_SORTED_TYPEARRAY_CMP(T, cmpfunc, name, wxBaseArrayChar)
#define WX_DEFINE_SORTED_EXPORTED_ARRAY_CMP_CHAR(T, cmpfunc, name)    \
    WX_DEFINE_SORTED_EXPORTED_TYPEARRAY_CMP(T, cmpfunc, name, wxBaseArrayChar)
#define WX_DEFINE_SORTED_USER_EXPORTED_ARRAY_CMP_CHAR(T, cmpfunc,      \
                                                       name, expmode)  \
    WX_DEFINE_SORTED_USER_EXPORTED_TYPEARRAY_CMP(T, cmpfunc, name,     \
                                                 wxBaseArrayChar,      \
                                                 wxARRAY_EMPTY expmode)

#define WX_DEFINE_SORTED_ARRAY_CMP_SHORT(T, cmpfunc, name)             \
    WX_DEFINE_SORTED_TYPEARRAY_CMP(T, cmpfunc, name, wxBaseArrayShort)
#define WX_DEFINE_SORTED_EXPORTED_ARRAY_CMP_SHORT(T, cmpfunc, name)    \
    WX_DEFINE_SORTED_EXPORTED_TYPEARRAY_CMP(T, cmpfunc, name, wxBaseArrayShort)
#define WX_DEFINE_SORTED_USER_EXPORTED_ARRAY_CMP_SHORT(T, cmpfunc,     \
                                                       name, expmode)  \
    WX_DEFINE_SORTED_USER_EXPORTED_TYPEARRAY_CMP(T, cmpfunc, name,     \
                                                 wxBaseArrayShort,     \
                                                 wxARRAY_EMPTY expmode)

#define WX_DEFINE_SORTED_ARRAY_CMP_INT(T, cmpfunc, name)               \
    WX_DEFINE_SORTED_TYPEARRAY_CMP(T, cmpfunc, name, wxBaseArrayInt)
#define WX_DEFINE_SORTED_EXPORTED_ARRAY_CMP_INT(T, cmpfunc, name)      \
    WX_DEFINE_SORTED_EXPORTED_TYPEARRAY_CMP(T, cmpfunc, name, wxBaseArrayInt)
#define WX_DEFINE_SORTED_USER_EXPORTED_ARRAY_CMP_INT(T, cmpfunc,       \
                                                     name, expmode)    \
    WX_DEFINE_SORTED_USER_EXPORTED_TYPEARRAY_CMP(T, cmpfunc, name,     \
                                                 wxBaseArrayInt,       \
                                                 wxARRAY_EMPTY expmode)

#define WX_DEFINE_SORTED_ARRAY_CMP_LONG(T, cmpfunc, name)              \
    WX_DEFINE_SORTED_TYPEARRAY_CMP(T, cmpfunc, name, wxBaseArrayLong)
#define WX_DEFINE_SORTED_EXPORTED_ARRAY_CMP_LONG(T, cmpfunc, name)     \
    WX_DEFINE_SORTED_EXPORTED_TYPEARRAY_CMP(T, cmpfunc, name, wxBaseArrayLong)
#define WX_DEFINE_SORTED_USER_EXPORTED_ARRAY_CMP_LONG(T, cmpfunc,      \
                                                      name, expmode)   \
    WX_DEFINE_SORTED_USER_EXPORTED_TYPEARRAY_CMP(T, cmpfunc, name,     \
                                                 wxBaseArrayLong,      \
                                                 wxARRAY_EMPTY expmode)

#define WX_DEFINE_SORTED_ARRAY_CMP_SIZE_T(T, cmpfunc, name)              \
    WX_DEFINE_SORTED_TYPEARRAY_CMP(T, cmpfunc, name, wxBaseArraySizeT)
#define WX_DEFINE_SORTED_EXPORTED_ARRAY_CMP_SIZE_T(T, cmpfunc, name)     \
    WX_DEFINE_SORTED_EXPORTED_TYPEARRAY_CMP(T, cmpfunc, name, wxBaseArraySizeT)
#define WX_DEFINE_SORTED_USER_EXPORTED_ARRAY_CMP_SIZE_T(T, cmpfunc,      \
                                                      name, expmode)   \
    WX_DEFINE_SORTED_USER_EXPORTED_TYPEARRAY_CMP(T, cmpfunc, name,     \
                                                 wxBaseArraySizeT,     \
                                                 wxARRAY_EMPTY expmode)

// ----------------------------------------------------------------------------
// Some commonly used predefined arrays
// ----------------------------------------------------------------------------

WX_DEFINE_USER_EXPORTED_ARRAY_SHORT(short, wxArrayShort, class WXDLLIMPEXP_BASE);
WX_DEFINE_USER_EXPORTED_ARRAY_INT(int, wxArrayInt, class WXDLLIMPEXP_BASE);
WX_DEFINE_USER_EXPORTED_ARRAY_DOUBLE(double, wxArrayDouble, class WXDLLIMPEXP_BASE);
WX_DEFINE_USER_EXPORTED_ARRAY_LONG(long, wxArrayLong, class WXDLLIMPEXP_BASE);
WX_DEFINE_USER_EXPORTED_ARRAY_PTR(void *, wxArrayPtrVoid, class WXDLLIMPEXP_BASE);

// -----------------------------------------------------------------------------
// convenience macros
// -----------------------------------------------------------------------------

// prepend all element of one array to another one; e.g. if first array contains
// elements X,Y,Z and the second contains A,B,C (in those orders), then the
// first array will be result as A,B,C,X,Y,Z
#define WX_PREPEND_ARRAY(array, other)                                        \
    {                                                                         \
        size_t wxAAcnt = (other).size();                                      \
        (array).Alloc(wxAAcnt);                                               \
        for ( size_t wxAAn = 0; wxAAn < wxAAcnt; wxAAn++ )                    \
        {                                                                     \
            (array).Insert((other)[wxAAn], wxAAn);                            \
        }                                                                     \
    }

// append all element of one array to another one
#define WX_APPEND_ARRAY(array, other)                                         \
    {                                                                         \
        size_t wxAAcnt = (other).size();                                      \
        (array).Alloc(wxAAcnt);                                               \
        for ( size_t wxAAn = 0; wxAAn < wxAAcnt; wxAAn++ )                    \
        {                                                                     \
            (array).push_back((other)[wxAAn]);                                \
        }                                                                     \
    }

// delete all array elements
//
// NB: the class declaration of the array elements must be visible from the
//     place where you use this macro, otherwise the proper destructor may not
//     be called (a decent compiler should give a warning about it, but don't
//     count on it)!
#define WX_CLEAR_ARRAY(array)                                                 \
    {                                                                         \
        size_t wxAAcnt = (array).size();                                      \
        for ( size_t wxAAn = 0; wxAAn < wxAAcnt; wxAAn++ )                    \
        {                                                                     \
            delete (array)[wxAAn];                                            \
        }                                                                     \
                                                                              \
        (array).clear();                                                      \
    }

#endif // _DYNARRAY_H
