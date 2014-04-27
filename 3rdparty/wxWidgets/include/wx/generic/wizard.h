///////////////////////////////////////////////////////////////////////////////
// Name:        wx/generic/wizard.h
// Purpose:     declaration of generic wxWizard class
// Author:      Vadim Zeitlin
// Modified by: Robert Vazan (sizers)
// Created:     28.09.99
// RCS-ID:      $Id: wizard.h 49563 2007-10-31 20:46:21Z VZ $
// Copyright:   (c) 1999 Vadim Zeitlin <zeitlin@dptmaths.ens-cachan.fr>
// Licence:     wxWindows licence
///////////////////////////////////////////////////////////////////////////////

#ifndef _WX_GENERIC_WIZARD_H_
#define _WX_GENERIC_WIZARD_H_

// ----------------------------------------------------------------------------
// wxWizard
// ----------------------------------------------------------------------------

class WXDLLIMPEXP_FWD_CORE wxButton;
class WXDLLIMPEXP_FWD_CORE wxStaticBitmap;
class WXDLLIMPEXP_FWD_ADV wxWizardEvent;
class WXDLLIMPEXP_FWD_CORE wxBoxSizer;
class WXDLLIMPEXP_FWD_ADV wxWizardSizer;

class WXDLLIMPEXP_ADV wxWizard : public wxWizardBase
{
public:
    // ctor
    wxWizard() { Init(); }
    wxWizard(wxWindow *parent,
             int id = wxID_ANY,
             const wxString& title = wxEmptyString,
             const wxBitmap& bitmap = wxNullBitmap,
             const wxPoint& pos = wxDefaultPosition,
             long style = wxDEFAULT_DIALOG_STYLE)
    {
        Init();
        Create(parent, id, title, bitmap, pos, style);
    }
    bool Create(wxWindow *parent,
             int id = wxID_ANY,
             const wxString& title = wxEmptyString,
             const wxBitmap& bitmap = wxNullBitmap,
             const wxPoint& pos = wxDefaultPosition,
             long style = wxDEFAULT_DIALOG_STYLE);
    void Init();

#if wxABI_VERSION >= 20804
    virtual ~wxWizard();
#endif

    // implement base class pure virtuals
    virtual bool RunWizard(wxWizardPage *firstPage);
    virtual wxWizardPage *GetCurrentPage() const;
    virtual void SetPageSize(const wxSize& size);
    virtual wxSize GetPageSize() const;
    virtual void FitToPage(const wxWizardPage *firstPage);
    virtual wxSizer *GetPageAreaSizer() const;
    virtual void SetBorder(int border);

    /// set/get bitmap
#if wxABI_VERSION >= 20805
    const wxBitmap& GetBitmap() const { return m_bitmap; }
    void SetBitmap(const wxBitmap& bitmap);
#endif

    // implementation only from now on
    // -------------------------------

    // is the wizard running?
    bool IsRunning() const { return m_page != NULL; }

    // show the prev/next page, but call TransferDataFromWindow on the current
    // page first and return false without changing the page if
    // TransferDataFromWindow() returns false - otherwise, returns true
    bool ShowPage(wxWizardPage *page, bool goingForward = true);

    // do fill the dialog with controls
    // this is app-overridable to, for example, set help and tooltip text
    virtual void DoCreateControls();

protected:
    // for compatibility only, doesn't do anything any more
    void FinishLayout() { }

private:
    // was the dialog really created?
    bool WasCreated() const { return m_btnPrev != NULL; }

    // event handlers
    void OnCancel(wxCommandEvent& event);
    void OnBackOrNext(wxCommandEvent& event);
    void OnHelp(wxCommandEvent& event);

    void OnWizEvent(wxWizardEvent& event);

    void AddBitmapRow(wxBoxSizer *mainColumn);
    void AddStaticLine(wxBoxSizer *mainColumn);
    void AddBackNextPair(wxBoxSizer *buttonRow);
    void AddButtonRow(wxBoxSizer *mainColumn);

    // the page size requested by user
    wxSize m_sizePage;

    // the dialog position from the ctor
    wxPoint m_posWizard;

    // wizard state
    wxWizardPage *m_page;       // the current page or NULL
    wxBitmap      m_bitmap;     // the default bitmap to show

    // wizard controls
    wxButton    *m_btnPrev,     // the "<Back" button
                *m_btnNext;     // the "Next>" or "Finish" button
    wxStaticBitmap *m_statbmp;  // the control for the bitmap

    // Border around page area sizer requested using SetBorder()
    int m_border;

    // Whether RunWizard() was called
    bool m_started;

    // Whether was modal (modeless has to be destroyed on finish or cancel)
    bool m_wasModal;

    // True if pages are laid out using the sizer
    bool m_usingSizer;

    // Page area sizer will be inserted here with padding
    wxBoxSizer *m_sizerBmpAndPage;

    // Actual position and size of pages
    wxWizardSizer *m_sizerPage;

    friend class wxWizardSizer;

    DECLARE_DYNAMIC_CLASS(wxWizard)
    DECLARE_EVENT_TABLE()
    DECLARE_NO_COPY_CLASS(wxWizard)
};

#endif // _WX_GENERIC_WIZARD_H_
