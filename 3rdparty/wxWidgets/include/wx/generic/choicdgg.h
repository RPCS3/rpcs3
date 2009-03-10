/////////////////////////////////////////////////////////////////////////////
// Name:        wx/generic/choicdgg.h
// Purpose:     Generic choice dialogs
// Author:      Julian Smart
// Modified by: 03.11.00: VZ to add wxArrayString and multiple sel functions
// Created:     01/02/97
// RCS-ID:      $Id: choicdgg.h 49563 2007-10-31 20:46:21Z VZ $
// Copyright:   (c) wxWidgets team
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef _WX_GENERIC_CHOICDGG_H_
#define _WX_GENERIC_CHOICDGG_H_

#include "wx/dynarray.h"
#include "wx/dialog.h"

class WXDLLIMPEXP_FWD_CORE wxListBoxBase;

// ----------------------------------------------------------------------------
// some (ugly...) constants
// ----------------------------------------------------------------------------

#define wxCHOICE_HEIGHT 150
#define wxCHOICE_WIDTH 200

#ifdef __WXWINCE__
#define wxCHOICEDLG_STYLE \
    (wxDEFAULT_DIALOG_STYLE | wxOK | wxCANCEL | wxCENTRE)
#else
#define wxCHOICEDLG_STYLE \
    (wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER | wxOK | wxCANCEL | wxCENTRE)
#endif

// ----------------------------------------------------------------------------
// wxAnyChoiceDialog: a base class for dialogs containing a listbox
// ----------------------------------------------------------------------------

class WXDLLEXPORT wxAnyChoiceDialog : public wxDialog
{
public:
    wxAnyChoiceDialog() { }

    wxAnyChoiceDialog(wxWindow *parent,
                      const wxString& message,
                      const wxString& caption,
                      int n, const wxString *choices,
                      long styleDlg = wxCHOICEDLG_STYLE,
                      const wxPoint& pos = wxDefaultPosition,
                      long styleLbox = wxLB_ALWAYS_SB)
    {
        (void)Create(parent, message, caption, n, choices,
                     styleDlg, pos, styleLbox);
    }
    wxAnyChoiceDialog(wxWindow *parent,
                      const wxString& message,
                      const wxString& caption,
                      const wxArrayString& choices,
                      long styleDlg = wxCHOICEDLG_STYLE,
                      const wxPoint& pos = wxDefaultPosition,
                      long styleLbox = wxLB_ALWAYS_SB)
    {
        (void)Create(parent, message, caption, choices,
                     styleDlg, pos, styleLbox);
    }

    bool Create(wxWindow *parent,
                const wxString& message,
                const wxString& caption,
                int n, const wxString *choices,
                long styleDlg = wxCHOICEDLG_STYLE,
                const wxPoint& pos = wxDefaultPosition,
                long styleLbox = wxLB_ALWAYS_SB);
    bool Create(wxWindow *parent,
                const wxString& message,
                const wxString& caption,
                const wxArrayString& choices,
                long styleDlg = wxCHOICEDLG_STYLE,
                const wxPoint& pos = wxDefaultPosition,
                long styleLbox = wxLB_ALWAYS_SB);

protected:
    wxListBoxBase *m_listbox;

    virtual wxListBoxBase *CreateList(int n,
                                      const wxString *choices,
                                      long styleLbox);

    DECLARE_NO_COPY_CLASS(wxAnyChoiceDialog)
};

// ----------------------------------------------------------------------------
// wxSingleChoiceDialog: a dialog with single selection listbox
// ----------------------------------------------------------------------------

class WXDLLEXPORT wxSingleChoiceDialog : public wxAnyChoiceDialog
{
public:
    wxSingleChoiceDialog()
    {
        m_selection = -1;
    }

