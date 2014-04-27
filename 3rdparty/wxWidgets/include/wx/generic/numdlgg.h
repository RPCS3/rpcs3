/////////////////////////////////////////////////////////////////////////////
// Name:        wx/generic/numdlgg.h
// Purpose:     wxNumberEntryDialog class
// Author:      John Labenski
// Modified by:
// Created:     07.02.04 (extracted from textdlgg.cpp)
// RCS-ID:      $Id: numdlgg.h 49563 2007-10-31 20:46:21Z VZ $
// Copyright:   (c) wxWidgets team
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef __NUMDLGH_G__
#define __NUMDLGH_G__

#include "wx/defs.h"

#if wxUSE_NUMBERDLG

#include "wx/dialog.h"

#if wxUSE_SPINCTRL
    class WXDLLIMPEXP_FWD_CORE wxSpinCtrl;
#else
    class WXDLLIMPEXP_FWD_CORE wxTextCtrl;
#endif // wxUSE_SPINCTRL

// ----------------------------------------------------------------------------
// wxNumberEntryDialog: a dialog with spin control, [ok] and [cancel] buttons
// ----------------------------------------------------------------------------

class WXDLLEXPORT wxNumberEntryDialog : public wxDialog
{
public:
    wxNumberEntryDialog(wxWindow *parent,
                        const wxString& message,
                        const wxString& prompt,
                        const wxString& caption,
                        long value, long min, long max,
                        const wxPoint& pos = wxDefaultPosition);

    long GetValue() const { return m_value; }

    // implementation only
    void OnOK(wxCommandEvent& event);
    void OnCancel(wxCommandEvent& event);

protected:

#if wxUSE_SPINCTRL
    wxSpinCtrl *m_spinctrl;
#else
    wxTextCtrl *m_spinctrl;
#endif // wxUSE_SPINCTRL

    long m_value, m_min, m_max;

private:
    DECLARE_EVENT_TABLE()
    DECLARE_DYNAMIC_CLASS(wxNumberEntryDialog)
    DECLARE_NO_COPY_CLASS(wxNumberEntryDialog)
};

// ----------------------------------------------------------------------------
// function to get a number from user
// ----------------------------------------------------------------------------

long WXDLLEXPORT
wxGetNumberFromUser(const wxString& message,
                    const wxString& prompt,
                    const wxString& caption,
                    long value = 0,
                    long min = 0,
                    long max = 100,
                    wxWindow *parent = (wxWindow *)NULL,
                    const wxPoint& pos = wxDefaultPosition);

#endif // wxUSE_NUMBERDLG

#endif // __NUMDLGH_G__
