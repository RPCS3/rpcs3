///////////////////////////////////////////////////////////////////////////////
// Name:        src/msw/regconf.cpp
// Purpose:
// Author:      Vadim Zeitlin
// Modified by:
// Created:     27.04.98
// RCS-ID:      $Id: regconf.cpp 44126 2007-01-07 16:36:54Z VZ $
// Copyright:   (c) 1998 Vadim Zeitlin <zeitlin@dptmaths.ens-cachan.fr>
// Licence:     wxWindows licence
///////////////////////////////////////////////////////////////////////////////

// For compilers that support precompilation, includes "wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#if wxUSE_CONFIG

#include "wx/config.h"

#ifndef WX_PRECOMP
    #include  "wx/string.h"
    #include  "wx/intl.h"
    #include "wx/log.h"
    #include "wx/event.h"
    #include "wx/app.h"
#endif //WX_PRECOMP

#include "wx/msw/registry.h"
#include "wx/msw/regconf.h"

// ----------------------------------------------------------------------------
// constants
// ----------------------------------------------------------------------------

// we put our data in HKLM\SOFTWARE_KEY\appname
#define SOFTWARE_KEY    wxString(wxT("Software\\"))

// ----------------------------------------------------------------------------
// global functions
// ----------------------------------------------------------------------------

// get the value if the key is opened and it exists
bool TryGetValue(const wxRegKey& key, const wxString& str, wxString& strVal)
{
  return key.IsOpened() && key.HasValue(str) && key.QueryValue(str, strVal);
}

bool TryGetValue(const wxRegKey& key, const wxString& str, long *plVal)
{
  return key.IsOpened() && key.HasValue(str) && key.QueryValue(str, plVal);
}

// ============================================================================
// implementation
// ============================================================================

// ----------------------------------------------------------------------------
// ctor/dtor
// ----------------------------------------------------------------------------

// create the config object which stores its data under HKCU\vendor\app and, if
// style & wxCONFIG_USE_GLOBAL_FILE, under HKLM\vendor\app
wxRegConfig::wxRegConfig(const wxString& appName, const wxString& vendorName,
                         const wxString& strLocal, const wxString& strGlobal,
                         long style)
           : wxConfigBase(appName, vendorName, strLocal, strGlobal, style)
{
  wxString strRoot;

  bool bDoUseGlobal = (style & wxCONFIG_USE_GLOBAL_FILE) != 0;

  // the convention is to put the programs keys under <vendor>\<appname>
  // (but it can be overriden by specifying the pathes explicitly in strLocal
  // and/or strGlobal)
  if ( strLocal.empty() || (strGlobal.empty() && bDoUseGlobal) )
  {
    if ( vendorName.empty() )
    {
      if ( wxTheApp )
        strRoot = wxTheApp->GetVendorName();
    }
    else
    {
      strRoot = vendorName;
    }

    // no '\\' needed if no vendor name
    if ( !strRoot.empty() )
    {
      strRoot += '\\';
    }

    if ( appName.empty() )
    {
      wxCHECK_RET( wxTheApp, wxT("No application name in wxRegConfig ctor!") );
      strRoot << wxTheApp->GetAppName();
    }
    else
    {
      strRoot << appName;
    }
  }
  //else: we don't need to do all the complicated stuff above

  wxString str = strLocal.empty() ? strRoot : strLocal;

  // as we're going to change the name of these keys fairly often and as
  // there are only few of wxRegConfig objects (usually 1), we can allow
  // ourselves to be generous and spend some memory to significantly improve
  // performance of SetPath()
  static const size_t MEMORY_PREALLOC = 512;

  m_keyLocalRoot.ReserveMemoryForName(MEMORY_PREALLOC);
  m_keyLocal.ReserveMemoryForName(MEMORY_PREALLOC);

  m_keyLocalRoot.SetName(wxRegKey::HKCU, SOFTWARE_KEY + str);
  m_keyLocal.SetName(m_keyLocalRoot, wxEmptyString);

  if ( bDoUseGlobal )
  {
    str = strGlobal.empty() ? strRoot : strGlobal;

    m_keyGlobalRoot.ReserveMemoryForName(MEMORY_PREALLOC);
    m_keyGlobal.ReserveMemoryForName(MEMORY_PREALLOC);

    m_keyGlobalRoot.SetName(wxRegKey::HKLM, SOFTWARE_KEY + str);
    m_keyGlobal.SetName(m_keyGlobalRoot, wxEmptyString);
  }

  // Create() will Open() if key already exists
  m_keyLocalRoot.Create();

  // as it's the same key, Open() shouldn't fail (i.e. no need for Create())
  m_keyLocal.Open();

  // OTOH, this key may perfectly not exist, so suppress error messages the call
  // to Open() might generate
  if ( bDoUseGlobal )
  {
    wxLogNull nolog;
    m_keyGlobalRoot.Open(wxRegKey::Read);
    m_keyGlobal.Open(wxRegKey::Read);
  }
}

