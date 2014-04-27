/////////////////////////////////////////////////////////////////////////////
// Name:        uri.cpp
// Purpose:     Implementation of a uri parser
// Author:      Ryan Norton
// Created:     10/26/04
// RCS-ID:      $Id: uri.cpp 58728 2009-02-07 22:03:30Z VZ $
// Copyright:   (c) 2004 Ryan Norton
// Licence:     wxWindows
/////////////////////////////////////////////////////////////////////////////

// ===========================================================================
// declarations
// ===========================================================================

// ---------------------------------------------------------------------------
// headers
// ---------------------------------------------------------------------------

// For compilers that support precompilation, includes "wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#include "wx/uri.h"

// ---------------------------------------------------------------------------
// definitions
// ---------------------------------------------------------------------------

IMPLEMENT_CLASS(wxURI, wxObject)

// ===========================================================================
// implementation
// ===========================================================================

// ---------------------------------------------------------------------------
// utilities
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
//
//                        wxURI
//
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
//  Constructors
// ---------------------------------------------------------------------------

wxURI::wxURI() : m_hostType(wxURI_REGNAME), m_fields(0)
{
}

wxURI::wxURI(const wxString& uri) : m_hostType(wxURI_REGNAME), m_fields(0)
{
    Create(uri);
}

wxURI::wxURI(const wxURI& uri)  : wxObject(), m_hostType(wxURI_REGNAME), m_fields(0)
{
    Assign(uri);
}

// ---------------------------------------------------------------------------
// Destructor and cleanup
// ---------------------------------------------------------------------------

wxURI::~wxURI()
{
    Clear();
}

void wxURI::Clear()
{
    m_scheme = m_userinfo = m_server = m_port = m_path =
    m_query = m_fragment = wxEmptyString;

    m_hostType = wxURI_REGNAME;

    m_fields = 0;
}

// ---------------------------------------------------------------------------
// Create
//
// This creates the URI - all we do here is call the main parsing method
// ---------------------------------------------------------------------------

const wxChar* wxURI::Create(const wxString& uri)
{
    if (m_fields)
        Clear();

    return Parse(uri);
}

// ---------------------------------------------------------------------------
// Escape Methods
//
// TranslateEscape unencodes a 3 character URL escape sequence
//
// Escape encodes an invalid URI character into a 3 character sequence
//
// IsEscape determines if the input string contains an escape sequence,
// if it does, then it moves the input string past the escape sequence
//
// Unescape unencodes all 3 character URL escape sequences in a wxString
// ---------------------------------------------------------------------------

wxChar wxURI::TranslateEscape(const wxChar* s)
{
    wxASSERT_MSG( IsHex(s[0]) && IsHex(s[1]), wxT("Invalid escape sequence!"));

    return wx_truncate_cast(wxChar, (CharToHex(s[0]) << 4 ) | CharToHex(s[1]));
}

wxString wxURI::Unescape(const wxString& uri)
{
    wxString new_uri;

    for(size_t i = 0; i < uri.length(); ++i)
    {
        if (uri[i] == wxT('%'))
        {
            new_uri += wxURI::TranslateEscape( &(uri.c_str()[i+1]) );
            i += 2;
        }
        else
            new_uri += uri[i];
    }

    return new_uri;
}

void wxURI::Escape(wxString& s, const wxChar& c)
{
    const wxChar* hdig = wxT("0123456789abcdef");
    s += wxT('%');
    s += hdig[(c >> 4) & 15];
    s += hdig[c & 15];
}

bool wxURI::IsEscape(const wxChar*& uri)
{
    // pct-encoded   = "%" HEXDIG HEXDIG
    if(*uri == wxT('%') && IsHex(*(uri+1)) && IsHex(*(uri+2)))
        return true;
    else
        return false;
}

// ---------------------------------------------------------------------------
// GetUser
// GetPassword
//
// Gets the username and password via the old URL method.
// ---------------------------------------------------------------------------
wxString wxURI::GetUser() const
{
    // if there is no colon at all, find() returns npos and this method returns
    // the entire string which is correct as it means that password was omitted
    return m_userinfo(0, m_userinfo.find(':'));
}

wxString wxURI::GetPassword() const
{
      size_t posColon = m_userinfo.find(':');

      if ( posColon == wxString::npos )
          return wxT("");

      return m_userinfo(posColon + 1, wxString::npos);
}

