/////////////////////////////////////////////////////////////////////////////
// Name:        wx/msw/setup.h
// Purpose:     Configuration for the library
// Author:      Julian Smart
// Modified by:
// Created:     01/02/97
// RCS-ID:      $Id: setup0.h 60190 2009-04-16 00:57:35Z KO $
// Copyright:   (c) Julian Smart
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef _WX_SETUP_H_
#define _WX_SETUP_H_

/* --- start common options --- */
// ----------------------------------------------------------------------------
// global settings
// ----------------------------------------------------------------------------

// define this to 0 when building wxBase library - this can also be done from
// makefile/project file overriding the value here
#ifndef wxUSE_GUI
    #define wxUSE_GUI            1
#endif // wxUSE_GUI

// ----------------------------------------------------------------------------
// compatibility settings
// ----------------------------------------------------------------------------

// This setting determines the compatibility with 2.4 API: set it to 1 to
// enable it but please consider updating your code instead.
//
// Default is 0
//
// Recommended setting: 0 (please update your code)
#define WXWIN_COMPATIBILITY_2_4 0

// This setting determines the compatibility with 2.6 API: set it to 0 to
// flag all cases of using deprecated functions.
//
// Default is 1 but please try building your code with 0 as the default will
// change to 0 in the next version and the deprecated functions will disappear
// in the version after it completely.
//
// Recommended setting: 0 (please update your code)
#define WXWIN_COMPATIBILITY_2_6 1

// MSW-only: Set to 0 for accurate dialog units, else 1 for old behaviour when
// default system font is used for wxWindow::GetCharWidth/Height() instead of
// the current font.
//
// Default is 0
//
// Recommended setting: 0
#define wxDIALOG_UNIT_COMPATIBILITY   0

// ----------------------------------------------------------------------------
// debugging settings
// ----------------------------------------------------------------------------

// Generic comment about debugging settings: they are very useful if you don't
// use any other memory leak detection tools such as Purify/BoundsChecker, but
// are probably redundant otherwise. Also, Visual C++ CRT has the same features
// as wxWidgets memory debugging subsystem built in since version 5.0 and you
// may prefer to use it instead of built in memory debugging code because it is
// faster and more fool proof.
//
// Using VC++ CRT memory debugging is enabled by default in debug mode
// (__WXDEBUG__) if wxUSE_GLOBAL_MEMORY_OPERATORS is *not* enabled (i.e. is 0)
// and if __NO_VC_CRTDBG__ is not defined.

// If 1, enables wxDebugContext, for writing error messages to file, etc. If
// __WXDEBUG__ is not defined, will still use the normal memory operators.
//
// Default is 0
//
// Recommended setting: 0
#define wxUSE_DEBUG_CONTEXT 0

// If 1, enables debugging versions of wxObject::new and wxObject::delete *IF*
// __WXDEBUG__ is also defined.
//
// WARNING: this code may not work with all architectures, especially if
// alignment is an issue. This switch is currently ignored for mingw / cygwin
//
// Default is 0
//
// Recommended setting: 1 if you are not using a memory debugging tool, else 0
#define wxUSE_MEMORY_TRACING 0

// In debug mode, cause new and delete to be redefined globally.
// If this causes problems (e.g. link errors which is a common problem
// especially if you use another library which also redefines the global new
// and delete), set this to 0.
// This switch is currently ignored for mingw / cygwin
//
// Default is 0
//
// Recommended setting: 0
#define wxUSE_GLOBAL_MEMORY_OPERATORS 0

// In debug mode, causes new to be defined to be WXDEBUG_NEW (see object.h). If
// this causes problems (e.g. link errors), set this to 0. You may need to set
// this to 0 if using templates (at least for VC++). This switch is currently
// ignored for mingw / cygwin / CodeWarrior
//
// Default is 0
//
// Recommended setting: 0
#define wxUSE_DEBUG_NEW_ALWAYS 0

// wxHandleFatalExceptions() may be used to catch the program faults at run
// time and, instead of terminating the program with a usual GPF message box,
// call the user-defined wxApp::OnFatalException() function. If you set
// wxUSE_ON_FATAL_EXCEPTION to 0, wxHandleFatalExceptions() will not work.
//
// This setting is for Win32 only and can only be enabled if your compiler
// supports Win32 structured exception handling (currently only VC++ does)
//
// Default is 1
//
// Recommended setting: 1 if your compiler supports it.
#define wxUSE_ON_FATAL_EXCEPTION 1

// Set this to 1 to be able to generate a human-readable (unlike
// machine-readable minidump created by wxCrashReport::Generate()) stack back
// trace when your program crashes using wxStackWalker
//
// Default is 1 if supported by the compiler.
//
// Recommended setting: 1, set to 0 if your programs never crash
#define wxUSE_STACKWALKER 1

// Set this to 1 to compile in wxDebugReport class which allows you to create
// and optionally upload to your web site a debug report consisting of back
// trace of the crash (if wxUSE_STACKWALKER == 1) and other information.
//
// Default is 1 if supported by the compiler.
//
// Recommended setting: 1, it is compiled into a separate library so there
//                         is no overhead if you don't use it
#define wxUSE_DEBUGREPORT 1

// ----------------------------------------------------------------------------
// Unicode support
// ----------------------------------------------------------------------------

// Set wxUSE_UNICODE to 1 to compile wxWidgets in Unicode mode: wxChar will be
// defined as wchar_t, wxString will use Unicode internally. If you set this
// to 1, you must use wxT() macro for all literal strings in the program.
//
// Unicode is currently only fully supported under Windows NT/2000/XP
// (Windows 9x doesn't support it and the programs compiled in Unicode mode
// will not run under 9x -- but see wxUSE_UNICODE_MSLU below).
//
// Default is 0
//
// Recommended setting: 0 (unless you only plan to use Windows NT/2000/XP)
#ifndef wxUSE_UNICODE
    #define wxUSE_UNICODE 0
