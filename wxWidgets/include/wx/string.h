///////////////////////////////////////////////////////////////////////////////
// Name:        wx/string.h
// Purpose:     wxString and wxArrayString classes
// Author:      Vadim Zeitlin
// Modified by:
// Created:     29/01/98
// RCS-ID:      $Id: string.h 61872 2009-09-09 22:37:05Z VZ $
// Copyright:   (c) 1998 Vadim Zeitlin <zeitlin@dptmaths.ens-cachan.fr>
// Licence:     wxWindows licence
///////////////////////////////////////////////////////////////////////////////

/*
    Efficient string class [more or less] compatible with MFC CString,
    wxWidgets version 1 wxString and std::string and some handy functions
    missing from string.h.
*/

#ifndef _WX_WXSTRINGH__
#define _WX_WXSTRINGH__

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

#include "wx/defs.h"        // everybody should include this

#if defined(__WXMAC__) || defined(__VISAGECPP__)
    #include <ctype.h>
#endif

#if defined(__VISAGECPP__) && __IBMCPP__ >= 400
   // problem in VACPP V4 with including stdlib.h multiple times
   // strconv includes it anyway
#  include <stdio.h>
#  include <string.h>
#  include <stdarg.h>
#  include <limits.h>
#else
#  include <string.h>
#  include <stdio.h>
#  include <stdarg.h>
#  include <limits.h>
#  include <stdlib.h>
#endif

#ifdef HAVE_STRCASECMP_IN_STRINGS_H
    #include <strings.h>    // for strcasecmp()
#endif // HAVE_STRCASECMP_IN_STRINGS_H

#ifdef __WXPALMOS__
    #include <StringMgr.h>
#endif

#include "wx/wxchar.h"      // for wxChar
#include "wx/buffer.h"      // for wxCharBuffer
#include "wx/strconv.h"     // for wxConvertXXX() macros and wxMBConv classes

class WXDLLIMPEXP_FWD_BASE wxString;

// ---------------------------------------------------------------------------
// macros
// ---------------------------------------------------------------------------

// casts [unfortunately!] needed to call some broken functions which require
// "char *" instead of "const char *"
#define   WXSTRINGCAST (wxChar *)(const wxChar *)
#define   wxCSTRINGCAST (wxChar *)(const wxChar *)
#define   wxMBSTRINGCAST (char *)(const char *)
#define   wxWCSTRINGCAST (wchar_t *)(const wchar_t *)

// implementation only
#define   wxASSERT_VALID_INDEX(i) \
    wxASSERT_MSG( (size_t)(i) <= length(), wxT("invalid index in wxString") )

// ----------------------------------------------------------------------------
// constants
// ----------------------------------------------------------------------------

#if WXWIN_COMPATIBILITY_2_6

// deprecated in favour of wxString::npos, don't use in new code
//
// maximum possible length for a string means "take all string" everywhere
#define wxSTRING_MAXLEN wxStringBase::npos

#endif // WXWIN_COMPATIBILITY_2_6

// ----------------------------------------------------------------------------
// global data
// ----------------------------------------------------------------------------

// global pointer to empty string
extern WXDLLIMPEXP_DATA_BASE(const wxChar*) wxEmptyString;

// ---------------------------------------------------------------------------
// global functions complementing standard C string library replacements for
// strlen() and portable strcasecmp()
//---------------------------------------------------------------------------

// Use wxXXX() functions from wxchar.h instead! These functions are for
// backwards compatibility only.

// checks whether the passed in pointer is NULL and if the string is empty
inline bool IsEmpty(const char *p) { return (!p || !*p); }

// safe version of strlen() (returns 0 if passed NULL pointer)
inline size_t Strlen(const char *psz)
  { return psz ? strlen(psz) : 0; }

// portable strcasecmp/_stricmp
inline int Stricmp(const char *psz1, const char *psz2)
{
#if defined(__VISUALC__) && defined(__WXWINCE__)
  register char c1, c2;
  do {
    c1 = tolower(*psz1++);
    c2 = tolower(*psz2++);
  } while ( c1 && (c1 == c2) );

  return c1 - c2;
#elif defined(__VISUALC__) || ( defined(__MWERKS__) && defined(__INTEL__) )
  return _stricmp(psz1, psz2);
#elif defined(__SC__)
  return _stricmp(psz1, psz2);
#elif defined(__SALFORDC__)
  return stricmp(psz1, psz2);
#elif defined(__BORLANDC__)
  return stricmp(psz1, psz2);
#elif defined(__WATCOMC__)
  return stricmp(psz1, psz2);
#elif defined(__DJGPP__)
  return stricmp(psz1, psz2);
#elif defined(__EMX__)
  return stricmp(psz1, psz2);
#elif defined(__WXPM__)
  return stricmp(psz1, psz2);
#elif defined(__WXPALMOS__) || \
      defined(HAVE_STRCASECMP_IN_STRING_H) || \
      defined(HAVE_STRCASECMP_IN_STRINGS_H) || \
      defined(__GNUWIN32__)
  return strcasecmp(psz1, psz2);
#elif defined(__MWERKS__) && !defined(__INTEL__)
  register char c1, c2;
  do {
    c1 = tolower(*psz1++);
    c2 = tolower(*psz2++);
  } while ( c1 && (c1 == c2) );

  return c1 - c2;
#else
  // almost all compilers/libraries provide this function (unfortunately under
  // different names), that's why we don't implement our own which will surely
  // be more efficient than this code (uncomment to use):
  /*
    register char c1, c2;
    do {
      c1 = tolower(*psz1++);
      c2 = tolower(*psz2++);
    } while ( c1 && (c1 == c2) );

    return c1 - c2;
  */

  #error  "Please define string case-insensitive compare for your OS/compiler"
#endif  // OS/compiler
}

// ----------------------------------------------------------------------------
// deal with STL/non-STL/non-STL-but-wxUSE_STD_STRING
// ----------------------------------------------------------------------------

// in both cases we need to define wxStdString
#if wxUSE_STL || wxUSE_STD_STRING

#include "wx/beforestd.h"
#include <string>
#include "wx/afterstd.h"

#if wxUSE_UNICODE
    #ifdef HAVE_STD_WSTRING
        typedef std::wstring wxStdString;
    #else
        typedef std::basic_string<wxChar> wxStdString;
    #endif
#else
    typedef std::string wxStdString;
#endif

#endif // need <string>

#if wxUSE_STL

    // we don't need an extra ctor from std::string when copy ctor already does
    // the work
    #undef wxUSE_STD_STRING
    #define wxUSE_STD_STRING 0

    #if (defined(__GNUG__) && (__GNUG__ < 3)) || \
        (defined(_MSC_VER) && (_MSC_VER <= 1200))
        #define wxSTRING_BASE_HASNT_CLEAR
    #endif

    typedef wxStdString wxStringBase;
#else // if !wxUSE_STL

#if !defined(HAVE_STD_STRING_COMPARE) && \
    (!defined(__WX_SETUP_H__) || wxUSE_STL == 0)
    #define HAVE_STD_STRING_COMPARE
#endif

// ---------------------------------------------------------------------------
// string data prepended with some housekeeping info (used by wxString class),
// is never used directly (but had to be put here to allow inlining)
// ---------------------------------------------------------------------------

struct WXDLLIMPEXP_BASE wxStringData
{
  int     nRefs;        // reference count
  size_t  nDataLength,  // actual string length
          nAllocLength; // allocated memory size

  // mimics declaration 'wxChar data[nAllocLength]'
  wxChar* data() const { return (wxChar*)(this + 1); }

  // empty string has a special ref count so it's never deleted
  bool  IsEmpty()   const { return (nRefs == -1); }
  bool  IsShared()  const { return (nRefs > 1);   }

  // lock/unlock
  void  Lock()   { if ( !IsEmpty() ) nRefs++;                    }

  // VC++ will refuse to inline Unlock but profiling shows that it is wrong
#if defined(__VISUALC__) && (__VISUALC__ >= 1200)
  __forceinline
#endif
  // VC++ free must take place in same DLL as allocation when using non dll
  // run-time library (e.g. Multithreaded instead of Multithreaded DLL)
#if defined(__VISUALC__) && defined(_MT) && !defined(_DLL)
  void  Unlock() { if ( !IsEmpty() && --nRefs == 0) Free();  }
  // we must not inline deallocation since allocation is not inlined
  void  Free();
#else
  void  Unlock() { if ( !IsEmpty() && --nRefs == 0) free(this);  }
#endif