// ---------------------------------------------------------------------------
// BuildURI
//
// BuildURI() builds the entire URI into a useable
// representation, including proper identification characters such as slashes
//
// BuildUnescapedURI() does the same thing as BuildURI(), only it unescapes
// the components that accept escape sequences
// ---------------------------------------------------------------------------

wxString wxURI::BuildURI() const
{
    wxString ret;

    if (HasScheme())
        ret = ret + m_scheme + wxT(":");

    if (HasServer())
    {
        ret += wxT("//");

        if (HasUserInfo())
            ret = ret + m_userinfo + wxT("@");

        ret += m_server;

        if (HasPort())
            ret = ret + wxT(":") + m_port;
    }

    ret += m_path;

    if (HasQuery())
        ret = ret + wxT("?") + m_query;

    if (HasFragment())
        ret = ret + wxT("#") + m_fragment;

    return ret;
}

wxString wxURI::BuildUnescapedURI() const
{
    wxString ret;

    if (HasScheme())
        ret = ret + m_scheme + wxT(":");

    if (HasServer())
    {
        ret += wxT("//");

        if (HasUserInfo())
            ret = ret + wxURI::Unescape(m_userinfo) + wxT("@");

        if (m_hostType == wxURI_REGNAME)
            ret += wxURI::Unescape(m_server);
        else
            ret += m_server;

        if (HasPort())
            ret = ret + wxT(":") + m_port;
    }

    ret += wxURI::Unescape(m_path);

    if (HasQuery())
        ret = ret + wxT("?") + wxURI::Unescape(m_query);

    if (HasFragment())
        ret = ret + wxT("#") + wxURI::Unescape(m_fragment);

    return ret;
}

// ---------------------------------------------------------------------------
// Assignment
// ---------------------------------------------------------------------------

wxURI& wxURI::Assign(const wxURI& uri)
{
    //assign fields
    m_fields = uri.m_fields;

    //ref over components
    m_scheme = uri.m_scheme;
    m_userinfo = uri.m_userinfo;
    m_server = uri.m_server;
    m_hostType = uri.m_hostType;
    m_port = uri.m_port;
    m_path = uri.m_path;
    m_query = uri.m_query;
    m_fragment = uri.m_fragment;

    return *this;
}

wxURI& wxURI::operator = (const wxURI& uri)
{
    return Assign(uri);
}

wxURI& wxURI::operator = (const wxString& string)
{
    Create(string);
    return *this;
}

// ---------------------------------------------------------------------------
// Comparison
// ---------------------------------------------------------------------------

bool wxURI::operator == (const wxURI& uri) const
{
    if (HasScheme())
    {
        if(m_scheme != uri.m_scheme)
            return false;
    }
    else if (uri.HasScheme())
        return false;


    if (HasServer())
    {
        if (HasUserInfo())
        {
            if (m_userinfo != uri.m_userinfo)
                return false;
        }
        else if (uri.HasUserInfo())
            return false;

        if (m_server != uri.m_server ||
            m_hostType != uri.m_hostType)
            return false;

        if (HasPort())
        {
            if(m_port != uri.m_port)
                return false;
        }
        else if (uri.HasPort())
            return false;
    }
    else if (uri.HasServer())
        return false;


    if (HasPath())
    {
        if(m_path != uri.m_path)
            return false;
    }
    else if (uri.HasPath())
        return false;

    if (HasQuery())
    {
        if (m_query != uri.m_query)
            return false;
    }
    else if (uri.HasQuery())
        return false;

    if (HasFragment())
    {
        if (m_fragment != uri.m_fragment)
            return false;
    }
    else if (uri.HasFragment())
        return false;

    return true;
}

// ---------------------------------------------------------------------------
// IsReference
//
// if there is no authority or scheme, it is a reference
// ---------------------------------------------------------------------------

bool wxURI::IsReference() const
{   return !HasScheme() || !HasServer();  }

// ---------------------------------------------------------------------------
// Parse
//
// Master URI parsing method.  Just calls the individual parsing methods
//
// URI = scheme ":" hier-part [ "?" query ] [ "#" fragment ]
// URI-reference = URI / relative
// ---------------------------------------------------------------------------

