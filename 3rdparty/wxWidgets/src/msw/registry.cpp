///////////////////////////////////////////////////////////////////////////////
// Name:        src/msw/registry.cpp
// Purpose:     implementation of registry classes and functions
// Author:      Vadim Zeitlin
// Modified by:
// Created:     03.04.98
// RCS-ID:      $Id: registry.cpp 47482 2007-07-15 14:12:08Z VS $
// Copyright:   (c) 1998 Vadim Zeitlin <zeitlin@dptmaths.ens-cachan.fr>
// Licence:     wxWindows licence
// TODO:        - parsing of registry key names
//              - support of other (than REG_SZ/REG_DWORD) registry types
//              - add high level functions (RegisterOleServer, ...)
///////////////////////////////////////////////////////////////////////////////

// for compilers that support precompilation, includes "wx.h".
#include  "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#ifndef WX_PRECOMP
    #include "wx/msw/wrapwin.h"
    #include "wx/string.h"
    #include "wx/intl.h"
    #include "wx/log.h"
#endif

#include "wx/file.h"
#include "wx/wfstream.h"

// Windows headers
#ifdef __WXWINCE__
#include "wx/msw/private.h"
#include <winbase.h>
#include <winreg.h>
#endif

// other std headers
#include  <stdlib.h>      // for _MAX_PATH

#ifndef _MAX_PATH
    #define _MAX_PATH 512
#endif

// our header
#define   HKEY_DEFINED    // already defined in windows.h
#include  "wx/msw/registry.h"

// some registry functions don't like signed chars
typedef unsigned char *RegString;
typedef BYTE* RegBinary;

#ifndef HKEY_PERFORMANCE_DATA
    #define HKEY_PERFORMANCE_DATA ((HKEY)0x80000004)
#endif

#ifndef HKEY_CURRENT_CONFIG
    #define HKEY_CURRENT_CONFIG ((HKEY)0x80000005)
#endif

#ifndef HKEY_DYN_DATA
    #define HKEY_DYN_DATA ((HKEY)0x80000006)
#endif

// ----------------------------------------------------------------------------
// constants
// ----------------------------------------------------------------------------

// the standard key names, short names and handles all bundled together for
// convenient access
static struct
{
  HKEY        hkey;
  const wxChar *szName;
  const wxChar *szShortName;
}
aStdKeys[] =
{
  { HKEY_CLASSES_ROOT,      wxT("HKEY_CLASSES_ROOT"),      wxT("HKCR") },
  { HKEY_CURRENT_USER,      wxT("HKEY_CURRENT_USER"),      wxT("HKCU") },
  { HKEY_LOCAL_MACHINE,     wxT("HKEY_LOCAL_MACHINE"),     wxT("HKLM") },
  { HKEY_USERS,             wxT("HKEY_USERS"),             wxT("HKU")  }, // short name?
  { HKEY_PERFORMANCE_DATA,  wxT("HKEY_PERFORMANCE_DATA"),  wxT("HKPD") },
  { HKEY_CURRENT_CONFIG,    wxT("HKEY_CURRENT_CONFIG"),    wxT("HKCC") },
  { HKEY_DYN_DATA,          wxT("HKEY_DYN_DATA"),          wxT("HKDD") }, // short name?
};

// the registry name separator (perhaps one day MS will change it to '/' ;-)
#define   REG_SEPARATOR     wxT('\\')

// useful for Windows programmers: makes somewhat more clear all these zeroes
// being passed to Windows APIs
#define   RESERVED        (0)

// ----------------------------------------------------------------------------
// macros
// ----------------------------------------------------------------------------

// const_cast<> is not yet supported by all compilers
#define CONST_CAST    ((wxRegKey *)this)->

// and neither is mutable which m_dwLastError should be
#define m_dwLastError   CONST_CAST m_dwLastError

// ----------------------------------------------------------------------------
// non member functions
// ----------------------------------------------------------------------------

// removes the trailing backslash from the string if it has one
static inline void RemoveTrailingSeparator(wxString& str);

// returns true if given registry key exists
static bool KeyExists(WXHKEY hRootKey, const wxChar *szKey);

// combines value and key name (uses static buffer!)
static const wxChar *GetFullName(const wxRegKey *pKey,
                               const wxChar *szValue = NULL);

static inline const wxChar *RegValueStr(const wxChar *szValue)
{
    return wxIsEmpty(szValue) ? NULL : szValue;
}

// ============================================================================
// implementation of wxRegKey class
// ============================================================================

// ----------------------------------------------------------------------------
// static functions and variables
// ----------------------------------------------------------------------------

const size_t wxRegKey::nStdKeys = WXSIZEOF(aStdKeys);

// @@ should take a `StdKey key', but as it's often going to be used in loops
//    it would require casts in user code.
const wxChar *wxRegKey::GetStdKeyName(size_t key)
{
  // return empty string if key is invalid
  wxCHECK_MSG( key < nStdKeys, wxEmptyString, wxT("invalid key in wxRegKey::GetStdKeyName") );

  return aStdKeys[key].szName;
}

const wxChar *wxRegKey::GetStdKeyShortName(size_t key)
{
  // return empty string if key is invalid
  wxCHECK( key < nStdKeys, wxEmptyString );

  return aStdKeys[key].szShortName;
}

