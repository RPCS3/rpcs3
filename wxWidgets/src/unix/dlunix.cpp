/////////////////////////////////////////////////////////////////////////////
// Name:        unix/dlunix.cpp
// Purpose:     Unix-specific part of wxDynamicLibrary and related classes
// Author:      Vadim Zeitlin
// Modified by:
// Created:     2005-01-16 (extracted from common/dynlib.cpp)
// RCS-ID:      $Id: dlunix.cpp 51903 2008-02-19 01:13:48Z DE $
// Copyright:   (c) 2000-2005 Vadim Zeitlin <vadim@wxwindows.org>
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

// ============================================================================
// declarations
// ============================================================================

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

#include  "wx/wxprec.h"

#ifdef __BORLANDC__
  #pragma hdrstop
#endif

#if wxUSE_DYNLIB_CLASS

#include "wx/dynlib.h"
#include "wx/ffile.h"

#ifndef WX_PRECOMP
    #include "wx/intl.h"
    #include "wx/log.h"
#endif

#ifdef HAVE_DLOPEN
    #include <dlfcn.h>
#endif

#ifdef __DARWIN__
    #include <AvailabilityMacros.h>
#endif

// if some flags are not supported, just ignore them
#ifndef RTLD_LAZY
    #define RTLD_LAZY 0
#endif

#ifndef RTLD_NOW
    #define RTLD_NOW 0
#endif

#ifndef RTLD_GLOBAL
    #define RTLD_GLOBAL 0
#endif


#if defined(HAVE_DLOPEN) || defined(__DARWIN__)
    #define USE_POSIX_DL_FUNCS
#elif !defined(HAVE_SHL_LOAD)
    #error "Don't know how to load dynamic libraries on this platform!"
#endif

// ----------------------------------------------------------------------------
// constants
// ----------------------------------------------------------------------------

// standard shared libraries extensions for different Unix versions
#if defined(__HPUX__)
    const wxChar *wxDynamicLibrary::ms_dllext = _T(".sl");
#elif defined(__DARWIN__)
    const wxChar *wxDynamicLibrary::ms_dllext = _T(".bundle");
#else
    const wxChar *wxDynamicLibrary::ms_dllext = _T(".so");
#endif

// ============================================================================
// wxDynamicLibrary implementation
// ============================================================================

// ----------------------------------------------------------------------------
// dlxxx() emulation for Darwin
// Only useful if the OS X version could be < 10.3 at runtime
// ----------------------------------------------------------------------------

#if defined(__DARWIN__) && (MAC_OS_X_VERSION_MIN_REQUIRED < MAC_OS_X_VERSION_10_3)
// ---------------------------------------------------------------------------
// For Darwin/Mac OS X
//   supply the sun style dlopen functions in terms of Darwin NS*
// ---------------------------------------------------------------------------

/* Porting notes:
 *   The dlopen port is a port from dl_next.xs by Anno Siegel.
 *   dl_next.xs is itself a port from dl_dlopen.xs by Paul Marquess.
 *   The method used here is just to supply the sun style dlopen etc.
 *   functions in terms of Darwin NS*.
 */

#include <stdio.h>
#include <mach-o/dyld.h>

static char dl_last_error[1024];

static const char *wx_darwin_dlerror()
{
    return dl_last_error;
}

static void *wx_darwin_dlopen(const char *path, int WXUNUSED(mode) /* mode is ignored */)
{
    NSObjectFileImage ofile;
    NSModule handle = NULL;

    unsigned dyld_result = NSCreateObjectFileImageFromFile(path, &ofile);
    if ( dyld_result != NSObjectFileImageSuccess )
    {
        handle = NULL;

        static const char *errorStrings[] =
        {
            "%d: Object Image Load Failure",
            "%d: Object Image Load Success",
            "%d: Not an recognisable object file",
            "%d: No valid architecture",
            "%d: Object image has an invalid format",
            "%d: Invalid access (permissions?)",
            "%d: Unknown error code from NSCreateObjectFileImageFromFile"
        };

        const int index = dyld_result < WXSIZEOF(errorStrings)
                            ? dyld_result
                            : WXSIZEOF(errorStrings) - 1;

        // this call to sprintf() is safe as strings above are fixed at
        // compile-time and are shorter than WXSIZEOF(dl_last_error)
        sprintf(dl_last_error, errorStrings[index], dyld_result);
    }
    else
    {
        handle = NSLinkModule
                 (
                    ofile,
                    path,
                    NSLINKMODULE_OPTION_BINDNOW |
                    NSLINKMODULE_OPTION_RETURN_ON_ERROR
                 );

        if ( !handle )
        {
            NSLinkEditErrors err;
            int code;
            const char *filename;
            const char *errmsg;

            NSLinkEditError(&err, &code, &filename, &errmsg);
            strncpy(dl_last_error, errmsg, WXSIZEOF(dl_last_error)-1);
            dl_last_error[WXSIZEOF(dl_last_error)-1] = '\0';
        }
    }


    return handle;
}

