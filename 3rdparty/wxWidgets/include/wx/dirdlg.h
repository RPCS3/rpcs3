/////////////////////////////////////////////////////////////////////////////
// Name:        wx/dirdlg.h
// Purpose:     wxDirDialog base class
// Author:      Robert Roebling
// Modified by:
// Created:
// Copyright:   (c) Robert Roebling
// RCS-ID:      $Id: dirdlg.h 44027 2006-12-21 19:26:48Z VZ $
// Licence:     wxWindows Licence
/////////////////////////////////////////////////////////////////////////////

#ifndef _WX_DIRDLG_H_BASE_
#define _WX_DIRDLG_H_BASE_

#if wxUSE_DIRDLG

#include "wx/dialog.h"

// ----------------------------------------------------------------------------
// constants
// ----------------------------------------------------------------------------

extern WXDLLEXPORT_DATA(const wxChar) wxDirDialogNameStr[];
extern WXDLLEXPORT_DATA(const wxChar) wxDirDialogDefaultFolderStr[];
extern WXDLLEXPORT_DATA(const wxChar) wxDirSelectorPromptStr[];

#define wxDD_CHANGE_DIR         0x0100
#define wxDD_DIR_MUST_EXIST     0x0200

// deprecated, on by default now, use wxDD_DIR_MUST_EXIST to disable it
#define wxDD_NEW_DIR_BUTTON     0

#ifdef __WXWINCE__
    #define wxDD_DEFAULT_STYLE      wxDEFAULT_DIALOG_STYLE
#else
    #define wxDD_DEFAULT_STYLE      (wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER)
#endif

//-------------------------------------------------------------------------
// wxDirDialogBase
//-------------------------------------------------------------------------

class WXDLLEXPORT wxDirDialogBase : public wxDialog
{
public:
    wxDirDialogBase() {}
    wxDirDialogBase(wxWindow *parent,
                    const wxString& title = wxDirSelectorPromptStr,
                    const wxString& defaultPath = wxEmptyString,
                    long style = wxDD_DEFAULT_STYLE,
                    const wxPoint& pos = wxDefaultPosition,
                    const wxSize& sz = wxDefaultSize,
                    const wxString& name = wxDirDialogNameStr)
    {
        Create(parent, title, defaultPath, style, pos, sz, name);
    }

    virtual ~wxDirDialogBase() {}


    bool Create(wxWindow *parent,
                const wxString& title = wxDirSelectorPromptStr,
                const wxString& defaultPath = wxEmptyString,
                long style = wxDD_DEFAULT_STYLE,
                const wxPoint& pos = wxDefaultPosition,
                const wxSize& sz = wxDefaultSize,
                const wxString& name = wxDirDialogNameStr)
    {
        if (!wxDialog::Create(parent, wxID_ANY, title, pos, sz, style, name))
            return false;
        m_path = defaultPath;
        m_message = title;
        return true;
    }

#if WXWIN_COMPATIBILITY_2_6

    wxDEPRECATED( long GetStyle() const );
    wxDEPRECATED( void SetStyle(long style) );

#endif  // WXWIN_COMPATIBILITY_2_6

    virtual void SetMessage(const wxString& message) { m_message = message; }
    virtual void SetPath(const wxString& path) { m_path = path; }

    virtual wxString GetMessage() const { return m_message; }
    virtual wxString GetPath() const { return m_path; }

protected:
    wxString m_message;
    wxString m_path;
};


// Universal and non-port related switches with need for generic implementation
#if defined(__WXUNIVERSAL__)
    #include "wx/generic/dirdlgg.h"
    #define wxDirDialog wxGenericDirDialog
#elif defined(__WXMSW__) && (defined(__SALFORDC__)    || \
                             !wxUSE_OLE               || \
                             (defined (__GNUWIN32__) && !wxUSE_NORLANDER_HEADERS))
    #include "wx/generic/dirdlgg.h"
    #define wxDirDialog wxGenericDirDialog
#elif defined(__WXMSW__) && defined(__WXWINCE__) && !defined(__HANDHELDPC__)
    #include "wx/generic/dirdlgg.h"     // MS PocketPC or MS Smartphone
    #define wxDirDialog wxGenericDirDialog
#elif defined(__WXMSW__)
    #include "wx/msw/dirdlg.h"  // Native MSW
#elif defined(__WXGTK24__)
    #include "wx/gtk/dirdlg.h"  // Native GTK for gtk2.4
#elif defined(__WXGTK__)
    #include "wx/generic/dirdlgg.h"
    #define wxDirDialog wxGenericDirDialog
#elif defined(__WXMAC__)
    #include "wx/mac/dirdlg.h"      // Native Mac
#elif defined(__WXCOCOA__)
    #include "wx/cocoa/dirdlg.h"    // Native Cocoa
#elif defined(__WXMOTIF__) || \
      defined(__WXX11__)   || \
      defined(__WXMGL__)   || \
      defined(__WXCOCOA__) || \
      defined(__WXPM__)
    #include "wx/generic/dirdlgg.h"     // Other ports use generic implementation
    #define wxDirDialog wxGenericDirDialog
#endif

// ----------------------------------------------------------------------------
// common ::wxDirSelector() function
// ----------------------------------------------------------------------------

WXDLLEXPORT wxString
wxDirSelector(const wxString& message = wxDirSelectorPromptStr,
              const wxString& defaultPath = wxEmptyString,
              long style = wxDD_DEFAULT_STYLE,
              const wxPoint& pos = wxDefaultPosition,
              wxWindow *parent = NULL);

#endif // wxUSE_DIRDLG

#endif
    // _WX_DIRDLG_H_BASE_
