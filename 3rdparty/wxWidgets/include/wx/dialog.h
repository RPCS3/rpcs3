/////////////////////////////////////////////////////////////////////////////
// Name:        wx/dialog.h
// Purpose:     wxDialogBase class
// Author:      Vadim Zeitlin
// Modified by:
// Created:     29.06.99
// RCS-ID:      $Id: dialog.h 49804 2007-11-10 01:09:42Z VZ $
// Copyright:   (c) Vadim Zeitlin
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef _WX_DIALOG_H_BASE_
#define _WX_DIALOG_H_BASE_

#include "wx/defs.h"
#include "wx/containr.h"
#include "wx/toplevel.h"

class WXDLLIMPEXP_FWD_CORE wxSizer;
class WXDLLIMPEXP_FWD_CORE wxStdDialogButtonSizer;

#define wxDIALOG_NO_PARENT      0x0001  // Don't make owned by apps top window

#ifdef __WXWINCE__
#define wxDEFAULT_DIALOG_STYLE  (wxCAPTION | wxMAXIMIZE | wxCLOSE_BOX | wxNO_BORDER)
#else
#define wxDEFAULT_DIALOG_STYLE  (wxCAPTION | wxSYSTEM_MENU | wxCLOSE_BOX)
#endif

extern WXDLLEXPORT_DATA(const wxChar) wxDialogNameStr[];

class WXDLLEXPORT wxDialogBase : public wxTopLevelWindow
{
public:
    enum
    {
        // all flags allowed in wxDialogBase::CreateButtonSizer()
        ButtonSizerFlags = wxOK|wxCANCEL|wxYES|wxNO|wxHELP|wxNO_DEFAULT
    };

    wxDialogBase() { Init(); }
    virtual ~wxDialogBase() { }

    // define public wxDialog methods to be implemented by the derived classes
    virtual int ShowModal() = 0;
    virtual void EndModal(int retCode) = 0;
    virtual bool IsModal() const = 0;


    // Modal dialogs have a return code - usually the id of the last
    // pressed button
    void SetReturnCode(int returnCode) { m_returnCode = returnCode; }
    int GetReturnCode() const { return m_returnCode; }

    // Set the identifier for the affirmative button: this button will close
    // the dialog after validating data and calling TransferDataFromWindow()
    void SetAffirmativeId(int affirmativeId);
    int GetAffirmativeId() const { return m_affirmativeId; }

    // Set identifier for Esc key translation: the button with this id will
    // close the dialog without doing anything else; special value wxID_NONE
    // means to not handle Esc at all while wxID_ANY means to map Esc to
    // wxID_CANCEL if present and GetAffirmativeId() otherwise
    void SetEscapeId(int escapeId);
    int GetEscapeId() const { return m_escapeId; }

#if wxUSE_STATTEXT // && wxUSE_TEXTCTRL
    // splits text up at newlines and places the
    // lines into a vertical wxBoxSizer
    wxSizer *CreateTextSizer( const wxString &message );
#endif // wxUSE_STATTEXT // && wxUSE_TEXTCTRL

    // returns a horizontal wxBoxSizer containing the given buttons
    //
    // notice that the returned sizer can be NULL if no buttons are put in the
    // sizer (this mostly happens under smart phones and other atypical
    // platforms which have hardware buttons replacing OK/Cancel and such)
    wxSizer *CreateButtonSizer(long flags);

    // returns the sizer containing CreateButtonSizer() below a separating
    // static line for the platforms which use static lines for items
    // separation (i.e. not Mac)
    wxSizer *CreateSeparatedButtonSizer(long flags);

#if wxUSE_BUTTON
    wxStdDialogButtonSizer *CreateStdDialogButtonSizer( long flags );
#endif // wxUSE_BUTTON

protected:
    // emulate click of a button with the given id if it's present in the dialog
    //
    // return true if button was "clicked" or false if we don't have it
    bool EmulateButtonClickIfPresent(int id);

    // this function is used by OnCharHook() to decide whether the given key
    // should close the dialog
    //
    // for most platforms the default implementation (which just checks for
    // Esc) is sufficient, but Mac port also adds Cmd-. here and other ports
    // could do something different if needed
    virtual bool IsEscapeKey(const wxKeyEvent& event);

    // end either modal or modeless dialog, for the modal dialog rc is used as
    // the dialog return code
    void EndDialog(int rc);

    // call Validate() and TransferDataFromWindow() and close dialog with
    // wxID_OK return code
    void AcceptAndClose();


    // The return code from modal dialog
    int m_returnCode;

    // The identifier for the affirmative button (usually wxID_OK)
    int m_affirmativeId;

    // The identifier for cancel button (usually wxID_CANCEL)
    int m_escapeId;

private:
    // common part of all ctors
    void Init();

    // handle Esc key presses
    void OnCharHook(wxKeyEvent& event);

    // handle closing the dialog window
    void OnCloseWindow(wxCloseEvent& event);

    // handle the standard buttons
    void OnButton(wxCommandEvent& event);

    // update the background colour
    void OnSysColourChanged(wxSysColourChangedEvent& event);


    DECLARE_NO_COPY_CLASS(wxDialogBase)
    DECLARE_EVENT_TABLE()
    WX_DECLARE_CONTROL_CONTAINER();
};


#if defined(__WXUNIVERSAL__) && !defined(__WXMICROWIN__)
    #include "wx/univ/dialog.h"
#else
    #if defined(__WXPALMOS__)
        #include "wx/palmos/dialog.h"
    #elif defined(__WXMSW__)
        #include "wx/msw/dialog.h"
    #elif defined(__WXMOTIF__)
        #include "wx/motif/dialog.h"
    #elif defined(__WXGTK20__)
        #include "wx/gtk/dialog.h"
    #elif defined(__WXGTK__)
        #include "wx/gtk1/dialog.h"
    #elif defined(__WXMAC__)
        #include "wx/mac/dialog.h"
    #elif defined(__WXCOCOA__)
        #include "wx/cocoa/dialog.h"
    #elif defined(__WXPM__)
        #include "wx/os2/dialog.h"
    #endif
#endif

#endif
    // _WX_DIALOG_H_BASE_
