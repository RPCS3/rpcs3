/////////////////////////////////////////////////////////////////////////////
// Name:        wx/dir.h
// Purpose:     wxDir is a class for enumerating the files in a directory
// Author:      Vadim Zeitlin
// Modified by:
// Created:     08.12.99
// RCS-ID:      $Id: dir.h 53135 2008-04-12 02:31:04Z VZ $
// Copyright:   (c) 1999 Vadim Zeitlin <zeitlin@dptmaths.ens-cachan.fr>
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef _WX_DIR_H_
#define _WX_DIR_H_

#include "wx/longlong.h"
#include "wx/string.h"

class WXDLLIMPEXP_FWD_BASE wxArrayString;

// ----------------------------------------------------------------------------
// constants
// ----------------------------------------------------------------------------

// these flags define what kind of filenames is included in the list of files
// enumerated by GetFirst/GetNext
enum
{
    wxDIR_FILES     = 0x0001,       // include files
    wxDIR_DIRS      = 0x0002,       // include directories
    wxDIR_HIDDEN    = 0x0004,       // include hidden files
    wxDIR_DOTDOT    = 0x0008,       // include '.' and '..'

    // by default, enumerate everything except '.' and '..'
    wxDIR_DEFAULT   = wxDIR_FILES | wxDIR_DIRS | wxDIR_HIDDEN
};

// these constants are possible return value of wxDirTraverser::OnDir()
enum wxDirTraverseResult
{
    wxDIR_IGNORE = -1,      // ignore this directory but continue with others
    wxDIR_STOP,             // stop traversing
    wxDIR_CONTINUE          // continue into this directory
};

// ----------------------------------------------------------------------------
// wxDirTraverser: helper class for wxDir::Traverse()
// ----------------------------------------------------------------------------

class WXDLLIMPEXP_BASE wxDirTraverser
{
public:
    /// a virtual dtor has been provided since this class has virtual members
    virtual ~wxDirTraverser() { }
    // called for each file found by wxDir::Traverse()
    //
    // return wxDIR_STOP or wxDIR_CONTINUE from here (wxDIR_IGNORE doesn't
    // make sense)
    virtual wxDirTraverseResult OnFile(const wxString& filename) = 0;

    // called for each directory found by wxDir::Traverse()
    //
    // return one of the enum elements defined above
    virtual wxDirTraverseResult OnDir(const wxString& dirname) = 0;

    // called for each directory which we couldn't open during our traversal
    // of the directory tyree
    //
    // this method can also return either wxDIR_STOP, wxDIR_IGNORE or
    // wxDIR_CONTINUE but the latter is treated specially: it means to retry
    // opening the directory and so may lead to infinite loop if it is
    // returned unconditionally, be careful with this!
    //
    // the base class version always returns wxDIR_IGNORE
    virtual wxDirTraverseResult OnOpenError(const wxString& dirname);
};

// ----------------------------------------------------------------------------
// wxDir: portable equivalent of {open/read/close}dir functions
// ----------------------------------------------------------------------------

class WXDLLIMPEXP_FWD_BASE wxDirData;

class WXDLLIMPEXP_BASE wxDir
{
public:
    // test for existence of a directory with the given name
    static bool Exists(const wxString& dir);

    // ctors
    // -----

    // default, use Open()
    wxDir() { m_data = NULL; }

    // opens the directory for enumeration, use IsOpened() to test success
    wxDir(const wxString& dir);

    // dtor cleans up the associated ressources
    ~wxDir();

    // open the directory for enumerating
    bool Open(const wxString& dir);

    // returns true if the directory was successfully opened
    bool IsOpened() const;

    // get the full name of the directory (without '/' at the end)
    wxString GetName() const;

    // file enumeration routines
    // -------------------------

    // start enumerating all files matching filespec (or all files if it is
    // empty) and flags, return true on success
    bool GetFirst(wxString *filename,
                  const wxString& filespec = wxEmptyString,
                  int flags = wxDIR_DEFAULT) const;

    // get next file in the enumeration started with GetFirst()
    bool GetNext(wxString *filename) const;

    // return true if this directory has any files in it
    bool HasFiles(const wxString& spec = wxEmptyString);

    // return true if this directory has any subdirectories
    bool HasSubDirs(const wxString& spec = wxEmptyString);

    // enumerate all files in this directory and its subdirectories
    //
    // return the number of files found
    size_t Traverse(wxDirTraverser& sink,
                    const wxString& filespec = wxEmptyString,
                    int flags = wxDIR_DEFAULT) const;

    // simplest version of Traverse(): get the names of all files under this
    // directory into filenames array, return the number of files
    static size_t GetAllFiles(const wxString& dirname,
                              wxArrayString *files,
                              const wxString& filespec = wxEmptyString,
                              int flags = wxDIR_DEFAULT);

    // check if there any files matching the given filespec under the given
    // directory (i.e. searches recursively), return the file path if found or
    // empty string otherwise
    static wxString FindFirst(const wxString& dirname,
                              const wxString& filespec,
                              int flags = wxDIR_DEFAULT);

    // returns the size of all directories recursively found in given path
    static wxULongLong GetTotalSize(const wxString &dir, wxArrayString *filesSkipped = NULL);

private:
    friend class wxDirData;

    wxDirData *m_data;

    DECLARE_NO_COPY_CLASS(wxDir)
};

#endif // _WX_DIR_H_