wxRegKey::StdKey wxRegKey::ExtractKeyName(wxString& strKey)
{
  wxString strRoot = strKey.BeforeFirst(REG_SEPARATOR);

  size_t ui;
  for ( ui = 0; ui < nStdKeys; ui++ ) {
    if ( strRoot.CmpNoCase(aStdKeys[ui].szName) == 0 ||
         strRoot.CmpNoCase(aStdKeys[ui].szShortName) == 0 ) {
      break;
    }
  }

  if ( ui == nStdKeys ) {
    wxFAIL_MSG(wxT("invalid key prefix in wxRegKey::ExtractKeyName."));

    ui = HKCR;
  }
  else {
    strKey = strKey.After(REG_SEPARATOR);
    if ( !strKey.empty() && strKey.Last() == REG_SEPARATOR )
      strKey.Truncate(strKey.Len() - 1);
  }

  return (StdKey)ui;
}

wxRegKey::StdKey wxRegKey::GetStdKeyFromHkey(WXHKEY hkey)
{
  for ( size_t ui = 0; ui < nStdKeys; ui++ ) {
    if ( aStdKeys[ui].hkey == (HKEY)hkey )
      return (StdKey)ui;
  }

  wxFAIL_MSG(wxT("non root hkey passed to wxRegKey::GetStdKeyFromHkey."));

  return HKCR;
}

// ----------------------------------------------------------------------------
// ctors and dtor
// ----------------------------------------------------------------------------

wxRegKey::wxRegKey()
{
  m_hRootKey = (WXHKEY) aStdKeys[HKCR].hkey;

  Init();
}

wxRegKey::wxRegKey(const wxString& strKey) : m_strKey(strKey)
{
  m_hRootKey  = (WXHKEY) aStdKeys[ExtractKeyName(m_strKey)].hkey;

  Init();
}

// parent is a predefined (and preopened) key
wxRegKey::wxRegKey(StdKey keyParent, const wxString& strKey) : m_strKey(strKey)
{
  RemoveTrailingSeparator(m_strKey);
  m_hRootKey  = (WXHKEY) aStdKeys[keyParent].hkey;

  Init();
}

// parent is a normal regkey
wxRegKey::wxRegKey(const wxRegKey& keyParent, const wxString& strKey)
        : m_strKey(keyParent.m_strKey)
{
  // combine our name with parent's to get the full name
  if ( !m_strKey.empty() &&
       (strKey.empty() || strKey[0] != REG_SEPARATOR) ) {
      m_strKey += REG_SEPARATOR;
  }

  m_strKey += strKey;
  RemoveTrailingSeparator(m_strKey);

  m_hRootKey  = keyParent.m_hRootKey;

  Init();
}

// dtor closes the key releasing system resource
wxRegKey::~wxRegKey()
{
  Close();
}

// ----------------------------------------------------------------------------
// change the key name/hkey
// ----------------------------------------------------------------------------

// set the full key name
void wxRegKey::SetName(const wxString& strKey)
{
  Close();

  m_strKey = strKey;
  m_hRootKey = (WXHKEY) aStdKeys[ExtractKeyName(m_strKey)].hkey;
}

// the name is relative to the parent key
void wxRegKey::SetName(StdKey keyParent, const wxString& strKey)
{
  Close();

  m_strKey = strKey;
  RemoveTrailingSeparator(m_strKey);
  m_hRootKey = (WXHKEY) aStdKeys[keyParent].hkey;
}

// the name is relative to the parent key
void wxRegKey::SetName(const wxRegKey& keyParent, const wxString& strKey)
{
  Close();

  // combine our name with parent's to get the full name

  // NB: this method is called by wxRegConfig::SetPath() which is a performance
  //     critical function and so it preallocates space for our m_strKey to
  //     gain some speed - this is why we only use += here and not = which
  //     would just free the prealloc'd buffer and would have to realloc it the
  //     next line!
  m_strKey.clear();
  m_strKey += keyParent.m_strKey;
  if ( !strKey.empty() && strKey[0] != REG_SEPARATOR )
    m_strKey += REG_SEPARATOR;
  m_strKey += strKey;

  RemoveTrailingSeparator(m_strKey);

  m_hRootKey = keyParent.m_hRootKey;
}

// hKey should be opened and will be closed in wxRegKey dtor
void wxRegKey::SetHkey(WXHKEY hKey)
{
  Close();

  m_hKey = hKey;
}

// ----------------------------------------------------------------------------
// info about the key
// ----------------------------------------------------------------------------

// returns true if the key exists
bool wxRegKey::Exists() const
{
  // opened key has to exist, try to open it if not done yet
  return IsOpened() ? true : KeyExists(m_hRootKey, m_strKey);
}

// returns the full name of the key (prefix is abbreviated if bShortPrefix)
wxString wxRegKey::GetName(bool bShortPrefix) const
{
  StdKey key = GetStdKeyFromHkey((WXHKEY) m_hRootKey);
  wxString str = bShortPrefix ? aStdKeys[key].szShortName
                              : aStdKeys[key].szName;
  if ( !m_strKey.empty() )
    str << _T("\\") << m_strKey;

  return str;
}

