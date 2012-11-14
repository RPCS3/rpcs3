///////////////////////////////////////////////////////////////////////////////
// Name:        wx/msw/registry.h
// Purpose:     Registry classes and functions
// Author:      Vadim Zeitlin
// Modified by:
// Created:     03.04.1998
// RCS-ID:      $Id: registry.h 49563 2007-10-31 20:46:21Z VZ $
// Copyright:   (c) 1998 Vadim Zeitlin <zeitlin@dptmaths.ens-cachan.fr>
// Licence:     wxWindows licence
///////////////////////////////////////////////////////////////////////////////

#ifndef _WX_MSW_REGISTRY_H_
#define _WX_MSW_REGISTRY_H_

class WXDLLIMPEXP_FWD_BASE wxOutputStream;

// ----------------------------------------------------------------------------
// class wxRegKey encapsulates window HKEY handle
// ----------------------------------------------------------------------------

class WXDLLIMPEXP_BASE wxRegKey
{
public:
  // NB: do _not_ change the values of elements in these enumerations!

  // registry value types (with comments from winnt.h)
  enum ValueType
  {
    Type_None,                       // No value type
    Type_String,                     // Unicode nul terminated string
    Type_Expand_String,              // Unicode nul terminated string
                                     // (with environment variable references)
    Type_Binary,                     // Free form binary
    Type_Dword,                      // 32-bit number
    Type_Dword_little_endian         // 32-bit number
        = Type_Dword,                // (same as Type_DWORD)
    Type_Dword_big_endian,           // 32-bit number
    Type_Link,                       // Symbolic Link (unicode)
    Type_Multi_String,               // Multiple Unicode strings
    Type_Resource_list,              // Resource list in the resource map
    Type_Full_resource_descriptor,   // Resource list in the hardware description
    Type_Resource_requirements_list  // ???
  };

  // predefined registry keys
  enum StdKey
  {
    HKCR,       // classes root
    HKCU,       // current user
    HKLM,       // local machine
    HKUSR,      // users
    HKPD,       // performance data (WinNT/2K only)
    HKCC,       // current config
    HKDD,       // dynamic data (Win95/98 only)
    HKMAX
  };

  // access mode for the key
  enum AccessMode
  {
      Read,     // read-only
      Write     // read and write
  };

  // information about standard (predefined) registry keys
    // number of standard keys
  static const size_t nStdKeys;
    // get the name of a standard key
  static const wxChar *GetStdKeyName(size_t key);
    // get the short name of a standard key
  static const wxChar *GetStdKeyShortName(size_t key);
    // get StdKey from root HKEY
  static StdKey GetStdKeyFromHkey(WXHKEY hkey);

  // extacts the std key prefix from the string (return value) and
  // leaves only the part after it (i.e. modifies the string passed!)
  static StdKey ExtractKeyName(wxString& str);

  // ctors
    // root key is set to HKCR (the only root key under Win16)
  wxRegKey();
    // strKey is the full name of the key (i.e. starting with HKEY_xxx...)
  wxRegKey(const wxString& strKey);
    // strKey is the name of key under (standard key) keyParent
  wxRegKey(StdKey keyParent, const wxString& strKey);
    // strKey is the name of key under (previously created) keyParent
  wxRegKey(const wxRegKey& keyParent, const wxString& strKey);
    // dtor closes the key
 ~wxRegKey();

  // change key (closes the previously opened key if any)
    // the name is absolute, i.e. should start with HKEY_xxx
  void  SetName(const wxString& strKey);
    // the name is relative to the parent key
  void  SetName(StdKey keyParent, const wxString& strKey);
    // the name is relative to the parent key
  void  SetName(const wxRegKey& keyParent, const wxString& strKey);
    // hKey should be opened and will be closed in wxRegKey dtor
  void  SetHkey(WXHKEY hKey);

  // get infomation about the key
    // get the (full) key name. Abbreviate std root keys if bShortPrefix.
  wxString GetName(bool bShortPrefix = true) const;
    // return true if the key exists
  bool  Exists() const;
    // get the info about key (any number of these pointers may be NULL)
  bool  GetKeyInfo(size_t *pnSubKeys,      // number of subkeys
                   size_t *pnMaxKeyLen,    // max len of subkey name
                   size_t *pnValues,       // number of values
                   size_t *pnMaxValueLen) const;
    // return true if the key is opened
  bool  IsOpened() const { return m_hKey != 0; }
    // for "if ( !key ) wxLogError(...)" kind of expressions
  operator bool()  const { return m_dwLastError == 0; }

