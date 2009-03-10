///////////////////////////////////////////////////////////////////////////////
// Name:        wx/arrimpl.cpp
// Purpose:     helper file for implementation of dynamic lists
// Author:      Vadim Zeitlin
// Modified by:
// Created:     16.10.97
// RCS-ID:      $Id: arrimpl.cpp 34241 2005-05-22 12:10:55Z JS $
// Copyright:   (c) 1997 Vadim Zeitlin <zeitlin@dptmaths.ens-cachan.fr>
// Licence:     wxWindows license
///////////////////////////////////////////////////////////////////////////////

/*****************************************************************************
 * Purpose: implements methods of "template" class declared in               *
 *          DECLARE_OBJARRAY macro and which couldn't be implemented inline  *
 *          (because they need the full definition of type T in scope)       *
 *                                                                           *
 * Usage:   1) #include dynarray.h                                           *
 *          2) WX_DECLARE_OBJARRAY                                           *
 *          3) #include arrimpl.cpp                                          *
 *          4) WX_DEFINE_OBJARRAY                                            *
 *****************************************************************************/

// needed to resolve the conflict between global T and macro parameter T

#define _WX_ERROR_REMOVE2(x)     wxT("bad index in ") wxT(#x) wxT("::RemoveAt()")

// macro implements remaining (not inline) methods of template list
// (it's private to this file)
#undef  _DEFINE_OBJARRAY
#define _DEFINE_OBJARRAY(T, name)                                             \
name::~name()                                                                 \
{                                                                             \
  Empty();                                                                    \
}                                                                             \
                                                                              \
void name::DoCopy(const name& src)                                            \
{                                                                             \
  for ( size_t ui = 0; ui < src.size(); ui++ )                                \
    Add(src[ui]);                                                             \
}                                                                             \
                                                                              \
name& name::operator=(const name& src)                                        \
{                                                                             \
  Empty();                                                                    \
  DoCopy(src);                                                                \
                                                                              \
  return *this;                                                               \
}                                                                             \
                                                                              \
name::name(const name& src) : wxArrayPtrVoid()                                \
{                                                                             \
  DoCopy(src);                                                                \
}                                                                             \
                                                                              \
void name::DoEmpty()                                                          \
{                                                                             \
  for ( size_t ui = 0; ui < size(); ui++ )                                    \
    delete (T*)base_array::operator[](ui);                                    \
}                                                                             \
                                                                              \
void name::RemoveAt(size_t uiIndex, size_t nRemove)                           \
{                                                                             \
  wxCHECK_RET( uiIndex < size(), _WX_ERROR_REMOVE2(name) );                   \
                                                                              \
  for (size_t i = 0; i < nRemove; i++ )                                       \
    delete (T*)base_array::operator[](uiIndex + i);                           \
                                                                              \
  base_array::erase(begin() + uiIndex, begin() + uiIndex + nRemove);          \
}                                                                             \
                                                                              \
void name::Add(const T& item, size_t nInsert)                                 \
{                                                                             \
  if (nInsert == 0)                                                           \
    return;                                                                   \
  T* pItem = new T(item);                                                     \
  size_t nOldSize = size();                                                   \
  if ( pItem != NULL )                                                        \
    base_array::insert(end(), nInsert, pItem);                                \
  for (size_t i = 1; i < nInsert; i++)                                        \
    base_array::operator[](nOldSize + i) = new T(item);                       \
}                                                                             \
                                                                              \
void name::Insert(const T& item, size_t uiIndex, size_t nInsert)              \
{                                                                             \
  if (nInsert == 0)                                                           \
    return;                                                                   \
  T* pItem = new T(item);                                                     \
  if ( pItem != NULL )                                                        \
    base_array::insert(begin() + uiIndex, nInsert, pItem);                    \
  for (size_t i = 1; i < nInsert; i++)                                        \
    base_array::operator[](uiIndex + i) = new T(item);                        \
}                                                                             \
                                                                              \
int name::Index(const T& Item, bool bFromEnd) const                           \
{                                                                             \
  if ( bFromEnd ) {                                                           \
    if ( size() > 0 ) {                                                       \
      size_t ui = size() - 1;                                                 \
      do {                                                                    \
        if ( (T*)base_array::operator[](ui) == &Item )                        \
          return wx_static_cast(int, ui);                                     \
        ui--;                                                                 \
      }                                                                       \
      while ( ui != 0 );                                                      \
    }                                                                         \
  }                                                                           \
  else {                                                                      \
    for( size_t ui = 0; ui < size(); ui++ ) {                                 \
      if( (T*)base_array::operator[](ui) == &Item )                           \
        return wx_static_cast(int, ui);                                       \
    }                                                                         \
  }                                                                           \
                                                                              \
  return wxNOT_FOUND;                                                         \
}

// redefine the macro so that now it will generate the class implementation
// old value would provoke a compile-time error if this file is not included
#undef  WX_DEFINE_OBJARRAY
#define WX_DEFINE_OBJARRAY(name) _DEFINE_OBJARRAY(_wxObjArray##name, name)
