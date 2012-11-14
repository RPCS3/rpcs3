/////////////////////////////////////////////////////////////////////////////
// Name:        src/common/filename.cpp
// Purpose:     wxFileName - encapsulates a file path
// Author:      Robert Roebling, Vadim Zeitlin
// Modified by:
// Created:     28.12.2000
// RCS-ID:      $Id: filename.cpp 66915 2011-02-16 21:46:49Z JS $
// Copyright:   (c) 2000 Robert Roebling
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

/*
   Here are brief descriptions of the filename formats supported by this class:

   wxPATH_UNIX: standard Unix format, used under Darwin as well, absolute file
                names have the form:
                /dir1/dir2/.../dirN/filename, "." and ".." stand for the
                current and parent directory respectively, "~" is parsed as the
                user HOME and "~username" as the HOME of that user

   wxPATH_DOS:  DOS/Windows format, absolute file names have the form:
                drive:\dir1\dir2\...\dirN\filename.ext where drive is a single
                letter. "." and ".." as for Unix but no "~".

                There are also UNC names of the form \\share\fullpath

   wxPATH_MAC:  Mac OS 8/9 and Mac OS X under CodeWarrior 7 format, absolute file
                names have the form
                    volume:dir1:...:dirN:filename
                and the relative file names are either
                    :dir1:...:dirN:filename
                or just
                    filename
                (although :filename works as well).
                Since the volume is just part of the file path, it is not
                treated like a separate entity as it is done under DOS and
                VMS, it is just treated as another dir.

   wxPATH_VMS:  VMS native format, absolute file names have the form
                    <device>:[dir1.dir2.dir3]file.txt
                or
                    <device>:[000000.dir1.dir2.dir3]file.txt

                the <device> is the physical device (i.e. disk). 000000 is the
                root directory on the device which can be omitted.

                Note that VMS uses different separators unlike Unix:
                 : always after the device. If the path does not contain : than
                   the default (the device of the current directory) is assumed.
                 [ start of directory specification
                 . separator between directory and subdirectory
                 ] between directory and file
 */

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
    #ifdef __WXMSW__
        #include "wx/msw/wrapwin.h" // For GetShort/LongPathName
    #endif
    #include "wx/dynarray.h"
    #include "wx/intl.h"
    #include "wx/log.h"
    #include "wx/utils.h"
#endif

#include "wx/filename.h"
#include "wx/private/filename.h"
#include "wx/tokenzr.h"
#include "wx/config.h"          // for wxExpandEnvVars
#include "wx/dynlib.h"

#if defined(__WIN32__) && defined(__MINGW32__)
    #include "wx/msw/gccpriv.h"
#endif

#ifdef __WXWINCE__
#include "wx/msw/private.h"
#endif

#if defined(__WXMAC__)
  #include  "wx/mac/private.h"  // includes mac headers
#endif

// utime() is POSIX so should normally be available on all Unices
#ifdef __UNIX_LIKE__
#include <sys/types.h>
#include <utime.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

#ifdef __DJGPP__
#include <unistd.h>
#endif

#ifdef __MWERKS__
#ifdef __MACH__
#include <sys/types.h>
#include <utime.h>
#include <sys/stat.h>
#include <unistd.h>
#else
#include <stat.h>
#include <unistd.h>
#include <unix.h>
#endif
#endif

#ifdef __WATCOMC__
#include <io.h>
#include <sys/utime.h>
#include <sys/stat.h>
#endif

#ifdef __VISAGECPP__
#ifndef MAX_PATH
#define MAX_PATH 256
#endif
#endif

#ifdef __EMX__
#include <os2.h>
#define MAX_PATH _MAX_PATH
#endif


wxULongLong wxInvalidSize = (unsigned)-1;


// ----------------------------------------------------------------------------
// private classes
// ----------------------------------------------------------------------------

// small helper class which opens and closes the file - we use it just to get
// a file handle for the given file name to pass it to some Win32 API function
#if defined(__WIN32__) && !defined(__WXMICROWIN__)

class wxFileHandle
{
public:
    enum OpenMode
    {
        Read,
        Write
    };

    wxFileHandle(const wxString& filename, OpenMode mode, int flags = 0)
    {
        m_hFile = ::CreateFile
                    (
                     filename,                      // name
                     mode == Read ? GENERIC_READ    // access mask
                                  : GENERIC_WRITE,
                     FILE_SHARE_READ |              // sharing mode
                     FILE_SHARE_WRITE,              // (allow everything)
                     NULL,                          // no secutity attr
                     OPEN_EXISTING,                 // creation disposition
                     flags,                         // flags
                     NULL                           // no template file
                    );

        if ( m_hFile == INVALID_HANDLE_VALUE )
        {
            wxLogSysError(_("Failed to open '%s' for %s"),
                          filename.c_str(),
                          mode == Read ? _("reading") : _("writing"));
        }
    }

    ~wxFileHandle()
    {
        if ( m_hFile != INVALID_HANDLE_VALUE )
        {
            if ( !::CloseHandle(m_hFile) )
            {
                wxLogSysError(_("Failed to close file handle"));
            }
        }
    }

    // return true only if the file could be opened successfully
    bool IsOk() const { return m_hFile != INVALID_HANDLE_VALUE; }

    // get the handle
    operator HANDLE() const { return m_hFile; }

private:
    HANDLE m_hFile;
};

#endif // __WIN32__

// ----------------------------------------------------------------------------
// private functions
// ----------------------------------------------------------------------------

#if wxUSE_DATETIME && defined(__WIN32__) && !defined(__WXMICROWIN__)

// convert between wxDateTime and FILETIME which is a 64-bit value representing
// the number of 100-nanosecond intervals since January 1, 1601.

