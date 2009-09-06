/////////////////////////////////////////////////////////////////////////////
// Name:        wx/msw/setup.h
// Purpose:     Configuration for the library
// Author:      Julian Smart
// Modified by:
// Created:     01/02/97
// RCS-ID:      $Id: setup_microwin.h 40766 2006-08-23 09:54:29Z VS $
// Copyright:   (c) Julian Smart
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef _WX_SETUP_H_
#define _WX_SETUP_H_

// ----------------------------------------------------------------------------
// global settings
// ----------------------------------------------------------------------------

//#define WXWIN_OS_DESCRIPTION wxT("MicroWindows")

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

// Set to 0 for accurate dialog units, else 1 to be as per 2.1.16 and before.
// If migrating between versions, your dialogs may seem to shrink.
//
// Default is 1
//
// Recommended setting: 0 (the new calculations are more correct!)
#define wxDIALOG_UNIT_COMPATIBILITY   1

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
// __WXDEBUG__ is not defined, will still use normal memory operators. It's
// recommended to set this to 1, since you may well need to output an error log
// in a production version (or non-debugging beta).
//
// Default is 1.
//
// Recommended setting: 1 but see comment above
#define wxUSE_DEBUG_CONTEXT 1

// If 1, enables debugging versions of wxObject::new and wxObject::delete *IF*
// __WXDEBUG__ is also defined.
//
// WARNING: this code may not work with all architectures, especially if
// alignment is an issue. This switch is currently ignored for mingw / cygwin
//
// Default is 1
//
// Recommended setting: 1 but see comment in the beginning of this section
#define wxUSE_MEMORY_TRACING 0

// In debug mode, cause new and delete to be redefined globally.
// If this causes problems (e.g. link errors), set this to 0.
// This switch is currently ignored for mingw / cygwin
//
// Default is 1
//
// Recommended setting: 1 but see comment in the beginning of this section
#define wxUSE_GLOBAL_MEMORY_OPERATORS 0

// In debug mode, causes new to be defined to be WXDEBUG_NEW (see object.h). If
// this causes problems (e.g. link errors), set this to 0. You may need to set
// this to 0 if using templates (at least for VC++). This switch is currently
// ignored for mingw / cygwin
//
// Default is 1
//
// Recommended setting: 1 but see comment in the beginning of this section
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
#ifdef _MSC_VER
    #define wxUSE_ON_FATAL_EXCEPTION 1
#else
    #define wxUSE_ON_FATAL_EXCEPTION 0
#endif

// ----------------------------------------------------------------------------
// Unicode support
// ----------------------------------------------------------------------------

// Set wxUSE_UNICODE to 1 to compile wxWidgets in Unicode mode: wxChar will be
// defined as wchar_t, wxString will use Unicode internally. If you set this
// to 1, you must use wxT() macro for all literal strings in the program.
//
// Unicode is currently only fully supported under Windows NT/2000 (Windows 9x
// doesn't support it and the programs compiled in Unicode mode will not run
// under 9x).
//
// Default is 0
//
// Recommended setting: 0 (unless you only plan to use Windows NT/2000)
#define wxUSE_UNICODE 0

// Set wxUSE_UNICODE_MSLU to 1 if you want to compile wxWidgets in Unicode mode
// and be able to run compiled apps under Windows 9x as well as NT/2000/XP. This
// setting enables use of unicows.dll from MSLU (MS Layer for Unicode, see
// http://www.microsoft.com/globaldev/Articles/mslu_announce.asp). Note that you
// will have to modify the makefiles to include unicows.lib import library as the first
// library.
//
// Default is 0
//
// Recommended setting: 0
#define wxUSE_UNICODE_MSLU 0

// Setting wxUSE_WCHAR_T to 1 gives you some degree of Unicode support without
// compiling the program in Unicode mode. More precisely, it will be possible
// to construct wxString from a wide (Unicode) string and convert any wxString
// to Unicode.
//
// Default is 1
//
// Recommended setting: 1
#define wxUSE_WCHAR_T 0

