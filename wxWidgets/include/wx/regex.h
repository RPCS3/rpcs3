///////////////////////////////////////////////////////////////////////////////
// Name:        wx/regex.h
// Purpose:     regular expression matching
// Author:      Karsten Ballueder
// Modified by: VZ at 13.07.01 (integrated to wxWin)
// Created:     05.02.2000
// RCS-ID:      $Id: regex.h 57779 2009-01-02 17:35:16Z PC $
// Copyright:   (c) 2000 Karsten Ballueder <ballueder@gmx.net>
// Licence:     wxWindows licence
///////////////////////////////////////////////////////////////////////////////

#ifndef _WX_REGEX_H_
#define _WX_REGEX_H_

#include "wx/defs.h"

#if wxUSE_REGEX

#include "wx/string.h"

// ----------------------------------------------------------------------------
// constants
// ----------------------------------------------------------------------------

// flags for regex compilation: these can be used with Compile()
enum
{
    // use extended regex syntax
    wxRE_EXTENDED = 0,

    // use advanced RE syntax (built-in regex only)
#ifdef wxHAS_REGEX_ADVANCED
    wxRE_ADVANCED = 1,
#endif

    // use basic RE syntax
    wxRE_BASIC    = 2,

    // ignore case in match
    wxRE_ICASE    = 4,

    // only check match, don't set back references
    wxRE_NOSUB    = 8,

    // if not set, treat '\n' as an ordinary character, otherwise it is
    // special: it is not matched by '.' and '^' and '$' always match
    // after/before it regardless of the setting of wxRE_NOT[BE]OL
    wxRE_NEWLINE  = 16,

    // default flags
    wxRE_DEFAULT  = wxRE_EXTENDED
};

// flags for regex matching: these can be used with Matches()
//
// these flags are mainly useful when doing several matches in a long string,
// they can be used to prevent erroneous matches for '^' and '$'
enum
{
    // '^' doesn't match at the start of line
    wxRE_NOTBOL = 32,

    // '$' doesn't match at the end of line
    wxRE_NOTEOL = 64
};

// ----------------------------------------------------------------------------
// wxRegEx: a regular expression
// ----------------------------------------------------------------------------

class WXDLLIMPEXP_FWD_BASE wxRegExImpl;

class WXDLLIMPEXP_BASE wxRegEx
{
public:
    // default ctor: use Compile() later
    wxRegEx() { Init(); }

    // create and compile
    wxRegEx(const wxString& expr, int flags = wxRE_DEFAULT)
    {
        Init();
        (void)Compile(expr, flags);
    }

    // return true if this is a valid compiled regular expression
    bool IsValid() const { return m_impl != NULL; }

    // compile the string into regular expression, return true if ok or false
    // if string has a syntax error
    bool Compile(const wxString& pattern, int flags = wxRE_DEFAULT);

    // matches the precompiled regular expression against a string, return
    // true if matches and false otherwise
    //
    // flags may be combination of wxRE_NOTBOL and wxRE_NOTEOL
    // len may be the length of text (ignored by most system regex libs)
    //
    // may only be called after successful call to Compile()
    bool Matches(const wxChar *text, int flags = 0) const;
    bool Matches(const wxChar *text, int flags, size_t len) const;
    bool Matches(const wxString& text, int flags = 0) const
        { return Matches(text.c_str(), flags, text.length()); }

    // get the start index and the length of the match of the expression
    // (index 0) or a bracketed subexpression (index != 0)
    //
    // may only be called after successful call to Matches()
    //
    // return false if no match or on error
    bool GetMatch(size_t *start, size_t *len, size_t index = 0) const;

    // return the part of string corresponding to the match, empty string is
    // returned if match failed
    //
    // may only be called after successful call to Matches()
    wxString GetMatch(const wxString& text, size_t index = 0) const;

    // return the size of the array of matches, i.e. the number of bracketed
    // subexpressions plus one for the expression itself, or 0 on error.
    //
    // may only be called after successful call to Compile()
    size_t GetMatchCount() const;

    // replaces the current regular expression in the string pointed to by
    // pattern, with the text in replacement and return number of matches
    // replaced (maybe 0 if none found) or -1 on error
    //
    // the replacement text may contain backreferences (\number) which will be
    // replaced with the value of the corresponding subexpression in the
    // pattern match
    //
    // maxMatches may be used to limit the number of replacements made, setting
    // it to 1, for example, will only replace first occurence (if any) of the
    // pattern in the text while default value of 0 means replace all
    int Replace(wxString *text, const wxString& replacement,
                size_t maxMatches = 0) const;

    // replace the first occurence
    int ReplaceFirst(wxString *text, const wxString& replacement) const
        { return Replace(text, replacement, 1); }

    // replace all occurences: this is actually a synonym for Replace()
    int ReplaceAll(wxString *text, const wxString& replacement) const
        { return Replace(text, replacement, 0); }

    // dtor not virtual, don't derive from this class
    ~wxRegEx();

private:
    // common part of all ctors
    void Init();

    // the real guts of this class
    wxRegExImpl *m_impl;

    // as long as the class wxRegExImpl is not ref-counted,
    // instances of the handle wxRegEx must not be copied.
    wxRegEx(const wxRegEx&);
    wxRegEx &operator=(const wxRegEx&);
};

#endif // wxUSE_REGEX

#endif // _WX_REGEX_H_

