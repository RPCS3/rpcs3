////////////////////////////////////////////////////
// Name:        progdlgg.h
// Purpose:     wxProgressDialog class
// Author:      Karsten Ballueder
// Modified by:
// Created:     09.05.1999
// RCS-ID:      $Id: progdlgg.h 50711 2007-12-15 02:57:58Z VZ $
// Copyright:   (c) Karsten Ballueder
// Licence:     wxWindows licence
////////////////////////////////////////////////////

#ifndef __PROGDLGH_G__
#define __PROGDLGH_G__

#include "wx/defs.h"
#include "wx/progdlg.h"

#if wxUSE_PROGRESSDLG

#include "wx/dialog.h"

class WXDLLIMPEXP_FWD_CORE wxButton;
class WXDLLIMPEXP_FWD_CORE wxGauge;
class WXDLLIMPEXP_FWD_CORE wxStaticText;

/* Progress dialog which shows a moving progress bar.
    Taken from the Mahogany project.*/

class WXDLLEXPORT wxProgressDialog : public wxDialog
{
DECLARE_DYNAMIC_CLASS(wxProgressDialog)
public:
   /* Creates and displays dialog, disables event handling for other
       frames or parent frame to avoid recursion problems.
       @param title title for window
       @param message message to display in window
       @param maximum value for status bar, if <= 0, no bar is shown
       @param parent window or NULL
       @param style is the bit mask of wxPD_XXX constants from wx/defs.h
   */
   wxProgressDialog(const wxString &title, wxString const &message,
                    int maximum = 100,
                    wxWindow *parent = NULL,
                    int style = wxPD_APP_MODAL | wxPD_AUTO_HIDE);
   /* Destructor.
       Re-enables event handling for other windows.
   */
   virtual ~wxProgressDialog();

   /* Update the status bar to the new value.
       @param value new value
       @param newmsg if used, new message to display
       @returns true if ABORT button has not been pressed
   */
   virtual bool Update(int value, const wxString& newmsg = wxEmptyString, bool *skip = NULL);

    /* Switches the dialog to use a gauge in indeterminate mode and calls
       wxGauge::Pulse() to show to the user a bit of progress */
    virtual bool Pulse(const wxString& newmsg = wxEmptyString, bool *skip = NULL);

    // Must provide overload to avoid hiding it (and warnings about it)
    virtual void Update() { wxDialog::Update(); }

   /* Can be called to continue after the cancel button has been pressed, but
       the program decided to continue the operation (e.g., user didn't
       confirm it)
   */
   void Resume();

   virtual bool Show( bool show = true );

protected:
   // callback for optional abort button
   void OnCancel(wxCommandEvent& event);

   // callback for optional skip button
   void OnSkip(wxCommandEvent& event);

   // callback to disable "hard" window closing
   void OnClose(wxCloseEvent& event);

   // must be called to reenable the other windows temporarily disabled while
   // the dialog was shown
   void ReenableOtherWindows();

private:
   // create the label with given text and another one to show the time nearby
   // as the next windows in the sizer, returns the created control
   wxStaticText *CreateLabel(const wxString& text, wxSizer *sizer);

    // updates the label message
   void UpdateMessage(const wxString &newmsg);

   // common part of Update() and Pulse(), returns true if not cancelled
   bool DoAfterUpdate(bool *skip);

   // shortcuts for enabling buttons
   void EnableClose();
   void EnableSkip(bool enable=true);
   void EnableAbort(bool enable=true);
   inline void DisableSkip() { EnableSkip(false); }
   inline void DisableAbort() { EnableAbort(false); }

   // the status bar
   wxGauge *m_gauge;
   // the message displayed
   wxStaticText *m_msg;
   // displayed elapsed, estimated, remaining time
   class wxStaticText *m_elapsed,
                      *m_estimated,
                      *m_remaining;
   // time when the dialog was created
   unsigned long m_timeStart;
   // time when the dialog was closed or cancelled
   unsigned long m_timeStop;
   // time between the moment the dialog was closed/cancelled and resume
   unsigned long m_break;

   // parent top level window (may be NULL)
   wxWindow *m_parentTop;

    // continue processing or not (return value for Update())
    enum
    {
        Uncancelable = -1,   // dialog can't be canceled
        Canceled,            // can be cancelled and, in fact, was
        Continue,            // can be cancelled but wasn't
        Finished             // finished, waiting to be removed from screen
    } m_state;

    // skip some portion
    bool m_skip;

#if !defined(__SMARTPHONE__)
    // the abort and skip buttons (or NULL if none)
    wxButton *m_btnAbort;
    wxButton *m_btnSkip;
#endif

    // the maximum value
    int m_maximum;

    // saves the time when elapsed time was updated so there is only one
    // update per second
    unsigned long m_last_timeupdate;
    // tells how often a change of the estimated time has to be confirmed
    // before it is actually displayed - this reduces the frequence of updates
    // of estimated and remaining time
    const int m_delay;
    // counts the confirmations
    int m_ctdelay;
    unsigned long m_display_estimated;

    bool m_hasAbortButton,
         m_hasSkipButton;

#if defined(__WXMSW__ ) || defined(__WXPM__)
    // the factor we use to always keep the value in 16 bit range as the native
    // control only supports ranges from 0 to 65,535
    size_t m_factor;
#endif // __WXMSW__

    // for wxPD_APP_MODAL case
    class WXDLLIMPEXP_FWD_CORE wxWindowDisabler *m_winDisabler;

    DECLARE_EVENT_TABLE()
    DECLARE_NO_COPY_CLASS(wxProgressDialog)
};

#endif // wxUSE_PROGRESSDLG

#endif // __PROGDLGH_G__