// ----------------------------------------------------------------------------
// global features
// ----------------------------------------------------------------------------

// Support for message/error logging. This includes wxLogXXX() functions and
// wxLog and derived classes. Don't set this to 0 unless you really know what
// you are doing.
//
// Default is 1
//
// Recommended setting: 1 (always)
#define wxUSE_LOG 1

// Support for command line parsing using wxCmdLineParser class.
//
// Default is 1
//
// Recommended setting: 1 (can be set to 0 if you don't use the cmd line)
#define wxUSE_CMDLINE_PARSER 1

// Recommended setting: 1
#define wxUSE_LOGWINDOW 1

// Recommended setting: 1
#define wxUSE_LOGGUI 1

// Recommended setting: 1
#define wxUSE_LOG_DIALOG 0

// Support for multithreaded applications: if 1, compile in thread classes
// (thread.h) and make the library a bit more thread safe. Although thread
// support is quite stable by now, you may still consider recompiling the
// library without it if you have no use for it - this will result in a
// somewhat smaller and faster operation.
//
// This is ignored under Win16, threads are only supported under Win32.
//
// Default is 1
//
// Recommended setting: 0 unless you do plan to develop MT applications
#define wxUSE_THREADS 0

// If enabled (1), compiles wxWidgets streams classes
#define wxUSE_STREAMS       1

// Use standard C++ streams if 1. If 0, use wxWin streams implementation.
#define wxUSE_STD_IOSTREAM  0

// Use serialization (requires utils/serialize)
#define wxUSE_SERIAL        0

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
#define wxUSE_CONFIG_NATIVE   0

// If wxUSE_DIALUP_MANAGER is 1, compile in wxDialUpManager class which allows
// to connect/disconnect from the network and be notified whenever the dial-up
// network connection is established/terminated. Requires wxUSE_DYNLIB_CLASS.
//
// Default is 1.
//
// Recommended setting: 1
#define wxUSE_DIALUP_MANAGER   0

// Compile in wxLibrary class for run-time DLL loading and function calling.
// Required by wxUSE_DIALUP_MANAGER.
//
// This setting is for Win32 only
//
// Default is 1.
//
// Recommended setting: 1
#define wxUSE_DYNAMIC_LOADER  0

#define wxUSE_DYNLIB_CLASS  0

// Set to 1 to use socket classes
#define wxUSE_SOCKETS       0

// Set to 1 to enable virtual file systems (required by wxHTML)
#define wxUSE_FILESYSTEM    0

// Set to 1 to enable virtual ZIP filesystem (requires wxUSE_FILESYSTEM)
#define wxUSE_FS_ZIP        0

// Set to 1 to enable virtual Internet filesystem (requires wxUSE_FILESYSTEM)
#define wxUSE_FS_INET       0

// Set to 1 to compile wxZipInput/OutputStream classes.
#define wxUSE_ZIPSTREAM     0

// Set to 1 to compile wxZlibInput/OutputStream classes. Also required by
// wxUSE_LIBPNG.
#define wxUSE_ZLIB          0

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
#define wxUSE_MIMETYPE 0

// wxSystemOptions class
#define wxUSE_SYSTEM_OPTIONS 1

// Support for regular expression matching via wxRegEx class: enable this to
// use POSIX regular expressions in your code. You need to compile regex
// library from src/regex to use it under Windows.
//
// Default is 0
//
// Recommended setting: 1 if your compiler supports it, if it doesn't please
// contribute us a makefile for src/regex for it
#define wxUSE_REGEX       0

// wxSound class
#define wxUSE_SOUND      0

// Use wxMediaCtrl
//
// Default is 1.
//
// Recommended setting: 1 
#define wxUSE_MEDIACTRL     1

// Use QuickTime
//
// Default is 0
//
// Recommended setting: 1 if you have the QT SDK installed and you need it, else 0
#define wxUSE_QUICKTIME     0

