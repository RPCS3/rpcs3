/////////////////////////////////////////////////////////////////////////////
// Name:        wx/dynlib.h
// Purpose:     Dynamic library loading classes
// Author:      Guilhem Lavaux, Vadim Zeitlin, Vaclav Slavik
// Modified by:
// Created:     20/07/98
// RCS-ID:      $Id: dynlib.h 61872 2009-09-09 22:37:05Z VZ $
// Copyright:   (c) 1998 Guilhem Lavaux
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef _WX_DYNLIB_H__
#define _WX_DYNLIB_H__

#include "wx/defs.h"

#if wxUSE_DYNLIB_CLASS

#include "wx/string.h"
#include "wx/dynarray.h"

#if defined(__OS2__) || defined(__EMX__)
#include "wx/os2/private.h"
#endif

#ifdef __WXMSW__
#include "wx/msw/private.h"
#endif

// note that we have our own dlerror() implementation under Darwin
#if (defined(HAVE_DLERROR) && !defined(__EMX__)) || defined(__DARWIN__)
    #define wxHAVE_DYNLIB_ERROR
#endif

class WXDLLIMPEXP_FWD_BASE wxDynamicLibraryDetailsCreator;

// ----------------------------------------------------------------------------
// conditional compilation
// ----------------------------------------------------------------------------

// Note: __OS2__/EMX has to be tested first, since we want to use
// native version, even if configure detected presence of DLOPEN.
#if defined(__OS2__) || defined(__EMX__) || defined(__WINDOWS__)
    typedef HMODULE             wxDllType;
#elif defined(__DARWIN__)
    // Don't include dlfcn.h on Darwin, we may be using our own replacements.
    typedef void               *wxDllType;
#elif defined(HAVE_DLOPEN)
    #include <dlfcn.h>
    typedef void               *wxDllType;
#elif defined(HAVE_SHL_LOAD)
    #include <dl.h>
    typedef shl_t               wxDllType;
#elif defined(__WXMAC__)
    #include <CodeFragments.h>
    typedef CFragConnectionID   wxDllType;
#else
    #error "Dynamic Loading classes can't be compiled on this platform, sorry."
#endif

// ----------------------------------------------------------------------------
// constants
// ----------------------------------------------------------------------------

enum wxDLFlags
{
    wxDL_LAZY       = 0x00000001,   // resolve undefined symbols at first use
                                    // (only works on some Unix versions)
    wxDL_NOW        = 0x00000002,   // resolve undefined symbols on load
                                    // (default, always the case under Win32)
    wxDL_GLOBAL     = 0x00000004,   // export extern symbols to subsequently
                                    // loaded libs.
    wxDL_VERBATIM   = 0x00000008,   // attempt to load the supplied library
                                    // name without appending the usual dll
                                    // filename extension.
    wxDL_NOSHARE    = 0x00000010,   // load new DLL, don't reuse already loaded
                                    // (only for wxPluginManager)

    wxDL_QUIET      = 0x00000020,   // don't log an error if failed to load

#if wxABI_VERSION >= 20810
    // this flag is dangerous, for internal use of wxMSW only, don't use at all
    // and especially don't use directly, use wxLoadedDLL instead if you really
    // do need it
    wxDL_GET_LOADED = 0x00000040,   // Win32 only: return handle of already
                                    // loaded DLL or NULL otherwise; Unload()
                                    // should not be called so don't forget to
                                    // Detach() if you use this function
#endif // wx 2.8.10+

    wxDL_DEFAULT    = wxDL_NOW      // default flags correspond to Win32
};

enum wxDynamicLibraryCategory
{
    wxDL_LIBRARY,       // standard library
    wxDL_MODULE         // loadable module/plugin
};

enum wxPluginCategory
{
    wxDL_PLUGIN_GUI,    // plugin that uses GUI classes
    wxDL_PLUGIN_BASE    // wxBase-only plugin
};

// ----------------------------------------------------------------------------
// macros
// ----------------------------------------------------------------------------