const wxChar* wxURI::Parse(const wxChar* uri)
{
    uri = ParseScheme(uri);
    uri = ParseAuthority(uri);
    uri = ParsePath(uri);
    uri = ParseQuery(uri);
    return ParseFragment(uri);
}

// ---------------------------------------------------------------------------
// ParseXXX
//
// Individual parsers for each URI component
// ---------------------------------------------------------------------------

const wxChar* wxURI::ParseScheme(const wxChar* uri)
{
    wxASSERT(uri != NULL);

    //copy of the uri - used for figuring out
    //length of each component
    const wxChar* uricopy = uri;

    //Does the uri have a scheme (first character alpha)?
    if (IsAlpha(*uri))
    {
        m_scheme += *uri++;

        //scheme        = ALPHA *( ALPHA / DIGIT / "+" / "-" / "." )
        while (IsAlpha(*uri) || IsDigit(*uri) ||
               *uri == wxT('+')   ||
               *uri == wxT('-')   ||
               *uri == wxT('.'))
        {
            m_scheme += *uri++;
        }

        //valid scheme?
        if (*uri == wxT(':'))
        {
            //mark the scheme as valid
            m_fields |= wxURI_SCHEME;

            //move reference point up to input buffer
            uricopy = ++uri;
        }
        else
            //relative uri with relative path reference
            m_scheme = wxEmptyString;
    }
//    else
        //relative uri with _possible_ relative path reference

    return uricopy;
}

const wxChar* wxURI::ParseAuthority(const wxChar* uri)
{
    // authority     = [ userinfo "@" ] host [ ":" port ]
    if (*uri == wxT('/') && *(uri+1) == wxT('/'))
    {
        //skip past the two slashes
        uri += 2;

        // ############# DEVIATION FROM RFC #########################
        // Don't parse the server component for file URIs
        if(m_scheme != wxT("file"))
        {
            //normal way
        uri = ParseUserInfo(uri);
        uri = ParseServer(uri);
        return ParsePort(uri);
        }
    }

    return uri;
}

const wxChar* wxURI::ParseUserInfo(const wxChar* uri)
{
    wxASSERT(uri != NULL);

    //copy of the uri - used for figuring out
    //length of each component
    const wxChar* uricopy = uri;

    // userinfo      = *( unreserved / pct-encoded / sub-delims / ":" )
    while(*uri && *uri != wxT('@') && *uri != wxT('/') && *uri != wxT('#') && *uri != wxT('?'))
    {
        if(IsUnreserved(*uri) ||
           IsSubDelim(*uri) || *uri == wxT(':'))
            m_userinfo += *uri++;
        else if (IsEscape(uri))
        {
            m_userinfo += *uri++;
            m_userinfo += *uri++;
            m_userinfo += *uri++;
        }
        else
            Escape(m_userinfo, *uri++);
    }

    if(*uri == wxT('@'))
    {
        //valid userinfo
        m_fields |= wxURI_USERINFO;

        uricopy = ++uri;
    }
    else
        m_userinfo = wxEmptyString;

    return uricopy;
}

const wxChar* wxURI::ParseServer(const wxChar* uri)
{
    wxASSERT(uri != NULL);

    //copy of the uri - used for figuring out
    //length of each component
    const wxChar* uricopy = uri;

    // host          = IP-literal / IPv4address / reg-name
    // IP-literal    = "[" ( IPv6address / IPvFuture  ) "]"
    if (*uri == wxT('['))
    {
        ++uri; //some compilers don't support *&ing a ++*
        if (ParseIPv6address(uri) && *uri == wxT(']'))
        {
            ++uri;
            m_hostType = wxURI_IPV6ADDRESS;

            wxStringBufferLength theBuffer(m_server, uri - uricopy);
            wxTmemcpy(theBuffer, uricopy, uri-uricopy);
            theBuffer.SetLength(uri-uricopy);
        }
        else
        {
            uri = uricopy;

            ++uri; //some compilers don't support *&ing a ++*
            if (ParseIPvFuture(uri) && *uri == wxT(']'))
            {
                ++uri;
                m_hostType = wxURI_IPVFUTURE;

                wxStringBufferLength theBuffer(m_server, uri - uricopy);
                wxTmemcpy(theBuffer, uricopy, uri-uricopy);
                theBuffer.SetLength(uri-uricopy);
            }
            else
                uri = uricopy;
        }
    }
    else
    {
        if (ParseIPv4address(uri))
        {
            m_hostType = wxURI_IPV4ADDRESS;

            wxStringBufferLength theBuffer(m_server, uri - uricopy);
            wxTmemcpy(theBuffer, uricopy, uri-uricopy);
            theBuffer.SetLength(uri-uricopy);
        }
        else
            uri = uricopy;
    }

    if(m_hostType == wxURI_REGNAME)
    {
        uri = uricopy;
        // reg-name      = *( unreserved / pct-encoded / sub-delims )
        while(*uri && *uri != wxT('/') && *uri != wxT(':') && *uri != wxT('#') && *uri != wxT('?'))
        {
            if(IsUnreserved(*uri) ||  IsSubDelim(*uri))
                m_server += *uri++;
            else if (IsEscape(uri))
            {
                m_server += *uri++;
                m_server += *uri++;
                m_server += *uri++;
            }
            else
                Escape(m_server, *uri++);
        }
    }

    //mark the server as valid
    m_fields |= wxURI_SERVER;

    return uri;
}