static int wx_darwin_dlclose(void *handle)
{
    NSUnLinkModule((NSModule)handle, NSUNLINKMODULE_OPTION_NONE);
    return 0;
}

static void *wx_darwin_dlsym(void *handle, const char *symbol)
{
    // as on many other systems, C symbols have prepended underscores under
    // Darwin but unlike the normal dlopen(), NSLookupSymbolInModule() is not
    // aware of this
    wxCharBuffer buf(strlen(symbol) + 1);
    char *p = buf.data();
    p[0] = '_';
    strcpy(p + 1, symbol);

    NSSymbol nsSymbol = NSLookupSymbolInModule((NSModule)handle, p );
    return nsSymbol ? NSAddressOfSymbol(nsSymbol) : NULL;
}

// Add the weak linking attribute to dlopen's declaration
extern void * dlopen(const char * __path, int __mode) AVAILABLE_MAC_OS_X_VERSION_10_3_AND_LATER;

// For all of these methods we test dlopen since all of the dl functions we use were added
// to OS X at the same time.  This also ensures we don't dlopen with the real function then
// dlclose with the internal implementation.

static inline void *wx_dlopen(const char *__path, int __mode)
{
#ifdef HAVE_DLOPEN
    if(&dlopen != NULL)
        return dlopen(__path, __mode);
    else
#endif
        return wx_darwin_dlopen(__path, __mode);
}

static inline int wx_dlclose(void *__handle)
{
#ifdef HAVE_DLOPEN
    if(&dlopen != NULL)
        return dlclose(__handle);
    else
#endif
        return wx_darwin_dlclose(__handle);
}

static inline const char *wx_dlerror()
{
#ifdef HAVE_DLOPEN
    if(&dlopen != NULL)
        return dlerror();
    else
#endif
        return wx_darwin_dlerror();
}

static inline void *wx_dlsym(void *__handle, const char *__symbol)
{
#ifdef HAVE_DLOPEN
    if(&dlopen != NULL)
        return dlsym(__handle, __symbol);
    else
#endif
        return wx_darwin_dlsym(__handle, __symbol);
}

#else // __DARWIN__/!__DARWIN__

// Use preprocessor definitions for non-Darwin or OS X >= 10.3
#define wx_dlopen(__path,__mode) dlopen(__path,__mode)
#define wx_dlclose(__handle) dlclose(__handle)
#define wx_dlerror() dlerror()
#define wx_dlsym(__handle,__symbol) dlsym(__handle,__symbol)

#endif // defined(__DARWIN__)

// ----------------------------------------------------------------------------
// loading/unloading DLLs
// ----------------------------------------------------------------------------

wxDllType wxDynamicLibrary::GetProgramHandle()
{
#ifdef USE_POSIX_DL_FUNCS
   return wx_dlopen(0, RTLD_LAZY);
#else
   return PROG_HANDLE;
#endif
}

/* static */
wxDllType wxDynamicLibrary::RawLoad(const wxString& libname, int flags)
{
    wxASSERT_MSG( !(flags & wxDL_NOW) || !(flags & wxDL_LAZY),
                  _T("wxDL_LAZY and wxDL_NOW are mutually exclusive.") );

#ifdef USE_POSIX_DL_FUNCS
    // we need to use either RTLD_NOW or RTLD_LAZY because if we call dlopen()
    // with flags == 0 recent versions of glibc just fail the call, so use
    // RTLD_NOW even if wxDL_NOW was not specified
    int rtldFlags = flags & wxDL_LAZY ? RTLD_LAZY : RTLD_NOW;

    if ( flags & wxDL_GLOBAL )
        rtldFlags |= RTLD_GLOBAL;

    return wx_dlopen(libname.fn_str(), rtldFlags);
#else // !USE_POSIX_DL_FUNCS
    int shlFlags = 0;

    if ( flags & wxDL_LAZY )
    {
        shlFlags |= BIND_DEFERRED;
    }
    else if ( flags & wxDL_NOW )
    {
        shlFlags |= BIND_IMMEDIATE;
    }

    return shl_load(libname.fn_str(), shlFlags, 0);
#endif // USE_POSIX_DL_FUNCS/!USE_POSIX_DL_FUNCS
}

/* static */
void wxDynamicLibrary::Unload(wxDllType handle)
{
#ifdef wxHAVE_DYNLIB_ERROR
    int rc =
#endif

#ifdef USE_POSIX_DL_FUNCS
    wx_dlclose(handle);
#else // !USE_POSIX_DL_FUNCS
    shl_unload(handle);
#endif // USE_POSIX_DL_FUNCS/!USE_POSIX_DL_FUNCS

#if defined(USE_POSIX_DL_FUNCS) && defined(wxHAVE_DYNLIB_ERROR)
    if ( rc != 0 )
        Error();
#endif
}