// when loading a function from a DLL you always have to cast the returned
// "void *" pointer to the correct type and, even more annoyingly, you have to
// repeat this type twice if you want to declare and define a function pointer
// all in one line
//
// this macro makes this slightly less painful by allowing you to specify the
// type only once, as the first parameter, and creating a variable of this type
// called "pfn<name>" initialized with the "name" from the "dynlib"
#define wxDYNLIB_FUNCTION(type, name, dynlib) \
    type pfn ## name = (type)(dynlib).GetSymbol(wxT(#name))

// ----------------------------------------------------------------------------
// wxDynamicLibraryDetails: contains details about a loaded wxDynamicLibrary
// ----------------------------------------------------------------------------

class WXDLLIMPEXP_BASE wxDynamicLibraryDetails
{
public:
    // ctor, normally never used as these objects are only created by
    // wxDynamicLibrary::ListLoaded()
    wxDynamicLibraryDetails() { m_address = NULL; m_length = 0; }

    // get the (base) name
    wxString GetName() const { return m_name; }

    // get the full path of this object
    wxString GetPath() const { return m_path; }

    // get the load address and the extent, return true if this information is
    // available
    bool GetAddress(void **addr, size_t *len) const
    {
        if ( !m_address )
            return false;

        if ( addr )
            *addr = m_address;
        if ( len )
            *len = m_length;

        return true;
    }

    // return the version of the DLL (may be empty if no version info)
    wxString GetVersion() const
    {
        return m_version;
    }

private:
    wxString m_name,
             m_path,
             m_version;

    void *m_address;
    size_t m_length;

    friend class wxDynamicLibraryDetailsCreator;
};

WX_DECLARE_USER_EXPORTED_OBJARRAY(wxDynamicLibraryDetails,
                                  wxDynamicLibraryDetailsArray,
                                  WXDLLIMPEXP_BASE);

// ----------------------------------------------------------------------------
// wxDynamicLibrary: represents a handle to a DLL/shared object
// ----------------------------------------------------------------------------

class WXDLLIMPEXP_BASE wxDynamicLibrary
{
public:
    // return a valid handle for the main program itself or NULL if back
    // linking is not supported by the current platform (e.g. Win32)
    static wxDllType         GetProgramHandle();

    // return the platform standard DLL extension (with leading dot)
    static const wxChar *GetDllExt() { return ms_dllext; }

    wxDynamicLibrary() : m_handle(0) { }
    wxDynamicLibrary(const wxString& libname, int flags = wxDL_DEFAULT)
        : m_handle(0)
    {
        Load(libname, flags);
    }

    // NOTE: this class is (deliberately) not virtual, do not attempt
    //       to use it polymorphically.
    ~wxDynamicLibrary() { Unload(); }

    // return true if the library was loaded successfully
    bool IsLoaded() const { return m_handle != 0; }

    // load the library with the given name (full or not), return true if ok
    bool Load(const wxString& libname, int flags = wxDL_DEFAULT);

    // raw function for loading dynamic libs: always behaves as if
    // wxDL_VERBATIM were specified and doesn't log error message if the
    // library couldn't be loaded but simply returns NULL
    static wxDllType RawLoad(const wxString& libname, int flags = wxDL_DEFAULT);

    // detach the library object from its handle, i.e. prevent the object from
    // unloading the library in its dtor -- the caller is now responsible for
    // doing this
    wxDllType Detach() { wxDllType h = m_handle; m_handle = 0; return h; }

    // unload the given library handle (presumably returned by Detach() before)
    static void Unload(wxDllType handle);

    // unload the library, also done automatically in dtor
    void Unload() { if ( IsLoaded() ) { Unload(m_handle); m_handle = 0; } }

    // Return the raw handle from dlopen and friends.
    wxDllType GetLibHandle() const { return m_handle; }

    // check if the given symbol is present in the library, useful to verify if
    // a loadable module is our plugin, for example, without provoking error
    // messages from GetSymbol()
    bool HasSymbol(const wxString& name) const
    {
        bool ok;
        DoGetSymbol(name, &ok);
        return ok;
    }