bool wxRegKey::GetKeyInfo(size_t *pnSubKeys,
                          size_t *pnMaxKeyLen,
                          size_t *pnValues,
                          size_t *pnMaxValueLen) const
{
    // old gcc headers incorrectly prototype RegQueryInfoKey()
#if defined(__GNUWIN32_OLD__) && !defined(__CYGWIN10__)
    #define REG_PARAM   (size_t *)
#else
    #define REG_PARAM   (LPDWORD)
#endif

  // it might be unexpected to some that this function doesn't open the key
  wxASSERT_MSG( IsOpened(), _T("key should be opened in GetKeyInfo") );

  m_dwLastError = ::RegQueryInfoKey
                  (
                    (HKEY) m_hKey,
                    NULL,           // class name
                    NULL,           // (ptr to) size of class name buffer
                    RESERVED,
                    REG_PARAM
                    pnSubKeys,      // [out] number of subkeys
                    REG_PARAM
                    pnMaxKeyLen,    // [out] max length of a subkey name
                    NULL,           // longest subkey class name
                    REG_PARAM
                    pnValues,       // [out] number of values
                    REG_PARAM
                    pnMaxValueLen,  // [out] max length of a value name
                    NULL,           // longest value data
                    NULL,           // security descriptor
                    NULL            // time of last modification
                  );

#undef REG_PARAM

  if ( m_dwLastError != ERROR_SUCCESS ) {
    wxLogSysError(m_dwLastError, _("Can't get info about registry key '%s'"),
                  GetName().c_str());
    return false;
  }

  return true;
}

// ----------------------------------------------------------------------------
// operations
// ----------------------------------------------------------------------------

// opens key (it's not an error to call Open() on an already opened key)
bool wxRegKey::Open(AccessMode mode)
{
    if ( IsOpened() )
    {
        if ( mode <= m_mode )
            return true;

        // we had been opened in read mode but now must be reopened in write
        Close();
    }

    HKEY tmpKey;
    m_dwLastError = ::RegOpenKeyEx
                    (
                        (HKEY) m_hRootKey,
                        m_strKey,
                        RESERVED,
                        mode == Read ? KEY_READ : KEY_ALL_ACCESS,
                        &tmpKey
                    );

    if ( m_dwLastError != ERROR_SUCCESS )
    {
        wxLogSysError(m_dwLastError, _("Can't open registry key '%s'"),
                      GetName().c_str());
        return false;
    }

    m_hKey = (WXHKEY) tmpKey;
    m_mode = mode;

    return true;
}

// creates key, failing if it exists and !bOkIfExists
bool wxRegKey::Create(bool bOkIfExists)
{
  // check for existence only if asked (i.e. order is important!)
  if ( !bOkIfExists && Exists() )
    return false;

  if ( IsOpened() )
    return true;

  HKEY tmpKey;
#ifdef __WXWINCE__
  DWORD disposition;
  m_dwLastError = RegCreateKeyEx((HKEY) m_hRootKey, m_strKey,
      NULL, // reserved
      NULL, // class string
      0,
      0,
      NULL,
      &tmpKey,
      &disposition);
#else
  m_dwLastError = RegCreateKey((HKEY) m_hRootKey, m_strKey, &tmpKey);
#endif
  if ( m_dwLastError != ERROR_SUCCESS ) {
    wxLogSysError(m_dwLastError, _("Can't create registry key '%s'"),
                  GetName().c_str());
    return false;
  }
  else
  {
    m_hKey = (WXHKEY) tmpKey;
    return true;
  }
}

// close the key, it's not an error to call it when not opened
bool wxRegKey::Close()
{
  if ( IsOpened() ) {
    m_dwLastError = RegCloseKey((HKEY) m_hKey);
    m_hKey = 0;

    if ( m_dwLastError != ERROR_SUCCESS ) {
      wxLogSysError(m_dwLastError, _("Can't close registry key '%s'"),
                    GetName().c_str());

      return false;
    }
  }

  return true;
}

bool wxRegKey::RenameValue(const wxChar *szValueOld, const wxChar *szValueNew)
{
    bool ok = true;
    if ( HasValue(szValueNew) ) {
        wxLogError(_("Registry value '%s' already exists."), szValueNew);

        ok = false;
    }

    if ( !ok ||
         !CopyValue(szValueOld, *this, szValueNew) ||
         !DeleteValue(szValueOld) ) {
        wxLogError(_("Failed to rename registry value '%s' to '%s'."),
                   szValueOld, szValueNew);

        return false;
    }

    return true;
}

bool wxRegKey::CopyValue(const wxChar *szValue,
                         wxRegKey& keyDst,
                         const wxChar *szValueNew)
{
    if ( wxIsEmpty(szValueNew) ) {
        // by default, use the same name
        szValueNew = szValue;
    }

    switch ( GetValueType(szValue) ) {
        case Type_String:
            {
                wxString strVal;
                return QueryValue(szValue, strVal) &&
                       keyDst.SetValue(szValueNew, strVal);
            }

        case Type_Dword:
        /* case Type_Dword_little_endian: == Type_Dword */
            {
                long dwVal;
                return QueryValue(szValue, &dwVal) &&
                       keyDst.SetValue(szValueNew, dwVal);
            }

        case Type_Binary:
        {
            wxMemoryBuffer buf;
            return QueryValue(szValue,buf) &&
                   keyDst.SetValue(szValueNew,buf);
        }

        // these types are unsupported because I am not sure about how
        // exactly they should be copied and because they shouldn't
        // occur among the application keys (supposedly created with
        // this class)
        case Type_None:
        case Type_Expand_String:
        case Type_Dword_big_endian:
        case Type_Link:
        case Type_Multi_String:
        case Type_Resource_list:
        case Type_Full_resource_descriptor:
        case Type_Resource_requirements_list:
        default:
            wxLogError(_("Can't copy values of unsupported type %d."),
                       GetValueType(szValue));
            return false;
    }
}