// ----------------------------------------------------------------------------
// path management
// ----------------------------------------------------------------------------

// this function is called a *lot* of times (as I learned after seeing from
// profiler output that it is called ~12000 times from Mahogany start up code!)
// so it is important to optimize it - in particular, avoid using generic
// string functions here and do everything manually because it is faster
//
// I still kept the old version to be able to check that the optimized code has
// the same output as the non optimized version.
void wxRegConfig::SetPath(const wxString& strPath)
{
    // remember the old path
    wxString strOldPath = m_strPath;

#ifdef WX_DEBUG_SET_PATH // non optimized version kept here for testing
    wxString m_strPathAlt;

    {
        wxArrayString aParts;

        // because GetPath() returns "" when we're at root, we must understand
        // empty string as "/"
        if ( strPath.empty() || (strPath[0] == wxCONFIG_PATH_SEPARATOR) ) {
            // absolute path
            wxSplitPath(aParts, strPath);
        }
        else {
            // relative path, combine with current one
            wxString strFullPath = GetPath();
            strFullPath << wxCONFIG_PATH_SEPARATOR << strPath;
            wxSplitPath(aParts, strFullPath);
        }

        // recombine path parts in one variable
        wxString strRegPath;
        m_strPathAlt.Empty();
        for ( size_t n = 0; n < aParts.Count(); n++ ) {
            strRegPath << '\\' << aParts[n];
            m_strPathAlt << wxCONFIG_PATH_SEPARATOR << aParts[n];
        }
    }
#endif // 0

    // check for the most common case first
    if ( strPath.empty() )
    {
        m_strPath = wxCONFIG_PATH_SEPARATOR;
    }
    else // not root
    {
        // construct the full path
        wxString strFullPath;
        if ( strPath[0u] == wxCONFIG_PATH_SEPARATOR )
        {
            // absolute path
            strFullPath = strPath;
        }
        else // relative path
        {
            strFullPath.reserve(2*m_strPath.length());

            strFullPath << m_strPath;
            if ( strFullPath.Len() == 0 ||
                 strFullPath.Last() != wxCONFIG_PATH_SEPARATOR )
                strFullPath << wxCONFIG_PATH_SEPARATOR;
            strFullPath << strPath;
        }

        // simplify it: we need to handle ".." here

        // count the total number of slashes we have to know if we can go upper
        size_t totalSlashes = 0;

        // position of the last slash to be able to backtrack to it quickly if
        // needed, but we set it to -1 if we don't have a valid position
        //
        // we only remember the last position which means that we handle ".."
        // quite efficiently but not "../.." - however the latter should be
        // much more rare, so it is probably ok
        int posLastSlash = -1;

        const wxChar *src = strFullPath.c_str();
        size_t len = strFullPath.length();
        const wxChar *end = src + len;

        wxStringBufferLength buf(m_strPath, len);
        wxChar *dst = buf;
        wxChar *start = dst;

        for ( ; src < end; src++, dst++ )
        {
            if ( *src == wxCONFIG_PATH_SEPARATOR )
            {
                // check for "/.."

                // note that we don't have to check for src < end here as
                // *end == 0 so can't be '.'
                if ( src[1] == _T('.') && src[2] == _T('.') &&
                     (src + 3 == end || src[3] == wxCONFIG_PATH_SEPARATOR) )
                {
                    if ( !totalSlashes )
                    {
                        wxLogWarning(_("'%s' has extra '..', ignored."),
                                     strFullPath.c_str());
                    }
                    else // return to the previous path component
                    {
                        // do we already have its position?
                        if ( posLastSlash == -1 )
                        {
                            // no, find it: note that we are sure to have one
                            // because totalSlashes > 0 so we don't have to
                            // check the boundary condition below

                            // this is more efficient than strrchr()
                            dst--;
                            while ( *dst != wxCONFIG_PATH_SEPARATOR )
                            {
                                dst--;
                            }
                        }
                        else // the position of last slash was stored
                        {
                            // go directly there
                            dst = start + posLastSlash;

                            // invalidate posLastSlash
                            posLastSlash = -1;
                        }

                        // we must have found a slash one way or another!
                        wxASSERT_MSG( *dst == wxCONFIG_PATH_SEPARATOR,
                                      _T("error in wxRegConfig::SetPath") );

                        // stay at the same position
                        dst--;

                        // we killed one
                        totalSlashes--;
                    }

                    // skip both dots
                    src += 2;
                }
                else // not "/.."
                {
                    if ( (dst == start) || (dst[-1] != wxCONFIG_PATH_SEPARATOR) )
                    {
                        *dst = wxCONFIG_PATH_SEPARATOR;

                        posLastSlash = dst - start;

                        totalSlashes++;
                    }
                    else // previous char was a slash too
                    {
                        // squeeze several subsequent slashes into one: i.e.
                        // just ignore this one
                        dst--;
                    }
                }
            }
            else // normal character
            {
                // just copy
                *dst = *src;
            }
        }

        // NUL terminate the string
        if ( dst[-1] == wxCONFIG_PATH_SEPARATOR && (dst != start + 1) )
        {
            // if it has a trailing slash we remove it unless it is the only
            // string character
            dst--;
        }

        *dst = _T('\0');
        buf.SetLength(dst - start);
    }

#ifdef WX_DEBUG_SET_PATH
    wxASSERT( m_strPath == m_strPathAlt );
#endif

    if ( m_strPath == strOldPath )
        return;

    // registry APIs want backslashes instead of slashes
    wxString strRegPath;
    if ( !m_strPath.empty() )
    {
        size_t len = m_strPath.length();

        const wxChar *src = m_strPath.c_str();
        wxStringBufferLength buf(strRegPath, len);
        wxChar *dst = buf;

        const wxChar *end = src + len;
        for ( ; src < end; src++, dst++ )
        {
            if ( *src == wxCONFIG_PATH_SEPARATOR )
                *dst = _T('\\');
            else
                *dst = *src;
        }

        buf.SetLength(len);
    }

    // this is not needed any longer as we don't create keys unnecessarily any
    // more (now it is done on demand, i.e. only when they're going to contain
    // something)
#if 0
    // as we create the registry key when SetPath(key) is done, we can be left
    // with plenty of empty keys if this was only done to try to read some
    // value which, in fact, doesn't exist - to prevent this from happening we
    // automatically delete the old key if it was empty
    if ( m_keyLocal.Exists() && LocalKey().IsEmpty() )
    {
        m_keyLocal.DeleteSelf();
    }
#endif // 0

    // change current key(s)
    m_keyLocal.SetName(m_keyLocalRoot, strRegPath);

    if ( GetStyle() & wxCONFIG_USE_GLOBAL_FILE )
    {
      m_keyGlobal.SetName(m_keyGlobalRoot, strRegPath);

      wxLogNull nolog;
      m_keyGlobal.Open(wxRegKey::Read);
    }
}