  // if we had taken control over string memory (GetWriteBuf), it's
  // intentionally put in invalid state
  void  Validate(bool b)  { nRefs = (b ? 1 : 0); }
  bool  IsValid() const   { return (nRefs != 0); }
};

class WXDLLIMPEXP_BASE wxStringBase
{
#if !wxUSE_STL
friend class WXDLLIMPEXP_FWD_BASE wxArrayString;
#endif
public :
  // an 'invalid' value for string index, moved to this place due to a CW bug
  static const size_t npos;
protected:
  // points to data preceded by wxStringData structure with ref count info
  wxChar *m_pchData;

  // accessor to string data
  wxStringData* GetStringData() const { return (wxStringData*)m_pchData - 1; }

  // string (re)initialization functions
    // initializes the string to the empty value (must be called only from
    // ctors, use Reinit() otherwise)
  void Init() { m_pchData = (wxChar *)wxEmptyString; }
    // initializes the string with (a part of) C-string
  void InitWith(const wxChar *psz, size_t nPos = 0, size_t nLen = npos);
    // as Init, but also frees old data
  void Reinit() { GetStringData()->Unlock(); Init(); }

  // memory allocation
    // allocates memory for string of length nLen
  bool AllocBuffer(size_t nLen);
    // copies data to another string
  bool AllocCopy(wxString&, int, int) const;
    // effectively copies data to string
  bool AssignCopy(size_t, const wxChar *);

  // append a (sub)string
  bool ConcatSelf(size_t nLen, const wxChar *src, size_t nMaxLen);
  bool ConcatSelf(size_t nLen, const wxChar *src)
    { return ConcatSelf(nLen, src, nLen); }

  // functions called before writing to the string: they copy it if there
  // are other references to our data (should be the only owner when writing)
  bool CopyBeforeWrite();
  bool AllocBeforeWrite(size_t);

    // compatibility with wxString
  bool Alloc(size_t nLen);
public:
  // standard types
  typedef wxChar value_type;
  typedef wxChar char_type;
  typedef size_t size_type;
  typedef value_type& reference;
  typedef const value_type& const_reference;
  typedef value_type* pointer;
  typedef const value_type* const_pointer;
  typedef value_type *iterator;
  typedef const value_type *const_iterator;

#define wxSTRING_REVERSE_ITERATOR(name, const_or_not)                         \
  class name                                                                  \
  {                                                                           \
  public:                                                                     \
      typedef wxChar value_type;                                              \
      typedef const_or_not value_type& reference;                             \
      typedef const_or_not value_type *pointer;                               \
      typedef const_or_not value_type *iterator_type;                         \
                                                                              \
      name(iterator_type i) : m_cur(i) { }                                    \
      name(const name& ri) : m_cur(ri.m_cur) { }                              \
                                                                              \
      iterator_type base() const { return m_cur; }                            \
                                                                              \
      reference operator*() const { return *(m_cur - 1); }                    \
                                                                              \
      name& operator++() { --m_cur; return *this; }                           \
      name operator++(int) { name tmp = *this; --m_cur; return tmp; }         \
      name& operator--() { ++m_cur; return *this; }                           \
      name operator--(int) { name tmp = *this; ++m_cur; return tmp; }         \
                                                                              \
      bool operator==(name ri) const { return m_cur == ri.m_cur; }            \
      bool operator!=(name ri) const { return !(*this == ri); }               \
                                                                              \
  private:                                                                    \
      iterator_type m_cur;                                                    \
  }

  wxSTRING_REVERSE_ITERATOR(const_reverse_iterator, const);

  #define wxSTRING_CONST
  wxSTRING_REVERSE_ITERATOR(reverse_iterator, wxSTRING_CONST);
  #undef wxSTRING_CONST

  #undef wxSTRING_REVERSE_ITERATOR


  // constructors and destructor
    // ctor for an empty string
  wxStringBase() { Init(); }
    // copy ctor
  wxStringBase(const wxStringBase& stringSrc)
  {
    wxASSERT_MSG( stringSrc.GetStringData()->IsValid(),
                  wxT("did you forget to call UngetWriteBuf()?") );

    if ( stringSrc.empty() ) {
      // nothing to do for an empty string
      Init();
    }
    else {
      m_pchData = stringSrc.m_pchData;            // share same data
      GetStringData()->Lock();                    // => one more copy
    }
  }
    // string containing nRepeat copies of ch
  wxStringBase(size_type nRepeat, wxChar ch);
    // ctor takes first nLength characters from C string
    // (default value of npos means take all the string)
  wxStringBase(const wxChar *psz)
      { InitWith(psz, 0, npos); }
  wxStringBase(const wxChar *psz, size_t nLength)
      { InitWith(psz, 0, nLength); }
  wxStringBase(const wxChar *psz,
               const wxMBConv& WXUNUSED(conv),
               size_t nLength = npos)
      { InitWith(psz, 0, nLength); }
    // take nLen chars starting at nPos
  wxStringBase(const wxStringBase& str, size_t nPos, size_t nLen)
  {
    wxASSERT_MSG( str.GetStringData()->IsValid(),
                  wxT("did you forget to call UngetWriteBuf()?") );
    Init();
    size_t strLen = str.length() - nPos; nLen = strLen < nLen ? strLen : nLen;
    InitWith(str.c_str(), nPos, nLen);
  }
    // take all characters from pStart to pEnd
  wxStringBase(const void *pStart, const void *pEnd);

    // dtor is not virtual, this class must not be inherited from!
  ~wxStringBase()
  {
#if defined(__VISUALC__) && (__VISUALC__ >= 1200)
      //RN - according to the above VC++ does indeed inline this,
      //even though it spits out two warnings
      #pragma warning (disable:4714)
#endif

      GetStringData()->Unlock();
  }

#if defined(__VISUALC__) && (__VISUALC__ >= 1200)
    //re-enable inlining warning
    #pragma warning (default:4714)
#endif
  // overloaded assignment
    // from another wxString
  wxStringBase& operator=(const wxStringBase& stringSrc);
    // from a character
  wxStringBase& operator=(wxChar ch);
    // from a C string
  wxStringBase& operator=(const wxChar *psz);

    // return the length of the string
  size_type length() const { return GetStringData()->nDataLength; }
    // return the length of the string
  size_type size() const { return length(); }
    // return the maximum size of the string
  size_type max_size() const { return npos; }
    // resize the string, filling the space with c if c != 0
  void resize(size_t nSize, wxChar ch = wxT('\0'));
    // delete the contents of the string
  void clear() { erase(0, npos); }
    // returns true if the string is empty
  bool empty() const { return length() == 0; }
    // inform string about planned change in size
  void reserve(size_t sz) { Alloc(sz); }
  size_type capacity() const { return GetStringData()->nAllocLength; }

  // lib.string.access
    // return the character at position n
  value_type at(size_type n) const
    { wxASSERT_VALID_INDEX( n ); return m_pchData[n]; }
    // returns the writable character at position n
  reference at(size_type n)
    { wxASSERT_VALID_INDEX( n ); CopyBeforeWrite(); return m_pchData[n]; }

  // lib.string.modifiers
    // append elements str[pos], ..., str[pos+n]
  wxStringBase& append(const wxStringBase& str, size_t pos, size_t n)
  {
    wxASSERT(pos <= str.length());
    ConcatSelf(n, str.c_str() + pos, str.length() - pos);
    return *this;
  }
    // append a string
  wxStringBase& append(const wxStringBase& str)
    { ConcatSelf(str.length(), str.c_str()); return *this; }
    // append first n (or all if n == npos) characters of sz
  wxStringBase& append(const wxChar *sz)
    { ConcatSelf(wxStrlen(sz), sz); return *this; }
  wxStringBase& append(const wxChar *sz, size_t n)
    { ConcatSelf(n, sz); return *this; }
    // append n copies of ch
  wxStringBase& append(size_t n, wxChar ch);
    // append from first to last
  wxStringBase& append(const_iterator first, const_iterator last)
    { ConcatSelf(last - first, first); return *this; }

