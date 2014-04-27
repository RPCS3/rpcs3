/////////////////////////////////////////////////////////////////////////////
// Name:        wx/fontmap.h
// Purpose:     wxFontMapper class
// Author:      Vadim Zeitlin
// Modified by:
// Created:     04.11.99
// RCS-ID:      $Id: fontmap.h 49563 2007-10-31 20:46:21Z VZ $
// Copyright:   (c) Vadim Zeitlin
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef _WX_FONTMAPPER_H_
#define _WX_FONTMAPPER_H_

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

#if wxUSE_FONTMAP

#include "wx/fontenc.h"         // for wxFontEncoding

#if wxUSE_GUI
    #include "wx/fontutil.h"    // for wxNativeEncodingInfo
#endif // wxUSE_GUI

#if wxUSE_CONFIG && wxUSE_FILECONFIG
    class WXDLLIMPEXP_FWD_BASE wxConfigBase;
#endif // wxUSE_CONFIG

class WXDLLIMPEXP_FWD_CORE wxFontMapper;

#if wxUSE_GUI
    class WXDLLIMPEXP_FWD_CORE wxWindow;
#endif // wxUSE_GUI

// ============================================================================
// wxFontMapper manages user-definable correspondence between wxWidgets font
// encodings and the fonts present on the machine.
//
// This is a singleton class, font mapper objects can only be accessed using
// wxFontMapper::Get().
// ============================================================================

// ----------------------------------------------------------------------------
// wxFontMapperBase: this is a non-interactive class which just uses its built
//                   in knowledge of the encodings equivalence
// ----------------------------------------------------------------------------

class WXDLLIMPEXP_BASE wxFontMapperBase
{
public:
    // constructtor and such
    // ---------------------

    // default ctor
    wxFontMapperBase();

    // virtual dtor for any base class
    virtual ~wxFontMapperBase();

    // return instance of the wxFontMapper singleton
    // wxBase code only cares that it's a wxFontMapperBase
    // In wxBase, wxFontMapper is only forward declared
    // so one cannot implicitly cast from it to wxFontMapperBase.
    static wxFontMapperBase *Get();

    // set the singleton to 'mapper' instance and return previous one
    static wxFontMapper *Set(wxFontMapper *mapper);

    // delete the existing font mapper if any
    static void Reset();


    // translates charset strings to encoding
    // --------------------------------------

    // returns the encoding for the given charset (in the form of RFC 2046) or
    // wxFONTENCODING_SYSTEM if couldn't decode it
    //
    // interactive parameter is ignored in the base class, we behave as if it
    // were always false
    virtual wxFontEncoding CharsetToEncoding(const wxString& charset,
                                             bool interactive = true);

    // information about supported encodings
    // -------------------------------------

    // get the number of font encodings we know about
    static size_t GetSupportedEncodingsCount();

    // get the n-th supported encoding
    static wxFontEncoding GetEncoding(size_t n);

    // return canonical name of this encoding (this is a short string,
    // GetEncodingDescription() returns a longer one)
    static wxString GetEncodingName(wxFontEncoding encoding);

    // return a list of all names of this encoding (see GetEncodingName)
    static const wxChar** GetAllEncodingNames(wxFontEncoding encoding);

    // return user-readable string describing the given encoding
    //
    // NB: hard-coded now, but might change later (read it from config?)
    static wxString GetEncodingDescription(wxFontEncoding encoding);

    // find the encoding corresponding to the given name, inverse of
    // GetEncodingName() and less general than CharsetToEncoding()
    //
    // returns wxFONTENCODING_MAX if the name is not a supported encoding
    static wxFontEncoding GetEncodingFromName(const wxString& name);


    // functions which allow to configure the config object used: by default,
    // the global one (from wxConfigBase::Get() will be used) and the default
    // root path for the config settings is the string returned by
    // GetDefaultConfigPath()
    // ----------------------------------------------------------------------

#if wxUSE_CONFIG && wxUSE_FILECONFIG
    // set the root config path to use (should be an absolute path)
    void SetConfigPath(const wxString& prefix);

    // return default config path
    static const wxChar *GetDefaultConfigPath();
#endif // wxUSE_CONFIG


    // returns true for the base class and false for a "real" font mapper object
    // (implementation-only)
    virtual bool IsDummy() { return true; }

protected:
#if wxUSE_CONFIG && wxUSE_FILECONFIG
    // get the config object we're using -- either the global config object
    // or a wxMemoryConfig object created by this class otherwise
    wxConfigBase *GetConfig();

    // gets the root path for our settings -- if it wasn't set explicitly, use
    // GetDefaultConfigPath()
    const wxString& GetConfigPath();

    // change to the given (relative) path in the config, return true if ok
    // (then GetConfig() will return something !NULL), false if no config
    // object
    //
    // caller should provide a pointer to the string variable which should be
    // later passed to RestorePath()
    bool ChangePath(const wxString& pathNew, wxString *pathOld);