// ----------------------------------------------------------------------------
// enumeration (works only with current group)
// ----------------------------------------------------------------------------

/*
  We want to enumerate all local keys/values after the global ones, but, of
  course, we don't want to repeat a key which appears locally as well as
  globally twice.

  We use the 15th bit of lIndex for distinction between global and local.
 */

#define LOCAL_MASK        0x8000
#define IS_LOCAL_INDEX(l) (((l) & LOCAL_MASK) != 0)

bool wxRegConfig::GetFirstGroup(wxString& str, long& lIndex) const
{
  lIndex = 0;
  return GetNextGroup(str, lIndex);
}

bool wxRegConfig::GetNextGroup(wxString& str, long& lIndex) const
{
  // are we already enumerating local entries?
  if ( m_keyGlobal.IsOpened() && !IS_LOCAL_INDEX(lIndex) ) {
    // try to find a global entry which doesn't appear locally
    while ( m_keyGlobal.GetNextKey(str, lIndex) ) {
      if ( !m_keyLocal.Exists() || !LocalKey().HasSubKey(str) ) {
        // ok, found one - return it
        return true;
      }
    }

    // no more global entries
    lIndex |= LOCAL_MASK;
  }

  // if we don't have the key at all, don't try to enumerate anything under it
  if ( !m_keyLocal.Exists() )
      return false;

  // much easier with local entries: get the next one we find
  // (don't forget to clear our flag bit and set it again later)
  lIndex &= ~LOCAL_MASK;
  bool bOk = LocalKey().GetNextKey(str, lIndex);
  lIndex |= LOCAL_MASK;

  return bOk;
}