// Use DirectShow
//
// Default is 0
//
// Recommended setting: 1 if the DirectX 7 SDK is installed (highly recommended), else 0
#define wxUSE_DIRECTSHOW    1

// Use wxWidget's XRC XML-based resource system.  Recommended.
//
// Default is 1
//
// Recommended setting: 1 (requires wxUSE_XML)
#define wxUSE_XRC       1

// XML parsing classes. Note that their API will change in the future, so
// using wxXmlDocument and wxXmlNode in your app is not recommended.
//
// Default is 1
//
// Recommended setting: 1 (required by XRC)
#if wxUSE_XRC
#  define wxUSE_XML       1
#else
#  define wxUSE_XML       0
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

// wxPopupWindow class is not used currently by wxMSW
//
// Default is 0
//
// Recommended setting: 0
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
#define wxUSE_BUTTON       1    // wxButton
#define wxUSE_BMPBUTTON    1    // wxBitmapButton
#define wxUSE_CALENDARCTRL 0    // wxCalendarCtrl
#define wxUSE_CHECKBOX     1    // wxCheckBox
#define wxUSE_CHECKLISTBOX 1    // wxCheckListBox (requires wxUSE_OWNER_DRAWN)
#define wxUSE_CHOICE       1    // wxChoice
#define wxUSE_COMBOBOX     1    // wxComboBox
#define wxUSE_GAUGE        1    // wxGauge
#define wxUSE_LISTBOX      1    // wxListBox
#define wxUSE_LISTCTRL     0    // wxListCtrl
#define wxUSE_RADIOBOX     1    // wxRadioBox
#define wxUSE_RADIOBTN     1    // wxRadioButton
#define wxUSE_SCROLLBAR    1    // wxScrollBar
#define wxUSE_SLIDER       1    // wxSlider
#define wxUSE_SPINBTN      1    // wxSpinButton
#define wxUSE_SPINCTRL     1    // wxSpinCtrl
#define wxUSE_STATBOX      1    // wxStaticBox
#define wxUSE_STATLINE     1    // wxStaticLine
#define wxUSE_STATTEXT     1    // wxStaticText
#define wxUSE_STATBMP      1    // wxStaticBitmap
#define wxUSE_TEXTCTRL     1    // wxTextCtrl
#define wxUSE_TOGGLEBTN    0    // requires wxButton
#define wxUSE_TREECTRL     0    // wxTreeCtrl

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
// Default is 1.
//
// Recommended setting: 1 (there is no advantage in using the generic one)
#define wxUSE_NATIVE_STATUSBAR        0

// wxToolBar related settings: if wxUSE_TOOLBAR is 0, don't compile any toolbar
// classes at all. Otherwise, use the native toolbar class unless
// wxUSE_TOOLBAR_NATIVE is 0.
//
// Default is 1 for all settings.
//
// Recommended setting: 1 for wxUSE_TOOLBAR and wxUSE_TOOLBAR_NATIVE.
#define wxUSE_TOOLBAR 0
#define wxUSE_TOOLBAR_NATIVE 0

// wxNotebook is a control with several "tabs" located on one of its sides. It
// may be used ot logically organise the data presented to the user instead of
// putting everything in one huge dialog. It replaces wxTabControl and related
// classes of wxWin 1.6x.
//
// Default is 1.
//
// Recommended setting: 1
#define wxUSE_NOTEBOOK 1

// wxTabDialog is a generic version of wxNotebook but it is incompatible with
// the new class. It shouldn't be used in new code.
//
// Default is 0.
//
// Recommended setting: 0 (use wxNotebook)
#define wxUSE_TAB_DIALOG    0

// wxGrid class
// Default is 1
//
#define wxUSE_GRID         0

// wxProperty[Value/Form/List] classes, used by Dialog Editor
#define wxUSE_PROPSHEET    0

// ----------------------------------------------------------------------------
// Miscellaneous GUI stuff
// ----------------------------------------------------------------------------