const wxChar* wxURI::ParsePort(const wxChar* uri)
{
    wxASSERT(uri != NULL);

    // port          = *DIGIT
    if(*uri == wxT(':'))
    {
        ++uri;
        while(IsDigit(*uri))
        {
            m_port += *uri++;
        }

        //mark the port as valid
        m_fields |= wxURI_PORT;
    }

    return uri;
}

const wxChar* wxURI::ParsePath(const wxChar* uri, bool bReference, bool bNormalize)
{
    wxASSERT(uri != NULL);

    //copy of the uri - used for figuring out
    //length of each component
    const wxChar* uricopy = uri;

    /// hier-part     = "//" authority path-abempty
    ///               / path-absolute
    ///               / path-rootless
    ///               / path-empty
    ///
    /// relative-part = "//" authority path-abempty
    ///               / path-absolute
    ///               / path-noscheme
    ///               / path-empty
    ///
    /// path-abempty  = *( "/" segment )
    /// path-absolute = "/" [ segment-nz *( "/" segment ) ]
    /// path-noscheme = segment-nz-nc *( "/" segment )
    /// path-rootless = segment-nz *( "/" segment )
    /// path-empty    = 0<pchar>
    ///
    /// segment       = *pchar
    /// segment-nz    = 1*pchar
    /// segment-nz-nc = 1*( unreserved / pct-encoded / sub-delims / "@" )
    ///               ; non-zero-length segment without any colon ":"
    ///
    /// pchar         = unreserved / pct-encoded / sub-delims / ":" / "@"
    if (*uri == wxT('/'))
    {
        m_path += *uri++;

        while(*uri && *uri != wxT('#') && *uri != wxT('?'))
        {
            if( IsUnreserved(*uri) || IsSubDelim(*uri) ||
                *uri == wxT(':') || *uri == wxT('@') || *uri == wxT('/'))
                m_path += *uri++;
            else if (IsEscape(uri))
            {
                m_path += *uri++;
                m_path += *uri++;
                m_path += *uri++;
            }
            else
                Escape(m_path, *uri++);
        }

        if (bNormalize)
        {
            wxStringBufferLength theBuffer(m_path, m_path.length() + 1);
#if wxUSE_STL
            wxTmemcpy(theBuffer, m_path.c_str(), m_path.length()+1);
#endif
            Normalize(theBuffer, true);
            theBuffer.SetLength(wxStrlen(theBuffer));
        }
        //mark the path as valid
        m_fields |= wxURI_PATH;
    }
    else if(*uri) //Relative path
    {
        if (bReference)
        {
            //no colon allowed
            while(*uri && *uri != wxT('#') && *uri != wxT('?'))
            {
                if(IsUnreserved(*uri) || IsSubDelim(*uri) ||
                  *uri == wxT('@') || *uri == wxT('/'))
                    m_path += *uri++;
                else if (IsEscape(uri))
                {
                    m_path += *uri++;
                    m_path += *uri++;
                    m_path += *uri++;
                }
                else
                    Escape(m_path, *uri++);
            }
        }
        else
        {
            while(*uri && *uri != wxT('#') && *uri != wxT('?'))
            {
                if(IsUnreserved(*uri) || IsSubDelim(*uri) ||
                   *uri == wxT(':') || *uri == wxT('@') || *uri == wxT('/'))
                    m_path += *uri++;
                else if (IsEscape(uri))
                {
                    m_path += *uri++;
                    m_path += *uri++;
                    m_path += *uri++;
                }
                else
                    Escape(m_path, *uri++);
            }
        }

        if (uri != uricopy)
        {
            if (bNormalize)
            {
                wxStringBufferLength theBuffer(m_path, m_path.length() + 1);
#if wxUSE_STL
                wxTmemcpy(theBuffer, m_path.c_str(), m_path.length()+1);
#endif
                Normalize(theBuffer);
                theBuffer.SetLength(wxStrlen(theBuffer));
            }

            //mark the path as valid
            m_fields |= wxURI_PATH;
        }
    }

    return uri;
}