#endif

// Setting wxUSE_WCHAR_T to 1 gives you some degree of Unicode support without
// compiling the program in Unicode mode. More precisely, it will be possible
// to construct wxString from a wide (Unicode) string and convert any wxString
// to Unicode.
//
// Default is 1
//
// Recommended setting: 1
#define wxUSE_WCHAR_T 1

// ----------------------------------------------------------------------------
// global features
// ----------------------------------------------------------------------------

// Compile library in exception-safe mode? If set to 1, the library will try to
// behave correctly in presence of exceptions (even though it still will not
// use the exceptions itself) and notify the user code about any unhandled
// exceptions. If set to 0, propagation of the exceptions through the library
// code will lead to undefined behaviour -- but the code itself will be
// slightly smaller and faster.
//
// Note that like wxUSE_THREADS this option is automatically set to 0 if
// wxNO_EXCEPTIONS is defined.
//
// Default is 1
//
// Recommended setting: depends on whether you intend to use C++ exceptions
//                      in your own code (1 if you do, 0 if you don't)
#define wxUSE_EXCEPTIONS    1

// Set wxUSE_EXTENDED_RTTI to 1 to use extended RTTI
//
// Default is 0
//
// Recommended setting: 0 (this is still work in progress...)
#define wxUSE_EXTENDED_RTTI 0

// Set wxUSE_STL to 1 to derive wxList(Foo) and wxArray(Foo) from
// std::list<Foo*> and std::vector<Foo*>, with a compatibility interface,
// and for wxHashMap to be implemented with templates.
//
// Default is 0
//
// Recommended setting: YMMV
#define wxUSE_STL 0

// Support for message/error logging. This includes wxLogXXX() functions and
// wxLog and derived classes. Don't set this to 0 unless you really know what
// you are doing.
//
// Default is 1
//
// Recommended setting: 1 (always)
#define wxUSE_LOG 1

// Recommended setting: 1
#define wxUSE_LOGWINDOW 1

// Recommended setting: 1
#define wxUSE_LOGGUI 1

// Recommended setting: 1
#define wxUSE_LOG_DIALOG 1

// Support for command line parsing using wxCmdLineParser class.
//
// Default is 1
//
// Recommended setting: 1 (can be set to 0 if you don't use the cmd line)
#define wxUSE_CMDLINE_PARSER 1

// Support for multithreaded applications: if 1, compile in thread classes
// (thread.h) and make the library a bit more thread safe. Although thread
// support is quite stable by now, you may still consider recompiling the
// library without it if you have no use for it - this will result in a
// somewhat smaller and faster operation.
//
// Notice that if wxNO_THREADS is defined, wxUSE_THREADS is automatically reset
// to 0 in wx/chkconf.h, so, for example, if you set USE_THREADS to 0 in
// build/msw/config.* file this value will have no effect.
//
// Default is 1
//
// Recommended setting: 0 unless you do plan to develop MT applications
#define wxUSE_THREADS 1

// If enabled, compiles wxWidgets streams classes
//
// wx stream classes are used for image IO, process IO redirection, network
// protocols implementation and much more and so disabling this results in a
// lot of other functionality being lost.
//
// Default is 1
//
// Recommended setting: 1 as setting it to 0 disables many other things
#define wxUSE_STREAMS       1

// Use standard C++ streams if 1 instead of wx streams in some places. If
// disabled (default), wx streams are used everywhere and wxWidgets doesn't
// depend on the standard streams library.
//
// Notice that enabling this does not replace wx streams with std streams
// everywhere, in a lot of places wx streams are used no matter what.
//
// Default is 0
//
// Recommended setting: 1 if you use the standard streams anyhow and so
//                      dependency on the standard streams library is not a
//                      problem
#define wxUSE_STD_IOSTREAM  0

// Enable conversion to standard C++ string if 1.
//
// Default is 1 for most compilers.
//
// Currently the Digital Mars and Watcom compilers come without standard C++
// library headers by default, wxUSE_STD_STRING can be set to 1 if you do have
// them (e.g. from STLPort).
//
// VC++ 5.0 does include standard C++ library header, however they produce
// many warnings that can't be turned off when compiled at warning level 4.
#if defined(__DMC__) || defined(__WATCOMC__) \
        || (defined(_MSC_VER) && _MSC_VER < 1200)
    #define wxUSE_STD_STRING  0
#else
    #define wxUSE_STD_STRING  1
#endif

// Support for positional parameters (e.g. %1$d, %2$s ...) in wxVsnprintf.
// Note that if the system's implementation does not support positional
// parameters, setting this to 1 forces the use of the wxWidgets implementation
// of wxVsnprintf. The standard vsnprintf() supports positional parameters on
// many Unix systems but usually doesn't under Windows.
//
// Positional parameters are very useful when translating a program since using
// them in formatting strings allow translators to correctly reorder the
// translated sentences.
//
// Default is 1
//
// Recommended setting: 1 if you want to support multiple languages
#define wxUSE_PRINTF_POS_PARAMS      1

// ----------------------------------------------------------------------------
// non GUI features selection
// ----------------------------------------------------------------------------

// Set wxUSE_LONGLONG to 1 to compile the wxLongLong class. This is a 64 bit
// integer which is implemented in terms of native 64 bit integers if any or
// uses emulation otherwise.
//
// This class is required by wxDateTime and so you should enable it if you want
// to use wxDateTime. For most modern platforms, it will use the native 64 bit
// integers in which case (almost) all of its functions are inline and it
// almost does not take any space, so there should be no reason to switch it
// off.
//
// Recommended setting: 1
#define wxUSE_LONGLONG      1