static void ConvertFileTimeToWx(wxDateTime *dt, const FILETIME &ft)
{
    FILETIME ftcopy = ft;
    FILETIME ftLocal;
    if ( !::FileTimeToLocalFileTime(&ftcopy, &ftLocal) )
    {
        wxLogLastError(_T("FileTimeToLocalFileTime"));
    }

    SYSTEMTIME st;
    if ( !::FileTimeToSystemTime(&ftLocal, &st) )
    {
        wxLogLastError(_T("FileTimeToSystemTime"));
    }

    dt->Set(st.wDay, wxDateTime::Month(st.wMonth - 1), st.wYear,
            st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
}

static void ConvertWxToFileTime(FILETIME *ft, const wxDateTime& dt)
{
    SYSTEMTIME st;
    st.wDay = dt.GetDay();
    st.wMonth = (WORD)(dt.GetMonth() + 1);
    st.wYear = (WORD)dt.GetYear();
    st.wHour = dt.GetHour();
    st.wMinute = dt.GetMinute();
    st.wSecond = dt.GetSecond();
    st.wMilliseconds = dt.GetMillisecond();

    FILETIME ftLocal;
    if ( !::SystemTimeToFileTime(&st, &ftLocal) )
    {
        wxLogLastError(_T("SystemTimeToFileTime"));
    }

    if ( !::LocalFileTimeToFileTime(&ftLocal, ft) )
    {
        wxLogLastError(_T("LocalFileTimeToFileTime"));
    }
}

#endif // wxUSE_DATETIME && __WIN32__

// return a string with the volume par
static wxString wxGetVolumeString(const wxString& volume, wxPathFormat format)
{
    wxString path;

    if ( !volume.empty() )
    {
        format = wxFileName::GetFormat(format);

        // Special Windows UNC paths hack, part 2: undo what we did in
        // SplitPath() and make an UNC path if we have a drive which is not a
        // single letter (hopefully the network shares can't be one letter only
        // although I didn't find any authoritative docs on this)
        if ( format == wxPATH_DOS && volume.length() > 1 )
        {
            path << wxFILE_SEP_PATH_DOS << wxFILE_SEP_PATH_DOS << volume;
        }
        else if  ( format == wxPATH_DOS || format == wxPATH_VMS )
        {
            path << volume << wxFileName::GetVolumeSeparator(format);
        }
        // else ignore
    }

    return path;
}

// return true if the character is a DOS path separator i.e. either a slash or
// a backslash
inline bool IsDOSPathSep(wxChar ch)
{
    return ch == wxFILE_SEP_PATH_DOS || ch == wxFILE_SEP_PATH_UNIX;
}

// return true if the format used is the DOS/Windows one and the string looks
// like a UNC path
static bool IsUNCPath(const wxString& path, wxPathFormat format)
{
    return format == wxPATH_DOS &&
                path.length() >= 4 && // "\\a" can't be a UNC path
                    IsDOSPathSep(path[0u]) &&
                        IsDOSPathSep(path[1u]) &&
                            !IsDOSPathSep(path[2u]);
}

// ============================================================================
// implementation
// ============================================================================

// ----------------------------------------------------------------------------
// wxFileName construction
// ----------------------------------------------------------------------------

void wxFileName::Assign( const wxFileName &filepath )
{
    if ( &filepath == this )
        return;

    m_volume = filepath.GetVolume();
    m_dirs = filepath.GetDirs();
    m_name = filepath.GetName();
    m_ext = filepath.GetExt();
    m_relative = filepath.m_relative;
    m_hasExt = filepath.m_hasExt;
}

void wxFileName::Assign(const wxString& volume,
                        const wxString& path,
                        const wxString& name,
                        const wxString& ext,
                        bool hasExt,
                        wxPathFormat format)
{
    // we should ignore paths which look like UNC shares because we already
    // have the volume here and the UNC notation (\\server\path) is only valid
    // for paths which don't start with a volume, so prevent SetPath() from
    // recognizing "\\foo\bar" in "c:\\foo\bar" as an UNC path
    //
    // note also that this is a rather ugly way to do what we want (passing
    // some kind of flag telling to ignore UNC paths to SetPath() would be
    // better) but this is the safest thing to do to avoid breaking backwards
    // compatibility in 2.8
    if ( IsUNCPath(path, format) )
    {
        // remove one of the 2 leading backslashes to ensure that it's not
        // recognized as an UNC path by SetPath()
        wxString pathNonUNC(path, 1, wxString::npos);
        SetPath(pathNonUNC, format);
    }
    else // no UNC complications
    {
        SetPath(path, format);
    }

    m_volume = volume;
    m_ext = ext;
    m_name = name;

    m_hasExt = hasExt;
}

void wxFileName::SetPath( const wxString& pathOrig, wxPathFormat format )
{
    m_dirs.Clear();

    if ( pathOrig.empty() )
    {
        // no path at all
        m_relative = true;

        return;
    }

    format = GetFormat( format );

    // 0) deal with possible volume part first
    wxString volume,
             path;
    SplitVolume(pathOrig, &volume, &path, format);
    if ( !volume.empty() )
    {
        m_relative = false;

        SetVolume(volume);
    }

    // 1) Determine if the path is relative or absolute.
    wxChar leadingChar = path[0u];

    switch (format)
    {
        case wxPATH_MAC:
            m_relative = leadingChar == wxT(':');

            // We then remove a leading ":". The reason is in our
            // storage form for relative paths:
            // ":dir:file.txt" actually means "./dir/file.txt" in
            // DOS notation and should get stored as
            // (relative) (dir) (file.txt)
            // "::dir:file.txt" actually means "../dir/file.txt"
            // stored as (relative) (..) (dir) (file.txt)
            // This is important only for the Mac as an empty dir
            // actually means <UP>, whereas under DOS, double
            // slashes can be ignored: "\\\\" is the same as "\\".
            if (m_relative)
                path.erase( 0, 1 );
            break;

        case wxPATH_VMS:
            // TODO: what is the relative path format here?
            m_relative = false;
            break;

        default:
            wxFAIL_MSG( _T("Unknown path format") );
            // !! Fall through !!

        case wxPATH_UNIX:
            // the paths of the form "~" or "~username" are absolute
            m_relative = leadingChar != wxT('/') && leadingChar != _T('~');
            break;

        case wxPATH_DOS:
            m_relative = !IsPathSeparator(leadingChar, format);
            break;

    }

    // 2) Break up the path into its members. If the original path
    //    was just "/" or "\\", m_dirs will be empty. We know from
    //    the m_relative field, if this means "nothing" or "root dir".

    wxStringTokenizer tn( path, GetPathSeparators(format) );

    while ( tn.HasMoreTokens() )
    {
        wxString token = tn.GetNextToken();

        // Remove empty token under DOS and Unix, interpret them
        // as .. under Mac.
        if (token.empty())
        {
            if (format == wxPATH_MAC)
                m_dirs.Add( wxT("..") );
            // else ignore
        }
        else
        {
           m_dirs.Add( token );
        }
    }
}

void wxFileName::Assign(const wxString& fullpath,
                        wxPathFormat format)
{
    wxString volume, path, name, ext;
    bool hasExt;
    SplitPath(fullpath, &volume, &path, &name, &ext, &hasExt, format);

    Assign(volume, path, name, ext, hasExt, format);
}

void wxFileName::Assign(const wxString& fullpathOrig,
                        const wxString& fullname,
                        wxPathFormat format)
{
    // always recognize fullpath as directory, even if it doesn't end with a
    // slash
    wxString fullpath = fullpathOrig;
    if ( !fullpath.empty() && !wxEndsWithPathSeparator(fullpath) )
    {
        fullpath += GetPathSeparator(format);
    }

    wxString volume, path, name, ext;
    bool hasExt;

    // do some consistency checks in debug mode: the name should be really just
    // the filename and the path should be really just a path
#ifdef __WXDEBUG__
    wxString volDummy, pathDummy, nameDummy, extDummy;

    SplitPath(fullname, &volDummy, &pathDummy, &name, &ext, &hasExt, format);

    wxASSERT_MSG( volDummy.empty() && pathDummy.empty(),
                  _T("the file name shouldn't contain the path") );

    SplitPath(fullpath, &volume, &path, &nameDummy, &extDummy, format);

    wxASSERT_MSG( nameDummy.empty() && extDummy.empty(),
                  _T("the path shouldn't contain file name nor extension") );

#else // !__WXDEBUG__
    SplitPath(fullname, NULL /* no volume */, NULL /* no path */,
                        &name, &ext, &hasExt, format);
    SplitPath(fullpath, &volume, &path, NULL, NULL, format);
#endif // __WXDEBUG__/!__WXDEBUG__

    Assign(volume, path, name, ext, hasExt, format);
}

void wxFileName::Assign(const wxString& pathOrig,
                        const wxString& name,
                        const wxString& ext,
                        wxPathFormat format)
{
    wxString volume,
             path;
    SplitVolume(pathOrig, &volume, &path, format);

    Assign(volume, path, name, ext, format);
}

void wxFileName::AssignDir(const wxString& dir, wxPathFormat format)
{
    Assign(dir, wxEmptyString, format);
}

void wxFileName::Clear()
{
    m_dirs.Clear();

    m_volume =
    m_name =
    m_ext = wxEmptyString;

    // we don't have any absolute path for now
    m_relative = true;

    // nor any extension
    m_hasExt = false;
}

/* static */
wxFileName wxFileName::FileName(const wxString& file, wxPathFormat format)
{
    return wxFileName(file, format);
}

/* static */
wxFileName wxFileName::DirName(const wxString& dir, wxPathFormat format)
{
    wxFileName fn;
    fn.AssignDir(dir, format);
    return fn;
}

// ----------------------------------------------------------------------------
// existence tests
// ----------------------------------------------------------------------------

bool wxFileName::FileExists() const
{
    return wxFileName::FileExists( GetFullPath() );
}

bool wxFileName::FileExists( const wxString &file )
{
    return ::wxFileExists( file );
}

bool wxFileName::DirExists() const
{
    return wxFileName::DirExists( GetPath() );
}

bool wxFileName::DirExists( const wxString &dir )
{
    return ::wxDirExists( dir );
}

// ----------------------------------------------------------------------------
// CWD and HOME stuff
// ----------------------------------------------------------------------------

void wxFileName::AssignCwd(const wxString& volume)
{
    AssignDir(wxFileName::GetCwd(volume));
}

/* static */
wxString wxFileName::GetCwd(const wxString& volume)
{
    // if we have the volume, we must get the current directory on this drive
    // and to do this we have to chdir to this volume - at least under Windows,
    // I don't know how to get the current drive on another volume elsewhere
    // (TODO)
    wxString cwdOld;
    if ( !volume.empty() )
    {
        cwdOld = wxGetCwd();
        SetCwd(volume + GetVolumeSeparator());
    }

    wxString cwd = ::wxGetCwd();

    if ( !volume.empty() )
    {
        SetCwd(cwdOld);
    }

    return cwd;
}

bool wxFileName::SetCwd()
{
    return wxFileName::SetCwd( GetPath() );
}

bool wxFileName::SetCwd( const wxString &cwd )
{
    return ::wxSetWorkingDirectory( cwd );
}

void wxFileName::AssignHomeDir()
{
    AssignDir(wxFileName::GetHomeDir());
}

wxString wxFileName::GetHomeDir()
{
    return ::wxGetHomeDir();
}


// ----------------------------------------------------------------------------
// CreateTempFileName
// ----------------------------------------------------------------------------

#if wxUSE_FILE || wxUSE_FFILE


#if !defined wx_fdopen && defined HAVE_FDOPEN
    #define wx_fdopen fdopen
#endif

// NB: GetTempFileName() under Windows creates the file, so using
//     O_EXCL there would fail
#ifdef __WINDOWS__
    #define wxOPEN_EXCL 0
#else
    #define wxOPEN_EXCL O_EXCL
#endif


#ifdef wxOpenOSFHandle
#define WX_HAVE_DELETE_ON_CLOSE
// On Windows create a file with the FILE_FLAGS_DELETE_ON_CLOSE flags.
//
static int wxOpenWithDeleteOnClose(const wxString& filename)
{
    DWORD access = GENERIC_READ | GENERIC_WRITE;

    DWORD disposition = OPEN_ALWAYS;

    DWORD attributes = FILE_ATTRIBUTE_TEMPORARY |
                       FILE_FLAG_DELETE_ON_CLOSE;

    HANDLE h = ::CreateFile(filename, access, 0, NULL,
                            disposition, attributes, NULL);

    return wxOpenOSFHandle(h, wxO_BINARY);
}
#endif // wxOpenOSFHandle


// Helper to open the file
//
static int wxTempOpen(const wxString& path, bool *deleteOnClose)
{
#ifdef WX_HAVE_DELETE_ON_CLOSE
    if (*deleteOnClose)
        return wxOpenWithDeleteOnClose(path);
#endif

    *deleteOnClose = false;

    return wxOpen(path, wxO_BINARY | O_RDWR | O_CREAT | wxOPEN_EXCL, 0600);
}


#if wxUSE_FFILE
// Helper to open the file and attach it to the wxFFile
//
static bool wxTempOpen(wxFFile *file, const wxString& path, bool *deleteOnClose)
{
#ifndef wx_fdopen
    *deleteOnClose = false;
    return file->Open(path, _T("w+b"));
#else // wx_fdopen
    int fd = wxTempOpen(path, deleteOnClose);
    if (fd == -1)
        return false;
    file->Attach(wx_fdopen(fd, "w+b"));
    return file->IsOpened();
#endif // wx_fdopen
}
#endif // wxUSE_FFILE


#if !wxUSE_FILE
    #define WXFILEARGS(x, y) y
#elif !wxUSE_FFILE
    #define WXFILEARGS(x, y) x
#else
    #define WXFILEARGS(x, y) x, y
#endif


// Implementation of wxFileName::CreateTempFileName().
//
static wxString wxCreateTempImpl(
        const wxString& prefix,
        WXFILEARGS(wxFile *fileTemp, wxFFile *ffileTemp),
        bool *deleteOnClose = NULL)
{
#if wxUSE_FILE && wxUSE_FFILE
    wxASSERT(fileTemp == NULL || ffileTemp == NULL);
#endif
    wxString path, dir, name;
    bool wantDeleteOnClose = false;

    if (deleteOnClose)
    {
        // set the result to false initially
        wantDeleteOnClose = *deleteOnClose;
        *deleteOnClose = false;
    }
    else
    {
        // easier if it alwasys points to something
        deleteOnClose = &wantDeleteOnClose;
    }

    // use the directory specified by the prefix
    wxFileName::SplitPath(prefix, &dir, &name, NULL /* extension */);

    if (dir.empty())
    {
        dir = wxFileName::GetTempDir();
    }

#if defined(__WXWINCE__)
    path = dir + wxT("\\") + name;
    int i = 1;
    while (wxFileName::FileExists(path))
    {
        path = dir + wxT("\\") + name ;
        path << i;
        i ++;
    }

#elif defined(__WINDOWS__) && !defined(__WXMICROWIN__)
    if ( !::GetTempFileName(dir, name, 0, wxStringBuffer(path, MAX_PATH + 1)) )
    {
        wxLogLastError(_T("GetTempFileName"));

        path.clear();
    }

#else // !Windows
    path = dir;

    if ( !wxEndsWithPathSeparator(dir) &&
            (name.empty() || !wxIsPathSeparator(name[0u])) )
    {
        path += wxFILE_SEP_PATH;
    }

    path += name;

#if defined(HAVE_MKSTEMP)
    // scratch space for mkstemp()
    path += _T("XXXXXX");

    // we need to copy the path to the buffer in which mkstemp() can modify it
    wxCharBuffer buf( wxConvFile.cWX2MB( path ) );

    // cast is safe because the string length doesn't change
    int fdTemp = mkstemp( (char*)(const char*) buf );
    if ( fdTemp == -1 )
    {
        // this might be not necessary as mkstemp() on most systems should have
        // already done it but it doesn't hurt neither...
        path.clear();
    }
    else // mkstemp() succeeded
    {
        path = wxConvFile.cMB2WX( (const char*) buf );

    #if wxUSE_FILE
        // avoid leaking the fd
        if ( fileTemp )
        {
            fileTemp->Attach(fdTemp);
        }
        else
    #endif

    #if wxUSE_FFILE
        if ( ffileTemp )
        {
        #ifdef wx_fdopen
            ffileTemp->Attach(wx_fdopen(fdTemp, "r+b"));
        #else
            ffileTemp->Open(path, _T("r+b"));
            close(fdTemp);
        #endif
        }
        else
    #endif

        {
            close(fdTemp);
        }
    }
#else // !HAVE_MKSTEMP

#ifdef HAVE_MKTEMP
    // same as above
    path += _T("XXXXXX");

    wxCharBuffer buf = wxConvFile.cWX2MB( path );
    if ( !mktemp( (char*)(const char*) buf ) )
    {
        path.clear();
    }
    else
    {
        path = wxConvFile.cMB2WX( (const char*) buf );
    }
#else // !HAVE_MKTEMP (includes __DOS__)
    // generate the unique file name ourselves
    #if !defined(__DOS__) && !defined(__PALMOS__) && (!defined(__MWERKS__) || defined(__DARWIN__) )
    path << (unsigned int)getpid();
    #endif

    wxString pathTry;

    static const size_t numTries = 1000;
    for ( size_t n = 0; n < numTries; n++ )
    {
        // 3 hex digits is enough for numTries == 1000 < 4096
        pathTry = path + wxString::Format(_T("%.03x"), (unsigned int) n);
        if ( !wxFileName::FileExists(pathTry) )
        {
            break;
        }

        pathTry.clear();
    }

    path = pathTry;
#endif // HAVE_MKTEMP/!HAVE_MKTEMP

#endif // HAVE_MKSTEMP/!HAVE_MKSTEMP

#endif // Windows/!Windows

    if ( path.empty() )
    {
        wxLogSysError(_("Failed to create a temporary file name"));
    }
    else
    {
        bool ok = true;

        // open the file - of course, there is a race condition here, this is
        // why we always prefer using mkstemp()...
    #if wxUSE_FILE
        if ( fileTemp && !fileTemp->IsOpened() )
        {
            *deleteOnClose = wantDeleteOnClose;
            int fd = wxTempOpen(path, deleteOnClose);
            if (fd != -1)
                fileTemp->Attach(fd);
            else
                ok = false;
        }
    #endif

    #if wxUSE_FFILE
        if ( ffileTemp && !ffileTemp->IsOpened() )
        {
            *deleteOnClose = wantDeleteOnClose;
            ok = wxTempOpen(ffileTemp, path, deleteOnClose);
        }
    #endif

        if ( !ok )
        {
            // FIXME: If !ok here should we loop and try again with another
            //        file name?  That is the standard recourse if open(O_EXCL)
            //        fails, though of course it should be protected against
            //        possible infinite looping too.

            wxLogError(_("Failed to open temporary file."));

            path.clear();
        }
    }

    return path;
}


static bool wxCreateTempImpl(
        const wxString& prefix,
        WXFILEARGS(wxFile *fileTemp, wxFFile *ffileTemp),
        wxString *name)
{
    bool deleteOnClose = true;

    *name = wxCreateTempImpl(prefix,
                             WXFILEARGS(fileTemp, ffileTemp),
                             &deleteOnClose);

    bool ok = !name->empty();

    if (deleteOnClose)
        name->clear();
#ifdef __UNIX__
    else if (ok && wxRemoveFile(*name))
        name->clear();
#endif

    return ok;
}


static void wxAssignTempImpl(
        wxFileName *fn,
        const wxString& prefix,
        WXFILEARGS(wxFile *fileTemp, wxFFile *ffileTemp))
{
    wxString tempname;
    tempname = wxCreateTempImpl(prefix, WXFILEARGS(fileTemp, ffileTemp));

    if ( tempname.empty() )
    {
        // error, failed to get temp file name
        fn->Clear();
    }
    else // ok
    {
        fn->Assign(tempname);
    }
}


void wxFileName::AssignTempFileName(const wxString& prefix)
{
    wxAssignTempImpl(this, prefix, WXFILEARGS(NULL, NULL));
}

/* static */
wxString wxFileName::CreateTempFileName(const wxString& prefix)
{
    return wxCreateTempImpl(prefix, WXFILEARGS(NULL, NULL));
}

#endif // wxUSE_FILE || wxUSE_FFILE


#if wxUSE_FILE

wxString wxCreateTempFileName(const wxString& prefix,
                              wxFile *fileTemp,
                              bool *deleteOnClose)
{
    return wxCreateTempImpl(prefix, WXFILEARGS(fileTemp, NULL), deleteOnClose);
}

bool wxCreateTempFile(const wxString& prefix,
                      wxFile *fileTemp,
                      wxString *name)
{
    return wxCreateTempImpl(prefix, WXFILEARGS(fileTemp, NULL), name);
}

void wxFileName::AssignTempFileName(const wxString& prefix, wxFile *fileTemp)
{
    wxAssignTempImpl(this, prefix, WXFILEARGS(fileTemp, NULL));
}

/* static */
wxString
wxFileName::CreateTempFileName(const wxString& prefix, wxFile *fileTemp)
{
    return wxCreateTempFileName(prefix, fileTemp);
}

#endif // wxUSE_FILE


#if wxUSE_FFILE

wxString wxCreateTempFileName(const wxString& prefix,
                              wxFFile *fileTemp,
                              bool *deleteOnClose)
{
    return wxCreateTempImpl(prefix, WXFILEARGS(NULL, fileTemp), deleteOnClose);
}

bool wxCreateTempFile(const wxString& prefix,
                      wxFFile *fileTemp,
                      wxString *name)
{
    return wxCreateTempImpl(prefix, WXFILEARGS(NULL, fileTemp), name);

}

void wxFileName::AssignTempFileName(const wxString& prefix, wxFFile *fileTemp)
{
    wxAssignTempImpl(this, prefix, WXFILEARGS(NULL, fileTemp));
}

/* static */
wxString
wxFileName::CreateTempFileName(const wxString& prefix, wxFFile *fileTemp)
{
    return wxCreateTempFileName(prefix, fileTemp);
}

#endif // wxUSE_FFILE


// ----------------------------------------------------------------------------
// directory operations
// ----------------------------------------------------------------------------

wxString wxFileName::GetTempDir()
{
    wxString dir;
    dir = wxGetenv(_T("TMPDIR"));
    if (dir.empty())
    {
        dir = wxGetenv(_T("TMP"));
        if (dir.empty())
        {
            dir = wxGetenv(_T("TEMP"));
        }
    }

#if defined(__WXWINCE__)
    if (dir.empty())
    {
        // FIXME. Create \temp dir?
        if (DirExists(wxT("\\temp")))
            dir = wxT("\\temp");
    }
#elif defined(__WINDOWS__) && !defined(__WXMICROWIN__)

    if ( dir.empty() )
    {
        if ( !::GetTempPath(MAX_PATH, wxStringBuffer(dir, MAX_PATH + 1)) )
        {
            wxLogLastError(_T("GetTempPath"));
        }

        if ( dir.empty() )
        {
            // GetTempFileName() fails if we pass it an empty string
            dir = _T('.');
        }
    }
#else // !Windows

    if ( dir.empty() )
    {
        // default
#if defined(__DOS__) || defined(__OS2__)
        dir = _T(".");
#elif defined(__WXMAC__)
        dir = wxMacFindFolder(short(kOnSystemDisk), kTemporaryFolderType, kCreateFolder);
#else
        dir = _T("/tmp");
#endif
    }
#endif

    return dir;
}

bool wxFileName::Mkdir( int perm, int flags )
{
    return wxFileName::Mkdir(GetPath(), perm, flags);
}

bool wxFileName::Mkdir( const wxString& dir, int perm, int flags )
{
    if ( flags & wxPATH_MKDIR_FULL )
    {
        // split the path in components
        wxFileName filename;
        filename.AssignDir(dir);

        wxString currPath;
        if ( filename.HasVolume())
        {
            currPath << wxGetVolumeString(filename.GetVolume(), wxPATH_NATIVE);
        }

        wxArrayString dirs = filename.GetDirs();
        size_t count = dirs.GetCount();
        for ( size_t i = 0; i < count; i++ )
        {
            if ( i > 0 ||
#if defined(__WXMAC__) && !defined(__DARWIN__)
            // relative pathnames are exactely the other way round under mac...
                !filename.IsAbsolute()
#else
                filename.IsAbsolute()
#endif
            )
                currPath += wxFILE_SEP_PATH;
            currPath += dirs[i];

            if (!DirExists(currPath))
            {
                if (!wxMkdir(currPath, perm))
                {
                    // no need to try creating further directories
                    return false;
                }
            }
        }

        return true;

    }

    return ::wxMkdir( dir, perm );
}

bool wxFileName::Rmdir()
{
    return wxFileName::Rmdir( GetPath() );
}

bool wxFileName::Rmdir( const wxString &dir )
{
    return ::wxRmdir( dir );
}

// ----------------------------------------------------------------------------
// path normalization
// ----------------------------------------------------------------------------

bool wxFileName::Normalize(int flags,
                           const wxString& cwd,
                           wxPathFormat format)
{
    // deal with env vars renaming first as this may seriously change the path
    if ( flags & wxPATH_NORM_ENV_VARS )
    {
        wxString pathOrig = GetFullPath(format);
        wxString path = wxExpandEnvVars(pathOrig);
        if ( path != pathOrig )
        {
            Assign(path);
        }
    }


    // the existing path components
    wxArrayString dirs = GetDirs();

    // the path to prepend in front to make the path absolute
    wxFileName curDir;

    format = GetFormat(format);

    // set up the directory to use for making the path absolute later
    if ( (flags & wxPATH_NORM_ABSOLUTE) && !IsAbsolute(format) )
    {
        if ( cwd.empty() )
        {
            curDir.AssignCwd(GetVolume());
        }
        else // cwd provided
        {
            curDir.AssignDir(cwd);
        }
    }

    // handle ~ stuff under Unix only
    if ( (format == wxPATH_UNIX) && (flags & wxPATH_NORM_TILDE) )
    {
        if ( !dirs.IsEmpty() )
        {
            wxString dir = dirs[0u];
            if ( !dir.empty() && dir[0u] == _T('~') )
            {
                // to make the path absolute use the home directory
                curDir.AssignDir(wxGetUserHome(dir.c_str() + 1));

                // if we are expanding the tilde, then this path
                // *should* be already relative (since we checked for
                // the tilde only in the first char of the first dir);
                // if m_relative==false, it's because it was initialized
                // from a string which started with /~; in that case
                // we reach this point but then need m_relative=true
                // for relative->absolute expansion later
                m_relative = true;

                dirs.RemoveAt(0u);
            }
        }
    }

    // transform relative path into abs one
    if ( curDir.IsOk() )
    {
        // this path may be relative because it doesn't have the volume name
        // and still have m_relative=true; in this case we shouldn't modify
        // our directory components but just set the current volume
        if ( !HasVolume() && curDir.HasVolume() )
        {
            SetVolume(curDir.GetVolume());

            if ( !m_relative )
        {
                // yes, it was the case - we don't need curDir then
                curDir.Clear();
            }
        }

        // finally, prepend curDir to the dirs array
        wxArrayString dirsNew = curDir.GetDirs();
        WX_PREPEND_ARRAY(dirs, dirsNew);

        // if we used e.g. tilde expansion previously and wxGetUserHome didn't
        // return for some reason an absolute path, then curDir maybe not be absolute!
        if ( curDir.IsAbsolute(format) )
        {
            // we have prepended an absolute path and thus we are now an absolute
            // file name too
            m_relative = false;
        }
        // else if (flags & wxPATH_NORM_ABSOLUTE):
        //   should we warn the user that we didn't manage to make the path absolute?
    }

    // now deal with ".", ".." and the rest
    m_dirs.Empty();
    size_t count = dirs.GetCount();
    for ( size_t n = 0; n < count; n++ )
    {
        wxString dir = dirs[n];

        if ( flags & wxPATH_NORM_DOTS )
        {
            if ( dir == wxT(".") )
            {
                // just ignore
                continue;
            }

            if ( dir == wxT("..") )
            {
                if ( m_dirs.IsEmpty() )
                {
                    wxLogError(_("The path '%s' contains too many \"..\"!"),
                               GetFullPath().c_str());
                    return false;
                }

                m_dirs.RemoveAt(m_dirs.GetCount() - 1);
                continue;
            }
        }

        m_dirs.Add(dir);
    }

#if defined(__WIN32__) && !defined(__WXWINCE__) && wxUSE_OLE
    if ( (flags & wxPATH_NORM_SHORTCUT) )
    {
        wxString filename;
        if (GetShortcutTarget(GetFullPath(format), filename))
        {
            m_relative = false;
            Assign(filename);
        }
    }
#endif

#if defined(__WIN32__)
    if ( (flags & wxPATH_NORM_LONG) && (format == wxPATH_DOS) )
    {
        Assign(GetLongPath());
    }
#endif // Win32

    // Change case  (this should be kept at the end of the function, to ensure
    // that the path doesn't change any more after we normalize its case)
    if ( (flags & wxPATH_NORM_CASE) && !IsCaseSensitive(format) )
    {
        m_volume.MakeLower();
        m_name.MakeLower();
        m_ext.MakeLower();

        // directory entries must be made lower case as well
        count = m_dirs.GetCount();
        for ( size_t i = 0; i < count; i++ )
        {
            m_dirs[i].MakeLower();
        }
    }

    return true;
}

// ----------------------------------------------------------------------------
// get the shortcut target
// ----------------------------------------------------------------------------

// WinCE (3) doesn't have CLSID_ShellLink, IID_IShellLink definitions.
// The .lnk file is a plain text file so it should be easy to
// make it work. Hint from Google Groups:
// "If you open up a lnk file, you'll see a
// number, followed by a pound sign (#), followed by more text. The
// number is the number of characters that follows the pound sign. The
// characters after the pound sign are the command line (which _can_
// include arguments) to be executed. Any path (e.g. \windows\program
// files\myapp.exe) that includes spaces needs to be enclosed in
// quotation marks."

#if defined(__WIN32__) && !defined(__WXWINCE__) && wxUSE_OLE
// The following lines are necessary under WinCE
// #include "wx/msw/private.h"
// #include <ole2.h>
#include <shlobj.h>
#if defined(__WXWINCE__)
#include <shlguid.h>
#endif

bool wxFileName::GetShortcutTarget(const wxString& shortcutPath,
                                   wxString& targetFilename,
                                   wxString* arguments)
{
    wxString path, file, ext;
    wxSplitPath(shortcutPath, & path, & file, & ext);

    HRESULT hres;
    IShellLink* psl;
    bool success = false;

    // Assume it's not a shortcut if it doesn't end with lnk
    if (ext.CmpNoCase(wxT("lnk"))!=0)
        return false;

    // create a ShellLink object
    hres = CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER,
                            IID_IShellLink, (LPVOID*) &psl);

    if (SUCCEEDED(hres))
    {
        IPersistFile* ppf;
        hres = psl->QueryInterface( IID_IPersistFile, (LPVOID *) &ppf);
        if (SUCCEEDED(hres))
        {
            WCHAR wsz[MAX_PATH];

            MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, shortcutPath.mb_str(), -1, wsz,
                                MAX_PATH);

            hres = ppf->Load(wsz, 0);
            ppf->Release();

            if (SUCCEEDED(hres))
            {
                wxChar buf[2048];
                // Wrong prototype in early versions
#if defined(__MINGW32__) && !wxCHECK_W32API_VERSION(2, 2)
                psl->GetPath((CHAR*) buf, 2048, NULL, SLGP_UNCPRIORITY);
#else
                psl->GetPath(buf, 2048, NULL, SLGP_UNCPRIORITY);
#endif
                targetFilename = wxString(buf);
                success = (shortcutPath != targetFilename);

                psl->GetArguments(buf, 2048);
                wxString args(buf);
                if (!args.empty() && arguments)
                {
                    *arguments = args;
                }
            }
        }

        psl->Release();
    }
    return success;
}