const wxChar* wxURI::ParseQuery(const wxChar* uri)
{
    wxASSERT(uri != NULL);

    // query         = *( pchar / "/" / "?" )
    if (*uri == wxT('?'))
    {
        ++uri;
        while(*uri && *uri != wxT('#'))
        {
            if (IsUnreserved(*uri) || IsSubDelim(*uri) ||
                *uri == wxT(':') || *uri == wxT('@') || *uri == wxT('/') || *uri == wxT('?'))
                  m_query += *uri++;
            else if (IsEscape(uri))
            {
                  m_query += *uri++;
                  m_query += *uri++;
                  m_query += *uri++;
            }
            else
                  Escape(m_query, *uri++);
        }

        //mark the server as valid
        m_fields |= wxURI_QUERY;
    }

    return uri;
}


const wxChar* wxURI::ParseFragment(const wxChar* uri)
{
    wxASSERT(uri != NULL);

    // fragment      = *( pchar / "/" / "?" )
    if (*uri == wxT('#'))
    {
        ++uri;
        while(*uri)
        {
            if (IsUnreserved(*uri) || IsSubDelim(*uri) ||
                *uri == wxT(':') || *uri == wxT('@') || *uri == wxT('/') || *uri == wxT('?'))
                  m_fragment += *uri++;
            else if (IsEscape(uri))
            {
                  m_fragment += *uri++;
                  m_fragment += *uri++;
                  m_fragment += *uri++;
            }
            else
                  Escape(m_fragment, *uri++);
        }

        //mark the server as valid
        m_fields |= wxURI_FRAGMENT;
    }

    return uri;
}

// ---------------------------------------------------------------------------
// Resolve
//
// Builds missing components of this uri from a base uri
//
// A version of the algorithm outlined in the RFC is used here
// (it is shown in comments)
//
// Note that an empty URI inherits all components
// ---------------------------------------------------------------------------