bool wxRegKey::Rename(const wxChar *szNewName)
{
    wxCHECK_MSG( !m_strKey.empty(), false, _T("registry hives can't be renamed") );

    if ( !Exists() ) {
        wxLogError(_("Registry key '%s' does not exist, cannot rename it."),
                   GetFullName(this));

        return false;
    }

    // do we stay in the same hive?
    bool inSameHive = !wxStrchr(szNewName, REG_SEPARATOR);

    // construct the full new name of the key
    wxRegKey keyDst;

    if ( inSameHive ) {
        // rename the key to the new name under the same parent
        wxString strKey = m_strKey.BeforeLast(REG_SEPARATOR);
        if ( !strKey.empty() ) {
            // don't add '\\' in the start if strFullNewName is empty
            strKey += REG_SEPARATOR;
        }

        strKey += szNewName;

        keyDst.SetName(GetStdKeyFromHkey(m_hRootKey), strKey);
    }
    else {
        // this is the full name already
        keyDst.SetName(szNewName);
    }

    bool ok = keyDst.Create(false /* fail if alredy exists */);
    if ( !ok ) {
        wxLogError(_("Registry key '%s' already exists."),
                   GetFullName(&keyDst));
    }
    else {
        ok = Copy(keyDst) && DeleteSelf();
    }

    if ( !ok ) {
        wxLogError(_("Failed to rename the registry key '%s' to '%s'."),
                   GetFullName(this), GetFullName(&keyDst));
    }
    else {
        m_hRootKey = keyDst.m_hRootKey;
        m_strKey = keyDst.m_strKey;
    }

    return ok;
}

bool wxRegKey::Copy(const wxChar *szNewName)
{
    // create the new key first
    wxRegKey keyDst(szNewName);
    bool ok = keyDst.Create(false /* fail if alredy exists */);
    if ( ok ) {
        ok = Copy(keyDst);

        // we created the dest key but copying to it failed - delete it
        if ( !ok ) {
            (void)keyDst.DeleteSelf();
        }
    }

    return ok;
}

bool wxRegKey::Copy(wxRegKey& keyDst)
{
    bool ok = true;

    // copy all sub keys to the new location
    wxString strKey;
    long lIndex;
    bool bCont = GetFirstKey(strKey, lIndex);
    while ( ok && bCont ) {
        wxRegKey key(*this, strKey);
        wxString keyName;
        keyName << GetFullName(&keyDst) << REG_SEPARATOR << strKey;
        ok = key.Copy((const wxChar*) keyName);

        if ( ok )
            bCont = GetNextKey(strKey, lIndex);
        else
            wxLogError(_("Failed to copy the registry subkey '%s' to '%s'."),
                   GetFullName(&key), keyName.c_str());

    }

    // copy all values
    wxString strVal;
    bCont = GetFirstValue(strVal, lIndex);
    while ( ok && bCont ) {
        ok = CopyValue(strVal, keyDst);

        if ( !ok ) {
            wxLogSysError(m_dwLastError,
                          _("Failed to copy registry value '%s'"),
                          strVal.c_str());
        }
        else {
            bCont = GetNextValue(strVal, lIndex);
        }
    }

    if ( !ok ) {
        wxLogError(_("Failed to copy the contents of registry key '%s' to '%s'."),
                   GetFullName(this), GetFullName(&keyDst));
    }

    return ok;
}

// ----------------------------------------------------------------------------
// delete keys/values
// ----------------------------------------------------------------------------
bool wxRegKey::DeleteSelf()
{
  {
    wxLogNull nolog;
    if ( !Open() ) {
      // it already doesn't exist - ok!
      return true;
    }
  }

  // prevent a buggy program from erasing one of the root registry keys or an
  // immediate subkey (i.e. one which doesn't have '\\' inside) of any other
  // key except HKCR (HKCR has some "deleteable" subkeys)
  if ( m_strKey.empty() ||
       ((m_hRootKey != (WXHKEY) aStdKeys[HKCR].hkey) &&
        (m_strKey.Find(REG_SEPARATOR) == wxNOT_FOUND)) ) {
      wxLogError(_("Registry key '%s' is needed for normal system operation,\ndeleting it will leave your system in unusable state:\noperation aborted."),
                 GetFullName(this));

      return false;
  }

  // we can't delete keys while enumerating because it confuses GetNextKey, so
  // we first save the key names and then delete them all
  wxArrayString astrSubkeys;

  wxString strKey;
  long lIndex;
  bool bCont = GetFirstKey(strKey, lIndex);
  while ( bCont ) {
    astrSubkeys.Add(strKey);

    bCont = GetNextKey(strKey, lIndex);
  }

  size_t nKeyCount = astrSubkeys.Count();
  for ( size_t nKey = 0; nKey < nKeyCount; nKey++ ) {
    wxRegKey key(*this, astrSubkeys[nKey]);
    if ( !key.DeleteSelf() )
      return false;
  }

  // now delete this key itself
  Close();

  m_dwLastError = RegDeleteKey((HKEY) m_hRootKey, m_strKey);
  // deleting a key which doesn't exist is not considered an error
  if ( m_dwLastError != ERROR_SUCCESS &&
          m_dwLastError != ERROR_FILE_NOT_FOUND ) {
    wxLogSysError(m_dwLastError, _("Can't delete key '%s'"),
                  GetName().c_str());
    return false;
  }

  return true;
}