    // same as `this_string = str'
  wxStringBase& assign(const wxStringBase& str)
    { return *this = str; }
    // same as ` = str[pos..pos + n]
  wxStringBase& assign(const wxStringBase& str, size_t pos, size_t n)
    { clear(); return append(str, pos, n); }
    // same as `= first n (or all if n == npos) characters of sz'
  wxStringBase& assign(const wxChar *sz)
    { clear(); return append(sz, wxStrlen(sz)); }
  wxStringBase& assign(const wxChar *sz, size_t n)
    { clear(); return append(sz, n); }
    // same as `= n copies of ch'
  wxStringBase& assign(size_t n, wxChar ch)
    { clear(); return append(n, ch); }
    // assign from first to last
  wxStringBase& assign(const_iterator first, const_iterator last)
    { clear(); return append(first, last); }

    // first valid index position
  const_iterator begin() const { return m_pchData; }
  iterator begin();
    // position one after the last valid one
  const_iterator end() const { return m_pchData + length(); }
  iterator end();

    // first element of the reversed string
  const_reverse_iterator rbegin() const { return const_reverse_iterator(end()); }
  reverse_iterator rbegin() { return reverse_iterator(end()); }
    // one beyond the end of the reversed string
  const_reverse_iterator rend() const { return const_reverse_iterator(begin()); }
  reverse_iterator rend() { return reverse_iterator(begin()); }

    // insert another string
  wxStringBase& insert(size_t nPos, const wxStringBase& str)
  {
    wxASSERT( str.GetStringData()->IsValid() );
    return insert(nPos, str.c_str(), str.length());
  }
    // insert n chars of str starting at nStart (in str)
  wxStringBase& insert(size_t nPos, const wxStringBase& str, size_t nStart, size_t n)
  {
    wxASSERT( str.GetStringData()->IsValid() );
    wxASSERT( nStart < str.length() );
    size_t strLen = str.length() - nStart;
    n = strLen < n ? strLen : n;
    return insert(nPos, str.c_str() + nStart, n);
  }
    // insert first n (or all if n == npos) characters of sz
  wxStringBase& insert(size_t nPos, const wxChar *sz, size_t n = npos);
    // insert n copies of ch
  wxStringBase& insert(size_t nPos, size_t n, wxChar ch)
    { return insert(nPos, wxStringBase(n, ch)); }
  iterator insert(iterator it, wxChar ch)
    { size_t idx = it - begin(); insert(idx, 1, ch); return begin() + idx; }
  void insert(iterator it, const_iterator first, const_iterator last)
    { insert(it - begin(), first, last - first); }
  void insert(iterator it, size_type n, wxChar ch)
    { insert(it - begin(), n, ch); }

    // delete characters from nStart to nStart + nLen
  wxStringBase& erase(size_type pos = 0, size_type n = npos);
  iterator erase(iterator first, iterator last)
  {
    size_t idx = first - begin();
    erase(idx, last - first);
    return begin() + idx;
  }
  iterator erase(iterator first);

  // explicit conversion to C string (use this with printf()!)
  const wxChar* c_str() const { return m_pchData; }
  const wxChar* data() const { return m_pchData; }

    // replaces the substring of length nLen starting at nStart
  wxStringBase& replace(size_t nStart, size_t nLen, const wxChar* sz);
    // replaces the substring of length nLen starting at nStart
  wxStringBase& replace(size_t nStart, size_t nLen, const wxStringBase& str)
    { return replace(nStart, nLen, str.c_str()); }
    // replaces the substring with nCount copies of ch
  wxStringBase& replace(size_t nStart, size_t nLen, size_t nCount, wxChar ch);
    // replaces a substring with another substring
  wxStringBase& replace(size_t nStart, size_t nLen,
                        const wxStringBase& str, size_t nStart2, size_t nLen2);
    // replaces the substring with first nCount chars of sz
  wxStringBase& replace(size_t nStart, size_t nLen,
                        const wxChar* sz, size_t nCount);
  wxStringBase& replace(iterator first, iterator last, const_pointer s)
    { return replace(first - begin(), last - first, s); }
  wxStringBase& replace(iterator first, iterator last, const_pointer s,
                        size_type n)
    { return replace(first - begin(), last - first, s, n); }
  wxStringBase& replace(iterator first, iterator last, const wxStringBase& s)
    { return replace(first - begin(), last - first, s); }
  wxStringBase& replace(iterator first, iterator last, size_type n, wxChar c)
    { return replace(first - begin(), last - first, n, c); }
  wxStringBase& replace(iterator first, iterator last,
                        const_iterator first1, const_iterator last1)
    { return replace(first - begin(), last - first, first1, last1 - first1); }

    // swap two strings
  void swap(wxStringBase& str);

    // All find() functions take the nStart argument which specifies the
    // position to start the search on, the default value is 0. All functions
    // return npos if there were no match.

    // find a substring
  size_t find(const wxStringBase& str, size_t nStart = 0) const;

    // find first n characters of sz
  size_t find(const wxChar* sz, size_t nStart = 0, size_t n = npos) const;

    // find the first occurence of character ch after nStart
  size_t find(wxChar ch, size_t nStart = 0) const;

    // rfind() family is exactly like find() but works right to left

    // as find, but from the end
  size_t rfind(const wxStringBase& str, size_t nStart = npos) const;

    // as find, but from the end
  size_t rfind(const wxChar* sz, size_t nStart = npos,
               size_t n = npos) const;
    // as find, but from the end
  size_t rfind(wxChar ch, size_t nStart = npos) const;

    // find first/last occurence of any character in the set

    // as strpbrk() but starts at nStart, returns npos if not found
  size_t find_first_of(const wxStringBase& str, size_t nStart = 0) const
    { return find_first_of(str.c_str(), nStart); }
    // same as above
  size_t find_first_of(const wxChar* sz, size_t nStart = 0) const;
  size_t find_first_of(const wxChar* sz, size_t nStart, size_t n) const;
    // same as find(char, size_t)
  size_t find_first_of(wxChar c, size_t nStart = 0) const
    { return find(c, nStart); }
    // find the last (starting from nStart) char from str in this string
  size_t find_last_of (const wxStringBase& str, size_t nStart = npos) const
    { return find_last_of(str.c_str(), nStart); }
    // same as above
  size_t find_last_of (const wxChar* sz, size_t nStart = npos) const;
  size_t find_last_of(const wxChar* sz, size_t nStart, size_t n) const;
    // same as above
  size_t find_last_of(wxChar c, size_t nStart = npos) const
    { return rfind(c, nStart); }

    // find first/last occurence of any character not in the set

    // as strspn() (starting from nStart), returns npos on failure
  size_t find_first_not_of(const wxStringBase& str, size_t nStart = 0) const
    { return find_first_not_of(str.c_str(), nStart); }
    // same as above
  size_t find_first_not_of(const wxChar* sz, size_t nStart = 0) const;
  size_t find_first_not_of(const wxChar* sz, size_t nStart, size_t n) const;
    // same as above
  size_t find_first_not_of(wxChar ch, size_t nStart = 0) const;
    //  as strcspn()
  size_t find_last_not_of(const wxStringBase& str, size_t nStart = npos) const
    { return find_last_not_of(str.c_str(), nStart); }
    // same as above
  size_t find_last_not_of(const wxChar* sz, size_t nStart = npos) const;
  size_t find_last_not_of(const wxChar* sz, size_t nStart, size_t n) const;
    // same as above
  size_t find_last_not_of(wxChar ch, size_t nStart = npos) const;

    // All compare functions return -1, 0 or 1 if the [sub]string is less,
    // equal or greater than the compare() argument.

    // comparison with another string
  int compare(const wxStringBase& str) const;
    // comparison with a substring
  int compare(size_t nStart, size_t nLen, const wxStringBase& str) const;
    // comparison of 2 substrings
  int compare(size_t nStart, size_t nLen,
              const wxStringBase& str, size_t nStart2, size_t nLen2) const;
    // comparison with a c string
  int compare(const wxChar* sz) const;
    // substring comparison with first nCount characters of sz
  int compare(size_t nStart, size_t nLen,
              const wxChar* sz, size_t nCount = npos) const;

  size_type copy(wxChar* s, size_type n, size_type pos = 0);

  // substring extraction
  wxStringBase substr(size_t nStart = 0, size_t nLen = npos) const;