void wxURI::Resolve(const wxURI& base, int flags)
{
    wxASSERT_MSG(!base.IsReference(),
                wxT("wxURI to inherit from must not be a reference!"));

    // If we arn't being strict, enable the older (pre-RFC2396)
    // loophole that allows this uri to inherit other
    // properties from the base uri - even if the scheme
    // is defined
    if ( !(flags & wxURI_STRICT) &&
            HasScheme() && base.HasScheme() &&
                m_scheme == base.m_scheme )
    {
        m_fields -= wxURI_SCHEME;
    }


    // Do nothing if this is an absolute wxURI
    //    if defined(R.scheme) then
    //       T.scheme    = R.scheme;
    //       T.authority = R.authority;
    //       T.path      = remove_dot_segments(R.path);
    //       T.query     = R.query;
    if (HasScheme())
    {
        return;
    }

    //No scheme - inherit
    m_scheme = base.m_scheme;
    m_fields |= wxURI_SCHEME;

    // All we need to do for relative URIs with an
    // authority component is just inherit the scheme
    //       if defined(R.authority) then
    //          T.authority = R.authority;
    //          T.path      = remove_dot_segments(R.path);
    //          T.query     = R.query;
    if (HasServer())
    {
        return;
    }

    //No authority - inherit
    if (base.HasUserInfo())
    {
        m_userinfo = base.m_userinfo;
        m_fields |= wxURI_USERINFO;
    }

    m_server = base.m_server;
    m_hostType = base.m_hostType;
    m_fields |= wxURI_SERVER;

    if (base.HasPort())
    {
        m_port = base.m_port;
        m_fields |= wxURI_PORT;
    }


    // Simple path inheritance from base
    if (!HasPath())
    {
        //             T.path = Base.path;
        m_path = base.m_path;
        m_fields |= wxURI_PATH;


        //             if defined(R.query) then
        //                T.query = R.query;
        //             else
        //                T.query = Base.query;
        //             endif;
        if (!HasQuery())
        {
            m_query = base.m_query;
            m_fields |= wxURI_QUERY;
        }
    }
    else
    {
        //             if (R.path starts-with "/") then
        //                T.path = remove_dot_segments(R.path);
        //             else
        //                T.path = merge(Base.path, R.path);
        //                T.path = remove_dot_segments(T.path);
        //             endif;
        //             T.query = R.query;
        if (m_path.empty() || m_path[0u] != wxT('/'))
        {
            //Merge paths
            const wxChar* op = m_path.c_str();
            const wxChar* bp = base.m_path.c_str() + base.m_path.Length();

            //not a ending directory?  move up
            if (base.m_path[0] && *(bp-1) != wxT('/'))
                UpTree(base.m_path, bp);

            //normalize directories
            while(*op == wxT('.') && *(op+1) == wxT('.') &&
                       (*(op+2) == '\0' || *(op+2) == wxT('/')) )
            {
                UpTree(base.m_path, bp);

                if (*(op+2) == '\0')
                    op += 2;
                else
                    op += 3;
            }

            m_path = base.m_path.substr(0, bp - base.m_path.c_str()) +
                    m_path.substr((op - m_path.c_str()), m_path.Length());
        }
    }

    //T.fragment = R.fragment;
}

// ---------------------------------------------------------------------------
// UpTree
//
// Moves a URI path up a directory
// ---------------------------------------------------------------------------

//static
void wxURI::UpTree(const wxChar* uristart, const wxChar*& uri)
{
    if (uri != uristart && *(uri-1) == wxT('/'))
    {
        uri -= 2;
    }

    for(;uri != uristart; --uri)
    {
        if (*uri == wxT('/'))
        {
            ++uri;
            break;
        }
    }

    //!!!TODO:HACK!!!//
    if (uri == uristart && *uri == wxT('/'))
        ++uri;
    //!!!//
}

// ---------------------------------------------------------------------------
// Normalize
//
// Normalizes directories in-place
//
// I.E. ./ and . are ignored
//
// ../ and .. are removed if a directory is before it, along
// with that directory (leading .. and ../ are kept)
// ---------------------------------------------------------------------------

//static
void wxURI::Normalize(wxChar* s, bool bIgnoreLeads)
{
    wxChar* cp = s;
    wxChar* bp = s;

    if(s[0] == wxT('/'))
        ++bp;

    while(*cp)
    {
        if (*cp == wxT('.') && (*(cp+1) == wxT('/') || *(cp+1) == '\0')
            && (bp == cp || *(cp-1) == wxT('/')))
        {
            //. _or_ ./  - ignore
            if (*(cp+1) == '\0')
                cp += 1;
            else
                cp += 2;
        }
        else if (*cp == wxT('.') && *(cp+1) == wxT('.') &&
                (*(cp+2) == wxT('/') || *(cp+2) == '\0')
                && (bp == cp || *(cp-1) == wxT('/')))
        {
            //.. _or_ ../ - go up the tree
            if (s != bp)
            {
                UpTree((const wxChar*)bp, (const wxChar*&)s);

                if (*(cp+2) == '\0')
                    cp += 2;
                else
                    cp += 3;
            }
            else if (!bIgnoreLeads)

            {
                *bp++ = *cp++;
                *bp++ = *cp++;
                if (*cp)
                    *bp++ = *cp++;

                s = bp;
            }
            else
            {
                if (*(cp+2) == '\0')
                    cp += 2;
                else
                    cp += 3;
            }
        }
        else
            *s++ = *cp++;
    }

    *s = '\0';
}

// ---------------------------------------------------------------------------
// ParseH16
//
// Parses 1 to 4 hex values.  Returns true if the first character of the input
// string is a valid hex character.  It is the caller's responsability to move
// the input string back to its original position on failure.
// ---------------------------------------------------------------------------