// wxAcceleratorTable/Entry classes and support for them in wxMenu(Bar)
#define wxUSE_ACCEL 0

// Use wxCaret: a class implementing a "cursor" in a text control (called caret
// under Windows).
//
// Default is 1.
//
// Recommended setting: 1 (can be safely set to 0, not used by the library)
#define wxUSE_CARET         1

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
#define wxUSE_TOOLTIPS      0

// wxValidator class and related methods
#define wxUSE_VALIDATORS 0

// wxDC cacheing implementation
#define wxUSE_DC_CACHEING 0

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
#define wxUSE_BUSYINFO      0

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
#define wxUSE_COLOURDLG     0

// wxDirDlg class for getting a directory name from user
#define wxUSE_DIRDLG 0

// TODO: setting to choose the generic or native one

// Use file open/save dialogs.
//
// Default is 1
//
// Recommended setting: 1 (used in many places in the library itself)
#define wxUSE_FILEDLG       0

// Use find/replace dialogs.
//
// Default is 1
//
// Recommended setting: 1 (but may be safely set to 0)
#define wxUSE_FINDREPLDLG       0

// Use font picker dialog
//
// Default is 1
//
// Recommended setting: 1 (used in the library itself)
#define wxUSE_FONTDLG       0

// Use wxMessageDialog and wxMessageBox.
//
// Default is 1
//
// Recommended setting: 1 (used in the library itself)
#define wxUSE_MSGDLG        1

// progress dialog class for lengthy operations
#define wxUSE_PROGRESSDLG 0

// support for startup tips (wxShowTip &c)
#define wxUSE_STARTUP_TIPS 0

// text entry dialog and wxGetTextFromUser function
#define wxUSE_TEXTDLG 0

// number entry dialog
#define wxUSE_NUMBERDLG 0

// splash screen class
#define wxUSE_SPLASH 0

// wizards
#define wxUSE_WIZARDDLG 0

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
#define wxUSE_METAFILE              0
#define wxUSE_ENH_METAFILE          0
#define wxUSE_WIN_METAFILES_ALWAYS  0

// ----------------------------------------------------------------------------
// Big GUI components
// ----------------------------------------------------------------------------

// Set to 0 to disable document/view architecture
#define wxUSE_DOC_VIEW_ARCHITECTURE 0

// Set to 0 to disable MDI document/view architecture
#define wxUSE_MDI_ARCHITECTURE    0

// Set to 0 to disable print/preview architecture code
#define wxUSE_PRINTING_ARCHITECTURE  0

// wxHTML sublibrary allows to display HTML in wxWindow programs and much,
// much more.
//
// Default is 1.
//
// Recommended setting: 1 (wxHTML is great!), set to 0 if you want compile a
// smaller library.
#define wxUSE_HTML          0

// OpenGL canvas
#define wxUSE_GLCANVAS       0

// wxTreeLayout class
#define wxUSE_TREELAYOUT     0

// ----------------------------------------------------------------------------
// Data transfer
// ----------------------------------------------------------------------------

// Use wxClipboard class for clipboard copy/paste.
//
// Default is 1.
//
// Recommended setting: 1
#define wxUSE_CLIPBOARD     0

// Use wxDataObject and related classes. Needed for clipboard and OLE drag and
// drop
//
// Default is 1.
//
// Recommended setting: 1
#define wxUSE_DATAOBJ       0

// Use wxDropTarget and wxDropSource classes for drag and drop (this is
// different from "built in" drag and drop in wxTreeCtrl which is always
// available). Requires wxUSE_DATAOBJ.
//
// Default is 1.
//
// Recommended setting: 1
#define wxUSE_DRAG_AND_DROP 0

// ----------------------------------------------------------------------------
// miscellaneous settings
// ----------------------------------------------------------------------------

// wxSingleInstanceChecker class allows to verify at startup if another program
// instance is running (it is only available under Win32)
//
// Default is 1
//
// Recommended setting: 1 (the class is tiny, disabling it won't save much
// space)
#define wxUSE_SNGLINST_CHECKER  0