#endif // __WIN32__ && !__WXWINCE__


// ----------------------------------------------------------------------------
// absolute/relative paths
// ----------------------------------------------------------------------------

bool wxFileName::IsAbsolute(wxPathFormat format) const
{
    // if our path doesn't start with a path separator, it's not an absolute
    // path
    if ( m_relative )
        return false;

    if ( !GetVolumeSeparator(format).empty() )
    {
        // this format has volumes and an absolute path must have one, it's not
        // enough to have the full path to bean absolute file under Windows
        if ( GetVolume().empty() )
            return false;
    }

    return true;
}

bool wxFileName::MakeRelativeTo(const wxString& pathBase, wxPathFormat format)
{
    wxFileName fnBase = wxFileName::DirName(pathBase, format);

    // get cwd only once - small time saving
    wxString cwd = wxGetCwd();
    Normalize(wxPATH_NORM_ALL & ~wxPATH_NORM_CASE, cwd, format);
    fnBase.Normalize(wxPATH_NORM_ALL & ~wxPATH_NORM_CASE, cwd, format);

    bool withCase = IsCaseSensitive(format);

    // we can't do anything if the files live on different volumes
    if ( !GetVolume().IsSameAs(fnBase.GetVolume(), withCase) )
    {
        // nothing done
        return false;
    }

    // same drive, so we don't need our volume
    m_volume.clear();

    // remove common directories starting at the top
    while ( !m_dirs.IsEmpty() && !fnBase.m_dirs.IsEmpty() &&
                m_dirs[0u].IsSameAs(fnBase.m_dirs[0u], withCase) )
    {
        m_dirs.RemoveAt(0);
        fnBase.m_dirs.RemoveAt(0);
    }

    // add as many ".." as needed
    size_t count = fnBase.m_dirs.GetCount();
    for ( size_t i = 0; i < count; i++ )
    {
        m_dirs.Insert(wxT(".."), 0u);
    }

    if ( format == wxPATH_UNIX || format == wxPATH_DOS )
    {
        // a directory made relative with respect to itself is '.' under Unix
        // and DOS, by definition (but we don't have to insert "./" for the
        // files)
        if ( m_dirs.IsEmpty() && IsDir() )
        {
            m_dirs.Add(_T('.'));
        }
    }

    m_relative = true;

    // we were modified
    return true;
}

