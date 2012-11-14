///////////////////////////////////////////////////////////////////////////////
// Name:        wizard.h
// Purpose:     wxWizard class: a GUI control presenting the user with a
//              sequence of dialogs which allows to simply perform some task
// Author:      Vadim Zeitlin (partly based on work by Ron Kuris and Kevin B.
//              Smith)
// Modified by: Robert Cavanaugh
//              Added capability to use .WXR resource files in Wizard pages
//              Added wxWIZARD_HELP event
//              Robert Vazan (sizers)
// Created:     15.08.99
// RCS-ID:      $Id: wizard.h 63262 2010-01-25 18:47:47Z JS $
// Copyright:   (c) 1999 Vadim Zeitlin <zeitlin@dptmaths.ens-cachan.fr>
// Licence:     wxWindows licence
///////////////////////////////////////////////////////////////////////////////

#ifndef _WX_WIZARD_H_
#define _WX_WIZARD_H_

#include "wx/defs.h"

#if wxUSE_WIZARDDLG

// ----------------------------------------------------------------------------
// headers and other simple declarations
// ----------------------------------------------------------------------------

#include "wx/dialog.h"      // the base class
#include "wx/panel.h"       // ditto
#include "wx/event.h"       // wxEVT_XXX constants
#include "wx/bitmap.h"

// Extended style to specify a help button
#define wxWIZARD_EX_HELPBUTTON   0x00000010

// forward declarations
class WXDLLIMPEXP_FWD_ADV wxWizard;

// ----------------------------------------------------------------------------
// wxWizardPage is one of the wizards screen: it must know what are the
// following and preceding pages (which may be NULL for the first/last page).
//
// Other than GetNext/Prev() functions, wxWizardPage is just a panel and may be
// used as such (i.e. controls may be placed directly on it &c).
// ----------------------------------------------------------------------------

class WXDLLIMPEXP_ADV wxWizardPage : public wxPanel
{
public:
    wxWizardPage() { Init(); }

    // ctor accepts an optional bitmap which will be used for this page instead
    // of the default one for this wizard (should be of the same size). Notice
    // that no other parameters are needed because the wizard will resize and
    // reposition the page anyhow
    wxWizardPage(wxWizard *parent,
                 const wxBitmap& bitmap = wxNullBitmap,
                 const wxChar* resource = NULL);

    bool Create(wxWizard *parent,
                const wxBitmap& bitmap = wxNullBitmap,
                const wxChar* resource = NULL);

    // these functions are used by the wizard to show another page when the
    // user chooses "Back" or "Next" button
    virtual wxWizardPage *GetPrev() const = 0;
    virtual wxWizardPage *GetNext() const = 0;

    // default GetBitmap() will just return m_bitmap which is ok in 99% of
    // cases - override this method if you want to create the bitmap to be used
    // dynamically or to do something even more fancy. It's ok to return
    // wxNullBitmap from here - the default one will be used then.
    virtual wxBitmap GetBitmap() const { return m_bitmap; }

#if wxUSE_VALIDATORS
    // Override the base functions to allow a validator to be assigned to this page.
    virtual bool TransferDataToWindow()
    {
        return GetValidator() ? GetValidator()->TransferToWindow()
                              : wxPanel::TransferDataToWindow();
    }

    virtual bool TransferDataFromWindow()
    {
        return GetValidator() ? GetValidator()->TransferFromWindow()
                              : wxPanel::TransferDataFromWindow();
    }

    virtual bool Validate()
    {
        return GetValidator() ? GetValidator()->Validate(this)
                              : wxPanel::Validate();
    }
#endif // wxUSE_VALIDATORS

protected:
    // common part of ctors:
    void Init();

    wxBitmap m_bitmap;

private:
    DECLARE_DYNAMIC_CLASS_NO_COPY(wxWizardPage)
};

// ----------------------------------------------------------------------------
// wxWizardPageSimple just returns the pointers given to the ctor and is useful
// to create a simple wizard where the order of pages never changes.
//
// OTOH, it is also possible to dynamicly decide which page to return (i.e.
// depending on the user's choices) as the wizard sample shows - in order to do
// this, you must derive from wxWizardPage directly.
// ----------------------------------------------------------------------------

class WXDLLIMPEXP_ADV wxWizardPageSimple : public wxWizardPage
{
public:
    wxWizardPageSimple() { Init(); }