#define wxUSE_DRAGIMAGE 0

#define wxUSE_IPC         0
                                // 0 for no interprocess comms
#define wxUSE_HELP        0
                                // 0 for no help facility
#define wxUSE_MS_HTML_HELP 0
                                // 0 for no MS HTML Help

// Use wxHTML-based help controller?
#define wxUSE_WXHTML_HELP 0

#define wxUSE_RESOURCES   0
                                // 0 for no wxGetResource/wxWriteResource
#define wxUSE_CONSTRAINTS 1
                                // 0 for no window layout constraint system

#define wxUSE_SPLINES     1
                                // 0 for no splines

#define wxUSE_XPM_IN_MSW   1
                                // 0 for no XPM support in wxBitmap.
                                // Default is 1, as XPM is now fully
                                // supported this makes easier the issue
                                // of portable icons and bitmaps.

#define wxUSE_IMAGE_LOADING_IN_MSW        0
                                // Use dynamic DIB loading/saving code in utils/dib under MSW.
#define wxUSE_RESOURCE_LOADING_IN_MSW     0
                                // Use dynamic icon/cursor loading/saving code
                                // under MSW.
#define wxUSE_WX_RESOURCES        0
                                // Use .wxr resource mechanism (requires PrologIO library)

#define wxUSE_MOUSEWHEEL        0
                                // Include mouse wheel support

// ----------------------------------------------------------------------------
// postscript support settings
// ----------------------------------------------------------------------------

// Set to 1 for PostScript device context.
#define wxUSE_POSTSCRIPT  0

// Set to 1 to use font metric files in GetTextExtent
#define wxUSE_AFM_FOR_POSTSCRIPT 0

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
// them. Set to 1 for <iostream.h>, 0 for <iostream>
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
#define wxUSE_LIBPNG        0

// Set to 1 for JPEG format support (requires libjpeg)
#define wxUSE_LIBJPEG       0

// Set to 1 for TIFF format support (requires libtiff)
#define wxUSE_LIBTIFF       0

// Set to 1 for GIF format support
#define wxUSE_GIF           0

// Set to 1 for PNM format support
#define wxUSE_PNM           0

// Set to 1 for PCX format support
#define wxUSE_PCX           0

// Set to 1 for IFF format support
#define wxUSE_IFF           0

// Set to 1 for XPM format support
#define wxUSE_XPM           1

// Set to 1 for MS Icons and Cursors format support
#define wxUSE_ICO_CUR       1

// Set to 1 to compile in wxPalette class
#define wxUSE_PALETTE       1

// ----------------------------------------------------------------------------
// Windows-only settings
// ----------------------------------------------------------------------------

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
#define wxUSE_OLE           0

// Set this to 1 to use Microsoft CTL3D library for "3D-look" under Win16 or NT
// 3.x. This setting is ignored under Win9x and NT 4.0+.
//
// Default is 0 for (most) Win32 (systems), 1 for Win16
//
// Recommended setting: same as default
#if defined(__WIN95__)
#define wxUSE_CTL3D                      0
#else
#define wxUSE_CTL3D                      1
#endif

// Set to 0 to disable PostScript print/preview architecture code under Windows
// (just use Windows printing).
#define wxUSE_POSTSCRIPT_ARCHITECTURE_IN_MSW 0

// Set this to 1 to use RICHEDIT controls for wxTextCtrl with style wxTE_RICH
// which allows to put more than ~32Kb of text in it even under Win9x (NT
// doesn't have such limitation).
//
// Default is 1 for compilers which support it
//
// Recommended setting: 1, only set it to 0 if your compiler doesn't have
//                      or can't compile <richedit.h>
#if defined(__WIN95__) && !defined(__WINE__) && !defined(__GNUWIN32_OLD__)
#define wxUSE_RICHEDIT  1