// Set wxUSE_(F)FILE to 1 to compile wx(F)File classes. wxFile uses low level
// POSIX functions for file access, wxFFile uses ANSI C stdio.h functions.
//
// Default is 1
//
// Recommended setting: 1 (wxFile is highly recommended as it is required by
// i18n code, wxFileConfig and others)
#define wxUSE_FILE          1
#define wxUSE_FFILE         1

// Use wxFSVolume class providing access to the configured/active mount points
//
// Default is 1
//
// Recommended setting: 1 (but may be safely disabled if you don't use it)
#define wxUSE_FSVOLUME      1

// Use wxStandardPaths class which allows to retrieve some standard locations
// in the file system
//
// Default is 1
//
// Recommended setting: 1 (may be disabled to save space, but not much)
#define wxUSE_STDPATHS      1

// use wxTextBuffer class: required by wxTextFile
#define wxUSE_TEXTBUFFER    1

// use wxTextFile class: requires wxFile and wxTextBuffer, required by
// wxFileConfig
#define wxUSE_TEXTFILE      1

// i18n support: _() macro, wxLocale class. Requires wxTextFile.
#define wxUSE_INTL          1

// Set wxUSE_DATETIME to 1 to compile the wxDateTime and related classes which
// allow to manipulate dates, times and time intervals. wxDateTime replaces the
// old wxTime and wxDate classes which are still provided for backwards
// compatibility (and implemented in terms of wxDateTime).
//
// Note that this class is relatively new and is still officially in alpha
// stage because some features are not yet (fully) implemented. It is already
// quite useful though and should only be disabled if you are aiming at
// absolutely minimal version of the library.
//
// Requires: wxUSE_LONGLONG
//
// Default is 1
//
// Recommended setting: 1
#define wxUSE_DATETIME      1

// Set wxUSE_TIMER to 1 to compile wxTimer class
//
// Default is 1
//
// Recommended setting: 1
#define wxUSE_TIMER         1

// Use wxStopWatch clas.
//
// Default is 1
//
// Recommended setting: 1 (needed by wxSocket)
#define wxUSE_STOPWATCH     1

// Setting wxUSE_CONFIG to 1 enables the use of wxConfig and related classes
// which allow the application to store its settings in the persistent
// storage. Setting this to 1 will also enable on-demand creation of the
// global config object in wxApp.
//
// See also wxUSE_CONFIG_NATIVE below.
//
// Recommended setting: 1
#define wxUSE_CONFIG        1

// If wxUSE_CONFIG is 1, you may choose to use either the native config
// classes under Windows (using .INI files under Win16 and the registry under
// Win32) or the portable text file format used by the config classes under
// Unix.
//
// Default is 1 to use native classes. Note that you may still use
// wxFileConfig even if you set this to 1 - just the config object created by
// default for the applications needs will be a wxRegConfig or wxIniConfig and
// not wxFileConfig.
//
// Recommended setting: 1
#define wxUSE_CONFIG_NATIVE   1

// If wxUSE_DIALUP_MANAGER is 1, compile in wxDialUpManager class which allows
// to connect/disconnect from the network and be notified whenever the dial-up
// network connection is established/terminated. Requires wxUSE_DYNAMIC_LOADER.
//
// Default is 1.
//
// Recommended setting: 1
#define wxUSE_DIALUP_MANAGER   1

// Compile in classes for run-time DLL loading and function calling.
// Required by wxUSE_DIALUP_MANAGER.
//
// This setting is for Win32 only
//
// Default is 1.
//
// Recommended setting: 1
#define wxUSE_DYNLIB_CLASS    1

// experimental, don't use for now
#define wxUSE_DYNAMIC_LOADER  1

// Set to 1 to use socket classes
#define wxUSE_SOCKETS       1

// Set to 1 to enable virtual file systems (required by wxHTML)
#define wxUSE_FILESYSTEM    1

// Set to 1 to enable virtual ZIP filesystem (requires wxUSE_FILESYSTEM)
#define wxUSE_FS_ZIP        1

// Set to 1 to enable virtual archive filesystem (requires wxUSE_FILESYSTEM)
#define wxUSE_FS_ARCHIVE    1

// Set to 1 to enable virtual Internet filesystem (requires wxUSE_FILESYSTEM)
#define wxUSE_FS_INET       1

// wxArchive classes for accessing archives such as zip and tar
#define wxUSE_ARCHIVE_STREAMS     1

// Set to 1 to compile wxZipInput/OutputStream classes.
#define wxUSE_ZIPSTREAM     1

// Set to 1 to compile wxTarInput/OutputStream classes.
#define wxUSE_TARSTREAM     1

// Set to 1 to compile wxZlibInput/OutputStream classes. Also required by
// wxUSE_LIBPNG
#define wxUSE_ZLIB          1

// If enabled, the code written by Apple will be used to write, in a portable
// way, float on the disk. See extended.c for the license which is different
// from wxWidgets one.
//
// Default is 1.
//
// Recommended setting: 1 unless you don't like the license terms (unlikely)
#define wxUSE_APPLE_IEEE          1

// Joystick support class
#define wxUSE_JOYSTICK            1

// wxFontMapper class
#define wxUSE_FONTMAP 1

// wxMimeTypesManager class
#define wxUSE_MIMETYPE 1

// wxProtocol and related classes: if you want to use either of wxFTP, wxHTTP
// or wxURL you need to set this to 1.
//
// Default is 1.
//
// Recommended setting: 1
#define wxUSE_PROTOCOL 1

// The settings for the individual URL schemes
#define wxUSE_PROTOCOL_FILE 1
#define wxUSE_PROTOCOL_FTP 1
#define wxUSE_PROTOCOL_HTTP 1

// Define this to use wxURL class.
#define wxUSE_URL 1

