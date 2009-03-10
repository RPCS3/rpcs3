///////////////////////////////////////////////////////////////////////////////
// Name:        src/common/dircmn.cpp
// Purpose:     wxDir methods common to all implementations
// Author:      Vadim Zeitlin
// Modified by:
// Created:     19.05.01
// RCS-ID:      $Id: dircmn.cpp 40665 2006-08-19 08:45:31Z JS $
// Copyright:   (c) 2001 Vadim Zeitlin <zeitlin@dptmaths.ens-cachan.fr>
// License:     wxWindows licence
///////////////////////////////////////////////////////////////////////////////

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
    #include "wx/string.h"
    #include "wx/log.h"
    #include "wx/intl.h"
    #include "wx/filefn.h"
    #include "wx/arrstr.h"
#endif //WX_PRECOMP

#include "wx/dir.h"
#include "wx/filename.h"

// ============================================================================
// implementation
// ============================================================================

// ----------------------------------------------------------------------------
// wxDirTraverser
// ----------------------------------------------------------------------------

wxDirTraverseResult
wxDirTraverser::OnOpenError(const wxString& WXUNUSED(dirname))
{
    return wxDIR_IGNORE;
}

// ----------------------------------------------------------------------------
// wxDir::HasFiles() and HasSubDirs()
// ----------------------------------------------------------------------------

// dumb generic implementation

bool wxDir::HasFiles(const wxString& spec)
{
    wxString s;
    return GetFirst(&s, spec, wxDIR_FILES | wxDIR_HIDDEN);
}

// we have a (much) faster version for Unix
#if (defined(__CYGWIN__) && defined(__WINDOWS__)) || !defined(__UNIX_LIKE__) || defined(__WXMAC__) || defined(__EMX__) || defined(__WINE__)

bool wxDir::HasSubDirs(const wxString& spec)
{
    wxString s;
    return GetFirst(&s, spec, wxDIR_DIRS | wxDIR_HIDDEN);
}

#endif // !Unix

// ----------------------------------------------------------------------------
// wxDir::Traverse()
// ----------------------------------------------------------------------------

size_t wxDir::Traverse(wxDirTraverser& sink,
                       const wxString& filespec,
                       int flags) const
{
    wxCHECK_MSG( IsOpened(), (size_t)-1,
                 _T("dir must be opened before traversing it") );

    // the total number of files found
    size_t nFiles = 0;

    // the name of this dir with path delimiter at the end
    wxString prefix = GetName();
    prefix += wxFILE_SEP_PATH;

    // first, recurse into subdirs
    if ( flags & wxDIR_DIRS )
    {
        wxString dirname;
        for ( bool cont = GetFirst(&dirname, wxEmptyString, wxDIR_DIRS | (flags & wxDIR_HIDDEN) );
              cont;
              cont = cont && GetNext(&dirname) )
        {
            const wxString fulldirname = prefix + dirname;

            switch ( sink.OnDir(fulldirname) )
            {
                default:
                    wxFAIL_MSG(_T("unexpected OnDir() return value") );
                    // fall through

                case wxDIR_STOP:
                    cont = false;
                    break;

                case wxDIR_CONTINUE:
                    {
                        wxDir subdir;

                        // don't give the error messages for the directories
                        // which we can't open: there can be all sorts of good
                        // reason for this (e.g. insufficient privileges) and
                        // this shouldn't be treated as an error -- instead
                        // let the user code decide what to do
                        bool ok;
                        do
                        {
                            wxLogNull noLog;
                            ok = subdir.Open(fulldirname);
                            if ( !ok )
                            {
                                // ask the user code what to do
                                bool tryagain;
                                switch ( sink.OnOpenError(fulldirname) )
                                {
                                    default:
                                        wxFAIL_MSG(_T("unexpected OnOpenError() return value") );
                                        // fall through

                                    case wxDIR_STOP:
                                        cont = false;
                                        // fall through

                                    case wxDIR_IGNORE:
                                        tryagain = false;
                                        break;

                                    case wxDIR_CONTINUE:
                                        tryagain = true;
                                }

                                if ( !tryagain )
                                    break;
                            }
                        }
                        while ( !ok );

                        if ( ok )
                        {
                            nFiles += subdir.Traverse(sink, filespec, flags);
                        }
                    }
                    break;

                case wxDIR_IGNORE:
                    // nothing to do
                    ;
            }
        }
    }

    // now enum our own files
    if ( flags & wxDIR_FILES )
    {
        flags &= ~wxDIR_DIRS;

        wxString filename;
        bool cont = GetFirst(&filename, filespec, flags);
        while ( cont )
        {
            wxDirTraverseResult res = sink.OnFile(prefix + filename);
            if ( res == wxDIR_STOP )
                break;

            wxASSERT_MSG( res == wxDIR_CONTINUE,
                          _T("unexpected OnFile() return value") );

            nFiles++;

            cont = GetNext(&filename);
        }
    }

    return nFiles;
}