bool wxURI::ParseH16(const wxChar*& uri)
{
    // h16           = 1*4HEXDIG
    if(!IsHex(*++uri))
        return false;

    if(IsHex(*++uri) && IsHex(*++uri) && IsHex(*++uri))
        ++uri;

    return true;
}

// ---------------------------------------------------------------------------
// ParseIPXXX
//
// Parses a certain version of an IP address and moves the input string past
// it.  Returns true if the input  string contains the proper version of an ip
// address.  It is the caller's responsability to move the input string back
// to its original position on failure.
// ---------------------------------------------------------------------------

bool wxURI::ParseIPv4address(const wxChar*& uri)
{
    //IPv4address   = dec-octet "." dec-octet "." dec-octet "." dec-octet
    //
    //dec-octet     =      DIGIT                    ; 0-9
    //                / %x31-39 DIGIT               ; 10-99
    //                / "1" 2DIGIT                  ; 100-199
    //                / "2" %x30-34 DIGIT           ; 200-249
    //                / "25" %x30-35                ; 250-255
    size_t iIPv4 = 0;
    if (IsDigit(*uri))
    {
        ++iIPv4;


        //each ip part must be between 0-255 (dupe of version in for loop)
        if( IsDigit(*++uri) && IsDigit(*++uri) &&
           //100 or less  (note !)
           !( (*(uri-2) < wxT('2')) ||
           //240 or less
             (*(uri-2) == wxT('2') &&
               (*(uri-1) < wxT('5') || (*(uri-1) == wxT('5') && *uri <= wxT('5')))
             )
            )
          )
        {
            return false;
        }

        if(IsDigit(*uri))++uri;

        //compilers should unroll this loop
        for(; iIPv4 < 4; ++iIPv4)
        {
            if (*uri != wxT('.') || !IsDigit(*++uri))
                break;

            //each ip part must be between 0-255
            if( IsDigit(*++uri) && IsDigit(*++uri) &&
               //100 or less  (note !)
               !( (*(uri-2) < wxT('2')) ||
               //240 or less
                 (*(uri-2) == wxT('2') &&
                   (*(uri-1) < wxT('5') || (*(uri-1) == wxT('5') && *uri <= wxT('5')))
                 )
                )
              )
            {
                return false;
            }
            if(IsDigit(*uri))++uri;
        }
    }
    return iIPv4 == 4;
}

bool wxURI::ParseIPv6address(const wxChar*& uri)
{
    // IPv6address   =                            6( h16 ":" ) ls32
    //               /                       "::" 5( h16 ":" ) ls32
    //               / [               h16 ] "::" 4( h16 ":" ) ls32
    //               / [ *1( h16 ":" ) h16 ] "::" 3( h16 ":" ) ls32
    //               / [ *2( h16 ":" ) h16 ] "::" 2( h16 ":" ) ls32
    //               / [ *3( h16 ":" ) h16 ] "::"    h16 ":"   ls32
    //               / [ *4( h16 ":" ) h16 ] "::"              ls32
    //               / [ *5( h16 ":" ) h16 ] "::"              h16
    //               / [ *6( h16 ":" ) h16 ] "::"

    size_t numPrefix = 0,
              maxPostfix;

    bool bEndHex = false;

    for( ; numPrefix < 6; ++numPrefix)
    {
        if(!ParseH16(uri))
        {
            --uri;
            bEndHex = true;
            break;
        }

        if(*uri != wxT(':'))
        {
            break;
        }
    }

    if(!bEndHex && !ParseH16(uri))
    {
        --uri;

        if (numPrefix)
            return false;

        if (*uri == wxT(':'))
        {
            if (*++uri != wxT(':'))
                return false;

            maxPostfix = 5;
        }
        else
            maxPostfix = 6;
    }
    else
    {
        if (*uri != wxT(':') || *(uri+1) != wxT(':'))
        {
            if (numPrefix != 6)
                return false;

            while (*--uri != wxT(':')) {}
            ++uri;

            const wxChar* uristart = uri;
            //parse ls32
            // ls32          = ( h16 ":" h16 ) / IPv4address
            if (ParseH16(uri) && *uri == wxT(':') && ParseH16(uri))
                return true;

            uri = uristart;

            if (ParseIPv4address(uri))
                return true;
            else
                return false;
        }
        else
        {
            uri += 2;

            if (numPrefix > 3)
                maxPostfix = 0;
            else
                maxPostfix = 4 - numPrefix;
        }
    }

    bool bAllowAltEnding = maxPostfix == 0;

    for(; maxPostfix != 0; --maxPostfix)
    {
        if(!ParseH16(uri) || *uri != wxT(':'))
            return false;
    }

    if(numPrefix <= 4)
    {
        const wxChar* uristart = uri;
        //parse ls32
        // ls32          = ( h16 ":" h16 ) / IPv4address
        if (ParseH16(uri) && *uri == wxT(':') && ParseH16(uri))
            return true;

        uri = uristart;

        if (ParseIPv4address(uri))
            return true;

        uri = uristart;

        if (!bAllowAltEnding)
            return false;
    }

    if(numPrefix <= 5 && ParseH16(uri))
        return true;

    return true;
}