      // string += string
  wxStringBase& operator+=(const wxStringBase& s) { return append(s); }
      // string += C string
  wxStringBase& operator+=(const wxChar *psz) { return append(psz); }
      // string += char
  wxStringBase& operator+=(wxChar ch) { return append(1, ch); }
};

#endif // !wxUSE_STL

// ----------------------------------------------------------------------------
// wxString: string class trying to be compatible with std::string, MFC
//           CString and wxWindows 1.x wxString all at once
// ---------------------------------------------------------------------------

class WXDLLIMPEXP_BASE wxString : public wxStringBase
{
#if !wxUSE_STL
friend class WXDLLIMPEXP_FWD_BASE wxArrayString;
#endif

  // NB: special care was taken in arranging the member functions in such order
  //     that all inline functions can be effectively inlined, verify that all
  //     performance critical functions are still inlined if you change order!
private:
  // if we hadn't made these operators private, it would be possible to
  // compile "wxString s; s = 17;" without any warnings as 17 is implicitly
  // converted to char in C and we do have operator=(char)
  //
  // NB: we don't need other versions (short/long and unsigned) as attempt
  //     to assign another numeric type to wxString will now result in
  //     ambiguity between operator=(char) and operator=(int)
  wxString& operator=(int);

  // these methods are not implemented - there is _no_ conversion from int to
  // string, you're doing something wrong if the compiler wants to call it!
  //
  // try `s << i' or `s.Printf("%d", i)' instead
  wxString(int);

public:
  // constructors and destructor
    // ctor for an empty string
  wxString() : wxStringBase() { }
    // copy ctor
  wxString(const wxStringBase& stringSrc) : wxStringBase(stringSrc) { }
  wxString(const wxString& stringSrc) : wxStringBase(stringSrc) { }
    // string containing nRepeat copies of ch
  wxString(wxChar ch, size_t nRepeat = 1)
      : wxStringBase(nRepeat, ch) { }
  wxString(size_t nRepeat, wxChar ch)
      : wxStringBase(nRepeat, ch) { }
    // ctor takes first nLength characters from C string
    // (default value of npos means take all the string)
  wxString(const wxChar *psz)
      : wxStringBase(psz ? psz : wxT("")) { }
  wxString(const wxChar *psz, size_t nLength)
      : wxStringBase(psz, nLength) { }
  wxString(const wxChar *psz,
           const wxMBConv& WXUNUSED(conv),
           size_t nLength = npos)
      : wxStringBase(psz, nLength == npos ? wxStrlen(psz) : nLength) { }

  // even if we're not built with wxUSE_STL == 1 it is very convenient to allow
  // implicit conversions from std::string to wxString as this allows to use
  // the same strings in non-GUI and GUI code, however we don't want to
  // unconditionally add this ctor as it would make wx lib dependent on
  // libstdc++ on some Linux versions which is bad, so instead we ask the
  // client code to define this wxUSE_STD_STRING symbol if they need it
#if wxUSE_STD_STRING
  wxString(const wxStdString& s)
      : wxStringBase(s.c_str()) { }
#endif // wxUSE_STD_STRING

#if wxUSE_UNICODE
    // from multibyte string
  wxString(const char *psz, const wxMBConv& conv, size_t nLength = npos);
    // from wxWCharBuffer (i.e. return from wxGetString)
  wxString(const wxWCharBuffer& psz) : wxStringBase(psz.data()) { }
#else // ANSI
    // from C string (for compilers using unsigned char)
  wxString(const unsigned char* psz)
      : wxStringBase((const char*)psz) { }
    // from part of C string (for compilers using unsigned char)
  wxString(const unsigned char* psz, size_t nLength)
      : wxStringBase((const char*)psz, nLength) { }

#if wxUSE_WCHAR_T
    // from wide (Unicode) string
  wxString(const wchar_t *pwz,
           const wxMBConv& conv = wxConvLibc,
           size_t nLength = npos);
#endif // !wxUSE_WCHAR_T

    // from wxCharBuffer
  wxString(const wxCharBuffer& psz)
      : wxStringBase(psz) { }
#endif // Unicode/ANSI

  // generic attributes & operations
    // as standard strlen()
  size_t Len() const { return length(); }
    // string contains any characters?
  bool IsEmpty() const { return empty(); }
    // empty string is "false", so !str will return true
  bool operator!() const { return empty(); }
    // truncate the string to given length
  wxString& Truncate(size_t uiLen);
    // empty string contents
  void Empty()
  {
    Truncate(0);

    wxASSERT_MSG( empty(), wxT("string not empty after call to Empty()?") );
  }
    // empty the string and free memory
  void Clear()
  {
    wxString tmp(wxEmptyString);
    swap(tmp);
  }

  // contents test
    // Is an ascii value
  bool IsAscii() const;
    // Is a number
  bool IsNumber() const;
    // Is a word
  bool IsWord() const;

  // data access (all indexes are 0 based)
    // read access
    wxChar  GetChar(size_t n) const
      { return at(n); }
    // read/write access
    wxChar& GetWritableChar(size_t n)
      { return at(n); }
    // write access
    void  SetChar(size_t n, wxChar ch)
      { at(n) = ch; }

    // get last character
    wxChar  Last() const
      {
          wxASSERT_MSG( !empty(), wxT("wxString: index out of bounds") );

          return at(length() - 1);
      }

    // get writable last character
    wxChar& Last()
      {
          wxASSERT_MSG( !empty(), wxT("wxString: index out of bounds") );
          return at(length() - 1);
      }

    /*
       Note that we we must define all of the overloads below to avoid
       ambiguity when using str[0]. Also note that for a conforming compiler we
       don't need const version of operatorp[] at all as indexed access to
       const string is provided by implicit conversion to "const wxChar *"
       below and defining them would only result in ambiguities, but some other
       compilers refuse to compile "str[0]" without them.
     */

#if defined(__BORLANDC__) || defined(__WATCOMC__) || defined(__MWERKS__)
    wxChar operator[](int n) const
      { return wxStringBase::at(n); }
    wxChar operator[](size_type n) const
      { return wxStringBase::at(n); }
#ifndef wxSIZE_T_IS_UINT
    wxChar operator[](unsigned int n) const
      { return wxStringBase::at(n); }
#endif // size_t != unsigned int
#endif // broken compiler


    // operator versions of GetWriteableChar()
    wxChar& operator[](int n)
      { return wxStringBase::at(n); }
    wxChar& operator[](size_type n)
      { return wxStringBase::at(n); }
#ifndef wxSIZE_T_IS_UINT
    wxChar& operator[](unsigned int n)
      { return wxStringBase::at(n); }
#endif // size_t != unsigned int

    // implicit conversion to C string
    operator const wxChar*() const { return c_str(); }

    // identical to c_str(), for wxWin 1.6x compatibility
    const wxChar* wx_str()  const { return c_str(); }
    // identical to c_str(), for MFC compatibility
    const wxChar* GetData() const { return c_str(); }

#if wxABI_VERSION >= 20804
    // conversion to *non-const* multibyte or widestring buffer; modifying
    // returned buffer won't affect the string, these methods are only useful
    // for passing values to const-incorrect functions
    wxWritableCharBuffer char_str(const wxMBConv& conv = wxConvLibc) const
      { return mb_str(conv); }
#if wxUSE_WCHAR_T
    wxWritableWCharBuffer wchar_str() const { return wc_str(wxConvLibc); }
#endif
#endif // wxABI_VERSION >= 20804