// ----------------------------------------------------------------------------
// filename kind tests
// ----------------------------------------------------------------------------

bool wxFileName::SameAs(const wxFileName& filepath, wxPathFormat format) const
{
    wxFileName fn1 = *this,
               fn2 = filepath;

    // get cwd only once - small time saving
    wxString cwd = wxGetCwd();
    fn1.Normalize(wxPATH_NORM_ALL | wxPATH_NORM_CASE, cwd, format);
    fn2.Normalize(wxPATH_NORM_ALL | wxPATH_NORM_CASE, cwd, format);

    if ( fn1.GetFullPath() == fn2.GetFullPath() )
        return true;

    // TODO: compare inodes for Unix, this works even when filenames are
    //       different but files are the same (symlinks) (VZ)

    return false;
}

/* static */
bool wxFileName::IsCaseSensitive( wxPathFormat format )
{
    // only Unix filenames are truely case-sensitive
    return GetFormat(format) == wxPATH_UNIX;
}

/* static */
wxString wxFileName::GetForbiddenChars(wxPathFormat format)
{
    // Inits to forbidden characters that are common to (almost) all platforms.
    wxString strForbiddenChars = wxT("*?");

    // If asserts, wxPathFormat has been changed. In case of a new path format
    // addition, the following code might have to be updated.
    wxCOMPILE_TIME_ASSERT(wxPATH_MAX == 5, wxPathFormatChanged);
    switch ( GetFormat(format) )
    {
        default :
            wxFAIL_MSG( wxT("Unknown path format") );
            // !! Fall through !!

        case wxPATH_UNIX:
            break;

        case wxPATH_MAC:
            // On a Mac even names with * and ? are allowed (Tested with OS
            // 9.2.1 and OS X 10.2.5)
            strForbiddenChars = wxEmptyString;
            break;

        case wxPATH_DOS:
            strForbiddenChars += wxT("\\/:\"<>|");
            break;

        case wxPATH_VMS:
            break;
    }

    return strForbiddenChars;
}