bool wxRegKey::DeleteKey(const wxChar *szKey)
{
  if ( !Open() )
    return false;

  wxRegKey key(*this, szKey);
  return key.DeleteSelf();
}

bool wxRegKey::DeleteValue(const wxChar *szValue)
{
    if ( !Open() )
        return false;

    m_dwLastError = RegDeleteValue((HKEY) m_hKey, RegValueStr(szValue));

    // deleting a value which doesn't exist is not considered an error
    if ( (m_dwLastError != ERROR_SUCCESS) &&
         (m_dwLastError != ERROR_FILE_NOT_FOUND) )
    {
        wxLogSysError(m_dwLastError, _("Can't delete value '%s' from key '%s'"),
                      szValue, GetName().c_str());
        return false;
    }

    return true;
}

// ----------------------------------------------------------------------------
// access to values and subkeys
// ----------------------------------------------------------------------------

// return true if value exists
bool wxRegKey::HasValue(const wxChar *szValue) const
{
    // this function should be silent, so suppress possible messages from Open()
    wxLogNull nolog;

    if ( !CONST_CAST Open(Read) )
        return false;

    LONG dwRet = ::RegQueryValueEx((HKEY) m_hKey,
                                   RegValueStr(szValue),
                                   RESERVED,
                                   NULL, NULL, NULL);
    return dwRet == ERROR_SUCCESS;
}

// returns true if this key has any values
bool wxRegKey::HasValues() const
{
  // suppress possible messages from GetFirstValue()
  wxLogNull nolog;

  // just call GetFirstValue with dummy parameters
  wxString str;
  long     l;
  return CONST_CAST GetFirstValue(str, l);
}

// returns true if this key has any subkeys
bool wxRegKey::HasSubkeys() const
{
  // suppress possible messages from GetFirstKey()
  wxLogNull nolog;

  // just call GetFirstKey with dummy parameters
  wxString str;
  long     l;
  return CONST_CAST GetFirstKey(str, l);
}

// returns true if given subkey exists
bool wxRegKey::HasSubKey(const wxChar *szKey) const
{
  // this function should be silent, so suppress possible messages from Open()
  wxLogNull nolog;

  if ( !CONST_CAST Open(Read) )
    return false;

  return KeyExists(m_hKey, szKey);
}

wxRegKey::ValueType wxRegKey::GetValueType(const wxChar *szValue) const
{
    if ( ! CONST_CAST Open(Read) )
      return Type_None;

    DWORD dwType;
    m_dwLastError = RegQueryValueEx((HKEY) m_hKey, RegValueStr(szValue), RESERVED,
                                    &dwType, NULL, NULL);
    if ( m_dwLastError != ERROR_SUCCESS ) {
      wxLogSysError(m_dwLastError, _("Can't read value of key '%s'"),
                    GetName().c_str());
      return Type_None;
    }

    return (ValueType)dwType;
}

bool wxRegKey::SetValue(const wxChar *szValue, long lValue)
{
  if ( CONST_CAST Open() ) {
    m_dwLastError = RegSetValueEx((HKEY) m_hKey, RegValueStr(szValue), (DWORD) RESERVED, REG_DWORD,
                                  (RegString)&lValue, sizeof(lValue));
    if ( m_dwLastError == ERROR_SUCCESS )
      return true;
  }

  wxLogSysError(m_dwLastError, _("Can't set value of '%s'"),
                GetFullName(this, szValue));
  return false;
}

bool wxRegKey::QueryValue(const wxChar *szValue, long *plValue) const
{
  if ( CONST_CAST Open(Read) ) {
    DWORD dwType, dwSize = sizeof(DWORD);
    RegString pBuf = (RegString)plValue;
    m_dwLastError = RegQueryValueEx((HKEY) m_hKey, RegValueStr(szValue), RESERVED,
                                    &dwType, pBuf, &dwSize);
    if ( m_dwLastError != ERROR_SUCCESS ) {
      wxLogSysError(m_dwLastError, _("Can't read value of key '%s'"),
                    GetName().c_str());
      return false;
    }
    else {
      // check that we read the value of right type
      wxASSERT_MSG( IsNumericValue(szValue),
                    wxT("Type mismatch in wxRegKey::QueryValue().")  );

      return true;
    }
  }
  else
    return false;
}

bool wxRegKey::SetValue(const wxChar *szValue,const wxMemoryBuffer& buffer)
{
#ifdef __TWIN32__
  wxFAIL_MSG("RegSetValueEx not implemented by TWIN32");
  return false;
#else
  if ( CONST_CAST Open() ) {
    m_dwLastError = RegSetValueEx((HKEY) m_hKey, RegValueStr(szValue), (DWORD) RESERVED, REG_BINARY,
                                  (RegBinary)buffer.GetData(),buffer.GetDataLen());
    if ( m_dwLastError == ERROR_SUCCESS )
      return true;
  }

  wxLogSysError(m_dwLastError, _("Can't set value of '%s'"),
                GetFullName(this, szValue));
  return false;
#endif
}