// TODO:  This should be ifdef'ed for any compilers that don't support
//        RichEdit 2.0 but do have RichEdit 1.0...
#define wxUSE_RICHEDIT2 1

#else
#define wxUSE_RICHEDIT  0
#define wxUSE_RICHEDIT2 0
#endif

// Set this to 1 to enable support for the owner-drawn menu and listboxes. This
// is required by wxUSE_CHECKLISTBOX.
//
// Default is 1.
//
// Recommended setting: 1, set to 0 for a small library size reduction
#define wxUSE_OWNER_DRAWN 0

// ----------------------------------------------------------------------------
// obsolete settings
// ----------------------------------------------------------------------------

// NB: all settings in this section are obsolete and should not be used/changed
//     at all, they will disappear

// Define 1 to use bitmap messages.
#define wxUSE_BITMAP_MESSAGE         1

// If 1, enables provision of run-time type information.
// NOW MANDATORY: don't change.
#define wxUSE_DYNAMIC_CLASSES     1

// ----------------------------------------------------------------------------
// disable the settings which don't work for some compilers
// ----------------------------------------------------------------------------

#ifndef wxUSE_NORLANDER_HEADERS
#if (defined(__MINGW32__) || defined(__CYGWIN__)) && ((__GNUC__>2) ||((__GNUC__==2) && (__GNUC_MINOR__>=95)))
#   define wxUSE_NORLANDER_HEADERS 1
#else
#   define wxUSE_NORLANDER_HEADERS 0
#endif
#endif

#if defined(__GNUWIN32__)
// These don't work as expected for mingw32 and cygwin32
#undef  wxUSE_MEMORY_TRACING
#define wxUSE_MEMORY_TRACING            0

#undef  wxUSE_GLOBAL_MEMORY_OPERATORS
#define wxUSE_GLOBAL_MEMORY_OPERATORS   0

#undef  wxUSE_DEBUG_NEW_ALWAYS
#define wxUSE_DEBUG_NEW_ALWAYS          0

// Cygwin betas don't have wcslen
#if defined(__CYGWIN__) || defined(__CYGWIN32__)
#  if ! ((__GNUC__>2) ||((__GNUC__==2) && (__GNUC_MINOR__>=95)))
#    undef wxUSE_WCHAR_T
#    define wxUSE_WCHAR_T 0
#  endif
#endif

#endif // __GNUWIN32__

// MFC duplicates these operators
#if wxUSE_MFC
#undef  wxUSE_GLOBAL_MEMORY_OPERATORS
#define wxUSE_GLOBAL_MEMORY_OPERATORS   0

#undef  wxUSE_DEBUG_NEW_ALWAYS
#define wxUSE_DEBUG_NEW_ALWAYS          0
#endif // wxUSE_MFC

#if (!defined(WIN32) && !defined(__WIN32__)) || (defined(__GNUWIN32__) && !wxUSE_NORLANDER_HEADERS)
// Can't use OLE drag and drop in Windows 3.1 because we don't know how
// to implement UUIDs
// GnuWin32 doesn't have appropriate headers for e.g. IUnknown.
#undef wxUSE_DRAG_AND_DROP
#define wxUSE_DRAG_AND_DROP 0
#endif

// Only WIN32 supports wxStatusBar95
#if !defined(__WIN32__) && wxUSE_NATIVE_STATUSBAR
#undef  wxUSE_NATIVE_STATUSBAR
#define wxUSE_NATIVE_STATUSBAR 0
#endif

#if !wxUSE_OWNER_DRAWN
#undef wxUSE_CHECKLISTBOX
#define wxUSE_CHECKLISTBOX 0
#endif

// Salford C++ doesn't like some of the memory operator definitions
#ifdef __SALFORDC__
#undef  wxUSE_MEMORY_TRACING
#define wxUSE_MEMORY_TRACING      0

#undef wxUSE_GLOBAL_MEMORY_OPERATORS
#define wxUSE_GLOBAL_MEMORY_OPERATORS 0