// Define this to use native platform url and protocol support.
// Currently valid only for MS-Windows.
// Note: if you set this to 1, you can open ftp/http/gopher sites
// and obtain a valid input stream for these sites
// even when you set wxUSE_PROTOCOL_FTP/HTTP to 0.
// Doing so reduces the code size.
//
// This code is experimental and subject to change.
#define wxUSE_URL_NATIVE 0

// Support for wxVariant class used in several places throughout the library,
// notably in wxDataViewCtrl API.
//
// Default is 1.
//
// Recommended setting: 1 unless you want to reduce the library size as much as
// possible in which case setting this to 0 can gain up to 100KB.
#define wxUSE_VARIANT 1

// Support for regular expression matching via wxRegEx class: enable this to
// use POSIX regular expressions in your code. You need to compile regex
// library from src/regex to use it under Windows.
//
// Default is 0
//
// Recommended setting: 1 if your compiler supports it, if it doesn't please
// contribute us a makefile for src/regex for it
#define wxUSE_REGEX       1

// wxSystemOptions class
#define wxUSE_SYSTEM_OPTIONS 1

// wxSound class
#define wxUSE_SOUND      1

// Use wxMediaCtrl
//
// Default is 1.
//
// Recommended setting: 1
#define wxUSE_MEDIACTRL     1

// Use GStreamer for Unix (req a lot of dependancies)
//
// Default is 0
//
// Recommended setting: 1 (wxMediaCtrl won't work by default without it)
#define wxUSE_GSTREAMER    0

// Use wxWidget's XRC XML-based resource system.  Recommended.
//
// Default is 1
//
// Recommended setting: 1 (requires wxUSE_XML)
#define wxUSE_XRC       1

// XML parsing classes. Note that their API will change in the future, so
// using wxXmlDocument and wxXmlNode in your app is not recommended.
//
// Default is the same as wxUSE_XRC, i.e. 1 by default.
//
// Recommended setting: 1 (required by XRC)
#define wxUSE_XML       wxUSE_XRC

// Use wxWidget's AUI docking system
//
// Default is 1
//
// Recommended setting: 1
#define wxUSE_AUI       1


// Enable the new wxGraphicsPath and wxGraphicsContext classes for an advanced
// 2D drawing API.  (Still somewhat experimental)
//
// Please note that on Windows you will need to link with gdiplus.lib (use
// USE_GDIPLUS=1 for makefile builds) and distribute gdiplus.dll with your
// application if you want it to be runnable on pre-XP systems.
//
// Default is 0
//
// Recommended setting: 1
#ifndef wxUSE_GRAPHICS_CONTEXT
#define wxUSE_GRAPHICS_CONTEXT 0
#endif

// Enable the new wxCairoContext classes for an advanced
// 2D drawing API.  (Still somewhat experimental)
//
// Please note that you will need to link with Cairo for this to work.
//
// Default is 0
//
// Recommended setting: 1
#ifndef wxUSE_CAIRO
#define wxUSE_CAIRO 0
#endif

// ----------------------------------------------------------------------------
// Individual GUI controls
// ----------------------------------------------------------------------------

// You must set wxUSE_CONTROLS to 1 if you are using any controls at all
// (without it, wxControl class is not compiled)
//
// Default is 1
//
// Recommended setting: 1 (don't change except for very special programs)
#define wxUSE_CONTROLS     1

// wxPopupWindow class is a top level transient window. It is currently used
// to implement wxTipWindow
//
// Default is 1
//
// Recommended setting: 1 (may be set to 0 if you don't wxUSE_TIPWINDOW)
#define wxUSE_POPUPWIN     1

// wxTipWindow allows to implement the custom tooltips, it is used by the
// context help classes. Requires wxUSE_POPUPWIN.
//
// Default is 1
//
// Recommended setting: 1 (may be set to 0)
#define wxUSE_TIPWINDOW    1

// Each of the settings below corresponds to one wxWidgets control. They are
// all switched on by default but may be disabled if you are sure that your
// program (including any standard dialogs it can show!) doesn't need them and
// if you desperately want to save some space. If you use any of these you must
// set wxUSE_CONTROLS as well.
//
// Default is 1
//
// Recommended setting: 1
#define wxUSE_ANIMATIONCTRL 1   // wxAnimationCtrl
#define wxUSE_BUTTON        1   // wxButton
#define wxUSE_BMPBUTTON     1   // wxBitmapButton
#define wxUSE_CALENDARCTRL  1   // wxCalendarCtrl
#define wxUSE_CHECKBOX      1   // wxCheckBox
#define wxUSE_CHECKLISTBOX  1   // wxCheckListBox (requires wxUSE_OWNER_DRAWN)
#define wxUSE_CHOICE        1   // wxChoice
#define wxUSE_COLLPANE      1   // wxCollapsiblePane
#define wxUSE_COLOURPICKERCTRL 1    // wxColourPickerCtrl
#define wxUSE_COMBOBOX      1   // wxComboBox
#define wxUSE_DATAVIEWCTRL  1   // wxDataViewCtrl
#define wxUSE_DATEPICKCTRL  1   // wxDatePickerCtrl
#define wxUSE_DIRPICKERCTRL 1   // wxDirPickerCtrl
#define wxUSE_FILEPICKERCTRL 1  // wxFilePickerCtrl
#define wxUSE_FONTPICKERCTRL 1  // wxFontPickerCtrl
#define wxUSE_GAUGE         1   // wxGauge
#define wxUSE_HYPERLINKCTRL 1   // wxHyperlinkCtrl
#define wxUSE_LISTBOX       1   // wxListBox
#define wxUSE_LISTCTRL      1   // wxListCtrl
#define wxUSE_RADIOBOX      1   // wxRadioBox
#define wxUSE_RADIOBTN      1   // wxRadioButton
#define wxUSE_SCROLLBAR     1   // wxScrollBar
#define wxUSE_SEARCHCTRL    1   // wxSearchCtrl
#define wxUSE_SLIDER        1   // wxSlider
#define wxUSE_SPINBTN       1   // wxSpinButton
#define wxUSE_SPINCTRL      1   // wxSpinCtrl
#define wxUSE_STATBOX       1   // wxStaticBox
#define wxUSE_STATLINE      1   // wxStaticLine
#define wxUSE_STATTEXT      1   // wxStaticText
#define wxUSE_STATBMP       1   // wxStaticBitmap
#define wxUSE_TEXTCTRL      1   // wxTextCtrl
#define wxUSE_TOGGLEBTN     1   // requires wxButton
#define wxUSE_TREECTRL      1   // wxTreeCtrl