bool wxRegKey::QueryValue(const wxChar *szValue, wxMemoryBuffer& buffer) const
{
  if ( CONST_CAST Open(Read) ) {
    // first get the type and size of the data
    DWORD dwType, dwSize;
    m_dwLastError = RegQueryValueEx((HKEY) m_hKey, RegValueStr(szValue), RESERVED,
                                      &dwType, NULL, &dwSize);

    if ( m_dwLastError == ERROR_SUCCESS ) {
        if ( dwSize ) {
            const RegBinary pBuf = (RegBinary)buffer.GetWriteBuf(dwSize);
            m_dwLastError = RegQueryValueEx((HKEY) m_hKey,
                                            RegValueStr(szValue),
                                            RESERVED,
                                            &dwType,
                                            pBuf,
                                            &dwSize);
            buffer.UngetWriteBuf(dwSize);
        } else {
            buffer.SetDataLen(0);
        }
    }


    if ( m_dwLastError != ERROR_SUCCESS ) {
      wxLogSysError(m_dwLastError, _("Can't read value of key '%s'"),
                    GetName().c_str());
      return false;
    }
    return true;
  }
  return false;
}



bool wxRegKey::QueryValue(const wxChar *szValue,
                          wxString& strValue,
                          bool WXUNUSED_IN_WINCE(raw)) const
{
    if ( CONST_CAST Open(Read) )
    {

        // first get the type and size of the data
        DWORD dwType=REG_NONE, dwSize=0;
        m_dwLastError = RegQueryValueEx((HKEY) m_hKey,
                                        RegValueStr(szValue),
                                        RESERVED,
                                        &dwType, NULL, &dwSize);
        if ( m_dwLastError == ERROR_SUCCESS )
        {
            if ( !dwSize )
            {
                // must treat this case specially as GetWriteBuf() doesn't like
                // being called with 0 size
                strValue.Empty();
            }
            else
            {
                m_dwLastError = RegQueryValueEx((HKEY) m_hKey,
                                                RegValueStr(szValue),
                                                RESERVED,
                                                &dwType,
                                                (RegString)(wxChar*)wxStringBuffer(strValue, dwSize),
                                                &dwSize);

                // expand the var expansions in the string unless disabled
#ifndef __WXWINCE__
                if ( (dwType == REG_EXPAND_SZ) && !raw )
                {
                    DWORD dwExpSize = ::ExpandEnvironmentStrings(strValue, NULL, 0);
                    bool ok = dwExpSize != 0;
                    if ( ok )
                    {
                        wxString strExpValue;
                        ok = ::ExpandEnvironmentStrings(strValue,
                                                        wxStringBuffer(strExpValue, dwExpSize),
                                                        dwExpSize
                                                        ) != 0;
                        strValue = strExpValue;
                    }

                    if ( !ok )
                    {
                        wxLogLastError(_T("ExpandEnvironmentStrings"));
                    }
                }
#endif
                // __WXWINCE__
            }

            if ( m_dwLastError == ERROR_SUCCESS )
            {
                // check that it was the right type
                wxASSERT_MSG( !IsNumericValue(szValue),
                              wxT("Type mismatch in wxRegKey::QueryValue().") );

              return true;
            }
        }
    }

    wxLogSysError(m_dwLastError, _("Can't read value of '%s'"),
                  GetFullName(this, szValue));
    return false;
}

bool wxRegKey::SetValue(const wxChar *szValue, const wxString& strValue)
{
  if ( CONST_CAST Open() ) {
      m_dwLastError = RegSetValueEx((HKEY) m_hKey, RegValueStr(szValue),
                                    (DWORD) RESERVED, REG_SZ,
                                    (RegString)strValue.c_str(),
                                    (strValue.Len() + 1)*sizeof(wxChar));
      if ( m_dwLastError == ERROR_SUCCESS )
        return true;
  }

  wxLogSysError(m_dwLastError, _("Can't set value of '%s'"),
                GetFullName(this, szValue));
  return false;
}

wxString wxRegKey::QueryDefaultValue() const
{
  wxString str;
  QueryValue(NULL, str);
  return str;
}

// ----------------------------------------------------------------------------
// enumeration
// NB: all these functions require an index variable which allows to have
//     several concurrently running indexations on the same key
// ----------------------------------------------------------------------------

bool wxRegKey::GetFirstValue(wxString& strValueName, long& lIndex)
{
  if ( !Open(Read) )
    return false;

  lIndex = 0;
  return GetNextValue(strValueName, lIndex);
}