    // conversion to/from plain (i.e. 7 bit) ASCII: this is useful for
    // converting numbers or strings which are certain not to contain special
    // chars (typically system functions, X atoms, environment variables etc.)
    //
    // the behaviour of these functions with the strings containing anything
    // else than 7 bit ASCII characters is undefined, use at your own risk.
#if wxUSE_UNICODE
    static wxString FromAscii(const char *ascii);  // string
    static wxString FromAscii(const char ascii);   // char
    const wxCharBuffer ToAscii() const;
#else // ANSI
    static wxString FromAscii(const char *ascii) { return wxString( ascii ); }
    static wxString FromAscii(const char ascii) { return wxString( ascii ); }
    const char *ToAscii() const { return c_str(); }
#endif // Unicode/!Unicode

#if wxABI_VERSION >= 20804
    // conversion to/from UTF-8:
#if wxUSE_UNICODE
    static wxString FromUTF8(const char *utf8)
      { return wxString(utf8, wxConvUTF8); }
    static wxString FromUTF8(const char *utf8, size_t len)
      { return wxString(utf8, wxConvUTF8, len); }
    const wxCharBuffer utf8_str() const { return mb_str(wxConvUTF8); }
    const wxCharBuffer ToUTF8() const { return utf8_str(); }
#elif wxUSE_WCHAR_T // ANSI
    static wxString FromUTF8(const char *utf8)
      { return wxString(wxConvUTF8.cMB2WC(utf8)); }
    static wxString FromUTF8(const char *utf8, size_t len)
    {
      size_t wlen;
      wxWCharBuffer buf(wxConvUTF8.cMB2WC(utf8, len == npos ? wxNO_LEN : len, &wlen));
      return wxString(buf.data(), wxConvLibc, wlen);
    }
    const wxCharBuffer utf8_str() const
      { return wxConvUTF8.cWC2MB(wc_str(wxConvLibc)); }
    const wxCharBuffer ToUTF8() const { return utf8_str(); }
#endif // Unicode/ANSI
#endif // wxABI_VERSION >= 20804

#if wxABI_VERSION >= 20804
    // functions for storing binary data in wxString:
#if wxUSE_UNICODE
    static wxString From8BitData(const char *data, size_t len)
      { return wxString(data, wxConvISO8859_1, len); }
    // version for NUL-terminated data:
    static wxString From8BitData(const char *data)
      { return wxString(data, wxConvISO8859_1); }
    const wxCharBuffer To8BitData() const { return mb_str(wxConvISO8859_1); }
#else // ANSI
    static wxString From8BitData(const char *data, size_t len)
      { return wxString(data, len); }
    // version for NUL-terminated data:
    static wxString From8BitData(const char *data)
      { return wxString(data); }
    const char *To8BitData() const { return c_str(); }
#endif // Unicode/ANSI
#endif // wxABI_VERSION >= 20804

    // conversions with (possible) format conversions: have to return a
    // buffer with temporary data
    //
    // the functions defined (in either Unicode or ANSI) mode are mb_str() to
    // return an ANSI (multibyte) string, wc_str() to return a wide string and
    // fn_str() to return a string which should be used with the OS APIs
    // accepting the file names. The return value is always the same, but the
    // type differs because a function may either return pointer to the buffer
    // directly or have to use intermediate buffer for translation.
#if wxUSE_UNICODE
    const wxCharBuffer mb_str(const wxMBConv& conv = wxConvLibc) const;

    const wxWX2MBbuf mbc_str() const { return mb_str(*wxConvCurrent); }

    const wxChar* wc_str() const { return c_str(); }

    // for compatibility with !wxUSE_UNICODE version
    const wxChar* wc_str(const wxMBConv& WXUNUSED(conv)) const { return c_str(); }

#if wxMBFILES
    const wxCharBuffer fn_str() const { return mb_str(wxConvFile); }
#else // !wxMBFILES
    const wxChar* fn_str() const { return c_str(); }
#endif // wxMBFILES/!wxMBFILES
#else // ANSI
    const wxChar* mb_str() const { return c_str(); }

    // for compatibility with wxUSE_UNICODE version
    const wxChar* mb_str(const wxMBConv& WXUNUSED(conv)) const { return c_str(); }

    const wxWX2MBbuf mbc_str() const { return mb_str(); }

#if wxUSE_WCHAR_T
    const wxWCharBuffer wc_str(const wxMBConv& conv) const;
#endif // wxUSE_WCHAR_T
#ifdef __WXOSX__
    const wxCharBuffer fn_str() const { return wxConvFile.cWC2WX( wc_str( wxConvLocal ) ); }
#else
    const wxChar* fn_str() const { return c_str(); }
#endif
#endif // Unicode/ANSI

  // overloaded assignment
    // from another wxString
  wxString& operator=(const wxStringBase& stringSrc)
    { return (wxString&)wxStringBase::operator=(stringSrc); }
    // from a character
  wxString& operator=(wxChar ch)
    { return (wxString&)wxStringBase::operator=(ch); }
    // from a C string - STL probably will crash on NULL,
    // so we need to compensate in that case
#if wxUSE_STL
  wxString& operator=(const wxChar *psz)
    { if(psz) wxStringBase::operator=(psz); else Clear(); return *this; }
#else
  wxString& operator=(const wxChar *psz)
    { return (wxString&)wxStringBase::operator=(psz); }
#endif

#if wxUSE_UNICODE
    // from wxWCharBuffer
  wxString& operator=(const wxWCharBuffer& psz)
    { (void) operator=((const wchar_t *)psz); return *this; }
#else // ANSI
    // from another kind of C string
  wxString& operator=(const unsigned char* psz);
#if wxUSE_WCHAR_T
    // from a wide string
  wxString& operator=(const wchar_t *pwz);
#endif
    // from wxCharBuffer
  wxString& operator=(const wxCharBuffer& psz)
    { (void) operator=((const char *)psz); return *this; }
#endif // Unicode/ANSI

  // string concatenation
    // in place concatenation
    /*
        Concatenate and return the result. Note that the left to right
        associativity of << allows to write things like "str << str1 << str2
        << ..." (unlike with +=)
     */
      // string += string
  wxString& operator<<(const wxString& s)
  {
#if !wxUSE_STL
    wxASSERT_MSG( s.GetStringData()->IsValid(),
                  wxT("did you forget to call UngetWriteBuf()?") );
#endif

    append(s);
    return *this;
  }
      // string += C string
  wxString& operator<<(const wxChar *psz)
    { append(psz); return *this; }
      // string += char
  wxString& operator<<(wxChar ch) { append(1, ch); return *this; }

      // string += buffer (i.e. from wxGetString)
#if wxUSE_UNICODE
  wxString& operator<<(const wxWCharBuffer& s)
    { (void)operator<<((const wchar_t *)s); return *this; }
  void operator+=(const wxWCharBuffer& s)
    { (void)operator<<((const wchar_t *)s); }
#else // !wxUSE_UNICODE
  wxString& operator<<(const wxCharBuffer& s)
    { (void)operator<<((const char *)s); return *this; }
  void operator+=(const wxCharBuffer& s)
    { (void)operator<<((const char *)s); }
#endif // wxUSE_UNICODE/!wxUSE_UNICODE

    // string += C string
  wxString& Append(const wxString& s)
    {
        // test for empty() to share the string if possible
        if ( empty() )
            *this = s;
        else
            append(s);
        return *this;
    }
  wxString& Append(const wxChar* psz)
    { append(psz); return *this; }
    // append count copies of given character
  wxString& Append(wxChar ch, size_t count = 1u)
    { append(count, ch); return *this; }
  wxString& Append(const wxChar* psz, size_t nLen)
    { append(psz, nLen); return *this; }

    // prepend a string, return the string itself
  wxString& Prepend(const wxString& str)
    { *this = str + *this; return *this; }

    // non-destructive concatenation
      // two strings
  friend wxString WXDLLIMPEXP_BASE operator+(const wxString& string1,
                                             const wxString& string2);
      // string with a single char
  friend wxString WXDLLIMPEXP_BASE operator+(const wxString& string, wxChar ch);
      // char with a string
  friend wxString WXDLLIMPEXP_BASE operator+(wxChar ch, const wxString& string);
      // string with C string
  friend wxString WXDLLIMPEXP_BASE operator+(const wxString& string,
                                             const wxChar *psz);
      // C string with string
  friend wxString WXDLLIMPEXP_BASE operator+(const wxChar *psz,
                                             const wxString& string);

  // stream-like functions
      // insert an int into string
  wxString& operator<<(int i)
    { return (*this) << Format(wxT("%d"), i); }
      // insert an unsigned int into string
  wxString& operator<<(unsigned int ui)
    { return (*this) << Format(wxT("%u"), ui); }
      // insert a long into string
  wxString& operator<<(long l)
    { return (*this) << Format(wxT("%ld"), l); }
      // insert an unsigned long into string
  wxString& operator<<(unsigned long ul)
    { return (*this) << Format(wxT("%lu"), ul); }
#if defined wxLongLong_t && !defined wxLongLongIsLong
      // insert a long long if they exist and aren't longs
  wxString& operator<<(wxLongLong_t ll)
    {
      const wxChar *fmt = wxT("%") wxLongLongFmtSpec wxT("d");
      return (*this) << Format(fmt, ll);
    }
      // insert an unsigned long long
  wxString& operator<<(wxULongLong_t ull)
    {
      const wxChar *fmt = wxT("%") wxLongLongFmtSpec wxT("u");
      return (*this) << Format(fmt , ull);
    }
#endif
      // insert a float into string
  wxString& operator<<(float f)
    { return (*this) << Format(wxT("%f"), f); }
      // insert a double into string
  wxString& operator<<(double d)
    { return (*this) << Format(wxT("%g"), d); }