// Use a status bar class? Depending on the value of wxUSE_NATIVE_STATUSBAR
// below either wxStatusBar95 or a generic wxStatusBar will be used.
//
// Default is 1
//
// Recommended setting: 1
#define wxUSE_STATUSBAR    1

// Two status bar implementations are available under Win32: the generic one
// or the wrapper around native control. For native look and feel the native
// version should be used.
//
// Default is 1 for the platforms where native status bar is supported.
//
// Recommended setting: 1 (there is no advantage in using the generic one)
#define wxUSE_NATIVE_STATUSBAR        1

// wxToolBar related settings: if wxUSE_TOOLBAR is 0, don't compile any toolbar
// classes at all. Otherwise, use the native toolbar class unless
// wxUSE_TOOLBAR_NATIVE is 0.
//
// Default is 1 for all settings.
//
// Recommended setting: 1 for wxUSE_TOOLBAR and wxUSE_TOOLBAR_NATIVE.
#define wxUSE_TOOLBAR 1
#define wxUSE_TOOLBAR_NATIVE 1

// wxNotebook is a control with several "tabs" located on one of its sides. It
// may be used to logically organise the data presented to the user instead of
// putting everything in one huge dialog. It replaces wxTabControl and related
// classes of wxWin 1.6x.
//
// Default is 1.
//
// Recommended setting: 1
#define wxUSE_NOTEBOOK 1

// wxListbook control is similar to wxNotebook but uses wxListCtrl instead of
// the tabs
//
// Default is 1.
//
// Recommended setting: 1
#define wxUSE_LISTBOOK 1

// wxChoicebook control is similar to wxNotebook but uses wxChoice instead of
// the tabs
//
// Default is 1.
//
// Recommended setting: 1
#define wxUSE_CHOICEBOOK 1

// wxTreebook control is similar to wxNotebook but uses wxTreeCtrl instead of
// the tabs
//
// Default is 1.
//
// Recommended setting: 1
#define wxUSE_TREEBOOK 1

// wxToolbook control is similar to wxNotebook but uses wxToolBar instead of
// tabs
//
// Default is 1.
//
// Recommended setting: 1
#define wxUSE_TOOLBOOK 1

// wxTabDialog is a generic version of wxNotebook but it is incompatible with
// the new class. It shouldn't be used in new code.
//
// Default is 0.
//
// Recommended setting: 0 (use wxNotebook)
#define wxUSE_TAB_DIALOG    0

// wxGrid class
//
// Default is 1, set to 0 to cut down compilation time and binaries size if you
// don't use it.
//
// Recommended setting: 1
//
#define wxUSE_GRID         1

// wxMiniFrame class: a frame with narrow title bar
//
// Default is 1.
//
// Recommended setting: 1 (it doesn't cost almost anything)
#define wxUSE_MINIFRAME 1

// wxComboCtrl and related classes: combobox with custom popup window and
// not necessarily a listbox.
//
// Default is 1.
//
// Recommended setting: 1 but can be safely set to 0 except for wxUniv where it
//                      it used by wxComboBox
#define wxUSE_COMBOCTRL 1

// wxOwnerDrawnComboBox is a custom combobox allowing to paint the combobox
// items.
//
// Default is 1.
//
// Recommended setting: 1 but can be safely set to 0, except where it is
//                      needed as a base class for generic wxBitmapComboBox.
#define wxUSE_ODCOMBOBOX 1

// wxBitmapComboBox is a combobox that can have images in front of text items.
//
// Default is 1.
//
// Recommended setting: 1 but can be safely set to 0
#define wxUSE_BITMAPCOMBOBOX 1

// ----------------------------------------------------------------------------
// Miscellaneous GUI stuff
// ----------------------------------------------------------------------------

// wxAcceleratorTable/Entry classes and support for them in wxMenu(Bar)
#define wxUSE_ACCEL 1

// Hotkey support (currently Windows only)
#define wxUSE_HOTKEY 1

// Use wxCaret: a class implementing a "cursor" in a text control (called caret
// under Windows).
//
// Default is 1.
//
// Recommended setting: 1 (can be safely set to 0, not used by the library)
#define wxUSE_CARET         1

// Use wxDisplay class: it allows enumerating all displays on a system and
// their geometries as well as finding the display on which the given point or
// window lies.
//
// Default is 1.
//
// Recommended setting: 1 if you need it, can be safely set to 0 otherwise
#define wxUSE_DISPLAY       1

// Miscellaneous geometry code: needed for Canvas library
#define wxUSE_GEOMETRY            1

// Use wxImageList. This class is needed by wxNotebook, wxTreeCtrl and
// wxListCtrl.
//
// Default is 1.
//
// Recommended setting: 1 (set it to 0 if you don't use any of the controls
// enumerated above, then this class is mostly useless too)
#define wxUSE_IMAGLIST      1

// Use wxMenu, wxMenuBar, wxMenuItem.
//
// Default is 1.
//
// Recommended setting: 1 (can't be disabled under MSW)
#define wxUSE_MENUS         1