bool wxRegKey::GetNextValue(wxString& strValueName, long& lIndex) const
{
  wxASSERT( IsOpened() );

  // are we already at the end of enumeration?
  if ( lIndex == -1 )
    return false;

    wxChar  szValueName[1024];                  // @@ use RegQueryInfoKey...
    DWORD dwValueLen = WXSIZEOF(szValueName);

    m_dwLastError = RegEnumValue((HKEY) m_hKey, lIndex++,
                                 szValueName, &dwValueLen,
                                 RESERVED,
                                 NULL,            // [out] type
                                 NULL,            // [out] buffer for value
                                 NULL);           // [i/o]  it's length

    if ( m_dwLastError != ERROR_SUCCESS ) {
      if ( m_dwLastError == ERROR_NO_MORE_ITEMS ) {
        m_dwLastError = ERROR_SUCCESS;
        lIndex = -1;
      }
      else {
        wxLogSysError(m_dwLastError, _("Can't enumerate values of key '%s'"),
                      GetName().c_str());
      }

      return false;
    }

    strValueName = szValueName;

  return true;
}

bool wxRegKey::GetFirstKey(wxString& strKeyName, long& lIndex)
{
  if ( !Open(Read) )
    return false;

  lIndex = 0;
  return GetNextKey(strKeyName, lIndex);
}

bool wxRegKey::GetNextKey(wxString& strKeyName, long& lIndex) const
{
  wxASSERT( IsOpened() );

  // are we already at the end of enumeration?
  if ( lIndex == -1 )
    return false;

  wxChar szKeyName[_MAX_PATH + 1];

#ifdef __WXWINCE__
  DWORD sizeName = WXSIZEOF(szKeyName);
  m_dwLastError = RegEnumKeyEx((HKEY) m_hKey, lIndex++, szKeyName, & sizeName,
      0, NULL, NULL, NULL);
#else
  m_dwLastError = RegEnumKey((HKEY) m_hKey, lIndex++, szKeyName, WXSIZEOF(szKeyName));
#endif

  if ( m_dwLastError != ERROR_SUCCESS ) {
    if ( m_dwLastError == ERROR_NO_MORE_ITEMS ) {
      m_dwLastError = ERROR_SUCCESS;
      lIndex = -1;
    }
    else {
      wxLogSysError(m_dwLastError, _("Can't enumerate subkeys of key '%s'"),
                    GetName().c_str());
    }

    return false;
  }

  strKeyName = szKeyName;
  return true;
}

// returns true if the value contains a number (else it's some string)
bool wxRegKey::IsNumericValue(const wxChar *szValue) const
{
    ValueType type = GetValueType(szValue);
    switch ( type ) {
        case Type_Dword:
        /* case Type_Dword_little_endian: == Type_Dword */
        case Type_Dword_big_endian:
            return true;

        default:
            return false;
    }
}

// ----------------------------------------------------------------------------
// exporting registry keys to file
// ----------------------------------------------------------------------------

#if wxUSE_STREAMS

// helper functions for writing ASCII strings (even in Unicode build)
static inline bool WriteAsciiChar(wxOutputStream& ostr, char ch)
{
    ostr.PutC(ch);
    return ostr.IsOk();
}

static inline bool WriteAsciiEOL(wxOutputStream& ostr)
{
    // as we open the file in text mode, it is enough to write LF without CR
    return WriteAsciiChar(ostr, '\n');
}

static inline bool WriteAsciiString(wxOutputStream& ostr, const char *p)
{
    return ostr.Write(p, strlen(p)).IsOk();
}

static inline bool WriteAsciiString(wxOutputStream& ostr, const wxString& s)
{
#if wxUSE_UNICODE
    wxCharBuffer name(s.mb_str());
    ostr.Write(name, strlen(name));
#else
    ostr.Write(s, s.length());
#endif

    return ostr.IsOk();
}

#endif // wxUSE_STREAMS

bool wxRegKey::Export(const wxString& filename) const
{
#if wxUSE_FFILE && wxUSE_STREAMS
    if ( wxFile::Exists(filename) )
    {
        wxLogError(_("Exporting registry key: file \"%s\" already exists and won't be overwritten."),
                   filename.c_str());
        return false;
    }

    wxFFileOutputStream ostr(filename, _T("w"));

    return ostr.Ok() && Export(ostr);
#else
    wxUnusedVar(filename);
    return false;
#endif
}

#if wxUSE_STREAMS
bool wxRegKey::Export(wxOutputStream& ostr) const
{
    // write out the header
    if ( !WriteAsciiString(ostr, "REGEDIT4\n\n") )
        return false;

    return DoExport(ostr);
}
#endif // wxUSE_STREAMS

static
wxString
FormatAsHex(const void *data,
            size_t size,
            wxRegKey::ValueType type = wxRegKey::Type_Binary)
{
    wxString value(_T("hex"));

    // binary values use just "hex:" prefix while the other ones must indicate
    // the real type
    if ( type != wxRegKey::Type_Binary )
        value << _T('(') << type << _T(')');
    value << _T(':');

    // write all the rest as comma-separated bytes
    value.reserve(3*size + 10);
    const char * const p = wx_static_cast(const char *, data);
    for ( size_t n = 0; n < size; n++ )
    {
        // TODO: line wrapping: although not required by regedit, this makes
        //       the generated files easier to read and compare with the files
        //       produced by regedit
        if ( n )
            value << _T(',');

        value << wxString::Format(_T("%02x"), (unsigned char)p[n]);
    }

    return value;
}

static inline
wxString FormatAsHex(const wxString& value, wxRegKey::ValueType type)
{
    return FormatAsHex(value.c_str(), value.length() + 1, type);
}