bool wxRegConfig::GetFirstEntry(wxString& str, long& lIndex) const
{
  lIndex = 0;
  return GetNextEntry(str, lIndex);
}

bool wxRegConfig::GetNextEntry(wxString& str, long& lIndex) const
{
  // are we already enumerating local entries?
  if ( m_keyGlobal.IsOpened() && !IS_LOCAL_INDEX(lIndex) ) {
    // try to find a global entry which doesn't appear locally
    while ( m_keyGlobal.GetNextValue(str, lIndex) ) {
      if ( !m_keyLocal.Exists() || !LocalKey().HasValue(str) ) {
        // ok, found one - return it
        return true;
      }
    }

    // no more global entries
    lIndex |= LOCAL_MASK;
  }

  // if we don't have the key at all, don't try to enumerate anything under it
  if ( !m_keyLocal.Exists() )
      return false;

  // much easier with local entries: get the next one we find
  // (don't forget to clear our flag bit and set it again later)
  lIndex &= ~LOCAL_MASK;
  bool bOk = LocalKey().GetNextValue(str, lIndex);
  lIndex |= LOCAL_MASK;

  return bOk;
}

size_t wxRegConfig::GetNumberOfEntries(bool WXUNUSED(bRecursive)) const
{
  size_t nEntries = 0;

  // dummy vars
  wxString str;
  long l;
  bool bCont = ((wxRegConfig*)this)->GetFirstEntry(str, l);
  while ( bCont ) {
    nEntries++;

    bCont = ((wxRegConfig*)this)->GetNextEntry(str, l);
  }

  return nEntries;
}

size_t wxRegConfig::GetNumberOfGroups(bool WXUNUSED(bRecursive)) const
{
  size_t nGroups = 0;

  // dummy vars
  wxString str;
  long l;
  bool bCont = ((wxRegConfig*)this)->GetFirstGroup(str, l);
  while ( bCont ) {
    nGroups++;

    bCont = ((wxRegConfig*)this)->GetNextGroup(str, l);
  }

  return nGroups;
}

// ----------------------------------------------------------------------------
// tests for existence
// ----------------------------------------------------------------------------

bool wxRegConfig::HasGroup(const wxString& key) const
{
    wxConfigPathChanger path(this, key);

    wxString strName(path.Name());

    return (m_keyLocal.Exists() && LocalKey().HasSubKey(strName)) ||
           m_keyGlobal.HasSubKey(strName);
}

bool wxRegConfig::HasEntry(const wxString& key) const
{
    wxConfigPathChanger path(this, key);

    wxString strName(path.Name());

    return (m_keyLocal.Exists() && LocalKey().HasValue(strName)) ||
           m_keyGlobal.HasValue(strName);
}

wxConfigBase::EntryType wxRegConfig::GetEntryType(const wxString& key) const
{
    wxConfigPathChanger path(this, key);

    wxString strName(path.Name());

    bool isNumeric;
    if ( m_keyLocal.Exists() && LocalKey().HasValue(strName) )
        isNumeric = m_keyLocal.IsNumericValue(strName);
    else if ( m_keyGlobal.HasValue(strName) )
        isNumeric = m_keyGlobal.IsNumericValue(strName);
    else
        return wxConfigBase::Type_Unknown;

    return isNumeric ? wxConfigBase::Type_Integer : wxConfigBase::Type_String;
}

// ----------------------------------------------------------------------------
// reading/writing
// ----------------------------------------------------------------------------

bool wxRegConfig::DoReadString(const wxString& key, wxString *pStr) const
{
    wxCHECK_MSG( pStr, false, _T("wxRegConfig::Read(): NULL param") );

  wxConfigPathChanger path(this, key);

  bool bQueryGlobal = true;

  // if immutable key exists in global key we must check that it's not
  // overriden by the local key with the same name
  if ( IsImmutable(path.Name()) ) {
    if ( TryGetValue(m_keyGlobal, path.Name(), *pStr) ) {
      if ( m_keyLocal.Exists() && LocalKey().HasValue(path.Name()) ) {
        wxLogWarning(wxT("User value for immutable key '%s' ignored."),
                   path.Name().c_str());
      }

      return true;
    }
    else {
      // don't waste time - it's not there anyhow
      bQueryGlobal = false;
    }
  }

  // first try local key
  if ( (m_keyLocal.Exists() && TryGetValue(LocalKey(), path.Name(), *pStr)) ||
       (bQueryGlobal && TryGetValue(m_keyGlobal, path.Name(), *pStr)) ) {
    return true;
  }

  return false;
}