// Use wxSashWindow class.
//
// Default is 1.
//
// Recommended setting: 1
#define wxUSE_SASH          1

// Use wxSplitterWindow class.
//
// Default is 1.
//
// Recommended setting: 1
#define wxUSE_SPLITTER      1

// Use wxToolTip and wxWindow::Set/GetToolTip() methods.
//
// Default is 1.
//
// Recommended setting: 1
#define wxUSE_TOOLTIPS      1

// wxValidator class and related methods
#define wxUSE_VALIDATORS 1

// ----------------------------------------------------------------------------
// common dialogs
// ----------------------------------------------------------------------------

// On rare occasions (e.g. using DJGPP) may want to omit common dialogs (e.g.
// file selector, printer dialog). Switching this off also switches off the
// printing architecture and interactive wxPrinterDC.
//
// Default is 1
//
// Recommended setting: 1 (unless it really doesn't work)
#define wxUSE_COMMON_DIALOGS 1

// wxBusyInfo displays window with message when app is busy. Works in same way
// as wxBusyCursor
#define wxUSE_BUSYINFO      1

// Use single/multiple choice dialogs.
//
// Default is 1
//
// Recommended setting: 1 (used in the library itself)
#define wxUSE_CHOICEDLG     1

// Use colour picker dialog
//
// Default is 1
//
// Recommended setting: 1
#define wxUSE_COLOURDLG     1

// wxDirDlg class for getting a directory name from user
#define wxUSE_DIRDLG 1

// TODO: setting to choose the generic or native one

// Use file open/save dialogs.
//
// Default is 1
//
// Recommended setting: 1 (used in many places in the library itself)
#define wxUSE_FILEDLG       1

// Use find/replace dialogs.
//
// Default is 1
//
// Recommended setting: 1 (but may be safely set to 0)
#define wxUSE_FINDREPLDLG       1

// Use font picker dialog
//
// Default is 1
//
// Recommended setting: 1 (used in the library itself)
#define wxUSE_FONTDLG       1

// Use wxMessageDialog and wxMessageBox.
//
// Default is 1
//
// Recommended setting: 1 (used in the library itself)
#define wxUSE_MSGDLG        1

// progress dialog class for lengthy operations
#define wxUSE_PROGRESSDLG 1

// support for startup tips (wxShowTip &c)
#define wxUSE_STARTUP_TIPS 1

// text entry dialog and wxGetTextFromUser function
#define wxUSE_TEXTDLG 1

// number entry dialog
#define wxUSE_NUMBERDLG 1

// splash screen class
#define wxUSE_SPLASH 1

// wizards
#define wxUSE_WIZARDDLG 1

// Compile in wxAboutBox() function showing the standard "About" dialog.
//
// Default is 1
//
// Recommended setting: 1 but can be set to 0 to save some space if you don't
//                      use this function
#define wxUSE_ABOUTDLG 1

// ----------------------------------------------------------------------------
// Metafiles support
// ----------------------------------------------------------------------------

// Windows supports the graphics format known as metafile which is, though not
// portable, is widely used under Windows and so is supported by wxWin (under
// Windows only, of course). Win16 (Win3.1) used the so-called "Window
// MetaFiles" or WMFs which were replaced with "Enhanced MetaFiles" or EMFs in
// Win32 (Win9x, NT, 2000). Both of these are supported in wxWin and, by
// default, WMFs will be used under Win16 and EMFs under Win32. This may be
// changed by setting wxUSE_WIN_METAFILES_ALWAYS to 1 and/or setting
// wxUSE_ENH_METAFILE to 0. You may also set wxUSE_METAFILE to 0 to not compile
// in any metafile related classes at all.
//
// Default is 1 for wxUSE_ENH_METAFILE and 0 for wxUSE_WIN_METAFILES_ALWAYS.
//
// Recommended setting: default or 0 for everything for portable programs.
#define wxUSE_METAFILE              1
#define wxUSE_ENH_METAFILE          1
#define wxUSE_WIN_METAFILES_ALWAYS  0

// ----------------------------------------------------------------------------
// Big GUI components
// ----------------------------------------------------------------------------

// Set to 0 to disable MDI support.
//
// Requires wxUSE_NOTEBOOK under platforms other than MSW.
//
// Default is 1.
//
// Recommended setting: 1, can be safely set to 0.
#define wxUSE_MDI 1

// Set to 0 to disable document/view architecture
#define wxUSE_DOC_VIEW_ARCHITECTURE 1

// Set to 0 to disable MDI document/view architecture
//
// Requires wxUSE_MDI && wxUSE_DOC_VIEW_ARCHITECTURE
#define wxUSE_MDI_ARCHITECTURE    1

// Set to 0 to disable print/preview architecture code
#define wxUSE_PRINTING_ARCHITECTURE  1

// wxHTML sublibrary allows to display HTML in wxWindow programs and much,
// much more.
//
// Default is 1.
//
// Recommended setting: 1 (wxHTML is great!), set to 0 if you want compile a
// smaller library.
#define wxUSE_HTML          1

// Setting wxUSE_GLCANVAS to 1 enables OpenGL support. You need to have OpenGL
// headers and libraries to be able to compile the library with wxUSE_GLCANVAS
// set to 1. Note that for some compilers (notably Microsoft Visual C++) you
// will need to manually add opengl32.lib and glu32.lib to the list of
// libraries linked with your program if you use OpenGL.
//
// Default is 0.
//
// Recommended setting: 1 if you intend to use OpenGL, 0 otherwise
#define wxUSE_GLCANVAS       1

// wxRichTextCtrl allows editing of styled text.
//
// Default is 1.
//
// Recommended setting: 1, set to 0 if you want compile a
// smaller library.
#define wxUSE_RICHTEXT       1

