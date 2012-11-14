/////////////////////////////////////////////////////////////////////////////
// Name:        unix/dir.cpp
// Purpose:     wxDir implementation for Unix/POSIX systems
// Author:      Vadim Zeitlin
// Modified by:
// Created:     08.12.99
// RCS-ID:      $Id: dir.cpp 56867 2008-11-20 18:12:43Z VZ $
// Copyright:   (c) 1999 Vadim Zeitlin <zeitlin@dptmaths.ens-cachan.fr>
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

#ifndef WX_PRECOMP
    #include "wx/intl.h"
    #include "wx/log.h"
#endif // PCH

#include "wx/dir.h"
#include "wx/filefn.h"          // for wxMatchWild

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <dirent.h>

// ----------------------------------------------------------------------------
// macros
// ----------------------------------------------------------------------------

#define M_DIR       ((wxDirData *)m_data)

// ----------------------------------------------------------------------------
// private classes
// ----------------------------------------------------------------------------

// this class stores everything we need to enumerate the files
class wxDirData
{
public:
    wxDirData(const wxString& dirname);
    ~wxDirData();

    bool IsOk() const { return m_dir != NULL; }

    void SetFileSpec(const wxString& filespec) { m_filespec = filespec; }
    void SetFlags(int flags) { m_flags = flags; }

    void Rewind() { rewinddir(m_dir); }
    bool Read(wxString *filename);

    const wxString& GetName() const { return m_dirname; }

private:
    DIR     *m_dir;

    wxString m_dirname;
    wxString m_filespec;

    int      m_flags;
};

// ============================================================================
// implementation
// ============================================================================

// ----------------------------------------------------------------------------
// wxDirData
// ----------------------------------------------------------------------------

#if !defined( __VMS__ ) || ( __VMS_VER >= 70000000 )

wxDirData::wxDirData(const wxString& dirname)
         : m_dirname(dirname)
{
    m_dir = NULL;

    // throw away the trailing slashes
    size_t n = m_dirname.length();
    wxCHECK_RET( n, _T("empty dir name in wxDir") );

    while ( n > 0 && m_dirname[--n] == '/' )
        ;

    m_dirname.Truncate(n + 1);

    // do open the dir
    m_dir = opendir(m_dirname.fn_str());
}

wxDirData::~wxDirData()
{
    if ( m_dir )
    {
        if ( closedir(m_dir) != 0 )
        {
            wxLogLastError(_T("closedir"));
        }
    }
}

bool wxDirData::Read(wxString *filename)
{
    dirent *de = (dirent *)NULL;    // just to silence compiler warnings
    bool matches = false;

    // speed up string concatenation in the loop a bit
    wxString path = m_dirname;
    path += _T('/');
    path.reserve(path.length() + 255);

    wxString de_d_name;

    while ( !matches )
    {
        de = readdir(m_dir);
        if ( !de )
            return false;

#if wxUSE_UNICODE
        de_d_name = wxConvFileName->cMB2WC( de->d_name );
#else
        de_d_name = de->d_name;
#endif

        // don't return "." and ".." unless asked for
        if ( de->d_name[0] == '.' &&
             ((de->d_name[1] == '.' && de->d_name[2] == '\0') ||
              (de->d_name[1] == '\0')) )
        {
            if ( !(m_flags & wxDIR_DOTDOT) )
                continue;

            // we found a valid match
            break;
        }

        // check the type now
        if ( !(m_flags & wxDIR_FILES) && !wxDir::Exists(path + de_d_name) )
        {
            // it's a file, but we don't want them
            continue;
        }
        else if ( !(m_flags & wxDIR_DIRS) && wxDir::Exists(path + de_d_name) )
        {
            // it's a dir, and we don't want it
            continue;
        }

        // finally, check the name
        if ( m_filespec.empty() )
        {
            matches = m_flags & wxDIR_HIDDEN ? true : de->d_name[0] != '.';
        }
        else
        {
            // test against the pattern
            matches = wxMatchWild(m_filespec, de_d_name,
                                  !(m_flags & wxDIR_HIDDEN));
        }
    }

    *filename = de_d_name;

    return true;
}

#else // old VMS (TODO)

wxDirData::wxDirData(const wxString& WXUNUSED(dirname))
{
    wxFAIL_MSG(_T("not implemented"));
}

wxDirData::~wxDirData()
{
}

bool wxDirData::Read(wxString * WXUNUSED(filename))
{
    return false;
}

#endif // not or new VMS/old VMS

// ----------------------------------------------------------------------------
// wxDir helpers
// ----------------------------------------------------------------------------

/* static */
bool wxDir::Exists(const wxString& dir)
{
    return wxDirExists(dir);
}

// ----------------------------------------------------------------------------
// wxDir construction/destruction
// ----------------------------------------------------------------------------

wxDir::wxDir(const wxString& dirname)
{
    m_data = NULL;

    (void)Open(dirname);
}

bool wxDir::Open(const wxString& dirname)
{
    delete M_DIR;
    m_data = new wxDirData(dirname);

    if ( !M_DIR->IsOk() )
    {
        delete M_DIR;
        m_data = NULL;

        return false;
    }

    return true;
}

bool wxDir::IsOpened() const
{
    return m_data != NULL;
}

wxString wxDir::GetName() const
{
    wxString name;
    if ( m_data )
    {
        name = M_DIR->GetName();
        if ( !name.empty() && (name.Last() == _T('/')) )
        {
            // chop off the last (back)slash
            name.Truncate(name.length() - 1);
        }
    }

    return name;
}

wxDir::~wxDir()
{
    delete M_DIR;
}

// ----------------------------------------------------------------------------
// wxDir enumerating
// ----------------------------------------------------------------------------

bool wxDir::GetFirst(wxString *filename,
                     const wxString& filespec,
                     int flags) const
{
    wxCHECK_MSG( IsOpened(), false, _T("must wxDir::Open() first") );

    M_DIR->Rewind();

    M_DIR->SetFileSpec(filespec);
    M_DIR->SetFlags(flags);

    return GetNext(filename);
}

bool wxDir::GetNext(wxString *filename) const
{
    wxCHECK_MSG( IsOpened(), false, _T("must wxDir::Open() first") );

    wxCHECK_MSG( filename, false, _T("bad pointer in wxDir::GetNext()") );

    return M_DIR->Read(filename);
}

bool wxDir::HasSubDirs(const wxString& spec)
{
    wxCHECK_MSG( IsOpened(), false, _T("must wxDir::Open() first") );

    if ( spec.empty() )
    {
        // faster check for presence of any subdirectory: normally each subdir
        // has a hard link to the parent directory and so, knowing that there
        // are at least "." and "..", we have a subdirectory if and only if
        // links number is > 2 - this is just a guess but it works fairly well
        // in practice
        //
        // note that we may guess wrongly in one direction only: i.e. we may
        // return true when there are no subdirectories but this is ok as the
        // caller will learn it soon enough when it calls GetFirst(wxDIR)
        // anyhow
        wxStructStat stBuf;
        if ( wxStat(M_DIR->GetName().c_str(), &stBuf) == 0 )
        {
            switch ( stBuf.st_nlink )
            {
                case 2:
                    // just "." and ".."
                    return false;

                case 0:
                case 1:
                    // weird filesystem, don't try to guess for it, use dumb
                    // method below
                    break;

                default:
                    // assume we have subdirs - may turn out to be wrong if we
                    // have other hard links to this directory but it's not
                    // that bad as explained above
                    return true;
            }
        }
    }

    // just try to find first directory
    wxString s;
    return GetFirst(&s, spec, wxDIR_DIRS | wxDIR_HIDDEN);
}