/* static */
void *wxDynamicLibrary::RawGetSymbol(wxDllType handle, const wxString& name)
{
    void *symbol;

#ifdef USE_POSIX_DL_FUNCS
    symbol = wx_dlsym(handle, name.fn_str());
#else // !USE_POSIX_DL_FUNCS
    // note that shl_findsym modifies the handle argument to indicate where the
    // symbol was found, but it's ok to modify the local handle copy here
    if ( shl_findsym(&handle, name.fn_str(), TYPE_UNDEFINED, &symbol) != 0 )
        symbol = 0;
#endif // USE_POSIX_DL_FUNCS/!USE_POSIX_DL_FUNCS

    return symbol;
}

// ----------------------------------------------------------------------------
// error handling
// ----------------------------------------------------------------------------

#ifdef wxHAVE_DYNLIB_ERROR

/* static */
void wxDynamicLibrary::Error()
{
#if wxUSE_UNICODE
    wxWCharBuffer buffer = wxConvLocal.cMB2WC( wx_dlerror() );
    const wxChar *err = buffer;
#else
    const wxChar *err = wx_dlerror();
#endif

    wxLogError(wxT("%s"), err ? err : _("Unknown dynamic library error"));
}

#endif // wxHAVE_DYNLIB_ERROR

// ----------------------------------------------------------------------------
// listing loaded modules
// ----------------------------------------------------------------------------

// wxDynamicLibraryDetails declares this class as its friend, so put the code
// initializing new details objects here
class wxDynamicLibraryDetailsCreator
{
public:
    // create a new wxDynamicLibraryDetails from the given data
    static wxDynamicLibraryDetails *
    New(void *start, void *end, const wxString& path)
    {
        wxDynamicLibraryDetails *details = new wxDynamicLibraryDetails;
        details->m_path = path;
        details->m_name = path.AfterLast(_T('/'));
        details->m_address = start;
        details->m_length = (char *)end - (char *)start;

        // try to extract the library version from its name
        const size_t posExt = path.rfind(_T(".so"));
        if ( posExt != wxString::npos )
        {
            if ( path.c_str()[posExt + 3] == _T('.') )
            {
                // assume "libfoo.so.x.y.z" case
                details->m_version.assign(path, posExt + 4, wxString::npos);
            }
            else
            {
                size_t posDash = path.find_last_of(_T('-'), posExt);
                if ( posDash != wxString::npos )
                {
                    // assume "libbar-x.y.z.so" case
                    posDash++;
                    details->m_version.assign(path, posDash, posExt - posDash);
                }
            }
        }

        return details;
    }
};

/* static */
wxDynamicLibraryDetailsArray wxDynamicLibrary::ListLoaded()
{
    wxDynamicLibraryDetailsArray dlls;

#ifdef __LINUX__
    // examine /proc/self/maps to find out what is loaded in our address space
    wxFFile file(_T("/proc/self/maps"));
    if ( file.IsOpened() )
    {
        // details of the module currently being parsed
        wxString pathCur;
        void *startCur = NULL,
             *endCur = NULL;

        char path[1024];
        char buf[1024];
        while ( fgets(buf, WXSIZEOF(buf), file.fp()) )
        {
            // format is: "start-end perm offset maj:min inode path", see proc(5)
            void *start,
                 *end;
            switch ( sscanf(buf, "%p-%p %*4s %*p %*02x:%*02x %*d %1024s\n",
                            &start, &end, path) )
            {
                case 2:
                    // there may be no path column
                    path[0] = '\0';
                    break;

                case 3:
                    // nothing to do, read everything we wanted
                    break;

                default:
                    // chop '\n'
                    buf[strlen(buf) - 1] = '\0';
                    wxLogDebug(_T("Failed to parse line \"%s\" in /proc/self/maps."),
                               buf);
                    continue;
            }

            wxASSERT_MSG( start >= endCur,
                          _T("overlapping regions in /proc/self/maps?") );

            wxString pathNew = wxString::FromAscii(path);
            if ( pathCur.empty() )
            {
                // new module start
                pathCur = pathNew;
                startCur = start;
                endCur = end;
            }
            else if ( pathCur == pathNew && endCur == end )
            {
                // continuation of the same module in the address space
                endCur = end;
            }
            else // end of the current module
            {
                dlls.Add(wxDynamicLibraryDetailsCreator::New(startCur,
                                                             endCur,
                                                             pathCur));
                pathCur.clear();
            }
        }
    }
#endif // __LINUX__

    return dlls;
}

#endif // wxUSE_DYNLIB_CLASS