/* static */
wxString wxFileName::GetVolumeSeparator(wxPathFormat WXUNUSED_IN_WINCE(format))
{
#ifdef __WXWINCE__
    return wxEmptyString;
#else
    wxString sepVol;

    if ( (GetFormat(format) == wxPATH_DOS) ||
         (GetFormat(format) == wxPATH_VMS) )
    {
        sepVol = wxFILE_SEP_DSK;
    }
    //else: leave empty

    return sepVol;
#endif
}

/* static */
wxString wxFileName::GetPathSeparators(wxPathFormat format)
{
    wxString seps;
    switch ( GetFormat(format) )
    {
        case wxPATH_DOS:
            // accept both as native APIs do but put the native one first as
            // this is the one we use in GetFullPath()
            seps << wxFILE_SEP_PATH_DOS << wxFILE_SEP_PATH_UNIX;
            break;

        default:
            wxFAIL_MSG( _T("Unknown wxPATH_XXX style") );
            // fall through

        case wxPATH_UNIX:
            seps = wxFILE_SEP_PATH_UNIX;
            break;

        case wxPATH_MAC:
            seps = wxFILE_SEP_PATH_MAC;
            break;

        case wxPATH_VMS:
            seps = wxFILE_SEP_PATH_VMS;
            break;
    }

    return seps;
}

/* static */
wxString wxFileName::GetPathTerminators(wxPathFormat format)
{
    format = GetFormat(format);

    // under VMS the end of the path is ']', not the path separator used to
    // separate the components
    return format == wxPATH_VMS ? wxString(_T(']')) : GetPathSeparators(format);
}

/* static */
bool wxFileName::IsPathSeparator(wxChar ch, wxPathFormat format)
{
    // wxString::Find() doesn't work as expected with NUL - it will always find
    // it, so test for it separately
    return ch != _T('\0') && GetPathSeparators(format).Find(ch) != wxNOT_FOUND;
}

// ----------------------------------------------------------------------------
// path components manipulation
// ----------------------------------------------------------------------------

/* static */ bool wxFileName::IsValidDirComponent(const wxString& dir)
{
    if ( dir.empty() )
    {
        wxFAIL_MSG( _T("empty directory passed to wxFileName::InsertDir()") );

        return false;
    }

    const size_t len = dir.length();
    for ( size_t n = 0; n < len; n++ )
    {
        if ( dir[n] == GetVolumeSeparator() || IsPathSeparator(dir[n]) )
        {
            wxFAIL_MSG( _T("invalid directory component in wxFileName") );

            return false;
        }
    }

    return true;
}

void wxFileName::AppendDir( const wxString& dir )
{
    if ( IsValidDirComponent(dir) )
        m_dirs.Add( dir );
}

void wxFileName::PrependDir( const wxString& dir )
{
    InsertDir(0, dir);
}

void wxFileName::InsertDir(size_t before, const wxString& dir)
{
    if ( IsValidDirComponent(dir) )
        m_dirs.Insert(dir, before);
}

