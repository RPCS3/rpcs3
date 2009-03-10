/////////////////////////////////////////////////////////////////////////////
// Name:        file.cpp
// Purpose:     wxFile - encapsulates low-level "file descriptor"
//              wxTempFile
// Author:      Vadim Zeitlin
// Modified by:
// Created:     29/01/98
// RCS-ID:      $Id: file.cpp 42876 2006-10-31 23:29:02Z SN $
// Copyright:   (c) 1998 Vadim Zeitlin <zeitlin@dptmaths.ens-cachan.fr>
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

// For compilers that support precompilation, includes "wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
  #pragma hdrstop
#endif

#if wxUSE_FILE

// standard
#if defined(__WXMSW__) && !defined(__GNUWIN32__) && !defined(__WXMICROWIN__) && !defined(__WXWINCE__)

#ifndef __SALFORDC__
    #define   WIN32_LEAN_AND_MEAN
    #define   NOSERVICE
    #define   NOIME
    #define   NOATOM
    #define   NOGDI
    #define   NOGDICAPMASKS
    #define   NOMETAFILE
    #define   NOMINMAX
    #define   NOMSG
    #define   NOOPENFILE
    #define   NORASTEROPS
    #define   NOSCROLL
    #define   NOSOUND
    #define   NOSYSMETRICS
    #define   NOTEXTMETRIC
    #define   NOWH
    #define   NOCOMM
    #define   NOKANJI
    #define   NOCRYPT
    #define   NOMCX
#endif

#elif defined(__WXMSW__) && defined(__WXWINCE__)
    #include  "wx/msw/missing.h"
#elif (defined(__OS2__))
    #include <io.h>
#elif (defined(__UNIX__) || defined(__GNUWIN32__))
    #include  <unistd.h>
    #include  <time.h>
    #include  <sys/stat.h>
    #ifdef __GNUWIN32__
        #include "wx/msw/wrapwin.h"
    #endif
#elif defined(__DOS__)
    #if defined(__WATCOMC__)
       #include <io.h>
    #elif defined(__DJGPP__)
       #include <io.h>
       #include <unistd.h>
       #include <stdio.h>
    #else
        #error  "Please specify the header with file functions declarations."
    #endif
#elif (defined(__WXSTUBS__))
    // Have to ifdef this for different environments
    #include <io.h>
#elif (defined(__WXMAC__))
#if __MSL__ < 0x6000
    int access( const char *path, int mode ) { return 0 ; }
#else
    int _access( const char *path, int mode ) { return 0 ; }
#endif
    char* mktemp( char * path ) { return path ;}
    #include <stat.h>
    #include <unistd.h>
#else
    #error  "Please specify the header with file functions declarations."
#endif  //Win/UNIX

#include  <stdio.h>       // SEEK_xxx constants

// Windows compilers don't have these constants
#ifndef W_OK
    enum
    {
        F_OK = 0,   // test for existence
        X_OK = 1,   //          execute permission
        W_OK = 2,   //          write
        R_OK = 4    //          read
    };
#endif // W_OK

#ifdef __SALFORDC__
    #include <unix.h>
#endif

// some broken compilers don't have 3rd argument in open() and creat()
#ifdef __SALFORDC__
    #define ACCESS(access)
    #define stat    _stat
#else // normal compiler
    #define ACCESS(access)  , (access)
#endif // Salford C

// wxWidgets
#ifndef WX_PRECOMP
    #include  "wx/string.h"
    #include  "wx/intl.h"
    #include  "wx/log.h"
#endif // !WX_PRECOMP

#include  "wx/filename.h"
#include  "wx/file.h"
#include  "wx/filefn.h"

// there is no distinction between text and binary files under Unix, so define
// O_BINARY as 0 if the system headers don't do it already
#if defined(__UNIX__) && !defined(O_BINARY)
    #define   O_BINARY    (0)
#endif  //__UNIX__

#ifdef __WXMSW__
    #include "wx/msw/mslu.h"
#endif

#ifdef __WXWINCE__
    #include "wx/msw/private.h"
#endif

#ifndef MAX_PATH
    #define MAX_PATH 512
#endif

// ============================================================================
// implementation of wxFile
// ============================================================================

// ----------------------------------------------------------------------------
// static functions
// ----------------------------------------------------------------------------

bool wxFile::Exists(const wxChar *name)
{
    return wxFileExists(name);
}

bool wxFile::Access(const wxChar *name, OpenMode mode)
{
    int how;

    switch ( mode )
    {
        default:
            wxFAIL_MSG(wxT("bad wxFile::Access mode parameter."));
            // fall through

        case read:
            how = R_OK;
            break;

        case write:
            how = W_OK;
            break;

        case read_write:
            how = R_OK | W_OK;
            break;
    }

    return wxAccess(name, how) == 0;
}

// ----------------------------------------------------------------------------
// opening/closing
// ----------------------------------------------------------------------------