bool wxURI::ParseIPvFuture(const wxChar*& uri)
{
    // IPvFuture     = "v" 1*HEXDIG "." 1*( unreserved / sub-delims / ":" )
    if (*++uri != wxT('v') || !IsHex(*++uri))
        return false;

    while (IsHex(*++uri)) {}

    if (*uri != wxT('.') || !(IsUnreserved(*++uri) || IsSubDelim(*uri) || *uri == wxT(':')))
        return false;

    while(IsUnreserved(*++uri) || IsSubDelim(*uri) || *uri == wxT(':')) {}

    return true;
}


// ---------------------------------------------------------------------------
// CharToHex
//
// Converts a character into a numeric hexidecimal value, or 0 if the
// passed in character is not a valid hex character
// ---------------------------------------------------------------------------

//static
wxChar wxURI::CharToHex(const wxChar& c)
{
    if ((c >= wxT('A')) && (c <= wxT('Z'))) return wxChar(c - wxT('A') + 0x0A);
    if ((c >= wxT('a')) && (c <= wxT('z'))) return wxChar(c - wxT('a') + 0x0a);
    if ((c >= wxT('0')) && (c <= wxT('9'))) return wxChar(c - wxT('0') + 0x00);

    return 0;
}

// ---------------------------------------------------------------------------
// IsXXX
//
// Returns true if the passed in character meets the criteria of the method
// ---------------------------------------------------------------------------

//! unreserved    = ALPHA / DIGIT / "-" / "." / "_" / "~"
bool wxURI::IsUnreserved (const wxChar& c)
{   return IsAlpha(c) || IsDigit(c) ||
           c == wxT('-') ||
           c == wxT('.') ||
           c == wxT('_') ||
           c == wxT('~') //tilde
           ;
}

bool wxURI::IsReserved (const wxChar& c)
{
    return IsGenDelim(c) || IsSubDelim(c);
}

//! gen-delims    = ":" / "/" / "?" / "#" / "[" / "]" / "@"
bool wxURI::IsGenDelim (const wxChar& c)
{
    return c == wxT(':') ||
           c == wxT('/') ||
           c == wxT('?') ||
           c == wxT('#') ||
           c == wxT('[') ||
           c == wxT(']') ||
           c == wxT('@');
}

//! sub-delims    = "!" / "$" / "&" / "'" / "(" / ")"
//!               / "*" / "+" / "," / ";" / "="
bool wxURI::IsSubDelim (const wxChar& c)
{
    return c == wxT('!') ||
           c == wxT('$') ||
           c == wxT('&') ||
           c == wxT('\'') ||
           c == wxT('(') ||
           c == wxT(')') ||
           c == wxT('*') ||
           c == wxT('+') ||
           c == wxT(',') ||
           c == wxT(';') ||
           c == wxT('=')
           ;
}

bool wxURI::IsHex(const wxChar& c)
{   return IsDigit(c) || (c >= wxT('a') && c <= wxT('f')) || (c >= wxT('A') && c <= wxT('F')); }

bool wxURI::IsAlpha(const wxChar& c)
{   return (c >= wxT('a') && c <= wxT('z')) || (c >= wxT('A') && c <= wxT('Z'));  }

bool wxURI::IsDigit(const wxChar& c)
{   return c >= wxT('0') && c <= wxT('9');        }


//end of uri.cpp