void wxFileName::RemoveDir(size_t pos)
{
    m_dirs.RemoveAt(pos);
}

// ----------------------------------------------------------------------------
// accessors
// ----------------------------------------------------------------------------

void wxFileName::SetFullName(const wxString& fullname)
{
    SplitPath(fullname, NULL /* no volume */, NULL /* no path */,
                        &m_name, &m_ext, &m_hasExt);
}

wxString wxFileName::GetFullName() const
{
    wxString fullname = m_name;
    if ( m_hasExt )
    {
        fullname << wxFILE_SEP_EXT << m_ext;
    }

    return fullname;
}

wxString wxFileName::GetPath( int flags, wxPathFormat format ) const
{
    format = GetFormat( format );

    wxString fullpath;

    // return the volume with the path as well if requested
    if ( flags & wxPATH_GET_VOLUME )
    {
        fullpath += wxGetVolumeString(GetVolume(), format);
    }

    // the leading character
    switch ( format )
    {
        case wxPATH_MAC:
            if ( m_relative )
                fullpath += wxFILE_SEP_PATH_MAC;
            break;

        case wxPATH_DOS:
            if ( !m_relative )
                fullpath += wxFILE_SEP_PATH_DOS;
            break;

        default:
            wxFAIL_MSG( wxT("Unknown path format") );
            // fall through

        case wxPATH_UNIX:
            if ( !m_relative )
            {
                // normally the absolute file names start with a slash
                // with one exception: the ones like "~/foo.bar" don't
                // have it
                if ( m_dirs.IsEmpty() || m_dirs[0u] != _T('~') )
                {
                    fullpath += wxFILE_SEP_PATH_UNIX;
                }
            }
            break;

        case wxPATH_VMS:
            // no leading character here but use this place to unset
            // wxPATH_GET_SEPARATOR flag: under VMS it doesn't make sense
            // as, if I understand correctly, there should never be a dot
            // before the closing bracket
            flags &= ~wxPATH_GET_SEPARATOR;
    }

    if ( m_dirs.empty() )
    {
        // there is nothing more
        return fullpath;
    }

    // then concatenate all the path components using the path separator
    if ( format == wxPATH_VMS )
    {
        fullpath += wxT('[');
    }

    const size_t dirCount = m_dirs.GetCount();
    for ( size_t i = 0; i < dirCount; i++ )
    {
        switch (format)
        {
            case wxPATH_MAC:
                if ( m_dirs[i] == wxT(".") )
                {
                    // skip appending ':', this shouldn't be done in this
                    // case as "::" is interpreted as ".." under Unix
                    continue;
                }

                // convert back from ".." to nothing
                if ( !m_dirs[i].IsSameAs(wxT("..")) )
                     fullpath += m_dirs[i];
                break;

            default:
                wxFAIL_MSG( wxT("Unexpected path format") );
                // still fall through

            case wxPATH_DOS:
            case wxPATH_UNIX:
                fullpath += m_dirs[i];
                break;

            case wxPATH_VMS:
                // TODO: What to do with ".." under VMS

                // convert back from ".." to nothing
                if ( !m_dirs[i].IsSameAs(wxT("..")) )
                    fullpath += m_dirs[i];
                break;
        }

        if ( (flags & wxPATH_GET_SEPARATOR) || (i != dirCount - 1) )
            fullpath += GetPathSeparator(format);
    }

    if ( format == wxPATH_VMS )
    {
        fullpath += wxT(']');
    }

    return fullpath;
}

wxString wxFileName::GetFullPath( wxPathFormat format ) const
{
    // we already have a function to get the path
    wxString fullpath = GetPath(wxPATH_GET_VOLUME | wxPATH_GET_SEPARATOR,
                                format);

    // now just add the file name and extension to it
    fullpath += GetFullName();

    return fullpath;
}

// Return the short form of the path (returns identity on non-Windows platforms)
wxString wxFileName::GetShortPath() const
{
    wxString path(GetFullPath());

#if defined(__WXMSW__) && defined(__WIN32__) && !defined(__WXMICROWIN__) && !defined(__WXWINCE__)
    DWORD sz = ::GetShortPathName(path, NULL, 0);
    if ( sz != 0 )
    {
        wxString pathOut;
        if ( ::GetShortPathName
               (
                path,
                wxStringBuffer(pathOut, sz),
                sz
               ) != 0 )
        {
            return pathOut;
        }
    }
#endif // Windows

    return path;
}

// Return the long form of the path (returns identity on non-Windows platforms)
wxString wxFileName::GetLongPath() const
{
    wxString pathOut,
             path = GetFullPath();

#if defined(__WIN32__) && !defined(__WXWINCE__) && !defined(__WXMICROWIN__)

#if wxUSE_DYNLIB_CLASS
    typedef DWORD (WINAPI *GET_LONG_PATH_NAME)(const wxChar *, wxChar *, DWORD);

    // this is MT-safe as in the worst case we're going to resolve the function
    // twice -- but as the result is the same in both threads, it's ok
    static GET_LONG_PATH_NAME s_pfnGetLongPathName = NULL;
    if ( !s_pfnGetLongPathName )
    {
        static bool s_triedToLoad = false;

        if ( !s_triedToLoad )
        {
            s_triedToLoad = true;

            wxDynamicLibrary dllKernel(_T("kernel32"));

            const wxChar* GetLongPathName = _T("GetLongPathName")
#if wxUSE_UNICODE
                              _T("W");
#else // ANSI
                              _T("A");
#endif // Unicode/ANSI

            if ( dllKernel.HasSymbol(GetLongPathName) )
            {
                s_pfnGetLongPathName = (GET_LONG_PATH_NAME)
                    dllKernel.GetSymbol(GetLongPathName);
            }

            // note that kernel32.dll can be unloaded, it stays in memory
            // anyhow as all Win32 programs link to it and so it's safe to call
            // GetLongPathName() even after unloading it
        }
    }

    if ( s_pfnGetLongPathName )
    {
        DWORD dwSize = (*s_pfnGetLongPathName)(path, NULL, 0);
        if ( dwSize > 0 )
        {
            if ( (*s_pfnGetLongPathName)
                 (
                  path,
                  wxStringBuffer(pathOut, dwSize),
                  dwSize
                 ) != 0 )
            {
                return pathOut;
            }
        }
    }
#endif // wxUSE_DYNLIB_CLASS

    // The OS didn't support GetLongPathName, or some other error.
    // We need to call FindFirstFile on each component in turn.

    WIN32_FIND_DATA findFileData;
    HANDLE hFind;

    if ( HasVolume() )
        pathOut = GetVolume() +
                  GetVolumeSeparator(wxPATH_DOS) +
                  GetPathSeparator(wxPATH_DOS);
    else
        pathOut = wxEmptyString;

    wxArrayString dirs = GetDirs();
    dirs.Add(GetFullName());

    wxString tmpPath;

    size_t count = dirs.GetCount();
    for ( size_t i = 0; i < count; i++ )
    {
        // We're using pathOut to collect the long-name path, but using a
        // temporary for appending the last path component which may be
        // short-name
        tmpPath = pathOut + dirs[i];

        if ( tmpPath.empty() )
            continue;

        // can't see this being necessary? MF
        if ( tmpPath.Last() == GetVolumeSeparator(wxPATH_DOS) )
        {
            // Can't pass a drive and root dir to FindFirstFile,
            // so continue to next dir
            tmpPath += wxFILE_SEP_PATH;
            pathOut = tmpPath;
            continue;
        }

        hFind = ::FindFirstFile(tmpPath, &findFileData);
        if (hFind == INVALID_HANDLE_VALUE)
        {
            // Error: most likely reason is that path doesn't exist, so
            // append any unprocessed parts and return
            for ( i += 1; i < count; i++ )
                tmpPath += wxFILE_SEP_PATH + dirs[i];

            return tmpPath;
        }

        pathOut += findFileData.cFileName;
        if ( (i < (count-1)) )
            pathOut += wxFILE_SEP_PATH;

        ::FindClose(hFind);
    }
#else // !Win32
    pathOut = path;
#endif // Win32/!Win32

    return pathOut;
}

wxPathFormat wxFileName::GetFormat( wxPathFormat format )
{
    if (format == wxPATH_NATIVE)
    {
#if defined(__WXMSW__) || defined(__OS2__) || defined(__DOS__)
        format = wxPATH_DOS;
#elif defined(__WXMAC__) && !defined(__DARWIN__)
        format = wxPATH_MAC;
#elif defined(__VMS)
        format = wxPATH_VMS;
#else
        format = wxPATH_UNIX;
#endif
    }
    return format;
}

// ----------------------------------------------------------------------------
// path splitting function
// ----------------------------------------------------------------------------