// ctors
wxFile::wxFile(const wxChar *szFileName, OpenMode mode)
{
    m_fd = fd_invalid;
    m_error = false;

    Open(szFileName, mode);
}

// create the file, fail if it already exists and bOverwrite
bool wxFile::Create(const wxChar *szFileName, bool bOverwrite, int accessMode)
{
    // if bOverwrite we create a new file or truncate the existing one,
    // otherwise we only create the new file and fail if it already exists
#if defined(__WXMAC__) && !defined(__UNIX__) && !wxUSE_UNICODE
    // Dominic Mazzoni [dmazzoni+@cs.cmu.edu] reports that open is still broken on the mac, so we replace
    // int fd = open( szFileName , O_CREAT | (bOverwrite ? O_TRUNC : O_EXCL), access);
    int fd = creat( szFileName , accessMode);
#else
    int fd = wxOpen( szFileName,
                     O_BINARY | O_WRONLY | O_CREAT |
                     (bOverwrite ? O_TRUNC : O_EXCL)
                     ACCESS(accessMode) );
#endif
    if ( fd == -1 )
    {
        wxLogSysError(_("can't create file '%s'"), szFileName);
        return false;
    }

    Attach(fd);
    return true;
}

// open the file
bool wxFile::Open(const wxChar *szFileName, OpenMode mode, int accessMode)
{
    int flags = O_BINARY;

    switch ( mode )
    {
        case read:
            flags |= O_RDONLY;
            break;

        case write_append:
            if ( wxFile::Exists(szFileName) )
            {
                flags |= O_WRONLY | O_APPEND;
                break;
            }
            //else: fall through as write_append is the same as write if the
            //      file doesn't exist

        case write:
            flags |= O_WRONLY | O_CREAT | O_TRUNC;
            break;

        case write_excl:
            flags |= O_WRONLY | O_CREAT | O_EXCL;
            break;

        case read_write:
            flags |= O_RDWR;
            break;
    }

#ifdef __WINDOWS__
    // only read/write bits for "all" are supported by this function under
    // Windows, and VC++ 8 returns EINVAL if any other bits are used in
    // accessMode, so clear them as they have at best no effect anyhow
    accessMode &= wxS_IRUSR | wxS_IWUSR;
#endif // __WINDOWS__

    int fd = wxOpen( szFileName, flags ACCESS(accessMode));

    if ( fd == -1 )
    {
        wxLogSysError(_("can't open file '%s'"), szFileName);
        return false;
    }

    Attach(fd);
    return true;
}

// close
bool wxFile::Close()
{
    if ( IsOpened() ) {
        if (wxClose(m_fd) == -1)
        {
            wxLogSysError(_("can't close file descriptor %d"), m_fd);
            m_fd = fd_invalid;
            return false;
        }
        else
            m_fd = fd_invalid;
    }

    return true;
}

// ----------------------------------------------------------------------------
// read/write
// ----------------------------------------------------------------------------

// read
ssize_t wxFile::Read(void *pBuf, size_t nCount)
{
    wxCHECK( (pBuf != NULL) && IsOpened(), 0 );

    ssize_t iRc = wxRead(m_fd, pBuf, nCount);

    if ( iRc == -1 )
    {
        wxLogSysError(_("can't read from file descriptor %d"), m_fd);
        return wxInvalidOffset;
    }

    return iRc;
}

// write
size_t wxFile::Write(const void *pBuf, size_t nCount)
{
    wxCHECK( (pBuf != NULL) && IsOpened(), 0 );

    ssize_t iRc = wxWrite(m_fd, pBuf, nCount);

    if ( iRc == -1 )
    {
        wxLogSysError(_("can't write to file descriptor %d"), m_fd);
        m_error = true;
        iRc = 0;
    }

    return iRc;
}

// flush
bool wxFile::Flush()
{
#ifdef HAVE_FSYNC
    // fsync() only works on disk files and returns errors for pipes, don't
    // call it then
    if ( IsOpened() && GetKind() == wxFILE_KIND_DISK )
    {
        if ( wxFsync(m_fd) == -1 )
        {
            wxLogSysError(_("can't flush file descriptor %d"), m_fd);
            return false;
        }
    }
#endif // HAVE_FSYNC

    return true;
}

// ----------------------------------------------------------------------------
// seek
// ----------------------------------------------------------------------------

// seek
wxFileOffset wxFile::Seek(wxFileOffset ofs, wxSeekMode mode)
{
    wxASSERT_MSG( IsOpened(), _T("can't seek on closed file") );
    wxCHECK_MSG( ofs != wxInvalidOffset || mode != wxFromStart,
                 wxInvalidOffset,
                 _T("invalid absolute file offset") );

    int origin;
    switch ( mode ) {
        default:
            wxFAIL_MSG(_("unknown seek origin"));

        case wxFromStart:
            origin = SEEK_SET;
            break;

        case wxFromCurrent:
            origin = SEEK_CUR;
            break;

        case wxFromEnd:
            origin = SEEK_END;
            break;
    }

    wxFileOffset iRc = wxSeek(m_fd, ofs, origin);
    if ( iRc == wxInvalidOffset )
    {
        wxLogSysError(_("can't seek on file descriptor %d"), m_fd);
    }

    return iRc;
}

