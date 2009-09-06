///////////////////////////////////////////////////////////////////////////////
// Name:        wx/fileconf.h
// Purpose:     wxFileConfig derivation of wxConfigBase
// Author:      Vadim Zeitlin
// Modified by:
// Created:     07.04.98 (adapted from appconf.cpp)
// RCS-ID:      $Id: fileconf.h 50711 2007-12-15 02:57:58Z VZ $
// Copyright:   (c) 1997 Karsten Ballueder   &  Vadim Zeitlin
//                       Ballueder@usa.net     <zeitlin@dptmaths.ens-cachan.fr>
// Licence:     wxWindows licence
///////////////////////////////////////////////////////////////////////////////

#ifndef   _FILECONF_H
#define   _FILECONF_H

#include "wx/defs.h"

#if wxUSE_CONFIG

#include "wx/textfile.h"
#include "wx/string.h"
#include "wx/confbase.h"

// ----------------------------------------------------------------------------
// wxFileConfig
// ----------------------------------------------------------------------------

/*
  wxFileConfig derives from base Config and implements file based config class,
  i.e. it uses ASCII disk files to store the information. These files are
  alternatively called INI, .conf or .rc in the documentation. They are
  organized in groups or sections, which can nest (i.e. a group contains
  subgroups, which contain their own subgroups &c). Each group has some
  number of entries, which are "key = value" pairs. More precisely, the format
  is:

  # comments are allowed after either ';' or '#' (Win/UNIX standard)

  # blank lines (as above) are ignored

  # global entries are members of special (no name) top group
  written_for = Windows
  platform    = Linux

  # the start of the group 'Foo'
  [Foo]                           # may put comments like this also
  # following 3 lines are entries
  key = value
  another_key = "  strings with spaces in the beginning should be quoted, \
                   otherwise the spaces are lost"
  last_key = but you don't have to put " normally (nor quote them, like here)

  # subgroup of the group 'Foo'
  # (order is not important, only the name is: separator is '/', as in paths)
  [Foo/Bar]
  # entries prefixed with "!" are immutable, i.e. can't be changed if they are
  # set in the system-wide config file
  !special_key = value
  bar_entry = whatever

  [Foo/Bar/Fubar]   # depth is (theoretically :-) unlimited
  # may have the same name as key in another section
  bar_entry = whatever not

  You have {read/write/delete}Entry functions (guess what they do) and also
  setCurrentPath to select current group. enum{Subgroups/Entries} allow you
  to get all entries in the config file (in the current group). Finally,
  flush() writes immediately all changed entries to disk (otherwise it would
  be done automatically in dtor)

  wxFileConfig manages not less than 2 config files for each program: global
  and local (or system and user if you prefer). Entries are read from both of
  them and the local entries override the global ones unless the latter is
  immutable (prefixed with '!') in which case a warning message is generated
  and local value is ignored. Of course, the changes are always written to local
  file only.

  The names of these files can be specified in a number of ways. First of all,
  you can use the standard convention: using the ctor which takes 'strAppName'
  parameter will probably be sufficient for 90% of cases. If, for whatever
  reason you wish to use the files with some other names, you can always use the
  second ctor.

  wxFileConfig also may automatically expand the values of environment variables
  in the entries it reads: for example, if you have an entry
    score_file = $HOME/.score
  a call to Read(&str, "score_file") will return a complete path to .score file
  unless the expansion was previously disabled with SetExpandEnvVars(false) call
  (it's on by default, the current status can be retrieved with
   IsExpandingEnvVars function).
*/
class WXDLLIMPEXP_FWD_BASE wxFileConfigGroup;
class WXDLLIMPEXP_FWD_BASE wxFileConfigEntry;
class WXDLLIMPEXP_FWD_BASE wxFileConfigLineList;

#if wxUSE_STREAMS
class WXDLLIMPEXP_FWD_BASE wxInputStream;
class WXDLLIMPEXP_FWD_BASE wxOutputStream;
#endif // wxUSE_STREAMS

class WXDLLIMPEXP_BASE wxFileConfig : public wxConfigBase
{
public:
  // construct the "standard" full name for global (system-wide) and
  // local (user-specific) config files from the base file name.
  //
  // the following are the filenames returned by this functions:
  //            global                local
  // Unix   /etc/file.ext           ~/.file
  // Win    %windir%\file.ext   %USERPROFILE%\file.ext
  //
  // where file is the basename of szFile, ext is its extension
  // or .conf (Unix) or .ini (Win) if it has none
  static wxString GetGlobalFileName(const wxChar *szFile);
  static wxString GetLocalFileName(const wxChar *szFile);