/* static */
void
wxFileName::SplitVolume(const wxString& fullpathWithVolume,
                        wxString *pstrVolume,
                        wxString *pstrPath,
                        wxPathFormat format)
{
    format = GetFormat(format);

    wxString fullpath = fullpathWithVolume;

    // special Windows UNC paths hack: transform \\share\path into share:path
    if ( IsUNCPath(fullpath, format) )
    {
        fullpath.erase(0, 2);

        size_t posFirstSlash =
            fullpath.find_first_of(GetPathTerminators(format));
        if ( posFirstSlash != wxString::npos )
        {
            fullpath[posFirstSlash] = wxFILE_SEP_DSK;

            // UNC paths are always absolute, right? (FIXME)
            fullpath.insert(posFirstSlash + 1, 1, wxFILE_SEP_PATH_DOS);
        }
    }

    // We separate the volume here
    if ( format == wxPATH_DOS || format == wxPATH_VMS )
    {
        wxString sepVol = GetVolumeSeparator(format);

        size_t posFirstColon = fullpath.find_first_of(sepVol);
        if ( posFirstColon != wxString::npos )
        {
            if ( pstrVolume )
            {
                *pstrVolume = fullpath.Left(posFirstColon);
            }

            // remove the volume name and the separator from the full path
            fullpath.erase(0, posFirstColon + sepVol.length());
        }
    }

    if ( pstrPath )
        *pstrPath = fullpath;
}

/* static */
void wxFileName::SplitPath(const wxString& fullpathWithVolume,
                           wxString *pstrVolume,
                           wxString *pstrPath,
                           wxString *pstrName,
                           wxString *pstrExt,
                           bool *hasExt,
                           wxPathFormat format)
{
    format = GetFormat(format);

    wxString fullpath;
    SplitVolume(fullpathWithVolume, pstrVolume, &fullpath, format);

    // find the positions of the last dot and last path separator in the path
    size_t posLastDot = fullpath.find_last_of(wxFILE_SEP_EXT);
    size_t posLastSlash = fullpath.find_last_of(GetPathTerminators(format));

    // check whether this dot occurs at the very beginning of a path component
    if ( (posLastDot != wxString::npos) &&
         (posLastDot == 0 ||
            IsPathSeparator(fullpath[posLastDot - 1]) ||
            (format == wxPATH_VMS && fullpath[posLastDot - 1] == _T(']'))) )
    {
        // dot may be (and commonly -- at least under Unix -- is) the first
        // character of the filename, don't treat the entire filename as
        // extension in this case
        posLastDot = wxString::npos;
    }

    // if we do have a dot and a slash, check that the dot is in the name part
    if ( (posLastDot != wxString::npos) &&
         (posLastSlash != wxString::npos) &&
         (posLastDot < posLastSlash) )
    {
        // the dot is part of the path, not the start of the extension
        posLastDot = wxString::npos;
    }

    // now fill in the variables provided by user
    if ( pstrPath )
    {
        if ( posLastSlash == wxString::npos )
        {
            // no path at all
            pstrPath->Empty();
        }
        else
        {
            // take everything up to the path separator but take care to make
            // the path equal to something like '/', not empty, for the files
            // immediately under root directory
            size_t len = posLastSlash;

            // this rule does not apply to mac since we do not start with colons (sep)
            // except for relative paths
            if ( !len && format != wxPATH_MAC)
                len++;

            *pstrPath = fullpath.Left(len);

            // special VMS hack: remove the initial bracket
            if ( format == wxPATH_VMS )
            {
                if ( (*pstrPath)[0u] == _T('[') )
                    pstrPath->erase(0, 1);
            }
        }
    }

    if ( pstrName )
    {
        // take all characters starting from the one after the last slash and
        // up to, but excluding, the last dot
        size_t nStart = posLastSlash == wxString::npos ? 0 : posLastSlash + 1;
        size_t count;
        if ( posLastDot == wxString::npos )
        {
            // take all until the end
            count = wxString::npos;
        }
        else if ( posLastSlash == wxString::npos )
        {
            count = posLastDot;
        }
        else // have both dot and slash
        {
            count = posLastDot - posLastSlash - 1;
        }

        *pstrName = fullpath.Mid(nStart, count);
    }

    // finally deal with the extension here: we have an added complication that
    // extension may be empty (but present) as in "foo." where trailing dot
    // indicates the empty extension at the end -- and hence we must remember
    // that we have it independently of pstrExt
    if ( posLastDot == wxString::npos )
    {
        // no extension
        if ( pstrExt )
            pstrExt->clear();
        if ( hasExt )
            *hasExt = false;
    }
    else
    {
        // take everything after the dot
        if ( pstrExt )
            *pstrExt = fullpath.Mid(posLastDot + 1);
        if ( hasExt )
            *hasExt = true;
    }
}

/* static */
void wxFileName::SplitPath(const wxString& fullpath,
                           wxString *path,
                           wxString *name,
                           wxString *ext,
                           wxPathFormat format)
{
    wxString volume;
    SplitPath(fullpath, &volume, path, name, ext, format);

    if ( path )
    {
        path->Prepend(wxGetVolumeString(volume, format));
    }
}

/* static */
wxString wxFileName::StripExtension(const wxString& fullpath)
{
    wxFileName fn(fullpath);
    fn.SetExt(_T(""));
    return fn.GetFullPath();
}

// ----------------------------------------------------------------------------
// time functions
// ----------------------------------------------------------------------------

#if wxUSE_DATETIME

bool wxFileName::SetTimes(const wxDateTime *dtAccess,
                          const wxDateTime *dtMod,
                          const wxDateTime *dtCreate)
{
#if defined(__WIN32__)
    FILETIME ftAccess, ftCreate, ftWrite;

    if ( dtCreate )
        ConvertWxToFileTime(&ftCreate, *dtCreate);
    if ( dtAccess )
        ConvertWxToFileTime(&ftAccess, *dtAccess);
    if ( dtMod )
        ConvertWxToFileTime(&ftWrite, *dtMod);

    wxString path;
    int flags;
    if ( IsDir() )
    {
        if ( wxGetOsVersion() == wxOS_WINDOWS_9X )
        {
            wxLogError(_("Setting directory access times is not supported under this OS version"));
            return false;
        }

        path = GetPath();
        flags = FILE_FLAG_BACKUP_SEMANTICS;
    }
    else // file
    {
        path = GetFullPath();
        flags = 0;
    }

    wxFileHandle fh(path, wxFileHandle::Write, flags);
    if ( fh.IsOk() )
    {
        if ( ::SetFileTime(fh,
                           dtCreate ? &ftCreate : NULL,
                           dtAccess ? &ftAccess : NULL,
                           dtMod ? &ftWrite : NULL) )
        {
            return true;
        }
    }
#elif defined(__UNIX_LIKE__) || (defined(__DOS__) && defined(__WATCOMC__))
    wxUnusedVar(dtCreate);

    if ( !dtAccess && !dtMod )
    {
        // can't modify the creation time anyhow, don't try
        return true;
    }

    // if dtAccess or dtMod is not specified, use the other one (which must be
    // non NULL because of the test above) for both times
    utimbuf utm;
    utm.actime = dtAccess ? dtAccess->GetTicks() : dtMod->GetTicks();
    utm.modtime = dtMod ? dtMod->GetTicks() : dtAccess->GetTicks();
    if ( utime(GetFullPath().fn_str(), &utm) == 0 )
    {
        return true;
    }
#else // other platform
    wxUnusedVar(dtAccess);
    wxUnusedVar(dtMod);
    wxUnusedVar(dtCreate);
#endif // platforms

    wxLogSysError(_("Failed to modify file times for '%s'"),
                  GetFullPath().c_str());

    return false;
}

bool wxFileName::Touch()
{
#if defined(__UNIX_LIKE__)
    // under Unix touching file is simple: just pass NULL to utime()
    if ( utime(GetFullPath().fn_str(), NULL) == 0 )
    {
        return true;
    }

    wxLogSysError(_("Failed to touch the file '%s'"), GetFullPath().c_str());

    return false;
#else // other platform
    wxDateTime dtNow = wxDateTime::Now();

    return SetTimes(&dtNow, &dtNow, NULL /* don't change create time */);
#endif // platforms
}

#ifdef wxNEED_WX_UNISTD_H

static int wxStat( const char *file_name, wxStructStat *buf )
{
    return stat( file_name , buf );
}

#endif