    wxSingleChoiceDialog(wxWindow *parent,
                         const wxString& message,
                         const wxString& caption,
                         int n,
                         const wxString *choices,
                         char **clientData = (char **)NULL,
                         long style = wxCHOICEDLG_STYLE,
                         const wxPoint& pos = wxDefaultPosition);
    wxSingleChoiceDialog(wxWindow *parent,
                         const wxString& message,
                         const wxString& caption,
                         const wxArrayString& choices,
                         char **clientData = (char **)NULL,
                         long style = wxCHOICEDLG_STYLE,
                         const wxPoint& pos = wxDefaultPosition);

    bool Create(wxWindow *parent,
                const wxString& message,
                const wxString& caption,
                int n,
                const wxString *choices,
                char **clientData = (char **)NULL,
                long style = wxCHOICEDLG_STYLE,
                const wxPoint& pos = wxDefaultPosition);
    bool Create(wxWindow *parent,
                const wxString& message,
                const wxString& caption,
                const wxArrayString& choices,
                char **clientData = (char **)NULL,
                long style = wxCHOICEDLG_STYLE,
                const wxPoint& pos = wxDefaultPosition);

    void SetSelection(int sel);
    int GetSelection() const { return m_selection; }
    wxString GetStringSelection() const { return m_stringSelection; }

    // obsolete function (NB: no need to make it return wxChar, it's untyped)
    char *GetSelectionClientData() const { return (char *)m_clientData; }

    // implementation from now on
    void OnOK(wxCommandEvent& event);
#ifndef __SMARTPHONE__
    void OnListBoxDClick(wxCommandEvent& event);
#endif
#ifdef __WXWINCE__
    void OnJoystickButtonDown(wxJoystickEvent& event);
#endif

protected:
    int         m_selection;
    wxString    m_stringSelection;

    void DoChoice();

private:
    DECLARE_DYNAMIC_CLASS_NO_COPY(wxSingleChoiceDialog)
    DECLARE_EVENT_TABLE()
};

// ----------------------------------------------------------------------------
// wxMultiChoiceDialog: a dialog with multi selection listbox
// ----------------------------------------------------------------------------

class WXDLLEXPORT wxMultiChoiceDialog : public wxAnyChoiceDialog
{
public:
    wxMultiChoiceDialog() { }

    wxMultiChoiceDialog(wxWindow *parent,
                        const wxString& message,
                        const wxString& caption,
                        int n,
                        const wxString *choices,
                        long style = wxCHOICEDLG_STYLE,
                        const wxPoint& pos = wxDefaultPosition)
    {
        (void)Create(parent, message, caption, n, choices, style, pos);
    }
    wxMultiChoiceDialog(wxWindow *parent,
                        const wxString& message,
                        const wxString& caption,
                        const wxArrayString& choices,
                        long style = wxCHOICEDLG_STYLE,
                        const wxPoint& pos = wxDefaultPosition)
    {
        (void)Create(parent, message, caption, choices, style, pos);
    }

    bool Create(wxWindow *parent,
                const wxString& message,
                const wxString& caption,
                int n,
                const wxString *choices,
                long style = wxCHOICEDLG_STYLE,
                const wxPoint& pos = wxDefaultPosition);
    bool Create(wxWindow *parent,
                const wxString& message,
                const wxString& caption,
                const wxArrayString& choices,
                long style = wxCHOICEDLG_STYLE,
                const wxPoint& pos = wxDefaultPosition);

    void SetSelections(const wxArrayInt& selections);
    wxArrayInt GetSelections() const { return m_selections; }

    // implementation from now on
    virtual bool TransferDataFromWindow();

protected:
#if wxUSE_CHECKLISTBOX
    virtual wxListBoxBase *CreateList(int n,
                                      const wxString *choices,
                                      long styleLbox);
#endif // wxUSE_CHECKLISTBOX

    wxArrayInt m_selections;

private:
    DECLARE_DYNAMIC_CLASS_NO_COPY(wxMultiChoiceDialog)
};