wxString wxRegKey::FormatValue(const wxString& name) const
{
    wxString rhs;
    const ValueType type = GetValueType(name);
    switch ( type )
    {
        case Type_String:
            {
                wxString value;
                if ( !QueryValue(name, value) )
                    break;

                // quotes and backslashes must be quoted, linefeeds are not
                // allowed in string values
                rhs.reserve(value.length() + 2);
                rhs = _T('"');

                // there can be no NULs here
                bool useHex = false;
                for ( const wxChar *p = value.c_str(); *p && !useHex; p++ )
                {
                    switch ( *p )
                    {
                        case _T('\n'):
                            // we can only represent this string in hex
                            useHex = true;
                            break;

                        case _T('"'):
                        case _T('\\'):
                            // escape special symbol
                            rhs += _T('\\');
                            // fall through

                        default:
                            rhs += *p;
                    }
                }

                if ( useHex )
                    rhs = FormatAsHex(value, Type_String);
                else
                    rhs += _T('"');
            }
            break;

        case Type_Dword:
        /* case Type_Dword_little_endian: == Type_Dword */
            {
                long value;
                if ( !QueryValue(name, &value) )
                    break;

                rhs.Printf(_T("dword:%08x"), (unsigned int)value);
            }
            break;

        case Type_Expand_String:
        case Type_Multi_String:
            {
                wxString value;
                if ( !QueryRawValue(name, value) )
                    break;

                rhs = FormatAsHex(value, type);
            }
            break;

        case Type_Binary:
            {
                wxMemoryBuffer buf;
                if ( !QueryValue(name, buf) )
                    break;

                rhs = FormatAsHex(buf.GetData(), buf.GetDataLen());
            }
            break;

        // no idea how those appear in REGEDIT4 files
        case Type_None:
        case Type_Dword_big_endian:
        case Type_Link:
        case Type_Resource_list:
        case Type_Full_resource_descriptor:
        case Type_Resource_requirements_list:
        default:
            wxLogWarning(_("Can't export value of unsupported type %d."), type);
    }

    return rhs;
}

#if wxUSE_STREAMS

bool wxRegKey::DoExportValue(wxOutputStream& ostr, const wxString& name) const
{
    // first examine the value type: if it's unsupported, simply skip it
    // instead of aborting the entire export process because we failed to
    // export a single value
    wxString value = FormatValue(name);
    if ( value.empty() )
    {
        wxLogWarning(_("Ignoring value \"%s\" of the key \"%s\"."),
                     name.c_str(), GetName().c_str());
        return true;
    }

    // we do have the text representation of the value, now write everything
    // out

    // special case: unnamed/default value is represented as just "@"
    if ( name.empty() )
    {
        if ( !WriteAsciiChar(ostr, '@') )
            return false;
    }
    else // normal, named, value
    {
        if ( !WriteAsciiChar(ostr, '"') ||
                !WriteAsciiString(ostr, name) ||
                    !WriteAsciiChar(ostr, '"') )
            return false;
    }

    if ( !WriteAsciiChar(ostr, '=') )
        return false;

    return WriteAsciiString(ostr, value) && WriteAsciiEOL(ostr);
}

bool wxRegKey::DoExport(wxOutputStream& ostr) const
{
    // write out this key name
    if ( !WriteAsciiChar(ostr, '[') )
        return false;

    if ( !WriteAsciiString(ostr, GetName(false /* no short prefix */)) )
        return false;

    if ( !WriteAsciiChar(ostr, ']') || !WriteAsciiEOL(ostr) )
        return false;

    // dump all our values
    long dummy;
    wxString name;
    wxRegKey& self = wx_const_cast(wxRegKey&, *this);
    bool cont = self.GetFirstValue(name, dummy);
    while ( cont )
    {
        if ( !DoExportValue(ostr, name) )
            return false;

        cont = GetNextValue(name, dummy);
    }

    // always terminate values by blank line, even if there were no values
    if ( !WriteAsciiEOL(ostr) )
        return false;

    // recurse to subkeys
    cont = self.GetFirstKey(name, dummy);
    while ( cont )
    {
        wxRegKey subkey(*this, name);
        if ( !subkey.DoExport(ostr) )
            return false;

        cont = GetNextKey(name, dummy);
    }

    return true;
}

#endif // wxUSE_STREAMS

// ============================================================================
// implementation of global private functions
// ============================================================================

bool KeyExists(WXHKEY hRootKey, const wxChar *szKey)
{
    // don't close this key itself for the case of empty szKey!
    if ( wxIsEmpty(szKey) )
        return true;

    HKEY hkeyDummy;
    if ( ::RegOpenKeyEx
         (
            (HKEY)hRootKey,
            szKey,
            RESERVED,
            KEY_READ,        // we might not have enough rights for rw access
            &hkeyDummy
         ) == ERROR_SUCCESS )
    {
        ::RegCloseKey(hkeyDummy);

        return true;
    }

    return false;
}

const wxChar *GetFullName(const wxRegKey *pKey, const wxChar *szValue)
{
  static wxString s_str;
  s_str = pKey->GetName();
  if ( !wxIsEmpty(szValue) )
    s_str << wxT("\\") << szValue;

  return s_str.c_str();
}

inline void RemoveTrailingSeparator(wxString& str)
{
  if ( !str.empty() && str.Last() == REG_SEPARATOR )
    str.Truncate(str.Len() - 1);
}
