///////////////////////////////////////////////////////////////////////////////
// Name:        include/wx/msw/iniconf.h
// Purpose:     INI-file based wxConfigBase implementation
// Author:      Vadim Zeitlin
// Modified by:
// Created:     27.07.98
// RCS-ID:      $Id: iniconf.h 62185 2009-09-28 10:02:42Z JS $
// Copyright:   (c) 1998 Vadim Zeitlin <zeitlin@dptmaths.ens-cachan.fr>
// Licence:     wxWindows licence
///////////////////////////////////////////////////////////////////////////////

#ifndef   _INICONF_H
#define   _INICONF_H

#include "wx/defs.h"

#if wxUSE_CONFIG

// ----------------------------------------------------------------------------
// wxIniConfig is a wxConfig implementation which uses MS Windows INI files to
// store the data. Because INI files don't really support arbitrary nesting of
// groups, we do the following:
//  (1) in win.ini file we store all entries in the [vendor] section and
//      the value group1/group2/key is mapped to the value group1_group2_key
//      in this section, i.e. all path separators are replaced with underscore
//  (2) in appname.ini file we map group1/group2/group3/key to the entry
//      group2_group3_key in [group1]
//
// Of course, it might lead to indesirable results if '_' is also used in key
// names (i.e. group/key is the same as group_key) and also GetPath() result
// may be not what you would expect it to be.
//
// Another limitation: the keys and section names are never case-sensitive
// which might differ from wxFileConfig it it was compiled with
// wxCONFIG_CASE_SENSITIVE option.
// ----------------------------------------------------------------------------

// for this class, "local" file is the file appname.ini and the global file
// is the [vendor] subsection of win.ini (default for "vendor" is to be the
// same as appname). The file name (strAppName parameter) may, in fact,
// contain the full path to the file. If it doesn't, the file is searched for
// in the Windows directory.
class WXDLLEXPORT wxIniConfig : public wxConfigBase
{
public:
  // ctor & dtor
    // if strAppName doesn't contain the extension and is not an absolute path,
    // ".ini" is appended to it. if strVendor is empty, it's taken to be the
    // same as strAppName.
  wxIniConfig(const wxString& strAppName = wxEmptyString, const wxString& strVendor = wxEmptyString,
    const wxString& localFilename = wxEmptyString, const wxString& globalFilename = wxEmptyString, long style = wxCONFIG_USE_LOCAL_FILE);
  virtual ~wxIniConfig();

  // implement inherited pure virtual functions
  virtual void SetPath(const wxString& strPath);
  virtual const wxString& GetPath() const;

  virtual bool GetFirstGroup(wxString& str, long& lIndex) const;
  virtual bool GetNextGroup (wxString& str, long& lIndex) const;
  virtual bool GetFirstEntry(wxString& str, long& lIndex) const;
  virtual bool GetNextEntry (wxString& str, long& lIndex) const;

  virtual size_t GetNumberOfEntries(bool bRecursive = false) const;
  virtual size_t GetNumberOfGroups(bool bRecursive = false) const;

  virtual bool HasGroup(const wxString& strName) const;
  virtual bool HasEntry(const wxString& strName) const;

  // return true if the current group is empty
  bool IsEmpty() const;

  virtual bool Flush(bool bCurrentOnly = false);

  virtual bool RenameEntry(const wxString& oldName, const wxString& newName);
  virtual bool RenameGroup(const wxString& oldName, const wxString& newName);

  virtual bool DeleteEntry(const wxString& Key, bool bGroupIfEmptyAlso = true);
  virtual bool DeleteGroup(const wxString& szKey);
  virtual bool DeleteAll();

protected:
  // read/write
  bool DoReadString(const wxString& key, wxString *pStr) const;
  bool DoReadLong(const wxString& key, long *plResult) const;

  bool DoWriteString(const wxString& key, const wxString& szValue);
  bool DoWriteLong(const wxString& key, long lValue);

private:
  // helpers
  wxString GetPrivateKeyName(const wxString& szKey) const;
  wxString GetKeyName(const wxString& szKey) const;

  wxString m_strLocalFilename;  // name of the private INI file
  wxString m_strGroup,    // current group in appname.ini file
           m_strPath;     // the rest of the path (no trailing '_'!)
};

#endif // wxUSE_CONFIG

#endif // _INICONF_H