  // string comparison
    // case-sensitive comparison (returns a value < 0, = 0 or > 0)
  int Cmp(const wxChar *psz) const;
  int Cmp(const wxString& s) const;
    // same as Cmp() but not case-sensitive
  int CmpNoCase(const wxChar *psz) const;
  int CmpNoCase(const wxString& s) const;
    // test for the string equality, either considering case or not
    // (if compareWithCase then the case matters)
  bool IsSameAs(const wxChar *psz, bool compareWithCase = true) const
    { return (compareWithCase ? Cmp(psz) : CmpNoCase(psz)) == 0; }
    // comparison with a single character: returns true if equal
  bool IsSameAs(wxChar c, bool compareWithCase = true) const
    {
      return (length() == 1) && (compareWithCase ? GetChar(0u) == c
                              : wxToupper(GetChar(0u)) == wxToupper(c));
    }

  // simple sub-string extraction
      // return substring starting at nFirst of length nCount (or till the end
      // if nCount = default value)
  wxString Mid(size_t nFirst, size_t nCount = npos) const;

      // operator version of Mid()
  wxString  operator()(size_t start, size_t len) const
    { return Mid(start, len); }

      // check if the string starts with the given prefix and return the rest
      // of the string in the provided pointer if it is not NULL; otherwise
      // return false
  bool StartsWith(const wxChar *prefix, wxString *rest = NULL) const;
      // check if the string ends with the given suffix and return the
      // beginning of the string before the suffix in the provided pointer if
      // it is not NULL; otherwise return false
  bool EndsWith(const wxChar *suffix, wxString *rest = NULL) const;

      // get first nCount characters
  wxString Left(size_t nCount) const;
      // get last nCount characters
  wxString Right(size_t nCount) const;
      // get all characters before the first occurance of ch
      // (returns the whole string if ch not found)
  wxString BeforeFirst(wxChar ch) const;
      // get all characters before the last occurence of ch
      // (returns empty string if ch not found)
  wxString BeforeLast(wxChar ch) const;
      // get all characters after the first occurence of ch
      // (returns empty string if ch not found)
  wxString AfterFirst(wxChar ch) const;
      // get all characters after the last occurence of ch
      // (returns the whole string if ch not found)
  wxString AfterLast(wxChar ch) const;

    // for compatibility only, use more explicitly named functions above
  wxString Before(wxChar ch) const { return BeforeLast(ch); }
  wxString After(wxChar ch) const { return AfterFirst(ch); }

  // case conversion
      // convert to upper case in place, return the string itself
  wxString& MakeUpper();
      // convert to upper case, return the copy of the string
      // Here's something to remember: BC++ doesn't like returns in inlines.
  wxString Upper() const ;
      // convert to lower case in place, return the string itself
  wxString& MakeLower();
      // convert to lower case, return the copy of the string
  wxString Lower() const ;

  // trimming/padding whitespace (either side) and truncating
      // remove spaces from left or from right (default) side
  wxString& Trim(bool bFromRight = true);
      // add nCount copies chPad in the beginning or at the end (default)
  wxString& Pad(size_t nCount, wxChar chPad = wxT(' '), bool bFromRight = true);

  // searching and replacing
      // searching (return starting index, or -1 if not found)
  int Find(wxChar ch, bool bFromEnd = false) const;   // like strchr/strrchr
      // searching (return starting index, or -1 if not found)
  int Find(const wxChar *pszSub) const;               // like strstr
      // replace first (or all of bReplaceAll) occurences of substring with
      // another string, returns the number of replacements made
  size_t Replace(const wxChar *szOld,
                 const wxChar *szNew,
                 bool bReplaceAll = true);

    // check if the string contents matches a mask containing '*' and '?'
  bool Matches(const wxChar *szMask) const;

    // conversion to numbers: all functions return true only if the whole
    // string is a number and put the value of this number into the pointer
    // provided, the base is the numeric base in which the conversion should be
    // done and must be comprised between 2 and 36 or be 0 in which case the
    // standard C rules apply (leading '0' => octal, "0x" => hex)
        // convert to a signed integer
    bool ToLong(long *val, int base = 10) const;
        // convert to an unsigned integer
    bool ToULong(unsigned long *val, int base = 10) const;
        // convert to wxLongLong
#if defined(wxLongLong_t)
    bool ToLongLong(wxLongLong_t *val, int base = 10) const;
        // convert to wxULongLong
    bool ToULongLong(wxULongLong_t *val, int base = 10) const;
#endif // wxLongLong_t
        // convert to a double
    bool ToDouble(double *val) const;



  // formatted input/output
    // as sprintf(), returns the number of characters written or < 0 on error
    // (take 'this' into account in attribute parameter count)
  int Printf(const wxChar *pszFormat, ...) ATTRIBUTE_PRINTF_2;
    // as vprintf(), returns the number of characters written or < 0 on error
  int PrintfV(const wxChar* pszFormat, va_list argptr);

    // returns the string containing the result of Printf() to it
  static wxString Format(const wxChar *pszFormat, ...) ATTRIBUTE_PRINTF_1;
    // the same as above, but takes a va_list
  static wxString FormatV(const wxChar *pszFormat, va_list argptr);

  // raw access to string memory
    // ensure that string has space for at least nLen characters
    // only works if the data of this string is not shared
  bool Alloc(size_t nLen) { reserve(nLen); /*return capacity() >= nLen;*/ return true; }
    // minimize the string's memory
    // only works if the data of this string is not shared
  bool Shrink();
#if !wxUSE_STL
    // get writable buffer of at least nLen bytes. Unget() *must* be called
    // a.s.a.p. to put string back in a reasonable state!
  wxChar *GetWriteBuf(size_t nLen);
    // call this immediately after GetWriteBuf() has been used
  void UngetWriteBuf();
  void UngetWriteBuf(size_t nLen);
#endif

  // wxWidgets version 1 compatibility functions

  // use Mid()
  wxString SubString(size_t from, size_t to) const
      { return Mid(from, (to - from + 1)); }
    // values for second parameter of CompareTo function
  enum caseCompare {exact, ignoreCase};
    // values for first parameter of Strip function
  enum stripType {leading = 0x1, trailing = 0x2, both = 0x3};

  // use Printf()
  // (take 'this' into account in attribute parameter count)
  int sprintf(const wxChar *pszFormat, ...) ATTRIBUTE_PRINTF_2;

    // use Cmp()
  inline int CompareTo(const wxChar* psz, caseCompare cmp = exact) const
    { return cmp == exact ? Cmp(psz) : CmpNoCase(psz); }

    // use Len
  size_t Length() const { return length(); }
    // Count the number of characters
  int Freq(wxChar ch) const;
    // use MakeLower
  void LowerCase() { MakeLower(); }
    // use MakeUpper
  void UpperCase() { MakeUpper(); }
    // use Trim except that it doesn't change this string
  wxString Strip(stripType w = trailing) const;

    // use Find (more general variants not yet supported)
  size_t Index(const wxChar* psz) const { return Find(psz); }
  size_t Index(wxChar ch)         const { return Find(ch);  }
    // use Truncate
  wxString& Remove(size_t pos) { return Truncate(pos); }
  wxString& RemoveLast(size_t n = 1) { return Truncate(length() - n); }

  wxString& Remove(size_t nStart, size_t nLen)
      { return (wxString&)erase( nStart, nLen ); }

    // use Find()
  int First( const wxChar ch ) const { return Find(ch); }
  int First( const wxChar* psz ) const { return Find(psz); }
  int First( const wxString &str ) const { return Find(str); }
  int Last( const wxChar ch ) const { return Find(ch, true); }
  bool Contains(const wxString& str) const { return Find(str) != wxNOT_FOUND; }