  // ctor & dtor
    // New constructor: one size fits all. Specify wxCONFIG_USE_LOCAL_FILE or
    // wxCONFIG_USE_GLOBAL_FILE to say which files should be used.
  wxFileConfig(const wxString& appName = wxEmptyString,
               const wxString& vendorName = wxEmptyString,
               const wxString& localFilename = wxEmptyString,
               const wxString& globalFilename = wxEmptyString,
               long style = wxCONFIG_USE_LOCAL_FILE | wxCONFIG_USE_GLOBAL_FILE,
               const wxMBConv& conv = wxConvAuto());

#if wxUSE_STREAMS
    // ctor that takes an input stream.
  wxFileConfig(wxInputStream &inStream, const wxMBConv& conv = wxConvAuto());
#endif // wxUSE_STREAMS

    // dtor will save unsaved data
  virtual ~wxFileConfig();

  // under Unix, set the umask to be used for the file creation, do nothing
  // under other systems
#ifdef __UNIX__
  void SetUmask(int mode) { m_umask = mode; }
#else // !__UNIX__
  void SetUmask(int WXUNUSED(mode)) { }
#endif // __UNIX__/!__UNIX__

  // implement inherited pure virtual functions
  virtual void SetPath(const wxString& strPath);
  virtual const wxString& GetPath() const { return m_strPath; }

  virtual bool GetFirstGroup(wxString& str, long& lIndex) const;
  virtual bool GetNextGroup (wxString& str, long& lIndex) const;
  virtual bool GetFirstEntry(wxString& str, long& lIndex) const;
  virtual bool GetNextEntry (wxString& str, long& lIndex) const;

  virtual size_t GetNumberOfEntries(bool bRecursive = false) const;
  virtual size_t GetNumberOfGroups(bool bRecursive = false) const;

  virtual bool HasGroup(const wxString& strName) const;
  virtual bool HasEntry(const wxString& strName) const;

  virtual bool Flush(bool bCurrentOnly = false);

  virtual bool RenameEntry(const wxString& oldName, const wxString& newName);
  virtual bool RenameGroup(const wxString& oldName, const wxString& newName);

  virtual bool DeleteEntry(const wxString& key, bool bGroupIfEmptyAlso = true);
  virtual bool DeleteGroup(const wxString& szKey);
  virtual bool DeleteAll();

  // additional, wxFileConfig-specific, functionality
#if wxUSE_STREAMS
  // save the entire config file text to the given stream, note that the text
  // won't be saved again in dtor when Flush() is called if you use this method
  // as it won't be "changed" any more
  virtual bool Save(wxOutputStream& os, const wxMBConv& conv = wxConvAuto());
#endif // wxUSE_STREAMS

public:
  // functions to work with this list
  wxFileConfigLineList *LineListAppend(const wxString& str);
  wxFileConfigLineList *LineListInsert(const wxString& str,
                           wxFileConfigLineList *pLine);    // NULL => Prepend()
  void      LineListRemove(wxFileConfigLineList *pLine);
  bool      LineListIsEmpty();

protected:
  virtual bool DoReadString(const wxString& key, wxString *pStr) const;
  virtual bool DoReadLong(const wxString& key, long *pl) const;

  virtual bool DoWriteString(const wxString& key, const wxString& szValue);
  virtual bool DoWriteLong(const wxString& key, long lValue);

private:
  // GetXXXFileName helpers: return ('/' terminated) directory names
  static wxString GetGlobalDir();
  static wxString GetLocalDir();

  // common part of all ctors (assumes that m_str{Local|Global}File are already
  // initialized
  void Init();

  // common part of from dtor and DeleteAll
  void CleanUp();

  // parse the whole file
  void Parse(const wxTextBuffer& buffer, bool bLocal);

  // the same as SetPath("/")
  void SetRootPath();

  // real SetPath() implementation, returns true if path could be set or false
  // if path doesn't exist and createMissingComponents == false
  bool DoSetPath(const wxString& strPath, bool createMissingComponents);

  // set/test the dirty flag
  void SetDirty() { m_isDirty = true; }
  void ResetDirty() { m_isDirty = false; }
  bool IsDirty() const { return m_isDirty; }


  // member variables
  // ----------------
  wxFileConfigLineList *m_linesHead,    // head of the linked list
                       *m_linesTail;    // tail

  wxString    m_strLocalFile,           // local  file name passed to ctor
              m_strGlobalFile;          // global
  wxString    m_strPath;                // current path (not '/' terminated)

  wxFileConfigGroup *m_pRootGroup,      // the top (unnamed) group
                    *m_pCurrentGroup;   // the current group

  wxMBConv    *m_conv;

#ifdef __UNIX__
  int m_umask;                          // the umask to use for file creation
#endif // __UNIX__

  bool m_isDirty;                       // if true, we have unsaved changes

  DECLARE_NO_COPY_CLASS(wxFileConfig)
};

#endif
  // wxUSE_CONFIG

#endif
  //_FILECONF_H