// ----------------------------------------------------------------------------
// wxDir::GetAllFiles()
// ----------------------------------------------------------------------------

class wxDirTraverserSimple : public wxDirTraverser
{
public:
    wxDirTraverserSimple(wxArrayString& files) : m_files(files) { }

    virtual wxDirTraverseResult OnFile(const wxString& filename)
    {
        m_files.push_back(filename);
        return wxDIR_CONTINUE;
    }

    virtual wxDirTraverseResult OnDir(const wxString& WXUNUSED(dirname))
    {
        return wxDIR_CONTINUE;
    }

private:
    wxArrayString& m_files;

    DECLARE_NO_COPY_CLASS(wxDirTraverserSimple)
};

/* static */
size_t wxDir::GetAllFiles(const wxString& dirname,
                          wxArrayString *files,
                          const wxString& filespec,
                          int flags)
{
    wxCHECK_MSG( files, (size_t)-1, _T("NULL pointer in wxDir::GetAllFiles") );

    size_t nFiles = 0;

    wxDir dir(dirname);
    if ( dir.IsOpened() )
    {
        wxDirTraverserSimple traverser(*files);

        nFiles += dir.Traverse(traverser, filespec, flags);
    }

    return nFiles;
}

// ----------------------------------------------------------------------------
// wxDir::FindFirst()
// ----------------------------------------------------------------------------

class wxDirTraverserFindFirst : public wxDirTraverser
{
public:
    wxDirTraverserFindFirst() { }

    virtual wxDirTraverseResult OnFile(const wxString& filename)
    {
        m_file = filename;
        return wxDIR_STOP;
    }

    virtual wxDirTraverseResult OnDir(const wxString& WXUNUSED(dirname))
    {
        return wxDIR_CONTINUE;
    }

    const wxString& GetFile() const
    {
        return m_file;
    }

private:
    wxString m_file;

    DECLARE_NO_COPY_CLASS(wxDirTraverserFindFirst)
};

/* static */
wxString wxDir::FindFirst(const wxString& dirname,
                          const wxString& filespec,
                          int flags)
{
    wxDir dir(dirname);
    if ( dir.IsOpened() )
    {
        wxDirTraverserFindFirst traverser;

        dir.Traverse(traverser, filespec, flags | wxDIR_FILES);
        return traverser.GetFile();
    }

    return wxEmptyString;
}


// ----------------------------------------------------------------------------
// wxDir::GetTotalSize()
// ----------------------------------------------------------------------------

class wxDirTraverserSumSize : public wxDirTraverser
{
public:
    wxDirTraverserSumSize() { }

    virtual wxDirTraverseResult OnFile(const wxString& filename)
    {
        wxULongLong sz = wxFileName::GetSize(filename);

        // wxFileName::GetSize won't use this class again as
        // we're passing it a file and not a directory;
        // thus we are sure to avoid an endless loop
        if (sz == wxInvalidSize)
        {
            // if the GetSize() failed (this can happen because e.g. a
            // file is locked by another process), we can proceed but
            // we need to at least warn the user that the resulting
            // final size could be not reliable (if e.g. the locked
            // file is very big).
            m_skippedFiles.Add(filename);
            return wxDIR_CONTINUE;
        }

        m_sz += sz;
        return wxDIR_CONTINUE;
    }

    virtual wxDirTraverseResult OnDir(const wxString& WXUNUSED(dirname))
    {
        return wxDIR_CONTINUE;
    }

    wxULongLong GetTotalSize() const
        { return m_sz; }
    wxArrayString &FilesSkipped()
        { return m_skippedFiles; }

protected:
    wxULongLong m_sz;
    wxArrayString m_skippedFiles;
};

wxULongLong wxDir::GetTotalSize(const wxString &dirname, wxArrayString *filesSkipped)
{
    if (!wxDirExists(dirname))
        return wxInvalidSize;

    // to get the size of this directory and its contents we need
    // to recursively walk it...
    wxDir dir(dirname);
    if ( !dir.IsOpened() )
        return wxInvalidSize;

    wxDirTraverserSumSize traverser;
    if (dir.Traverse(traverser) == (size_t)-1 ||
        traverser.GetTotalSize() == 0)
        return wxInvalidSize;

    if (filesSkipped)
        *filesSkipped = traverser.FilesSkipped();

    return traverser.GetTotalSize();
}