    // use empty()
  bool IsNull() const { return empty(); }

  // std::string compatibility functions

    // take nLen chars starting at nPos
  wxString(const wxString& str, size_t nPos, size_t nLen)
      : wxStringBase(str, nPos, nLen) { }
    // take all characters from pStart to pEnd
  wxString(const void *pStart, const void *pEnd)
      : wxStringBase((const wxChar*)pStart, (const wxChar*)pEnd) { }
#if wxUSE_STL
  wxString(const_iterator first, const_iterator last)
      : wxStringBase(first, last) { }
#endif

  // lib.string.modifiers
    // append elements str[pos], ..., str[pos+n]
  wxString& append(const wxString& str, size_t pos, size_t n)
    { return (wxString&)wxStringBase::append(str, pos, n); }
    // append a string
  wxString& append(const wxString& str)
    { return (wxString&)wxStringBase::append(str); }
    // append first n (or all if n == npos) characters of sz
  wxString& append(const wxChar *sz)
    { return (wxString&)wxStringBase::append(sz); }
  wxString& append(const wxChar *sz, size_t n)
    { return (wxString&)wxStringBase::append(sz, n); }
    // append n copies of ch
  wxString& append(size_t n, wxChar ch)
    { return (wxString&)wxStringBase::append(n, ch); }
    // append from first to last
  wxString& append(const_iterator first, const_iterator last)
    { return (wxString&)wxStringBase::append(first, last); }

    // same as `this_string = str'
  wxString& assign(const wxString& str)
    { return (wxString&)wxStringBase::assign(str); }
    // same as ` = str[pos..pos + n]
  wxString& assign(const wxString& str, size_t pos, size_t n)
    { return (wxString&)wxStringBase::assign(str, pos, n); }
    // same as `= first n (or all if n == npos) characters of sz'
  wxString& assign(const wxChar *sz)
    { return (wxString&)wxStringBase::assign(sz); }
  wxString& assign(const wxChar *sz, size_t n)
    { return (wxString&)wxStringBase::assign(sz, n); }
    // same as `= n copies of ch'
  wxString& assign(size_t n, wxChar ch)
    { return (wxString&)wxStringBase::assign(n, ch); }
    // assign from first to last
  wxString& assign(const_iterator first, const_iterator last)
    { return (wxString&)wxStringBase::assign(first, last); }

    // string comparison
#if !defined(HAVE_STD_STRING_COMPARE)
  int compare(const wxStringBase& str) const;
    // comparison with a substring
  int compare(size_t nStart, size_t nLen, const wxStringBase& str) const;
    // comparison of 2 substrings
  int compare(size_t nStart, size_t nLen,
              const wxStringBase& str, size_t nStart2, size_t nLen2) const;
    // just like strcmp()
  int compare(const wxChar* sz) const;
    // substring comparison with first nCount characters of sz
  int compare(size_t nStart, size_t nLen,
              const wxChar* sz, size_t nCount = npos) const;
#endif // !defined HAVE_STD_STRING_COMPARE

    // insert another string
  wxString& insert(size_t nPos, const wxString& str)
    { return (wxString&)wxStringBase::insert(nPos, str); }
    // insert n chars of str starting at nStart (in str)
  wxString& insert(size_t nPos, const wxString& str, size_t nStart, size_t n)
    { return (wxString&)wxStringBase::insert(nPos, str, nStart, n); }
    // insert first n (or all if n == npos) characters of sz
  wxString& insert(size_t nPos, const wxChar *sz)
    { return (wxString&)wxStringBase::insert(nPos, sz); }
  wxString& insert(size_t nPos, const wxChar *sz, size_t n)
    { return (wxString&)wxStringBase::insert(nPos, sz, n); }
    // insert n copies of ch
  wxString& insert(size_t nPos, size_t n, wxChar ch)
    { return (wxString&)wxStringBase::insert(nPos, n, ch); }
  iterator insert(iterator it, wxChar ch)
    { return wxStringBase::insert(it, ch); }
  void insert(iterator it, const_iterator first, const_iterator last)
    { wxStringBase::insert(it, first, last); }
  void insert(iterator it, size_type n, wxChar ch)
    { wxStringBase::insert(it, n, ch); }

    // delete characters from nStart to nStart + nLen
  wxString& erase(size_type pos = 0, size_type n = npos)
    { return (wxString&)wxStringBase::erase(pos, n); }
  iterator erase(iterator first, iterator last)
    { return wxStringBase::erase(first, last); }
  iterator erase(iterator first)
    { return wxStringBase::erase(first); }

#ifdef wxSTRING_BASE_HASNT_CLEAR
  void clear() { erase(); }
#endif

    // replaces the substring of length nLen starting at nStart
  wxString& replace(size_t nStart, size_t nLen, const wxChar* sz)
    { return (wxString&)wxStringBase::replace(nStart, nLen, sz); }
    // replaces the substring of length nLen starting at nStart
  wxString& replace(size_t nStart, size_t nLen, const wxString& str)
    { return (wxString&)wxStringBase::replace(nStart, nLen, str); }
    // replaces the substring with nCount copies of ch
  wxString& replace(size_t nStart, size_t nLen, size_t nCount, wxChar ch)
    { return (wxString&)wxStringBase::replace(nStart, nLen, nCount, ch); }
    // replaces a substring with another substring
  wxString& replace(size_t nStart, size_t nLen,
                    const wxString& str, size_t nStart2, size_t nLen2)
    { return (wxString&)wxStringBase::replace(nStart, nLen, str,
                                              nStart2, nLen2); }
     // replaces the substring with first nCount chars of sz
  wxString& replace(size_t nStart, size_t nLen,
                    const wxChar* sz, size_t nCount)
    { return (wxString&)wxStringBase::replace(nStart, nLen, sz, nCount); }
  wxString& replace(iterator first, iterator last, const_pointer s)
    { return (wxString&)wxStringBase::replace(first, last, s); }
  wxString& replace(iterator first, iterator last, const_pointer s,
                    size_type n)
    { return (wxString&)wxStringBase::replace(first, last, s, n); }
  wxString& replace(iterator first, iterator last, const wxString& s)
    { return (wxString&)wxStringBase::replace(first, last, s); }
  wxString& replace(iterator first, iterator last, size_type n, wxChar c)
    { return (wxString&)wxStringBase::replace(first, last, n, c); }
  wxString& replace(iterator first, iterator last,
                    const_iterator first1, const_iterator last1)
    { return (wxString&)wxStringBase::replace(first, last, first1, last1); }

      // string += string
  wxString& operator+=(const wxString& s)
    { return (wxString&)wxStringBase::operator+=(s); }
      // string += C string
  wxString& operator+=(const wxChar *psz)
    { return (wxString&)wxStringBase::operator+=(psz); }
      // string += char
  wxString& operator+=(wxChar ch)
    { return (wxString&)wxStringBase::operator+=(ch); }
};

// notice that even though for many compilers the friend declarations above are
// enough, from the point of view of C++ standard we must have the declarations
// here as friend ones are not injected in the enclosing namespace and without
// them the code fails to compile with conforming compilers such as xlC or g++4
wxString WXDLLIMPEXP_BASE operator+(const wxString& string1,  const wxString& string2);
wxString WXDLLIMPEXP_BASE operator+(const wxString& string, wxChar ch);
wxString WXDLLIMPEXP_BASE operator+(wxChar ch, const wxString& string);
wxString WXDLLIMPEXP_BASE operator+(const wxString& string, const wxChar *psz);
wxString WXDLLIMPEXP_BASE operator+(const wxChar *psz, const wxString& string);


// define wxArrayString, for compatibility
#if WXWIN_COMPATIBILITY_2_4 && !wxUSE_STL
    #include "wx/arrstr.h"
#endif

#if wxUSE_STL
    // return an empty wxString (not very useful with wxUSE_STL == 1)
    inline const wxString wxGetEmptyString() { return wxString(); }
#else // !wxUSE_STL
    // return an empty wxString (more efficient than wxString() here)
    inline const wxString& wxGetEmptyString()
    {
        return *(wxString *)&wxEmptyString;
    }
#endif // wxUSE_STL/!wxUSE_STL

// ----------------------------------------------------------------------------
// wxStringBuffer: a tiny class allowing to get a writable pointer into string
// ----------------------------------------------------------------------------

