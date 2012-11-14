///////////////////////////////////////////////////////////////////////////////
// Name:        msw/regconf.h
// Purpose:     Registry based implementation of wxConfigBase
// Author:      Vadim Zeitlin
// Modified by:
// Created:     27.04.98
// RCS-ID:      $Id: regconf.h 62185 2009-09-28 10:02:42Z JS $
// Copyright:   (c) 1998 Vadim Zeitlin <zeitlin@dptmaths.ens-cachan.fr>
// Licence:     wxWindows licence
///////////////////////////////////////////////////////////////////////////////

#ifndef   _REGCONF_H
#define   _REGCONF_H

#include "wx/defs.h"

#if wxUSE_CONFIG

#ifndef   _REGISTRY_H
  #include "wx/msw/registry.h"
#endif

#include "wx/object.h"
#include "wx/confbase.h"

// ----------------------------------------------------------------------------
// wxRegConfig
// ----------------------------------------------------------------------------

class WXDLLIMPEXP_BASE wxRegConfig : public wxConfigBase
{
public:
  // ctor & dtor
    // will store data in HKLM\appName and HKCU\appName
  wxRegConfig(const wxString& appName = wxEmptyString,
              const wxString& vendorName = wxEmptyString,
              const wxString& localFilename = wxEmptyString,
              const wxString& globalFilename = wxEmptyString,
              long style = wxCONFIG_USE_GLOBAL_FILE);

    // dtor will save unsaved data
  virtual ~wxRegConfig(){}

  // implement inherited pure virtual functions
  // ------------------------------------------

  // path management
  virtual void SetPath(const wxString& strPath);
  virtual const wxString& GetPath() const { return m_strPath; }

  // entry/subgroup info
    // enumerate all of them
  virtual bool GetFirstGroup(wxString& str, long& lIndex) const;
  virtual bool GetNextGroup (wxString& str, long& lIndex) const;
  virtual bool GetFirstEntry(wxString& str, long& lIndex) const;
  virtual bool GetNextEntry (wxString& str, long& lIndex) const;

    // tests for existence
  virtual bool HasGroup(const wxString& strName) const;
  virtual bool HasEntry(const wxString& strName) const;
  virtual EntryType GetEntryType(const wxString& name) const;

    // get number of entries/subgroups in the current group, with or without
    // it's subgroups
  virtual size_t GetNumberOfEntries(bool bRecursive = false) const;
  virtual size_t GetNumberOfGroups(bool bRecursive = false) const;

  virtual bool Flush(bool WXUNUSED(bCurrentOnly) = false) { return true; }

  // rename
  virtual bool RenameEntry(const wxString& oldName, const wxString& newName);
  virtual bool RenameGroup(const wxString& oldName, const wxString& newName);

  // delete
  virtual bool DeleteEntry(const wxString& key, bool bGroupIfEmptyAlso = true);
  virtual bool DeleteGroup(const wxString& key);
  virtual bool DeleteAll();

protected:
  // opens the local key creating it if necessary and returns it
  wxRegKey& LocalKey() const // must be const to be callable from const funcs
  {
      wxRegConfig* self = wxConstCast(this, wxRegConfig);

      if ( !m_keyLocal.IsOpened() )
      {
          // create on demand
          self->m_keyLocal.Create();
      }

      return self->m_keyLocal;
  }

  // implement read/write methods
  virtual bool DoReadString(const wxString& key, wxString *pStr) const;
  virtual bool DoReadLong(const wxString& key, long *plResult) const;

  virtual bool DoWriteString(const wxString& key, const wxString& szValue);
  virtual bool DoWriteLong(const wxString& key, long lValue);

private:
  // no copy ctor/assignment operator
  wxRegConfig(const wxRegConfig&);
  wxRegConfig& operator=(const wxRegConfig&);

  // these keys are opened during all lifetime of wxRegConfig object
  wxRegKey  m_keyLocalRoot,  m_keyLocal,
            m_keyGlobalRoot, m_keyGlobal;

  // current path (not '/' terminated)
  wxString  m_strPath;
};

#endif // wxUSE_CONFIG

#endif // _REGCONF_H
