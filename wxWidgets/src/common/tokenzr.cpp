/////////////////////////////////////////////////////////////////////////////
// Name:        src/common/tokenzr.cpp
// Purpose:     String tokenizer
// Author:      Guilhem Lavaux
// Modified by: Vadim Zeitlin (almost full rewrite)
// Created:     04/22/98
// RCS-ID:      $Id: tokenzr.cpp 39694 2006-06-13 11:30:40Z ABX $
// Copyright:   (c) Guilhem Lavaux
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

// ============================================================================
// declarations
// ============================================================================

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

// For compilers that support precompilation, includes "wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#include "wx/tokenzr.h"

#ifndef WX_PRECOMP
    #include "wx/arrstr.h"
#endif

// Required for wxIs... functions
#include <ctype.h>

// ============================================================================
// implementation
// ============================================================================

// ----------------------------------------------------------------------------
// wxStringTokenizer construction
// ----------------------------------------------------------------------------

wxStringTokenizer::wxStringTokenizer(const wxString& str,
                                     const wxString& delims,
                                     wxStringTokenizerMode mode)
{
    SetString(str, delims, mode);
}

void wxStringTokenizer::SetString(const wxString& str,
                                  const wxString& delims,
                                  wxStringTokenizerMode mode)
{
    if ( mode == wxTOKEN_DEFAULT )
    {
        // by default, we behave like strtok() if the delimiters are only
        // whitespace characters and as wxTOKEN_RET_EMPTY otherwise (for
        // whitespace delimiters, strtok() behaviour is better because we want
        // to count consecutive spaces as one delimiter)
        const wxChar *p;
        for ( p = delims.c_str(); *p; p++ )
        {
            if ( !wxIsspace(*p) )
                break;
        }

        if ( *p )
        {
            // not whitespace char in delims
            mode = wxTOKEN_RET_EMPTY;
        }
        else
        {
            // only whitespaces
            mode = wxTOKEN_STRTOK;
        }
    }

    m_delims = delims;
    m_mode = mode;

    Reinit(str);
}

void wxStringTokenizer::Reinit(const wxString& str)
{
    wxASSERT_MSG( IsOk(), _T("you should call SetString() first") );

    m_string = str;
    m_pos = 0;
    m_lastDelim = _T('\0');
}

// ----------------------------------------------------------------------------
// access to the tokens
// ----------------------------------------------------------------------------

// do we have more of them?
bool wxStringTokenizer::HasMoreTokens() const
{
    wxCHECK_MSG( IsOk(), false, _T("you should call SetString() first") );

    if ( m_string.find_first_not_of(m_delims, m_pos) != wxString::npos )
    {
        // there are non delimiter characters left, so we do have more tokens
        return true;
    }

    switch ( m_mode )
    {
        case wxTOKEN_RET_EMPTY:
        case wxTOKEN_RET_DELIMS:
            // special hack for wxTOKEN_RET_EMPTY: we should return the initial
            // empty token even if there are only delimiters after it
            return m_pos == 0 && !m_string.empty();

        case wxTOKEN_RET_EMPTY_ALL:
            // special hack for wxTOKEN_RET_EMPTY_ALL: we can know if we had
            // already returned the trailing empty token after the last
            // delimiter by examining m_lastDelim: it is set to NUL if we run
            // up to the end of the string in GetNextToken(), but if it is not
            // NUL yet we still have this last token to return even if m_pos is
            // already at m_string.length()
            return m_pos < m_string.length() || m_lastDelim != _T('\0');

        case wxTOKEN_INVALID:
        case wxTOKEN_DEFAULT:
            wxFAIL_MSG( _T("unexpected tokenizer mode") );
            // fall through

        case wxTOKEN_STRTOK:
            // never return empty delimiters
            break;
    }

    return false;
}

// count the number of (remaining) tokens in the string
size_t wxStringTokenizer::CountTokens() const
{
    wxCHECK_MSG( IsOk(), 0, _T("you should call SetString() first") );

    // VZ: this function is IMHO not very useful, so it's probably not very
    //     important if its implementation here is not as efficient as it
    //     could be -- but OTOH like this we're sure to get the correct answer
    //     in all modes
    wxStringTokenizer tkz(m_string.c_str() + m_pos, m_delims, m_mode);

    size_t count = 0;
    while ( tkz.HasMoreTokens() )
    {
        count++;

        (void)tkz.GetNextToken();
    }

    return count;
}

// ----------------------------------------------------------------------------
// token extraction
// ----------------------------------------------------------------------------

wxString wxStringTokenizer::GetNextToken()
{
    wxString token;
    do
    {
        if ( !HasMoreTokens() )
        {
            break;
        }

        // find the end of this token
        size_t pos = m_string.find_first_of(m_delims, m_pos);

        // and the start of the next one
        if ( pos == wxString::npos )
        {
            // no more delimiters, the token is everything till the end of
            // string
            token.assign(m_string, m_pos, wxString::npos);

            // skip the token
            m_pos = m_string.length();

            // it wasn't terminated
            m_lastDelim = _T('\0');
        }
        else // we found a delimiter at pos
        {
            // in wxTOKEN_RET_DELIMS mode we return the delimiter character
            // with token, otherwise leave it out
            size_t len = pos - m_pos;
            if ( m_mode == wxTOKEN_RET_DELIMS )
                len++;

            token.assign(m_string, m_pos, len);

            // skip the token and the trailing delimiter
            m_pos = pos + 1;

            m_lastDelim = m_string[pos];
        }
    }
    while ( !AllowEmpty() && token.empty() );

    return token;
}

// ----------------------------------------------------------------------------
// public functions
// ----------------------------------------------------------------------------

wxArrayString wxStringTokenize(const wxString& str,
                               const wxString& delims,
                               wxStringTokenizerMode mode)
{
    wxArrayString tokens;
    wxStringTokenizer tk(str, delims, mode);
    while ( tk.HasMoreTokens() )
    {
        tokens.Add(tk.GetNextToken());
    }

    return tokens;
}