bool wxFileName::GetTimes(wxDateTime *dtAccess,
                          wxDateTime *dtMod,
                          wxDateTime *dtCreate) const
{
#if defined(__WIN32__)
    // we must use different methods for the files and directories under
    // Windows as CreateFile(GENERIC_READ) doesn't work for the directories and
    // CreateFile(FILE_FLAG_BACKUP_SEMANTICS) works -- but only under NT and
    // not 9x
    bool ok;
    FILETIME ftAccess, ftCreate, ftWrite;
    if ( IsDir() )
    {
        // implemented in msw/dir.cpp
        extern bool wxGetDirectoryTimes(const wxString& dirname,
                                        FILETIME *, FILETIME *, FILETIME *);

        // we should pass the path without the trailing separator to
        // wxGetDirectoryTimes()
        ok = wxGetDirectoryTimes(GetPath(wxPATH_GET_VOLUME),
                                 &ftAccess, &ftCreate, &ftWrite);
    }
    else // file
    {
        wxFileHandle fh(GetFullPath(), wxFileHandle::Read);
        if ( fh.IsOk() )
        {
            ok = ::GetFileTime(fh,
                               dtCreate ? &ftCreate : NULL,
                               dtAccess ? &ftAccess : NULL,
                               dtMod ? &ftWrite : NULL) != 0;
        }
        else
        {
            ok = false;
        }
    }

    if ( ok )
    {
        if ( dtCreate )
            ConvertFileTimeToWx(dtCreate, ftCreate);
        if ( dtAccess )
            ConvertFileTimeToWx(dtAccess, ftAccess);
        if ( dtMod )
            ConvertFileTimeToWx(dtMod, ftWrite);

        return true;
    }
#elif defined(__UNIX_LIKE__) || defined(__WXMAC__) || defined(__OS2__) || (defined(__DOS__) && defined(__WATCOMC__))
    // no need to test for IsDir() here
    wxStructStat stBuf;
    if ( wxStat( GetFullPath().fn_str(), &stBuf) == 0 )
    {
        if ( dtAccess )
            dtAccess->Set(stBuf.st_atime);
        if ( dtMod )
            dtMod->Set(stBuf.st_mtime);
        if ( dtCreate )
            dtCreate->Set(stBuf.st_ctime);

        return true;
    }
#else // other platform
    wxUnusedVar(dtAccess);
    wxUnusedVar(dtMod);
    wxUnusedVar(dtCreate);
#endif // platforms

    wxLogSysError(_("Failed to retrieve file times for '%s'"),
                  GetFullPath().c_str());

    return false;
}

#endif // wxUSE_DATETIME


// ----------------------------------------------------------------------------
// file size functions
// ----------------------------------------------------------------------------

/* static */
wxULongLong wxFileName::GetSize(const wxString &filename)
{
    if (!wxFileExists(filename))
        return wxInvalidSize;

#if defined(__WXPALMOS__)
    // TODO
    return wxInvalidSize;
#elif defined(__WIN32__)
    wxFileHandle f(filename, wxFileHandle::Read);
    if (!f.IsOk())
        return wxInvalidSize;

    DWORD lpFileSizeHigh;
    DWORD ret = GetFileSize(f, &lpFileSizeHigh);
    if ( ret == INVALID_FILE_SIZE && ::GetLastError() != NO_ERROR )
        return wxInvalidSize;

    return wxULongLong(lpFileSizeHigh, ret);
#else // ! __WIN32__
    wxStructStat st;
#ifndef wxNEED_WX_UNISTD_H
    if (wxStat( filename.fn_str() , &st) != 0)
#else
    if (wxStat( filename, &st) != 0)
#endif
        return wxInvalidSize;
    return wxULongLong(st.st_size);
#endif
}

/* static */
wxString wxFileName::GetHumanReadableSize(const wxULongLong &bs,
                                          const wxString &nullsize,
                                          int precision)
{
    static const double KILOBYTESIZE = 1024.0;
    static const double MEGABYTESIZE = 1024.0*KILOBYTESIZE;
    static const double GIGABYTESIZE = 1024.0*MEGABYTESIZE;
    static const double TERABYTESIZE = 1024.0*GIGABYTESIZE;

    if (bs == 0 || bs == wxInvalidSize)
        return nullsize;

    double bytesize = bs.ToDouble();
    if (bytesize < KILOBYTESIZE)
        return wxString::Format(_("%s B"), bs.ToString().c_str());
    if (bytesize < MEGABYTESIZE)
        return wxString::Format(_("%.*f kB"), precision, bytesize/KILOBYTESIZE);
    if (bytesize < GIGABYTESIZE)
        return wxString::Format(_("%.*f MB"), precision, bytesize/MEGABYTESIZE);
    if (bytesize < TERABYTESIZE)
        return wxString::Format(_("%.*f GB"), precision, bytesize/GIGABYTESIZE);

    return wxString::Format(_("%.*f TB"), precision, bytesize/TERABYTESIZE);
}

wxULongLong wxFileName::GetSize() const
{
    return GetSize(GetFullPath());
}

wxString wxFileName::GetHumanReadableSize(const wxString &failmsg, int precision) const
{
    return GetHumanReadableSize(GetSize(), failmsg, precision);
}


// ----------------------------------------------------------------------------
// Mac-specific functions
// ----------------------------------------------------------------------------

#ifdef __WXMAC__

const short kMacExtensionMaxLength = 16 ;
class MacDefaultExtensionRecord
{
public :
  MacDefaultExtensionRecord()
  {
    m_ext[0] = 0 ;
    m_type = m_creator = 0 ;
  }
  MacDefaultExtensionRecord( const MacDefaultExtensionRecord& from )
  {
    wxStrcpy( m_ext , from.m_ext ) ;
    m_type = from.m_type ;
    m_creator = from.m_creator ;
  }
  MacDefaultExtensionRecord( const wxChar * extension , OSType type , OSType creator )
  {
    wxStrncpy( m_ext , extension , kMacExtensionMaxLength ) ;
    m_ext[kMacExtensionMaxLength] = 0 ;
    m_type = type ;
    m_creator = creator ;
  }
  wxChar m_ext[kMacExtensionMaxLength] ;
  OSType m_type ;
  OSType m_creator ;
}  ;

WX_DECLARE_OBJARRAY(MacDefaultExtensionRecord, MacDefaultExtensionArray) ;

bool gMacDefaultExtensionsInited = false ;

#include "wx/arrimpl.cpp"

WX_DEFINE_EXPORTED_OBJARRAY(MacDefaultExtensionArray) ;

MacDefaultExtensionArray gMacDefaultExtensions ;

// load the default extensions
MacDefaultExtensionRecord gDefaults[] =
{
    MacDefaultExtensionRecord( wxT("txt") , 'TEXT' , 'ttxt' ) ,
    MacDefaultExtensionRecord( wxT("tif") , 'TIFF' , '****' ) ,
    MacDefaultExtensionRecord( wxT("jpg") , 'JPEG' , '****' ) ,
} ;

static void MacEnsureDefaultExtensionsLoaded()
{
    if ( !gMacDefaultExtensionsInited )
    {
        // we could load the pc exchange prefs here too
        for ( size_t i = 0 ; i < WXSIZEOF( gDefaults ) ; ++i )
        {
            gMacDefaultExtensions.Add( gDefaults[i] ) ;
        }
        gMacDefaultExtensionsInited = true ;
    }
}

bool wxFileName::MacSetTypeAndCreator( wxUint32 type , wxUint32 creator )
{
    FSRef fsRef ;
    FSCatalogInfo catInfo;
    FileInfo *finfo ;

    if ( wxMacPathToFSRef( GetFullPath() , &fsRef ) == noErr )
    {
        if ( FSGetCatalogInfo (&fsRef, kFSCatInfoFinderInfo, &catInfo, NULL, NULL, NULL) == noErr )
        {
            finfo = (FileInfo*)&catInfo.finderInfo;
            finfo->fileType = type ;
            finfo->fileCreator = creator ;
            FSSetCatalogInfo( &fsRef, kFSCatInfoFinderInfo, &catInfo ) ;
            return true ;
        }
    }
    return false ;
}

bool wxFileName::MacGetTypeAndCreator( wxUint32 *type , wxUint32 *creator )
{
    FSRef fsRef ;
    FSCatalogInfo catInfo;
    FileInfo *finfo ;

    if ( wxMacPathToFSRef( GetFullPath() , &fsRef ) == noErr )
    {
        if ( FSGetCatalogInfo (&fsRef, kFSCatInfoFinderInfo, &catInfo, NULL, NULL, NULL) == noErr )
        {
            finfo = (FileInfo*)&catInfo.finderInfo;
            *type = finfo->fileType ;
            *creator = finfo->fileCreator ;
            return true ;
        }
    }
    return false ;
}

bool wxFileName::MacSetDefaultTypeAndCreator()
{
    wxUint32 type , creator ;
    if ( wxFileName::MacFindDefaultTypeAndCreator(GetExt() , &type ,
      &creator ) )
    {
        return MacSetTypeAndCreator( type , creator ) ;
    }
    return false;
}

bool wxFileName::MacFindDefaultTypeAndCreator( const wxString& ext , wxUint32 *type , wxUint32 *creator )
{
  MacEnsureDefaultExtensionsLoaded() ;
  wxString extl = ext.Lower() ;
  for( int i = gMacDefaultExtensions.Count() - 1 ; i >= 0 ; --i )
  {
    if ( gMacDefaultExtensions.Item(i).m_ext == extl )
    {
      *type = gMacDefaultExtensions.Item(i).m_type ;
      *creator = gMacDefaultExtensions.Item(i).m_creator ;
      return true ;
    }
  }
  return false ;
}

void wxFileName::MacRegisterDefaultTypeAndCreator( const wxString& ext , wxUint32 type , wxUint32 creator )
{
  MacEnsureDefaultExtensionsLoaded() ;
  MacDefaultExtensionRecord rec ;
  rec.m_type = type ;
  rec.m_creator = creator ;
  wxStrncpy( rec.m_ext , ext.Lower().c_str() , kMacExtensionMaxLength ) ;
  gMacDefaultExtensions.Add( rec ) ;
}
#endif