    // restore the config path after use
    void RestorePath(const wxString& pathOld);

    // config object and path (in it) to use
    wxConfigBase *m_configDummy;

    wxString m_configRootPath;
#endif // wxUSE_CONFIG

    // the real implementation of the base class version of CharsetToEncoding()
    //
    // returns wxFONTENCODING_UNKNOWN if encoding is unknown and we shouldn't
    // ask the user about it, wxFONTENCODING_SYSTEM if it is unknown but we
    // should/could ask the user
    int NonInteractiveCharsetToEncoding(const wxString& charset);

private:
    // the global fontmapper object or NULL
    static wxFontMapper *sm_instance;

    friend class wxFontMapperPathChanger;

    DECLARE_NO_COPY_CLASS(wxFontMapperBase)
};

// ----------------------------------------------------------------------------
// wxFontMapper: interactive extension of wxFontMapperBase
//
// The default implementations of all functions will ask the user if they are
// not capable of finding the answer themselves and store the answer in a
// config file (configurable via SetConfigXXX functions). This behaviour may
// be disabled by giving the value of false to "interactive" parameter.
// However, the functions will always consult the config file to allow the
// user-defined values override the default logic and there is no way to
// disable this -- which shouldn't be ever needed because if "interactive" was
// never true, the config file is never created anyhow.
// ----------------------------------------------------------------------------

#if wxUSE_GUI

class WXDLLIMPEXP_CORE wxFontMapper : public wxFontMapperBase
{
public:
    // default ctor
    wxFontMapper();

    // virtual dtor for a base class
    virtual ~wxFontMapper();

    // working with the encodings
    // --------------------------

    // returns the encoding for the given charset (in the form of RFC 2046) or
    // wxFONTENCODING_SYSTEM if couldn't decode it
    virtual wxFontEncoding CharsetToEncoding(const wxString& charset,
                                             bool interactive = true);

    // find an alternative for the given encoding (which is supposed to not be
    // available on this system). If successful, return true and fill info
    // structure with the parameters required to create the font, otherwise
    // return false
    virtual bool GetAltForEncoding(wxFontEncoding encoding,
                                   wxNativeEncodingInfo *info,
                                   const wxString& facename = wxEmptyString,
                                   bool interactive = true);

    // version better suitable for 'public' use. Returns wxFontEcoding
    // that can be used it wxFont ctor
    bool GetAltForEncoding(wxFontEncoding encoding,
                           wxFontEncoding *alt_encoding,
                           const wxString& facename = wxEmptyString,
                           bool interactive = true);

    // checks whether given encoding is available in given face or not.
    //
    // if no facename is given (default), return true if it's available in any
    // facename at alll.
    virtual bool IsEncodingAvailable(wxFontEncoding encoding,
                                     const wxString& facename = wxEmptyString);


    // configure the appearance of the dialogs we may popup
    // ----------------------------------------------------

    // the parent window for modal dialogs
    void SetDialogParent(wxWindow *parent) { m_windowParent = parent; }

    // the title for the dialogs (note that default is quite reasonable)
    void SetDialogTitle(const wxString& title) { m_titleDialog = title; }

    // GUI code needs to know it's a wxFontMapper because there
    // are additional methods in the subclass.
    static wxFontMapper *Get();

    // pseudo-RTTI since we aren't a wxObject.
    virtual bool IsDummy() { return false; }

protected:
    // GetAltForEncoding() helper: tests for the existence of the given
    // encoding and saves the result in config if ok - this results in the
    // following (desired) behaviour: when an unknown/unavailable encoding is
    // requested for the first time, the user is asked about a replacement,
    // but if he doesn't choose any and the default logic finds one, it will
    // be saved in the config so that the user won't be asked about it any
    // more
    bool TestAltEncoding(const wxString& configEntry,
                         wxFontEncoding encReplacement,
                         wxNativeEncodingInfo *info);

    // the title for our dialogs
    wxString m_titleDialog;

    // the parent window for our dialogs
    wxWindow *m_windowParent;

private:
    DECLARE_NO_COPY_CLASS(wxFontMapper)
};

#endif // wxUSE_GUI

// ----------------------------------------------------------------------------
// global variables
// ----------------------------------------------------------------------------

// the default font mapper for wxWidgets programs do NOT use! This is for
// backward compatibility, use wxFontMapper::Get() instead
#define wxTheFontMapper (wxFontMapper::Get())

#else // !wxUSE_FONTMAP

#if wxUSE_GUI
    // wxEncodingToCodepage (utils.cpp) needs wxGetNativeFontEncoding
    #include "wx/fontutil.h"
#endif

#endif // wxUSE_FONTMAP/!wxUSE_FONTMAP

#endif // _WX_FONTMAPPER_H_