    // resolve a symbol in a loaded DLL, such as a variable or function name.
    // 'name' is the (possibly mangled) name of the symbol. (use extern "C" to
    // export unmangled names)
    //
    // Since it is perfectly valid for the returned symbol to actually be NULL,
    // that is not always indication of an error.  Pass and test the parameter
    // 'success' for a true indication of success or failure to load the
    // symbol.
    //
    // Returns a pointer to the symbol on success, or NULL if an error occurred
    // or the symbol wasn't found.
    void *GetSymbol(const wxString& name, bool *success = NULL) const;

    // low-level version of GetSymbol()
    static void *RawGetSymbol(wxDllType handle, const wxString& name);
    void *RawGetSymbol(const wxString& name) const
    {
#if defined (__WXPM__) || defined(__EMX__)
        return GetSymbol(name);
#else
        return RawGetSymbol(m_handle, name);
#endif
    }

#ifdef __WXMSW__
    // this function is useful for loading functions from the standard Windows
    // DLLs: such functions have an 'A' (in ANSI build) or 'W' (in Unicode, or
    // wide character build) suffix if they take string parameters
    static void *RawGetSymbolAorW(wxDllType handle, const wxString& name)
    {
        return RawGetSymbol
               (
                handle,
                name +
#if wxUSE_UNICODE
                L'W'
#else
                'A'
#endif
               );
    }

    void *GetSymbolAorW(const wxString& name) const
    {
        return RawGetSymbolAorW(m_handle, name);
    }
#endif // __WXMSW__

    // return all modules/shared libraries in the address space of this process
    //
    // returns an empty array if not implemented or an error occurred
    static wxDynamicLibraryDetailsArray ListLoaded();

    // return platform-specific name of dynamic library with proper extension
    // and prefix (e.g. "foo.dll" on Windows or "libfoo.so" on Linux)
    static wxString CanonicalizeName(const wxString& name,
                                     wxDynamicLibraryCategory cat = wxDL_LIBRARY);

    // return name of wxWidgets plugin (adds compiler and version info
    // to the filename):
    static wxString
    CanonicalizePluginName(const wxString& name,
                           wxPluginCategory cat = wxDL_PLUGIN_GUI);

    // return plugin directory on platforms where it makes sense and empty
    // string on others:
    static wxString GetPluginsDirectory();


protected:
    // common part of GetSymbol() and HasSymbol()
    void *DoGetSymbol(const wxString& name, bool *success = 0) const;

#ifdef wxHAVE_DYNLIB_ERROR
    // log the error after a dlxxx() function failure
    static void Error();
#endif // wxHAVE_DYNLIB_ERROR


    // platform specific shared lib suffix.
    static const wxChar *ms_dllext;

    // the handle to DLL or NULL
    wxDllType m_handle;

    // no copy ctor/assignment operators (or we'd try to unload the library
    // twice)
    DECLARE_NO_COPY_CLASS(wxDynamicLibrary)
};

#if defined(__WXMSW__) && wxABI_VERSION >= 20810

// ----------------------------------------------------------------------------
// wxLoadedDLL is a MSW-only internal helper class allowing to dynamically bind
// to a DLL already loaded into the project address space
// ----------------------------------------------------------------------------

class wxLoadedDLL : public wxDynamicLibrary
{
public:
    wxLoadedDLL(const wxString& dllname)
        : wxDynamicLibrary(dllname, wxDL_GET_LOADED | wxDL_VERBATIM | wxDL_QUIET)
    {
    }

    ~wxLoadedDLL()
    {
        Detach();
    }
};

#endif // __WXMSW__

// ----------------------------------------------------------------------------
// Interesting defines
// ----------------------------------------------------------------------------

#define WXDLL_ENTRY_FUNCTION() \
extern "C" WXEXPORT const wxClassInfo *wxGetClassFirst(); \
const wxClassInfo *wxGetClassFirst() { \
  return wxClassInfo::GetFirst(); \
}

#endif // wxUSE_DYNLIB_CLASS

#endif // _WX_DYNLIB_H__