// get current file offset
wxFileOffset wxFile::Tell() const
{
    wxASSERT( IsOpened() );

    wxFileOffset iRc = wxTell(m_fd);
    if ( iRc == wxInvalidOffset )
    {
        wxLogSysError(_("can't get seek position on file descriptor %d"), m_fd);
    }

    return iRc;
}

// get current file length
wxFileOffset wxFile::Length() const
{
    wxASSERT( IsOpened() );

    wxFileOffset iRc = Tell();
    if ( iRc != wxInvalidOffset ) {
        // have to use const_cast :-(
        wxFileOffset iLen = ((wxFile *)this)->SeekEnd();
        if ( iLen != wxInvalidOffset ) {
            // restore old position
            if ( ((wxFile *)this)->Seek(iRc) == wxInvalidOffset ) {
                // error
                iLen = wxInvalidOffset;
            }
        }

        iRc = iLen;
    }

    if ( iRc == wxInvalidOffset )
    {
        wxLogSysError(_("can't find length of file on file descriptor %d"), m_fd);
    }

    return iRc;
}

// is end of file reached?
bool wxFile::Eof() const
{
    wxASSERT( IsOpened() );

    wxFileOffset iRc;

#if defined(__DOS__) || defined(__UNIX__) || defined(__GNUWIN32__) || defined( __MWERKS__ ) || defined(__SALFORDC__)
    // @@ this doesn't work, of course, on unseekable file descriptors
    wxFileOffset ofsCur = Tell(),
    ofsMax = Length();
    if ( ofsCur == wxInvalidOffset || ofsMax == wxInvalidOffset )
        iRc = wxInvalidOffset;
    else
        iRc = ofsCur == ofsMax;
#else  // Windows and "native" compiler
    iRc = wxEof(m_fd);
#endif // Windows/Unix

    if ( iRc == 1)
        {}
    else if ( iRc == 0 )
        return false;
    else if ( iRc == wxInvalidOffset )
        wxLogSysError(_("can't determine if the end of file is reached on descriptor %d"), m_fd);
    else
        wxFAIL_MSG(_("invalid eof() return value."));

    return true;
}

// ============================================================================
// implementation of wxTempFile
// ============================================================================

// ----------------------------------------------------------------------------
// construction
// ----------------------------------------------------------------------------

wxTempFile::wxTempFile(const wxString& strName)
{
    Open(strName);
}

bool wxTempFile::Open(const wxString& strName)
{
    // we must have an absolute filename because otherwise CreateTempFileName()
    // would create the temp file in $TMP (i.e. the system standard location
    // for the temp files) which might be on another volume/drive/mount and
    // wxRename()ing it later to m_strName from Commit() would then fail
    //
    // with the absolute filename, the temp file is created in the same
    // directory as this one which ensures that wxRename() may work later
    wxFileName fn(strName);
    if ( !fn.IsAbsolute() )
    {
        fn.Normalize(wxPATH_NORM_ABSOLUTE);
    }

    m_strName = fn.GetFullPath();

    m_strTemp = wxFileName::CreateTempFileName(m_strName, &m_file);

    if ( m_strTemp.empty() )
    {
        // CreateTempFileName() failed
        return false;
    }

#ifdef __UNIX__
    // the temp file should have the same permissions as the original one
    mode_t mode;

    wxStructStat st;
    if ( stat( (const char*) m_strName.fn_str(), &st) == 0 )
    {
        mode = st.st_mode;
    }
    else
    {
        // file probably didn't exist, just give it the default mode _using_
        // user's umask (new files creation should respect umask)
        mode_t mask = umask(0777);
        mode = 0666 & ~mask;
        umask(mask);
    }

    if ( chmod( (const char*) m_strTemp.fn_str(), mode) == -1 )
    {
#ifndef __OS2__
        wxLogSysError(_("Failed to set temporary file permissions"));
#endif
    }
#endif // Unix

    return true;
}

// ----------------------------------------------------------------------------
// destruction
// ----------------------------------------------------------------------------

wxTempFile::~wxTempFile()
{
    if ( IsOpened() )
        Discard();
}

bool wxTempFile::Commit()
{
    m_file.Close();

    if ( wxFile::Exists(m_strName) && wxRemove(m_strName) != 0 ) {
        wxLogSysError(_("can't remove file '%s'"), m_strName.c_str());
        return false;
    }

    if ( !wxRenameFile(m_strTemp, m_strName)  ) {
        wxLogSysError(_("can't commit changes to file '%s'"), m_strName.c_str());
        return false;
    }

    return true;
}

void wxTempFile::Discard()
{
    m_file.Close();
    if ( wxRemove(m_strTemp) != 0 )
        wxLogSysError(_("can't remove temporary file '%s'"), m_strTemp.c_str());
}

#endif // wxUSE_FILE