#if wxUSE_STL

class WXDLLIMPEXP_BASE wxStringBuffer
{
public:
    wxStringBuffer(wxString& str, size_t lenWanted = 1024)
        : m_str(str), m_buf(lenWanted)
        { }

    ~wxStringBuffer() { m_str.assign(m_buf.data(), wxStrlen(m_buf.data())); }

    operator wxChar*() { return m_buf.data(); }

private:
    wxString& m_str;
#if wxUSE_UNICODE
    wxWCharBuffer m_buf;
#else
    wxCharBuffer m_buf;
#endif

    DECLARE_NO_COPY_CLASS(wxStringBuffer)
};

class WXDLLIMPEXP_BASE wxStringBufferLength
{
public:
    wxStringBufferLength(wxString& str, size_t lenWanted = 1024)
        : m_str(str), m_buf(lenWanted), m_len(0), m_lenSet(false)
        { }

    ~wxStringBufferLength()
    {
        wxASSERT(m_lenSet);
        m_str.assign(m_buf.data(), m_len);
    }

    operator wxChar*() { return m_buf.data(); }
    void SetLength(size_t length) { m_len = length; m_lenSet = true; }

private:
    wxString& m_str;
#if wxUSE_UNICODE
    wxWCharBuffer m_buf;
#else
    wxCharBuffer  m_buf;
#endif
    size_t        m_len;
    bool          m_lenSet;

    DECLARE_NO_COPY_CLASS(wxStringBufferLength)
};

#else // if !wxUSE_STL

class WXDLLIMPEXP_BASE wxStringBuffer
{
public:
    wxStringBuffer(wxString& str, size_t lenWanted = 1024)
        : m_str(str), m_buf(NULL)
        { m_buf = m_str.GetWriteBuf(lenWanted); }

    ~wxStringBuffer() { m_str.UngetWriteBuf(); }

    operator wxChar*() const { return m_buf; }

private:
    wxString& m_str;
    wxChar   *m_buf;

    DECLARE_NO_COPY_CLASS(wxStringBuffer)
};

class WXDLLIMPEXP_BASE wxStringBufferLength
{
public:
    wxStringBufferLength(wxString& str, size_t lenWanted = 1024)
        : m_str(str), m_buf(NULL), m_len(0), m_lenSet(false)
    {
        m_buf = m_str.GetWriteBuf(lenWanted);
        wxASSERT(m_buf != NULL);
    }

    ~wxStringBufferLength()
    {
        wxASSERT(m_lenSet);
        m_str.UngetWriteBuf(m_len);
    }

    operator wxChar*() const { return m_buf; }
    void SetLength(size_t length) { m_len = length; m_lenSet = true; }

private:
    wxString& m_str;
    wxChar   *m_buf;
    size_t    m_len;
    bool      m_lenSet;

    DECLARE_NO_COPY_CLASS(wxStringBufferLength)
};

#endif // !wxUSE_STL

// ---------------------------------------------------------------------------
// wxString comparison functions: operator versions are always case sensitive
// ---------------------------------------------------------------------------

// note that when wxUSE_STL == 1 the comparison operators taking std::string
// are used and defining them also for wxString would only result in
// compilation ambiguities when comparing std::string and wxString
#if !wxUSE_STL

inline bool operator==(const wxString& s1, const wxString& s2)
    { return (s1.Len() == s2.Len()) && (s1.Cmp(s2) == 0); }
inline bool operator==(const wxString& s1, const wxChar  * s2)
    { return s1.Cmp(s2) == 0; }
inline bool operator==(const wxChar  * s1, const wxString& s2)
    { return s2.Cmp(s1) == 0; }
inline bool operator!=(const wxString& s1, const wxString& s2)
    { return (s1.Len() != s2.Len()) || (s1.Cmp(s2) != 0); }
inline bool operator!=(const wxString& s1, const wxChar  * s2)
    { return s1.Cmp(s2) != 0; }
inline bool operator!=(const wxChar  * s1, const wxString& s2)
    { return s2.Cmp(s1) != 0; }
inline bool operator< (const wxString& s1, const wxString& s2)
    { return s1.Cmp(s2) < 0; }
inline bool operator< (const wxString& s1, const wxChar  * s2)
    { return s1.Cmp(s2) <  0; }
inline bool operator< (const wxChar  * s1, const wxString& s2)
    { return s2.Cmp(s1) >  0; }
inline bool operator> (const wxString& s1, const wxString& s2)
    { return s1.Cmp(s2) >  0; }
inline bool operator> (const wxString& s1, const wxChar  * s2)
    { return s1.Cmp(s2) >  0; }
inline bool operator> (const wxChar  * s1, const wxString& s2)
    { return s2.Cmp(s1) <  0; }
inline bool operator<=(const wxString& s1, const wxString& s2)
    { return s1.Cmp(s2) <= 0; }
inline bool operator<=(const wxString& s1, const wxChar  * s2)
    { return s1.Cmp(s2) <= 0; }
inline bool operator<=(const wxChar  * s1, const wxString& s2)
    { return s2.Cmp(s1) >= 0; }
inline bool operator>=(const wxString& s1, const wxString& s2)
    { return s1.Cmp(s2) >= 0; }
inline bool operator>=(const wxString& s1, const wxChar  * s2)
    { return s1.Cmp(s2) >= 0; }
inline bool operator>=(const wxChar  * s1, const wxString& s2)
    { return s2.Cmp(s1) <= 0; }

#if wxUSE_UNICODE
inline bool operator==(const wxString& s1, const wxWCharBuffer& s2)
    { return (s1.Cmp((const wchar_t *)s2) == 0); }
inline bool operator==(const wxWCharBuffer& s1, const wxString& s2)
    { return (s2.Cmp((const wchar_t *)s1) == 0); }
inline bool operator!=(const wxString& s1, const wxWCharBuffer& s2)
    { return (s1.Cmp((const wchar_t *)s2) != 0); }
inline bool operator!=(const wxWCharBuffer& s1, const wxString& s2)
    { return (s2.Cmp((const wchar_t *)s1) != 0); }
#else // !wxUSE_UNICODE
inline bool operator==(const wxString& s1, const wxCharBuffer& s2)
    { return (s1.Cmp((const char *)s2) == 0); }
inline bool operator==(const wxCharBuffer& s1, const wxString& s2)
    { return (s2.Cmp((const char *)s1) == 0); }
inline bool operator!=(const wxString& s1, const wxCharBuffer& s2)
    { return (s1.Cmp((const char *)s2) != 0); }
inline bool operator!=(const wxCharBuffer& s1, const wxString& s2)
    { return (s2.Cmp((const char *)s1) != 0); }
#endif // wxUSE_UNICODE/!wxUSE_UNICODE

#if wxUSE_UNICODE
inline wxString operator+(const wxString& string, const wxWCharBuffer& buf)
    { return string + (const wchar_t *)buf; }
inline wxString operator+(const wxWCharBuffer& buf, const wxString& string)
    { return (const wchar_t *)buf + string; }
#else // !wxUSE_UNICODE
inline wxString operator+(const wxString& string, const wxCharBuffer& buf)
    { return string + (const char *)buf; }
inline wxString operator+(const wxCharBuffer& buf, const wxString& string)
    { return (const char *)buf + string; }
#endif // wxUSE_UNICODE/!wxUSE_UNICODE

#endif // !wxUSE_STL

// comparison with char (those are not defined by std::[w]string and so should
// be always available)
inline bool operator==(wxChar c, const wxString& s) { return s.IsSameAs(c); }
inline bool operator==(const wxString& s, wxChar c) { return s.IsSameAs(c); }
inline bool operator!=(wxChar c, const wxString& s) { return !s.IsSameAs(c); }
inline bool operator!=(const wxString& s, wxChar c) { return !s.IsSameAs(c); }

// ---------------------------------------------------------------------------
// Implementation only from here until the end of file
// ---------------------------------------------------------------------------

// don't pollute the library user's name space
#undef wxASSERT_VALID_INDEX

#if wxUSE_STD_IOSTREAM

#include "wx/iosfwrap.h"

WXDLLIMPEXP_BASE wxSTD ostream& operator<<(wxSTD ostream&, const wxString&);

#endif  // wxSTD_STRING_COMPATIBILITY

#endif  // _WX_WXSTRINGH__