// this exactly reproduces the string version above except for ExpandEnvVars(),
// we really should avoid this code duplication somehow...

bool wxRegConfig::DoReadLong(const wxString& key, long *plResult) const
{
    wxCHECK_MSG( plResult, false, _T("wxRegConfig::Read(): NULL param") );

  wxConfigPathChanger path(this, key);

  bool bQueryGlobal = true;

  // if immutable key exists in global key we must check that it's not
  // overriden by the local key with the same name
  if ( IsImmutable(path.Name()) ) {
    if ( TryGetValue(m_keyGlobal, path.Name(), plResult) ) {
      if ( m_keyLocal.Exists() && LocalKey().HasValue(path.Name()) ) {
        wxLogWarning(wxT("User value for immutable key '%s' ignored."),
                     path.Name().c_str());
      }

      return true;
    }
    else {
      // don't waste time - it's not there anyhow
      bQueryGlobal = false;
    }
  }

  // first try local key
  if ( (m_keyLocal.Exists() && TryGetValue(LocalKey(), path.Name(), plResult)) ||
       (bQueryGlobal && TryGetValue(m_keyGlobal, path.Name(), plResult)) ) {
    return true;
  }

  return false;
}

bool wxRegConfig::DoWriteString(const wxString& key, const wxString& szValue)
{
  wxConfigPathChanger path(this, key);

  if ( IsImmutable(path.Name()) ) {
    wxLogError(wxT("Can't change immutable entry '%s'."), path.Name().c_str());
    return false;
  }

  return LocalKey().SetValue(path.Name(), szValue);
}

bool wxRegConfig::DoWriteLong(const wxString& key, long lValue)
{
  wxConfigPathChanger path(this, key);

  if ( IsImmutable(path.Name()) ) {
    wxLogError(wxT("Can't change immutable entry '%s'."), path.Name().c_str());
    return false;
  }

  return LocalKey().SetValue(path.Name(), lValue);
}

// ----------------------------------------------------------------------------
// renaming
// ----------------------------------------------------------------------------

bool wxRegConfig::RenameEntry(const wxString& oldName, const wxString& newName)
{
    // check that the old entry exists...
    if ( !HasEntry(oldName) )
        return false;

    // and that the new one doesn't
    if ( HasEntry(newName) )
        return false;

    return m_keyLocal.RenameValue(oldName, newName);
}

bool wxRegConfig::RenameGroup(const wxString& oldName, const wxString& newName)
{
    // check that the old group exists...
    if ( !HasGroup(oldName) )
        return false;

    // and that the new one doesn't
    if ( HasGroup(newName) )
        return false;

    return wxRegKey(m_keyLocal, oldName).Rename(newName);
}

// ----------------------------------------------------------------------------
// deleting
// ----------------------------------------------------------------------------

bool wxRegConfig::DeleteEntry(const wxString& value, bool bGroupIfEmptyAlso)
{
  wxConfigPathChanger path(this, value);

  if ( m_keyLocal.Exists() ) {
    if ( !m_keyLocal.DeleteValue(path.Name()) )
      return false;

    if ( bGroupIfEmptyAlso && m_keyLocal.IsEmpty() ) {
      wxString strKey = GetPath().AfterLast(wxCONFIG_PATH_SEPARATOR);
      SetPath(_T(".."));  // changes m_keyLocal
      return LocalKey().DeleteKey(strKey);
    }
  }

  return true;
}

bool wxRegConfig::DeleteGroup(const wxString& key)
{
  wxConfigPathChanger path(this, RemoveTrailingSeparator(key));

  if ( !m_keyLocal.Exists() )
  {
      // nothing to do
      return true;
  }

  if ( !LocalKey().DeleteKey(path.Name()) )
      return false;

  path.UpdateIfDeleted();

  return true;
}

bool wxRegConfig::DeleteAll()
{
  m_keyLocal.Close();
  m_keyGlobal.Close();

  bool bOk = m_keyLocalRoot.DeleteSelf();

  // make sure that we opened m_keyGlobalRoot and so it has a reasonable name:
  // otherwise we will delete HKEY_CLASSES_ROOT recursively
  if ( bOk && m_keyGlobalRoot.IsOpened() )
    bOk = m_keyGlobalRoot.DeleteSelf();

  return bOk;
}

#endif // wxUSE_CONFIG