#undef wxUSE_DEBUG_NEW_ALWAYS
#define wxUSE_DEBUG_NEW_ALWAYS 0

#undef wxUSE_THREADS
#define wxUSE_THREADS 0

#undef wxUSE_OWNER_DRAWN
#define wxUSE_OWNER_DRAWN 0
#endif // __SALFORDC__

// BC++/Win16 can't cope with the amount of data in resource.cpp
#if defined(__WIN16__) && defined(__BORLANDC__)
#undef wxUSE_WX_RESOURCES
#define wxUSE_WX_RESOURCES        0

#undef wxUSE_ODBC
#define wxUSE_ODBC                0

#undef wxUSE_NEW_GRID
#define wxUSE_NEW_GRID            0
#endif

#if defined(__BORLANDC__) && (__BORLANDC__ < 0x500)
// BC++ 4.0 can't compile JPEG library
#undef wxUSE_LIBJPEG
#define wxUSE_LIBJPEG 0
#endif

// wxUSE_DEBUG_NEW_ALWAYS = 1 not compatible with BC++ in DLL mode
#if defined(__BORLANDC__) && (defined(WXMAKINGDLL) || defined(WXUSINGDLL))
#undef wxUSE_DEBUG_NEW_ALWAYS
#define wxUSE_DEBUG_NEW_ALWAYS 0
#endif

#if defined(__WXMSW__) && defined(__WATCOMC__)
/*
#undef  wxUSE_GLCANVAS
#define wxUSE_GLCANVAS 0
*/

#undef wxUSE_WCHAR_T
#define wxUSE_WCHAR_T 0
#endif

#if defined(__WXMSW__) && !defined(__WIN32__)

#undef wxUSE_SOCKETS
#define wxUSE_SOCKETS 0

#undef wxUSE_THREADS
#define wxUSE_THREADS 0

#undef wxUSE_TOOLTIPS
#define wxUSE_TOOLTIPS 0

#undef wxUSE_SPINCTRL
#define wxUSE_SPINCTRL 0

#undef wxUSE_SPINBTN
#define wxUSE_SPINBTN 0

#undef wxUSE_LIBPNG
#define wxUSE_LIBPNG 0

#undef wxUSE_LIBJPEG
#define wxUSE_LIBJPEG 0

#undef wxUSE_LIBTIFF
#define wxUSE_LIBTIFF 0

#undef wxUSE_GIF
#define wxUSE_GIF 0

#undef wxUSE_PNM
#define wxUSE_PNM 0

#undef wxUSE_PCX
#define wxUSE_PCX 0

#undef wxUSE_GLCANVAS
#define wxUSE_GLCANVAS 0

#undef wxUSE_MS_HTML_HELP
#define wxUSE_MS_HTML_HELP 0

#undef wxUSE_WCHAR_T
#define wxUSE_WCHAR_T 0

#endif // Win16

// ----------------------------------------------------------------------------
// undef the things which don't make sense for wxBase build
// ----------------------------------------------------------------------------

#if !wxUSE_GUI

#undef wxUSE_HTML
#define wxUSE_HTML 0

#endif // !wxUSE_GUI

// ----------------------------------------------------------------------------
// check the settings consistency: do it here to abort compilation immediately
// and not almost in the very end when the relevant file fails to compile and
// you need to modify setup.h and rebuild everything
// ----------------------------------------------------------------------------

#if wxUSE_DATETIME && !wxUSE_LONGLONG
    #error wxDateTime requires wxLongLong
#endif

#if wxUSE_TEXTFILE && !wxUSE_FILE
    #error You cannot compile wxTextFile without wxFile
#endif

#if wxUSE_FILESYSTEM && !wxUSE_STREAMS
    #error You cannot compile virtual file systems without wxUSE_STREAMS
#endif

#if wxUSE_HTML && !wxUSE_FILESYSTEM
    #error You cannot compile wxHTML without virtual file systems
#endif

// add more tests here...

#endif
    // _WX_SETUP_H_
