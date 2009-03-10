///////////////////////////////////////////////////////////////////////////////
// Name:        src/common/config.cpp
// Purpose:     implementation of wxConfigBase class
// Author:      Vadim Zeitlin
// Modified by:
// Created:     07.04.98
// RCS-ID:      $Id: config.cpp 50711 2007-12-15 02:57:58Z VZ $
// Copyright:   (c) 1997 Karsten Ballueder   Ballueder@usa.net
//                       Vadim Zeitlin      <zeitlin@dptmaths.ens-cachan.fr>
// Licence:     wxWindows licence
///////////////////////////////////////////////////////////////////////////////

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif  //__BORLANDC__

#ifndef wxUSE_CONFIG_NATIVE
    #define wxUSE_CONFIG_NATIVE 1
#endif

#include "wx/config.h"

#ifndef WX_PRECOMP
    #include "wx/intl.h"
    #include "wx/log.h"
    #include "wx/app.h"
    #include "wx/utils.h"
    #include "wx/arrstr.h"
    #include "wx/math.h"
#endif //WX_PRECOMP

#if wxUSE_CONFIG && ((wxUSE_FILE && wxUSE_TEXTFILE) || wxUSE_CONFIG_NATIVE)

#include "wx/file.h"

#include <stdlib.h>
#include <ctype.h>
#include <limits.h>     // for INT_MAX

// ----------------------------------------------------------------------------
// global and class static variables
// ----------------------------------------------------------------------------

wxConfigBase *wxConfigBase::ms_pConfig     = NULL;
bool          wxConfigBase::ms_bAutoCreate = true;

// ============================================================================
// implementation
// ============================================================================

// ----------------------------------------------------------------------------
// wxConfigBase
// ----------------------------------------------------------------------------

// Not all args will always be used by derived classes, but including them all
// in each class ensures compatibility.
wxConfigBase::wxConfigBase(const wxString& appName,
                           const wxString& vendorName,
                           const wxString& WXUNUSED(localFilename),
                           const wxString& WXUNUSED(globalFilename),
                           long style)
            : m_appName(appName), m_vendorName(vendorName), m_style(style)
{
    m_bExpandEnvVars = true;
    m_bRecordDefaults = false;
}

wxConfigBase::~wxConfigBase()
{
    // required here for Darwin
}

wxConfigBase *wxConfigBase::Set(wxConfigBase *pConfig)
{
  wxConfigBase *pOld = ms_pConfig;
  ms_pConfig = pConfig;
  return pOld;
}

wxConfigBase *wxConfigBase::Create()
{
  if ( ms_bAutoCreate && ms_pConfig == NULL ) {
    ms_pConfig =
    #if defined(__WXMSW__) && wxUSE_CONFIG_NATIVE
        new wxRegConfig(wxTheApp->GetAppName(), wxTheApp->GetVendorName());
    #elif defined(__WXPALMOS__) && wxUSE_CONFIG_NATIVE
        new wxPrefConfig(wxTheApp->GetAppName());
    #else // either we're under Unix or wish to use files even under Windows
      new wxFileConfig(wxTheApp->GetAppName());
    #endif
  }

  return ms_pConfig;
}

// ----------------------------------------------------------------------------
// wxConfigBase reading entries
// ----------------------------------------------------------------------------