// ----------------------------------------------------------------------------
// Data transfer
// ----------------------------------------------------------------------------

// Use wxClipboard class for clipboard copy/paste.
//
// Default is 1.
//
// Recommended setting: 1
#define wxUSE_CLIPBOARD     1

// Use wxDataObject and related classes. Needed for clipboard and OLE drag and
// drop
//
// Default is 1.
//
// Recommended setting: 1
#define wxUSE_DATAOBJ       1

// Use wxDropTarget and wxDropSource classes for drag and drop (this is
// different from "built in" drag and drop in wxTreeCtrl which is always
// available). Requires wxUSE_DATAOBJ.
//
// Default is 1.
//
// Recommended setting: 1
#define wxUSE_DRAG_AND_DROP 1

// Use wxAccessible for enhanced and customisable accessibility.
// Depends on wxUSE_OLE.
//
// Default is 0.
//
// Recommended setting (at present): 0
#define wxUSE_ACCESSIBILITY 0

// ----------------------------------------------------------------------------
// miscellaneous settings
// ----------------------------------------------------------------------------

// wxSingleInstanceChecker class allows to verify at startup if another program
// instance is running.
//
// Default is 1
//
// Recommended setting: 1 (the class is tiny, disabling it won't save much
// space)
#define wxUSE_SNGLINST_CHECKER  1

#define wxUSE_DRAGIMAGE 1

#define wxUSE_IPC         1
                                // 0 for no interprocess comms
#define wxUSE_HELP        1
                                // 0 for no help facility

// Should we use MS HTML help for wxHelpController? If disabled, neither
// wxCHMHelpController nor wxBestHelpController are available.
//
// Default is 1 under MSW, 0 is always used for the other platforms.
//
// Recommended setting: 1, only set to 0 if you have trouble compiling
// wxCHMHelpController (could be a problem with really ancient compilers)
#define wxUSE_MS_HTML_HELP 1


// Use wxHTML-based help controller?
#define wxUSE_WXHTML_HELP 1

#define wxUSE_RESOURCES   0
                                // 0 for no wxGetResource/wxWriteResource
#define wxUSE_CONSTRAINTS 1
                                // 0 for no window layout constraint system

#define wxUSE_SPLINES     1
                                // 0 for no splines

#define wxUSE_MOUSEWHEEL        1
                                // Include mouse wheel support

// ----------------------------------------------------------------------------
// postscript support settings
// ----------------------------------------------------------------------------

// Set to 1 for PostScript device context.
#define wxUSE_POSTSCRIPT  0

// Set to 1 to use font metric files in GetTextExtent
#define wxUSE_AFM_FOR_POSTSCRIPT 1

// ----------------------------------------------------------------------------
// database classes
// ----------------------------------------------------------------------------

// Define 1 to use ODBC classes
#define wxUSE_ODBC          0

// For backward compatibility reasons, this parameter now only controls the
// default scrolling method used by cursors.  This default behavior can be
// overriden by setting the second param of wxDB::wxDbGetConnection() or
// wxDb() constructor to indicate whether the connection (and any wxDbTable()s
// that use the connection) should support forward only scrolling of cursors,
// or both forward and backward support for backward scrolling cursors is
// dependent on the data source as well as the ODBC driver being used.
#define wxODBC_FWD_ONLY_CURSORS	 1

// Default is 0.  Set to 1 to use the deprecated classes, enum types, function,
// member variables.  With a setting of 1, full backward compatibility with the
// 2.0.x release is possible. It is STRONGLY recommended that this be set to 0,
// as future development will be done only on the non-deprecated
// functions/classes/member variables/etc.
#define wxODBC_BACKWARD_COMPATABILITY 0

// ----------------------------------------------------------------------------
// other compiler (mis)features
// ----------------------------------------------------------------------------

// Set this to 0 if your compiler can't cope with omission of prototype
// parameters.
//
// Default is 1.
//
// Recommended setting: 1 (should never need to set this to 0)
#define REMOVE_UNUSED_ARG   1

// VC++ 4.2 and above allows <iostream> and <iostream.h> but you can't mix
// them. Set to 1 for <iostream.h>, 0 for <iostream>. Note that VC++ 7.1
// and later doesn't support wxUSE_IOSTREAMH == 1 and so <iostream> will be
// used anyhow.
//
// Default is 1.
//
// Recommended setting: whatever your compiler likes more
#define wxUSE_IOSTREAMH     1

// ----------------------------------------------------------------------------
// image format support
// ----------------------------------------------------------------------------

// wxImage supports many different image formats which can be configured at
// compile-time. BMP is always supported, others are optional and can be safely
// disabled if you don't plan to use images in such format sometimes saving
// substantial amount of code in the final library.
//
// Some formats require an extra library which is included in wxWin sources
// which is mentioned if it is the case.

// Set to 1 for wxImage support (recommended).
#define wxUSE_IMAGE         1

// Set to 1 for PNG format support (requires libpng). Also requires wxUSE_ZLIB.
#define wxUSE_LIBPNG        1

// Set to 1 for JPEG format support (requires libjpeg)
#define wxUSE_LIBJPEG       1

// Set to 1 for TIFF format support (requires libtiff)
#define wxUSE_LIBTIFF       1

// Set to 1 for TGA format support (loading only)
#define wxUSE_TGA           1

// Set to 1 for GIF format support
#define wxUSE_GIF           1

// Set to 1 for PNM format support
#define wxUSE_PNM           1

// Set to 1 for PCX format support
#define wxUSE_PCX           1

// Set to 1 for IFF format support (Amiga format)
#define wxUSE_IFF           0

// Set to 1 for XPM format support
#define wxUSE_XPM           1

// Set to 1 for MS Icons and Cursors format support
#define wxUSE_ICO_CUR       1