    // ctor takes the previous and next pages
    wxWizardPageSimple(wxWizard *parent,
                       wxWizardPage *prev = (wxWizardPage *)NULL,
                       wxWizardPage *next = (wxWizardPage *)NULL,
                       const wxBitmap& bitmap = wxNullBitmap,
                       const wxChar* resource = NULL)
    {
        Create(parent, prev, next, bitmap, resource);
    }

    bool Create(wxWizard *parent = NULL, // let it be default ctor too
                wxWizardPage *prev = (wxWizardPage *)NULL,
                wxWizardPage *next = (wxWizardPage *)NULL,
                const wxBitmap& bitmap = wxNullBitmap,
                const wxChar* resource = NULL)
    {
        m_prev = prev;
        m_next = next;
        return wxWizardPage::Create(parent, bitmap, resource);
    }

    // the pointers may be also set later - but before starting the wizard
    void SetPrev(wxWizardPage *prev) { m_prev = prev; }
    void SetNext(wxWizardPage *next) { m_next = next; }

    // a convenience function to make the pages follow each other
    static void Chain(wxWizardPageSimple *first, wxWizardPageSimple *second)
    {
        wxCHECK_RET( first && second,
                     wxT("NULL passed to wxWizardPageSimple::Chain") );

        first->SetNext(second);
        second->SetPrev(first);
    }

    // base class pure virtuals
    virtual wxWizardPage *GetPrev() const;
    virtual wxWizardPage *GetNext() const;

private:
    // common part of ctors:
    void Init()
    {
        m_prev = m_next = NULL;
    }

    // pointers are private, the derived classes shouldn't mess with them -
    // just derive from wxWizardPage directly to implement different behaviour
    wxWizardPage *m_prev,
                 *m_next;

    DECLARE_DYNAMIC_CLASS_NO_COPY(wxWizardPageSimple)
};

// ----------------------------------------------------------------------------
// wxWizard
// ----------------------------------------------------------------------------

class WXDLLIMPEXP_ADV wxWizardBase : public wxDialog
{
public:
    /*
       The derived class (i.e. the real wxWizard) has a ctor and Create()
       function taking the following arguments:

        wxWizard(wxWindow *parent,
                 int id = wxID_ANY,
                 const wxString& title = wxEmptyString,
                 const wxBitmap& bitmap = wxNullBitmap,
                 const wxPoint& pos = wxDefaultPosition,
                 long style = wxDEFAULT_DIALOG_STYLE);
    */
    wxWizardBase() { }

    // executes the wizard starting from the given page, returns true if it was
    // successfully finished, false if user cancelled it
    virtual bool RunWizard(wxWizardPage *firstPage) = 0;

    // get the current page (NULL if RunWizard() isn't running)
    virtual wxWizardPage *GetCurrentPage() const = 0;

    // set the min size which should be available for the pages: a
    // wizard will take into account the size of the bitmap (if any)
    // itself and will never be less than some predefined fixed size
    virtual void SetPageSize(const wxSize& size) = 0;

    // get the size available for the page
    virtual wxSize GetPageSize() const = 0;

    // set the best size for the wizard, i.e. make it big enough to contain all
    // of the pages starting from the given one
    //
    // this function may be called several times and possible with different
    // pages in which case it will only increase the page size if needed (this
    // may be useful if not all pages are accessible from the first one by
    // default)
    virtual void FitToPage(const wxWizardPage *firstPage) = 0;

    // Adding pages to page area sizer enlarges wizard
    virtual wxSizer *GetPageAreaSizer() const = 0;

    // Set border around page area. Default is 0 if you add at least one
    // page to GetPageAreaSizer and 5 if you don't.
    virtual void SetBorder(int border) = 0;

    // the methods below may be overridden by the derived classes to provide
    // custom logic for determining the pages order

    virtual bool HasNextPage(wxWizardPage *page)
        { return page->GetNext() != NULL; }

    virtual bool HasPrevPage(wxWizardPage *page)
        { return page->GetPrev() != NULL; }

    /// Override these functions to stop InitDialog from calling TransferDataToWindow
    /// for _all_ pages when the wizard starts. Instead 'ShowPage' will call
    /// TransferDataToWindow for the first page only.
    bool TransferDataToWindow() { return true; }
    bool TransferDataFromWindow() { return true; }
    bool Validate() { return true; }

private:
    DECLARE_NO_COPY_CLASS(wxWizardBase)
};