// implement both Read() overloads for the given type in terms of DoRead()
#define IMPLEMENT_READ_FOR_TYPE(name, type, deftype, extra)                 \
    bool wxConfigBase::Read(const wxString& key, type *val) const           \
    {                                                                       \
        wxCHECK_MSG( val, false, _T("wxConfig::Read(): NULL parameter") );  \
                                                                            \
        if ( !DoRead##name(key, val) )                                      \
            return false;                                                   \
                                                                            \
        *val = extra(*val);                                                 \
                                                                            \
        return true;                                                        \
    }                                                                       \
                                                                            \
    bool wxConfigBase::Read(const wxString& key,                            \
                            type *val,                                      \
                            deftype defVal) const                           \
    {                                                                       \
        wxCHECK_MSG( val, false, _T("wxConfig::Read(): NULL parameter") );  \
                                                                            \
        bool read = DoRead##name(key, val);                                 \
        if ( !read )                                                        \
        {                                                                   \
            if ( IsRecordingDefaults() )                                    \
            {                                                               \
                ((wxConfigBase *)this)->DoWrite##name(key, defVal);         \
            }                                                               \
                                                                            \
            *val = defVal;                                                  \
        }                                                                   \
                                                                            \
        *val = extra(*val);                                                 \
                                                                            \
        return read;                                                        \
    }


IMPLEMENT_READ_FOR_TYPE(String, wxString, const wxString&, ExpandEnvVars)
IMPLEMENT_READ_FOR_TYPE(Long, long, long, long)
IMPLEMENT_READ_FOR_TYPE(Int, int, int, int)
IMPLEMENT_READ_FOR_TYPE(Double, double, double, double)
IMPLEMENT_READ_FOR_TYPE(Bool, bool, bool, bool)

#undef IMPLEMENT_READ_FOR_TYPE

// the DoReadXXX() for the other types have implementation in the base class
// but can be overridden in the derived ones
bool wxConfigBase::DoReadInt(const wxString& key, int *pi) const
{
    wxCHECK_MSG( pi, false, _T("wxConfig::Read(): NULL parameter") );

    long l;
    if ( !DoReadLong(key, &l) )
        return false;

    wxASSERT_MSG( l < INT_MAX, _T("overflow in wxConfig::DoReadInt") );

    *pi = (int)l;

    return true;
}

bool wxConfigBase::DoReadBool(const wxString& key, bool* val) const
{
    wxCHECK_MSG( val, false, _T("wxConfig::Read(): NULL parameter") );

    long l;
    if ( !DoReadLong(key, &l) )
        return false;

    wxASSERT_MSG( l == 0 || l == 1, _T("bad bool value in wxConfig::DoReadInt") );

    *val = l != 0;

    return true;
}

bool wxConfigBase::DoReadDouble(const wxString& key, double* val) const
{
    wxString str;
    if ( Read(key, &str) )
    {
        return str.ToDouble(val);
    }

    return false;
}

// string reading helper
wxString wxConfigBase::ExpandEnvVars(const wxString& str) const
{
    wxString tmp; // Required for BC++
    if (IsExpandingEnvVars())
        tmp = wxExpandEnvVars(str);
    else
        tmp = str;
    return tmp;
}

// ----------------------------------------------------------------------------
// wxConfigBase writing
// ----------------------------------------------------------------------------

bool wxConfigBase::DoWriteDouble(const wxString& key, double val)
{
    return DoWriteString(key, wxString::Format(_T("%g"), val));
}

bool wxConfigBase::DoWriteInt(const wxString& key, int value)
{
    return DoWriteLong(key, (long)value);
}

bool wxConfigBase::DoWriteBool(const wxString& key, bool value)
{
    return DoWriteLong(key, value ? 1l : 0l);
}

// ----------------------------------------------------------------------------
// wxConfigPathChanger
// ----------------------------------------------------------------------------

wxConfigPathChanger::wxConfigPathChanger(const wxConfigBase *pContainer,
                                         const wxString& strEntry)
{
  m_bChanged = false;
  m_pContainer = (wxConfigBase *)pContainer;

  // the path is everything which precedes the last slash
  wxString strPath = strEntry.BeforeLast(wxCONFIG_PATH_SEPARATOR);

  // except in the special case of "/keyname" when there is nothing before "/"
  if ( strPath.empty() &&
       ((!strEntry.empty()) && strEntry[0] == wxCONFIG_PATH_SEPARATOR) )
  {
    strPath = wxCONFIG_PATH_SEPARATOR;
  }

  if ( !strPath.empty() )
  {
    if ( m_pContainer->GetPath() != strPath )
    {
        // we do change the path so restore it later
        m_bChanged = true;

        /* JACS: work around a memory bug that causes an assert
           when using wxRegConfig, related to reference-counting.
           Can be reproduced by removing (const wxChar*) below and
           adding the following code to the config sample OnInit under
           Windows:

           pConfig->SetPath(wxT("MySettings"));
           pConfig->SetPath(wxT(".."));
           int value;
           pConfig->Read(_T("MainWindowX"), & value);
        */
        m_strOldPath = (const wxChar*) m_pContainer->GetPath();
        if ( *m_strOldPath.c_str() != wxCONFIG_PATH_SEPARATOR )
          m_strOldPath += wxCONFIG_PATH_SEPARATOR;
        m_pContainer->SetPath(strPath);
    }

    // in any case, use the just the name, not full path
    m_strName = strEntry.AfterLast(wxCONFIG_PATH_SEPARATOR);
  }
  else {
    // it's a name only, without path - nothing to do
    m_strName = strEntry;
  }
}

void wxConfigPathChanger::UpdateIfDeleted()
{
    // we don't have to do anything at all if we didn't change the path
    if ( !m_bChanged )
        return;

    // find the deepest still existing parent path of the original path
    while ( !m_pContainer->HasGroup(m_strOldPath) )
    {
        m_strOldPath = m_strOldPath.BeforeLast(wxCONFIG_PATH_SEPARATOR);
        if ( m_strOldPath.empty() )
            m_strOldPath = wxCONFIG_PATH_SEPARATOR;
    }
}

wxConfigPathChanger::~wxConfigPathChanger()
{
  // only restore path if it was changed
  if ( m_bChanged ) {
    m_pContainer->SetPath(m_strOldPath);
  }
}

// this is a wxConfig method but it's mainly used with wxConfigPathChanger
/* static */
wxString wxConfigBase::RemoveTrailingSeparator(const wxString& key)
{
    wxString path(key);

    // don't remove the only separator from a root group path!
    while ( path.length() > 1 )
    {
        if ( *path.rbegin() != wxCONFIG_PATH_SEPARATOR )
            break;

        path.erase(path.end() - 1);
    }

    return path;
}

#endif // wxUSE_CONFIG

// ----------------------------------------------------------------------------
// static & global functions
// ----------------------------------------------------------------------------

// understands both Unix and Windows (but only under Windows) environment
// variables expansion: i.e. $var, $(var) and ${var} are always understood
// and in addition under Windows %var% is also.

// don't change the values the enum elements: they must be equal
// to the matching [closing] delimiter.
enum Bracket
{
  Bracket_None,
  Bracket_Normal  = ')',
  Bracket_Curly   = '}',
#ifdef  __WXMSW__
  Bracket_Windows = '%',    // yeah, Windows people are a bit strange ;-)
#endif
  Bracket_Max
};

wxString wxExpandEnvVars(const wxString& str)
{
  wxString strResult;
  strResult.Alloc(str.length());

  size_t m;
  for ( size_t n = 0; n < str.length(); n++ ) {
    switch ( str[n] ) {
#ifdef  __WXMSW__
      case wxT('%'):
#endif  //WINDOWS
      case wxT('$'):
        {
          Bracket bracket;
          #ifdef  __WXMSW__
            if ( str[n] == wxT('%') )
              bracket = Bracket_Windows;
            else
          #endif  //WINDOWS
          if ( n == str.length() - 1 ) {
            bracket = Bracket_None;
          }
          else {
            switch ( str[n + 1] ) {
              case wxT('('):
                bracket = Bracket_Normal;
                n++;                   // skip the bracket
                break;

              case wxT('{'):
                bracket = Bracket_Curly;
                n++;                   // skip the bracket
                break;

              default:
                bracket = Bracket_None;
            }
          }

          m = n + 1;

          while ( m < str.length() && (wxIsalnum(str[m]) || str[m] == wxT('_')) )
            m++;

          wxString strVarName(str.c_str() + n + 1, m - n - 1);

#ifdef __WXWINCE__
          const wxChar *pszValue = NULL;
#else
          // NB: use wxGetEnv instead of wxGetenv as otherwise variables
          //     set through wxSetEnv may not be read correctly!
          const wxChar *pszValue = NULL;
          wxString tmp;
          if (wxGetEnv(strVarName, &tmp))
              pszValue = tmp;
#endif
          if ( pszValue != NULL ) {
            strResult += pszValue;
          }
          else {
            // variable doesn't exist => don't change anything
            #ifdef  __WXMSW__
              if ( bracket != Bracket_Windows )
            #endif
                if ( bracket != Bracket_None )
                  strResult << str[n - 1];
            strResult << str[n] << strVarName;
          }

          // check the closing bracket
          if ( bracket != Bracket_None ) {
            if ( m == str.length() || str[m] != (wxChar)bracket ) {
              // under MSW it's common to have '%' characters in the registry
              // and it's annoying to have warnings about them each time, so
              // ignroe them silently if they are not used for env vars
              //
              // under Unix, OTOH, this warning could be useful for the user to
              // understand why isn't the variable expanded as intended
              #ifndef __WXMSW__
                wxLogWarning(_("Environment variables expansion failed: missing '%c' at position %u in '%s'."),
                             (char)bracket, (unsigned int) (m + 1), str.c_str());
              #endif // __WXMSW__
            }
            else {
              // skip closing bracket unless the variables wasn't expanded
              if ( pszValue == NULL )
                strResult << (wxChar)bracket;
              m++;
            }
          }

          n = m - 1;  // skip variable name
        }
        break;

      case '\\':
        // backslash can be used to suppress special meaning of % and $
        if ( n != str.length() - 1 &&
                (str[n + 1] == wxT('%') || str[n + 1] == wxT('$')) ) {
          strResult += str[++n];

          break;
        }
        //else: fall through

      default:
        strResult += str[n];
    }
  }

  return strResult;
}

// this function is used to properly interpret '..' in path
void wxSplitPath(wxArrayString& aParts, const wxChar *sz)
{
  aParts.clear();

  wxString strCurrent;
  const wxChar *pc = sz;
  for ( ;; ) {
    if ( *pc == wxT('\0') || *pc == wxCONFIG_PATH_SEPARATOR ) {
      if ( strCurrent == wxT(".") ) {
        // ignore
      }
      else if ( strCurrent == wxT("..") ) {
        // go up one level
        if ( aParts.size() == 0 )
          wxLogWarning(_("'%s' has extra '..', ignored."), sz);
        else
          aParts.erase(aParts.end() - 1);

        strCurrent.Empty();
      }
      else if ( !strCurrent.empty() ) {
        aParts.push_back(strCurrent);
        strCurrent.Empty();
      }
      //else:
        // could log an error here, but we prefer to ignore extra '/'

      if ( *pc == wxT('\0') )
        break;
    }
    else
      strCurrent += *pc;

    pc++;
  }
}