// Set to 1 to compile in wxPalette class
#define wxUSE_PALETTE       1

// ----------------------------------------------------------------------------
// wxUniversal-only options
// ----------------------------------------------------------------------------

// Set to 1 to enable compilation of all themes, this is the default
#define wxUSE_ALL_THEMES    1

// Set to 1 to enable the compilation of individual theme if wxUSE_ALL_THEMES
// is unset, if it is set these options are not used; notice that metal theme
// uses Win32 one
#define wxUSE_THEME_GTK     0
#define wxUSE_THEME_METAL   0
#define wxUSE_THEME_MONO    0
#define wxUSE_THEME_WIN32   0


/* --- end common options --- */

// ----------------------------------------------------------------------------
// Windows-only settings
// ----------------------------------------------------------------------------

// Set wxUSE_UNICODE_MSLU to 1 if you're compiling wxWidgets in Unicode mode
// and want to run your programs under Windows 9x and not only NT/2000/XP.
// This setting enables use of unicows.dll from MSLU (MS Layer for Unicode, see
// http://www.microsoft.com/globaldev/handson/dev/mslu_announce.mspx). Note
// that you will have to modify the makefiles to include unicows.lib import
// library as the first library (see installation instructions in install.txt
// to learn how to do it when building the library or samples).
//
// If your compiler doesn't have unicows.lib, you can get a version of it at
// http://libunicows.sourceforge.net
//
// Default is 0
//
// Recommended setting: 0 (1 if you want to deploy Unicode apps on 9x systems)
#ifndef wxUSE_UNICODE_MSLU
    #define wxUSE_UNICODE_MSLU 0
#endif

// Set this to 1 if you want to use wxWidgets and MFC in the same program. This
// will override some other settings (see below)
//
// Default is 0.
//
// Recommended setting: 0 unless you really have to use MFC
#define wxUSE_MFC           0

// Set this to 1 for generic OLE support: this is required for drag-and-drop,
// clipboard, OLE Automation. Only set it to 0 if your compiler is very old and
// can't compile/doesn't have the OLE headers.
//
// Default is 1.
//
// Recommended setting: 1
#define wxUSE_OLE           1

// Set this to 1 to enable wxAutomationObject class.
//
// Default is 1.
//
// Recommended setting: 1 if you need to control other applications via OLE
// Automation, can be safely set to 0 otherwise
#define wxUSE_OLE_AUTOMATION 1

// Set this to 1 to enable wxActiveXContainer class allowing to embed OLE
// controls in wx.
//
// Default is 1.
//
// Recommended setting: 1, required by wxMediaCtrl
#define wxUSE_ACTIVEX 1

// wxDC cacheing implementation
#define wxUSE_DC_CACHEING 1

// Set this to 1 to enable the use of DIB's for wxBitmap to support
// bitmaps > 16MB on Win95/98/Me.  Set to 0 to use DDB's only.
#define wxUSE_DIB_FOR_BITMAP 0

// Set this to 1 to enable wxDIB class used internally for manipulating
// wxBitmao data.
//
// Default is 1, set it to 0 only if you don't use wxImage neither
//
// Recommended setting: 1 (without it conversion to/from wxImage won't work)
#define wxUSE_WXDIB 1

// Set to 0 to disable PostScript print/preview architecture code under Windows
// (just use Windows printing).
#define wxUSE_POSTSCRIPT_ARCHITECTURE_IN_MSW 1

// Set this to 1 to use RICHEDIT controls for wxTextCtrl with style wxTE_RICH
// which allows to put more than ~32Kb of text in it even under Win9x (NT
// doesn't have such limitation).
//
// Default is 1 for compilers which support it
//
// Recommended setting: 1, only set it to 0 if your compiler doesn't have
//                      or can't compile <richedit.h>
#define wxUSE_RICHEDIT  1

// Set this to 1 to use extra features of richedit v2 and later controls
//
// Default is 1 for compilers which support it
//
// Recommended setting: 1
#define wxUSE_RICHEDIT2 1

// Set this to 1 to enable support for the owner-drawn menu and listboxes. This
// is required by wxUSE_CHECKLISTBOX.
//
// Default is 1.
//
// Recommended setting: 1, set to 0 for a small library size reduction
#define wxUSE_OWNER_DRAWN 1

// Set to 1 to compile MS Windows XP theme engine support
#define wxUSE_UXTHEME           1

// Set to 1 to auto-adapt to MS Windows XP themes where possible
// (notably, wxNotebook pages)
#define wxUSE_UXTHEME_AUTO      1

// Set to 1 to use InkEdit control (Tablet PC), if available
#define wxUSE_INKEDIT  0

// ----------------------------------------------------------------------------
// Generic versions of native controls
// ----------------------------------------------------------------------------

// Set this to 1 to be able to use wxDatePickerCtrlGeneric in addition to the
// native wxDatePickerCtrl
//
// Default is 0.
//
// Recommended setting: 0, this is mainly used for testing
#define wxUSE_DATEPICKCTRL_GENERIC 0

// ----------------------------------------------------------------------------
// Crash debugging helpers
// ----------------------------------------------------------------------------

// Set this to 1 to be able to use wxCrashReport::Generate() to create mini
// dumps of your program when it crashes (or at any other moment)
//
// Default is 1 if supported by the compiler (VC++ and recent BC++ only).
//
// Recommended setting: 1, set to 0 if your programs never crash
#define wxUSE_CRASHREPORT 1

// ----------------------------------------------------------------------------
// obsolete settings
// ----------------------------------------------------------------------------

// NB: all settings in this section are obsolete and should not be used/changed
//     at all, they will disappear

// Define 1 to use bitmap messages.
#define wxUSE_BITMAP_MESSAGE         1

#endif // _WX_SETUP_H_