// include the real class declaration
#include "wx/generic/wizard.h"

// ----------------------------------------------------------------------------
// wxWizardEvent class represents an event generated by the wizard: this event
// is first sent to the page itself and, if not processed there, goes up the
// window hierarchy as usual
// ----------------------------------------------------------------------------

class WXDLLIMPEXP_ADV wxWizardEvent : public wxNotifyEvent
{
public:
    wxWizardEvent(wxEventType type = wxEVT_NULL,
                  int id = wxID_ANY,
                  bool direction = true,
                  wxWizardPage* page = NULL);

    // for EVT_WIZARD_PAGE_CHANGING, return true if we're going forward or
    // false otherwise and for EVT_WIZARD_PAGE_CHANGED return true if we came
    // from the previous page and false if we returned from the next one
    // (this function doesn't make sense for CANCEL events)
    bool GetDirection() const { return m_direction; }

    wxWizardPage*   GetPage() const { return m_page; }

private:
    bool m_direction;
    wxWizardPage*    m_page;

    DECLARE_DYNAMIC_CLASS(wxWizardEvent)
    DECLARE_NO_COPY_CLASS(wxWizardEvent)
};

// ----------------------------------------------------------------------------
// macros for handling wxWizardEvents
// ----------------------------------------------------------------------------

BEGIN_DECLARE_EVENT_TYPES()
    DECLARE_EXPORTED_EVENT_TYPE(WXDLLIMPEXP_ADV, wxEVT_WIZARD_PAGE_CHANGED, 900)
    DECLARE_EXPORTED_EVENT_TYPE(WXDLLIMPEXP_ADV, wxEVT_WIZARD_PAGE_CHANGING, 901)
    DECLARE_EXPORTED_EVENT_TYPE(WXDLLIMPEXP_ADV, wxEVT_WIZARD_CANCEL, 902)
    DECLARE_EXPORTED_EVENT_TYPE(WXDLLIMPEXP_ADV, wxEVT_WIZARD_HELP, 903)
    DECLARE_EXPORTED_EVENT_TYPE(WXDLLIMPEXP_ADV, wxEVT_WIZARD_FINISHED, 903)
#if wxABI_VERSION >= 20811
    DECLARE_EXPORTED_EVENT_TYPE(WXDLLIMPEXP_ADV, wxEVT_WIZARD_PAGE_SHOWN, 904)
#endif
END_DECLARE_EVENT_TYPES()

typedef void (wxEvtHandler::*wxWizardEventFunction)(wxWizardEvent&);

#define wxWizardEventHandler(func) \
    (wxObjectEventFunction)(wxEventFunction)wxStaticCastEvent(wxWizardEventFunction, &func)

#define wx__DECLARE_WIZARDEVT(evt, id, fn) \
    wx__DECLARE_EVT1(wxEVT_WIZARD_ ## evt, id, wxWizardEventHandler(fn))

// notifies that the page has just been changed (can't be vetoed)
#define EVT_WIZARD_PAGE_CHANGED(id, fn) wx__DECLARE_WIZARDEVT(PAGE_CHANGED, id, fn)

// the user pressed "<Back" or "Next>" button and the page is going to be
// changed - unless the event handler vetoes the event
#define EVT_WIZARD_PAGE_CHANGING(id, fn) wx__DECLARE_WIZARDEVT(PAGE_CHANGING, id, fn)

// the user pressed "Cancel" button and the wizard is going to be dismissed -
// unless the event handler vetoes the event
#define EVT_WIZARD_CANCEL(id, fn) wx__DECLARE_WIZARDEVT(CANCEL, id, fn)

// the user pressed "Finish" button and the wizard is going to be dismissed -
#define EVT_WIZARD_FINISHED(id, fn) wx__DECLARE_WIZARDEVT(FINISHED, id, fn)

// the user pressed "Help" button
#define EVT_WIZARD_HELP(id, fn) wx__DECLARE_WIZARDEVT(HELP, id, fn)

// the page was just shown and laid out
#define EVT_WIZARD_PAGE_SHOWN(id, fn) wx__DECLARE_WIZARDEVT(PAGE_SHOWN, id, fn)

#endif // wxUSE_WIZARDDLG

#endif // _WX_WIZARD_H_