// ----------------------------------------------------------------------------
// wrapper functions which can be used to get selection(s) from the user
// ----------------------------------------------------------------------------

// get the user selection as a string
WXDLLEXPORT wxString wxGetSingleChoice(const wxString& message,
                                       const wxString& caption,
                                       const wxArrayString& choices,
                                       wxWindow *parent = NULL,
                                       int x = wxDefaultCoord,
                                       int y = wxDefaultCoord,
                                       bool centre = true,
                                       int width = wxCHOICE_WIDTH,
                                       int height = wxCHOICE_HEIGHT);

WXDLLEXPORT wxString wxGetSingleChoice(const wxString& message,
                                       const wxString& caption,
                                       int n, const wxString *choices,
                                       wxWindow *parent = NULL,
                                       int x = wxDefaultCoord,
                                       int y = wxDefaultCoord,
                                       bool centre = true,
                                       int width = wxCHOICE_WIDTH,
                                       int height = wxCHOICE_HEIGHT);

// Same as above but gets position in list of strings, instead of string,
// or -1 if no selection
WXDLLEXPORT int wxGetSingleChoiceIndex(const wxString& message,
                                       const wxString& caption,
                                       const wxArrayString& choices,
                                       wxWindow *parent = NULL,
                                       int x = wxDefaultCoord,
                                       int y = wxDefaultCoord,
                                       bool centre = true,
                                       int width = wxCHOICE_WIDTH,
                                       int height = wxCHOICE_HEIGHT);

WXDLLEXPORT int wxGetSingleChoiceIndex(const wxString& message,
                                       const wxString& caption,
                                       int n, const wxString *choices,
                                       wxWindow *parent = NULL,
                                       int x = wxDefaultCoord,
                                       int y = wxDefaultCoord,
                                       bool centre = true,
                                       int width = wxCHOICE_WIDTH,
                                       int height = wxCHOICE_HEIGHT);

// Return client data instead or NULL if cancelled
WXDLLEXPORT void* wxGetSingleChoiceData(const wxString& message,
                                        const wxString& caption,
                                        const wxArrayString& choices,
                                        void **client_data,
                                        wxWindow *parent = NULL,
                                        int x = wxDefaultCoord,
                                        int y = wxDefaultCoord,
                                        bool centre = true,
                                        int width = wxCHOICE_WIDTH,
                                        int height = wxCHOICE_HEIGHT);

WXDLLEXPORT void* wxGetSingleChoiceData(const wxString& message,
                                        const wxString& caption,
                                        int n, const wxString *choices,
                                        void **client_data,
                                        wxWindow *parent = NULL,
                                        int x = wxDefaultCoord,
                                        int y = wxDefaultCoord,
                                        bool centre = true,
                                        int width = wxCHOICE_WIDTH,
                                        int height = wxCHOICE_HEIGHT);

// fill the array with the indices of the chosen items, it will be empty
// if no items were selected or Cancel was pressed - return the number of
// selections
WXDLLEXPORT size_t wxGetMultipleChoices(wxArrayInt& selections,
                                        const wxString& message,
                                        const wxString& caption,
                                        int n, const wxString *choices,
                                        wxWindow *parent = NULL,
                                        int x = wxDefaultCoord,
                                        int y = wxDefaultCoord,
                                        bool centre = true,
                                        int width = wxCHOICE_WIDTH,
                                        int height = wxCHOICE_HEIGHT);

WXDLLEXPORT size_t wxGetMultipleChoices(wxArrayInt& selections,
                                        const wxString& message,
                                        const wxString& caption,
                                        const wxArrayString& choices,
                                        wxWindow *parent = NULL,
                                        int x = wxDefaultCoord,
                                        int y = wxDefaultCoord,
                                        bool centre = true,
                                        int width = wxCHOICE_WIDTH,
                                        int height = wxCHOICE_HEIGHT);

#endif // _WX_GENERIC_CHOICDGG_H_