  // operations on the key itself
    // explicitly open the key (will be automatically done by all functions
    // which need the key to be opened if the key is not opened yet)
  bool  Open(AccessMode mode = Write);
    // create the key: will fail if the key already exists and !bOkIfExists
  bool  Create(bool bOkIfExists = true);
    // rename a value from old name to new one
  bool  RenameValue(const wxChar *szValueOld, const wxChar *szValueNew);
    // rename the key
  bool  Rename(const wxChar *szNewName);
    // copy value to another key possibly changing its name (by default it will
    // remain the same)
  bool  CopyValue(const wxChar *szValue, wxRegKey& keyDst,
                  const wxChar *szNewName = NULL);
    // copy the entire contents of the key recursively to another location
  bool  Copy(const wxChar *szNewName);
    // same as Copy() but using a key and not the name
  bool  Copy(wxRegKey& keyDst);
    // close the key (will be automatically done in dtor)
  bool  Close();

  // deleting keys/values
    // deletes this key and all of it's subkeys/values
  bool  DeleteSelf();
    // deletes the subkey with all of it's subkeys/values recursively
  bool  DeleteKey(const wxChar *szKey);
    // deletes the named value (may be NULL to remove the default value)
  bool  DeleteValue(const wxChar *szValue);

  // access to values and subkeys
    // get value type
  ValueType GetValueType(const wxChar *szValue) const;
    // returns true if the value contains a number (else it's some string)
  bool IsNumericValue(const wxChar *szValue) const;

    // assignment operators set the default value of the key
  wxRegKey& operator=(const wxString& strValue)
    { SetValue(NULL, strValue); return *this; }
  wxRegKey& operator=(long lValue)
    { SetValue(NULL, lValue); return *this; }

    // query the default value of the key: implicitly or explicitly
  wxString QueryDefaultValue() const;
  operator wxString() const { return QueryDefaultValue(); }

    // named values

    // set the string value
  bool  SetValue(const wxChar *szValue, const wxString& strValue);
    // retrieve the string value
  bool  QueryValue(const wxChar *szValue, wxString& strValue) const
    { return QueryValue(szValue, strValue, false); }
    // retrieve raw string value
  bool  QueryRawValue(const wxChar *szValue, wxString& strValue) const
    { return QueryValue(szValue, strValue, true); }
    // retrieve either raw or expanded string value
  bool  QueryValue(const wxChar *szValue, wxString& strValue, bool raw) const;

    // set the numeric value
  bool  SetValue(const wxChar *szValue, long lValue);
    // return the numeric value
  bool  QueryValue(const wxChar *szValue, long *plValue) const;
    // set the binary value
  bool  SetValue(const wxChar *szValue, const wxMemoryBuffer& buf);
    // return the binary value
  bool  QueryValue(const wxChar *szValue, wxMemoryBuffer& buf) const;

  // query existence of a key/value
    // return true if value exists
  bool HasValue(const wxChar *szKey) const;
    // return true if given subkey exists
  bool HasSubKey(const wxChar *szKey) const;
    // return true if any subkeys exist
  bool HasSubkeys() const;
    // return true if any values exist
  bool HasValues() const;
    // return true if the key is empty (nothing under this key)
  bool IsEmpty() const { return !HasSubkeys() && !HasValues(); }

  // enumerate values and subkeys
  bool  GetFirstValue(wxString& strValueName, long& lIndex);
  bool  GetNextValue (wxString& strValueName, long& lIndex) const;

  bool  GetFirstKey  (wxString& strKeyName  , long& lIndex);
  bool  GetNextKey   (wxString& strKeyName  , long& lIndex) const;

  // export the contents of this key and all its subkeys to the given file
  // (which won't be overwritten, it's an error if it already exists)
  //
  // note that we export the key in REGEDIT4 format, not RegSaveKey() binary
  // format nor newer REGEDIT5 one
  bool Export(const wxString& filename) const;

  // same as above but write to the given (opened) stream
  bool Export(wxOutputStream& ostr) const;


  // for wxRegConfig usage only: preallocate some memory for the name
  void ReserveMemoryForName(size_t bytes) { m_strKey.reserve(bytes); }

private:
  // common part of all ctors
  void Init()
  {
    m_hKey = (WXHKEY) NULL;
    m_dwLastError = 0;
  }

  // recursive helper for Export()
  bool DoExport(wxOutputStream& ostr) const;

  // export a single value
  bool DoExportValue(wxOutputStream& ostr, const wxString& name) const;

  // return the text representation (in REGEDIT4 format) of the value with the
  // given name
  wxString FormatValue(const wxString& name) const;


  WXHKEY      m_hKey,           // our handle
              m_hRootKey;       // handle of the top key (i.e. StdKey)
  wxString    m_strKey;         // key name (relative to m_hRootKey)

  AccessMode  m_mode;           // valid only if key is opened
  long        m_dwLastError;    // last error (0 if none)


  DECLARE_NO_COPY_CLASS(wxRegKey)
};

#endif // _WX_MSW_REGISTRY_H_

