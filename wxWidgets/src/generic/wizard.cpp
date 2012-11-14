///////////////////////////////////////////////////////////////////////////////
// Name:        src/generic/wizard.cpp
// Purpose:     generic implementation of wxWizard class
// Author:      Vadim Zeitlin
// Modified by: Robert Cavanaugh
//              1) Added capability for wxWizardPage to accept resources
//              2) Added "Help" button handler stub
//              3) Fixed ShowPage() bug on displaying bitmaps
//              Robert Vazan (sizers)
// Created:     15.08.99
// RCS-ID:      $Id: wizard.cpp 63262 2010-01-25 18:47:47Z JS $
// Copyright:   (c) 1999 Vadim Zeitlin <zeitlin@dptmaths.ens-cachan.fr>
// Licence:     wxWindows licence
///////////////////////////////////////////////////////////////////////////////

// ============================================================================
// declarations
// ============================================================================

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

// For compilers that support precompilation, includes "wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#if wxUSE_WIZARDDLG

#ifndef WX_PRECOMP
    #include "wx/dynarray.h"
    #include "wx/intl.h"
    #include "wx/statbmp.h"
    #include "wx/button.h"
    #include "wx/settings.h"
    #include "wx/sizer.h"
#endif //WX_PRECOMP

#include "wx/statline.h"

#include "wx/wizard.h"

// ----------------------------------------------------------------------------
// wxWizardSizer
// ----------------------------------------------------------------------------

class wxWizardSizer : public wxSizer
{
public:
    wxWizardSizer(wxWizard *owner);

    virtual wxSizerItem *Insert(size_t index, wxSizerItem *item);

    virtual void RecalcSizes();
    virtual wxSize CalcMin();

    // get the max size of all wizard pages
    wxSize GetMaxChildSize();

    // return the border which can be either set using wxWizard::SetBorder() or
    // have default value
    int GetBorder() const;

    // hide the pages which we temporarily "show" when they're added to this
    // sizer (see Insert())
    void HidePages();

private:
    wxSize SiblingSize(wxSizerItem *child);

    wxWizard *m_owner;
    wxSize m_childSize;
};

// ----------------------------------------------------------------------------
// event tables and such
// ----------------------------------------------------------------------------

DEFINE_EVENT_TYPE(wxEVT_WIZARD_PAGE_CHANGED)
DEFINE_EVENT_TYPE(wxEVT_WIZARD_PAGE_CHANGING)
DEFINE_EVENT_TYPE(wxEVT_WIZARD_CANCEL)
DEFINE_EVENT_TYPE(wxEVT_WIZARD_FINISHED)
DEFINE_EVENT_TYPE(wxEVT_WIZARD_HELP)
DEFINE_EVENT_TYPE(wxEVT_WIZARD_PAGE_SHOWN)

BEGIN_EVENT_TABLE(wxWizard, wxDialog)
    EVT_BUTTON(wxID_CANCEL, wxWizard::OnCancel)
    EVT_BUTTON(wxID_BACKWARD, wxWizard::OnBackOrNext)
    EVT_BUTTON(wxID_FORWARD, wxWizard::OnBackOrNext)
    EVT_BUTTON(wxID_HELP, wxWizard::OnHelp)

    EVT_WIZARD_PAGE_CHANGED(wxID_ANY, wxWizard::OnWizEvent)
    EVT_WIZARD_PAGE_CHANGING(wxID_ANY, wxWizard::OnWizEvent)
    EVT_WIZARD_CANCEL(wxID_ANY, wxWizard::OnWizEvent)
    EVT_WIZARD_FINISHED(wxID_ANY, wxWizard::OnWizEvent)
    EVT_WIZARD_HELP(wxID_ANY, wxWizard::OnWizEvent)
END_EVENT_TABLE()

IMPLEMENT_DYNAMIC_CLASS(wxWizard, wxDialog)

/*
    TODO PROPERTIES :
    wxWizard
        extstyle
        title
*/

IMPLEMENT_ABSTRACT_CLASS(wxWizardPage, wxPanel)
IMPLEMENT_DYNAMIC_CLASS(wxWizardPageSimple, wxWizardPage)
IMPLEMENT_DYNAMIC_CLASS(wxWizardEvent, wxNotifyEvent)

// ============================================================================
// implementation
// ============================================================================

// ----------------------------------------------------------------------------
// wxWizardPage
// ----------------------------------------------------------------------------

void wxWizardPage::Init()
{
    m_bitmap = wxNullBitmap;
}

wxWizardPage::wxWizardPage(wxWizard *parent,
                           const wxBitmap& bitmap,
                           const wxChar *resource)
{
    Create(parent, bitmap, resource);
}

bool wxWizardPage::Create(wxWizard *parent,
                          const wxBitmap& bitmap,
                          const wxChar *resource)
{
    if ( !wxPanel::Create(parent, wxID_ANY) )
        return false;

    if ( resource != NULL )
    {
#if wxUSE_WX_RESOURCES
#if 0
       if ( !LoadFromResource(this, resource) )
        {
            wxFAIL_MSG(wxT("wxWizardPage LoadFromResource failed!!!!"));
        }
#endif
#endif // wxUSE_RESOURCES
    }

    m_bitmap = bitmap;

    // initially the page is hidden, it's shown only when it becomes current
    Hide();

    return true;
}

// ----------------------------------------------------------------------------
// wxWizardPageSimple
// ----------------------------------------------------------------------------

wxWizardPage *wxWizardPageSimple::GetPrev() const
{
    return m_prev;
}

wxWizardPage *wxWizardPageSimple::GetNext() const
{
    return m_next;
}

// ----------------------------------------------------------------------------
// wxWizardSizer
// ----------------------------------------------------------------------------

wxWizardSizer::wxWizardSizer(wxWizard *owner)
             : m_owner(owner),
               m_childSize(wxDefaultSize)
{
}

wxSizerItem *wxWizardSizer::Insert(size_t index, wxSizerItem *item)
{
    m_owner->m_usingSizer = true;

    if ( item->IsWindow() )
    {
        // we must pretend that the window is shown as otherwise it wouldn't be
        // taken into account for the layout -- but avoid really showing it, so
        // just set the internal flag instead of calling wxWindow::Show()
        item->GetWindow()->wxWindowBase::Show();
    }

    return wxSizer::Insert(index, item);
}

void wxWizardSizer::HidePages()
{
    for ( wxSizerItemList::compatibility_iterator node = GetChildren().GetFirst();
          node;
          node = node->GetNext() )
    {
        wxSizerItem * const item = node->GetData();
        if ( item->IsWindow() )
            item->GetWindow()->wxWindowBase::Show(false);
    }
}

void wxWizardSizer::RecalcSizes()
{
    // Effect of this function depends on m_owner->m_page and
    // it should be called whenever it changes (wxWizard::ShowPage)
    if ( m_owner->m_page )
    {
        m_owner->m_page->SetSize(wxRect(m_position, m_size));
    }
}

wxSize wxWizardSizer::CalcMin()
{
    return m_owner->GetPageSize();
}

wxSize wxWizardSizer::GetMaxChildSize()
{
#if !defined(__WXDEBUG__)
    if ( m_childSize.IsFullySpecified() )
        return m_childSize;
#endif

    wxSize maxOfMin;

    for ( wxSizerItemList::compatibility_iterator childNode = m_children.GetFirst();
          childNode;
          childNode = childNode->GetNext() )
    {
        wxSizerItem *child = childNode->GetData();
        maxOfMin.IncTo(child->CalcMin());
        maxOfMin.IncTo(SiblingSize(child));
    }

#ifdef __WXDEBUG__
    if ( m_childSize.IsFullySpecified() && m_childSize != maxOfMin )
    {
        wxFAIL_MSG( _T("Size changed in wxWizard::GetPageAreaSizer()")
                    _T("after RunWizard().\n")
                    _T("Did you forget to call GetSizer()->Fit(this) ")
                    _T("for some page?")) ;

        return m_childSize;
    }
#endif // __WXDEBUG__

    if ( m_owner->m_started )
    {
        m_childSize = maxOfMin;
    }

    return maxOfMin;
}

int wxWizardSizer::GetBorder() const
{
    return m_owner->m_border;
}

wxSize wxWizardSizer::SiblingSize(wxSizerItem *child)
{
    wxSize maxSibling;

    if ( child->IsWindow() )
    {
        wxWizardPage *page = wxDynamicCast(child->GetWindow(), wxWizardPage);
        if ( page )
        {
            for ( wxWizardPage *sibling = page->GetNext();
                  sibling;
                  sibling = sibling->GetNext() )
            {
                if ( sibling->GetSizer() )
                {
                    maxSibling.IncTo(sibling->GetSizer()->CalcMin());
                }
            }
        }
    }

    return maxSibling;
}

// ----------------------------------------------------------------------------
// generic wxWizard implementation
// ----------------------------------------------------------------------------

void wxWizard::Init()
{
    m_posWizard = wxDefaultPosition;
    m_page = (wxWizardPage *)NULL;
    m_btnPrev = m_btnNext = NULL;
    m_statbmp = NULL;
    m_sizerBmpAndPage = NULL;
    m_sizerPage = NULL;
    m_border = 5;
    m_started = false;
    m_wasModal = false;
    m_usingSizer = false;
}

bool wxWizard::Create(wxWindow *parent,
                      int id,
                      const wxString& title,
                      const wxBitmap& bitmap,
                      const wxPoint& pos,
                      long style)
{
    bool result = wxDialog::Create(parent,id,title,pos,wxDefaultSize,style);

    m_posWizard = pos;
    m_bitmap = bitmap ;

    DoCreateControls();

    return result;
}

wxWizard::~wxWizard()
{
    // normally we don't have to delete this sizer as it's deleted by the
    // associated window but if we never used it or didn't set it as the window
    // sizer yet, do delete it manually
    if ( !m_usingSizer || !m_started )
        delete m_sizerPage;
}

void wxWizard::AddBitmapRow(wxBoxSizer *mainColumn)
{
    m_sizerBmpAndPage = new wxBoxSizer(wxHORIZONTAL);
    mainColumn->Add(
        m_sizerBmpAndPage,
        1, // Vertically stretchable
        wxEXPAND // Horizonal stretching, no border
    );
    mainColumn->Add(0,5,
        0, // No vertical stretching
        wxEXPAND // No border, (mostly useless) horizontal stretching
    );

#if wxUSE_STATBMP
    if ( m_bitmap.Ok() )
    {
        m_statbmp = new wxStaticBitmap(this, wxID_ANY, m_bitmap);
        m_sizerBmpAndPage->Add(
            m_statbmp,
            0, // No horizontal stretching
            wxALL, // Border all around, top alignment
            5 // Border width
        );
        m_sizerBmpAndPage->Add(
            5,0,
            0, // No horizontal stretching
            wxEXPAND // No border, (mostly useless) vertical stretching
        );
    }
#endif

    // Added to m_sizerBmpAndPage later
    m_sizerPage = new wxWizardSizer(this);
}

void wxWizard::AddStaticLine(wxBoxSizer *mainColumn)
{
#if wxUSE_STATLINE
    mainColumn->Add(
        new wxStaticLine(this, wxID_ANY),
        0, // Vertically unstretchable
        wxEXPAND | wxALL, // Border all around, horizontally stretchable
        5 // Border width
    );
    mainColumn->Add(0,5,
        0, // No vertical stretching
        wxEXPAND // No border, (mostly useless) horizontal stretching
    );
#else
    (void)mainColumn;
#endif // wxUSE_STATLINE
}

void wxWizard::AddBackNextPair(wxBoxSizer *buttonRow)
{
    wxASSERT_MSG( m_btnNext && m_btnPrev,
                  _T("You must create the buttons before calling ")
                  _T("wxWizard::AddBackNextPair") );

    // margin between Back and Next buttons
#ifdef __WXMAC__
    static const int BACKNEXT_MARGIN = 10;
#else
    static const int BACKNEXT_MARGIN = 0;
#endif

    wxBoxSizer *backNextPair = new wxBoxSizer(wxHORIZONTAL);
    buttonRow->Add(
        backNextPair,
        0, // No horizontal stretching
        wxALL, // Border all around
        5 // Border width
    );

    backNextPair->Add(m_btnPrev);
    backNextPair->Add(BACKNEXT_MARGIN,0,
        0, // No horizontal stretching
        wxEXPAND // No border, (mostly useless) vertical stretching
    );
    backNextPair->Add(m_btnNext);
}

void wxWizard::AddButtonRow(wxBoxSizer *mainColumn)
{
    // the order in which the buttons are created determines the TAB order - at least under MSWindows...
    // although the 'back' button appears before the 'next' button, a more userfriendly tab order is
    // to activate the 'next' button first (create the next button before the back button).
    // The reason is: The user will repeatedly enter information in the wizard pages and then wants to
    // press 'next'. If a user uses mostly the keyboard, he would have to skip the 'back' button
    // everytime. This is annoying. There is a second reason: RETURN acts as TAB. If the 'next'
    // button comes first in the TAB order, the user can enter information very fast using the RETURN
    // key to TAB to the next entry field and page. This would not be possible, if the 'back' button
    // was created before the 'next' button.

    bool isPda = (wxSystemSettings::GetScreenType() <= wxSYS_SCREEN_PDA);
    int buttonStyle = isPda ? wxBU_EXACTFIT : 0;

    wxBoxSizer *buttonRow = new wxBoxSizer(wxHORIZONTAL);
#ifdef __WXMAC__
    if (GetExtraStyle() & wxWIZARD_EX_HELPBUTTON)
        mainColumn->Add(
            buttonRow,
            0, // Vertically unstretchable
            wxGROW|wxALIGN_CENTRE
            );
    else
#endif
    mainColumn->Add(
        buttonRow,
        0, // Vertically unstretchable
        wxALIGN_RIGHT // Right aligned, no border
    );

    // Desired TAB order is 'next', 'cancel', 'help', 'back'. This makes the 'back' button the last control on the page.
    // Create the buttons in the right order...
    wxButton *btnHelp=0;
#ifdef __WXMAC__
    if (GetExtraStyle() & wxWIZARD_EX_HELPBUTTON)
        btnHelp=new wxButton(this, wxID_HELP, _("&Help"), wxDefaultPosition, wxDefaultSize, buttonStyle);
#endif

    m_btnNext = new wxButton(this, wxID_FORWARD, _("&Next >"));
    wxButton *btnCancel=new wxButton(this, wxID_CANCEL, _("&Cancel"), wxDefaultPosition, wxDefaultSize, buttonStyle);
#ifndef __WXMAC__
    if (GetExtraStyle() & wxWIZARD_EX_HELPBUTTON)
        btnHelp=new wxButton(this, wxID_HELP, _("&Help"), wxDefaultPosition, wxDefaultSize, buttonStyle);
#endif
    m_btnPrev = new wxButton(this, wxID_BACKWARD, _("< &Back"), wxDefaultPosition, wxDefaultSize, buttonStyle);

    if (btnHelp)
    {
        buttonRow->Add(
            btnHelp,
            0, // Horizontally unstretchable
            wxALL, // Border all around, top aligned
            5 // Border width
            );
#ifdef __WXMAC__
        // Put stretchable space between help button and others
        buttonRow->Add(0, 0, 1, wxALIGN_CENTRE, 0);
#endif
    }

    AddBackNextPair(buttonRow);

    buttonRow->Add(
        btnCancel,
        0, // Horizontally unstretchable
        wxALL, // Border all around, top aligned
        5 // Border width
    );
}

void wxWizard::DoCreateControls()
{
    // do nothing if the controls were already created
    if ( WasCreated() )
        return;

    bool isPda = (wxSystemSettings::GetScreenType() <= wxSYS_SCREEN_PDA);

    // Horizontal stretching, and if not PDA, border all around
    int mainColumnSizerFlags = isPda ? wxEXPAND : wxALL|wxEXPAND ;

    // wxWindow::SetSizer will be called at end
    wxBoxSizer *windowSizer = new wxBoxSizer(wxVERTICAL);

    wxBoxSizer *mainColumn = new wxBoxSizer(wxVERTICAL);
    windowSizer->Add(
        mainColumn,
        1, // Vertical stretching
        mainColumnSizerFlags,
        5 // Border width
    );

    AddBitmapRow(mainColumn);

    if (!isPda)
        AddStaticLine(mainColumn);

    AddButtonRow(mainColumn);

    SetSizer(windowSizer);
}

void wxWizard::SetPageSize(const wxSize& size)
{
    wxCHECK_RET(!m_started, wxT("wxWizard::SetPageSize after RunWizard"));
    m_sizePage = size;
}

void wxWizard::FitToPage(const wxWizardPage *page)
{
    wxCHECK_RET(!m_started, wxT("wxWizard::FitToPage after RunWizard"));

    while ( page )
    {
        wxSize size = page->GetBestSize();

        m_sizePage.IncTo(size);

        page = page->GetNext();
    }
}

bool wxWizard::ShowPage(wxWizardPage *page, bool goingForward)
{
    wxASSERT_MSG( page != m_page, wxT("this is useless") );

    wxSizerFlags flags(1);
    flags.Border(wxALL, m_border).Expand();

    if ( !m_started )
    {
        if ( m_usingSizer )
        {
            m_sizerBmpAndPage->Add(m_sizerPage, flags);

            // now that our layout is computed correctly, hide the pages
            // artificially shown in wxWizardSizer::Insert() back again
            m_sizerPage->HidePages();
        }
    }


    // we'll use this to decide whether we have to change the label of this
    // button or not (initially the label is "Next")
    bool btnLabelWasNext = true;

    // remember the old bitmap (if any) to compare with the new one later
    wxBitmap bmpPrev;

    // check for previous page
    if ( m_page )
    {
        // send the event to the old page
        wxWizardEvent event(wxEVT_WIZARD_PAGE_CHANGING, GetId(),
                            goingForward, m_page);
        if ( m_page->GetEventHandler()->ProcessEvent(event) &&
             !event.IsAllowed() )
        {
            // vetoed by the page
            return false;
        }

        m_page->Hide();

        btnLabelWasNext = HasNextPage(m_page);

        bmpPrev = m_page->GetBitmap();

        if ( !m_usingSizer )
            m_sizerBmpAndPage->Detach(m_page);
    }

    // set the new page
    m_page = page;

    // is this the end?
    if ( !m_page )
    {
        // terminate successfully
        if ( IsModal() )
        {
            EndModal(wxID_OK);
        }
        else
        {
            SetReturnCode(wxID_OK);
            Hide();
        }

        // and notify the user code (this is especially useful for modeless
        // wizards)
        wxWizardEvent event(wxEVT_WIZARD_FINISHED, GetId(), false, 0);
        (void)GetEventHandler()->ProcessEvent(event);

        return true;
    }

    // position and show the new page
    (void)m_page->TransferDataToWindow();

    if ( m_usingSizer )
    {
        // wxWizardSizer::RecalcSizes wants to be called when m_page changes
        m_sizerPage->RecalcSizes();
    }
    else // pages are not managed by the sizer
    {
        m_sizerBmpAndPage->Add(m_page, flags);
        m_sizerBmpAndPage->SetItemMinSize(m_page, GetPageSize());
    }

#if wxUSE_STATBMP
    // update the bitmap if:it changed
    if ( m_statbmp )
    {
        wxBitmap bmp = m_page->GetBitmap();
        if ( !bmp.Ok() )
            bmp = m_bitmap;

        if ( !bmpPrev.Ok() )
            bmpPrev = m_bitmap;

        if ( !bmp.IsSameAs(bmpPrev) )
            m_statbmp->SetBitmap(bmp);
    }
#endif // wxUSE_STATBMP


    // and update the buttons state
    m_btnPrev->Enable(HasPrevPage(m_page));

    bool hasNext = HasNextPage(m_page);
    if ( btnLabelWasNext != hasNext )
    {
        m_btnNext->SetLabel(hasNext ? _("&Next >") : _("&Finish"));
    }
    // nothing to do: the label was already correct

    m_btnNext->SetDefault();


    // send the change event to the new page now
    wxWizardEvent event(wxEVT_WIZARD_PAGE_CHANGED, GetId(), goingForward, m_page);
    (void)m_page->GetEventHandler()->ProcessEvent(event);

    // and finally show it
    m_page->Show();
    m_page->SetFocus();

    if ( !m_usingSizer )
        m_sizerBmpAndPage->Layout();

    if ( !m_started )
    {
        m_started = true;

        if ( wxSystemSettings::GetScreenType() > wxSYS_SCREEN_PDA )
        {
            GetSizer()->SetSizeHints(this);
            if ( m_posWizard == wxDefaultPosition )
                CentreOnScreen();
        }
    }

    wxWizardEvent pageShownEvent(wxEVT_WIZARD_PAGE_SHOWN, GetId(),
        goingForward, m_page);
    m_page->GetEventHandler()->ProcessEvent(pageShownEvent);

    return true;
}

bool wxWizard::RunWizard(wxWizardPage *firstPage)
{
    wxCHECK_MSG( firstPage, false, wxT("can't run empty wizard") );

    // can't return false here because there is no old page
    (void)ShowPage(firstPage, true /* forward */);

    m_wasModal = true;

    return ShowModal() == wxID_OK;
}

wxWizardPage *wxWizard::GetCurrentPage() const
{
    return m_page;
}

wxSize wxWizard::GetPageSize() const
{
    // default width and height of the page
    int DEFAULT_PAGE_WIDTH,
        DEFAULT_PAGE_HEIGHT;
    if ( wxSystemSettings::GetScreenType() <= wxSYS_SCREEN_PDA )
    {
        // Make the default page size small enough to fit on screen
        DEFAULT_PAGE_WIDTH = wxSystemSettings::GetMetric(wxSYS_SCREEN_X) / 2;
        DEFAULT_PAGE_HEIGHT = wxSystemSettings::GetMetric(wxSYS_SCREEN_Y) / 2;
    }
    else // !PDA
    {
        DEFAULT_PAGE_WIDTH =
        DEFAULT_PAGE_HEIGHT = 270;
    }

    // start with default minimal size
    wxSize pageSize(DEFAULT_PAGE_WIDTH, DEFAULT_PAGE_HEIGHT);

    // make the page at least as big as specified by user
    pageSize.IncTo(m_sizePage);

    if ( m_statbmp )
    {
        // make the page at least as tall as the bitmap
        pageSize.IncTo(wxSize(0, m_bitmap.GetHeight()));
    }

    if ( m_usingSizer )
    {
        // make it big enough to contain all pages added to the sizer
        pageSize.IncTo(m_sizerPage->GetMaxChildSize());
    }

    return pageSize;
}

wxSizer *wxWizard::GetPageAreaSizer() const
{
    return m_sizerPage;
}

void wxWizard::SetBorder(int border)
{
    wxCHECK_RET(!m_started, wxT("wxWizard::SetBorder after RunWizard"));

    m_border = border;
}

void wxWizard::OnCancel(wxCommandEvent& WXUNUSED(eventUnused))
{
    // this function probably can never be called when we don't have an active
    // page, but a small extra check won't hurt
    wxWindow *win = m_page ? (wxWindow *)m_page : (wxWindow *)this;

    wxWizardEvent event(wxEVT_WIZARD_CANCEL, GetId(), false, m_page);
    if ( !win->GetEventHandler()->ProcessEvent(event) || event.IsAllowed() )
    {
        // no objections - close the dialog
        if(IsModal())
        {
            EndModal(wxID_CANCEL);
        }
        else
        {
            SetReturnCode(wxID_CANCEL);
            Hide();
        }
    }
    //else: request to Cancel ignored
}

void wxWizard::OnBackOrNext(wxCommandEvent& event)
{
    wxASSERT_MSG( (event.GetEventObject() == m_btnNext) ||
                  (event.GetEventObject() == m_btnPrev),
                  wxT("unknown button") );

    wxCHECK_RET( m_page, _T("should have a valid current page") );

    // ask the current page first: notice that we do it before calling
    // GetNext/Prev() because the data transfered from the controls of the page
    // may change the value returned by these methods
    if ( !m_page->Validate() || !m_page->TransferDataFromWindow() )
    {
        // the page data is incorrect, don't do anything
        return;
    }

    bool forward = event.GetEventObject() == m_btnNext;

    wxWizardPage *page;
    if ( forward )
    {
        page = m_page->GetNext();
    }
    else // back
    {
        page = m_page->GetPrev();

        wxASSERT_MSG( page, wxT("\"<Back\" button should have been disabled") );
    }

    // just pass to the new page (or maybe not - but we don't care here)
    (void)ShowPage(page, forward);
}

void wxWizard::OnHelp(wxCommandEvent& WXUNUSED(event))
{
    // this function probably can never be called when we don't have an active
    // page, but a small extra check won't hurt
    if(m_page != NULL)
    {
        // Create and send the help event to the specific page handler
        // event data contains the active page so that context-sensitive
        // help is possible
        wxWizardEvent eventHelp(wxEVT_WIZARD_HELP, GetId(), true, m_page);
        (void)m_page->GetEventHandler()->ProcessEvent(eventHelp);
    }
}

void wxWizard::OnWizEvent(wxWizardEvent& event)
{
    // the dialogs have wxWS_EX_BLOCK_EVENTS style on by default but we want to
    // propagate wxEVT_WIZARD_XXX to the parent (if any), so do it manually
    if ( !(GetExtraStyle() & wxWS_EX_BLOCK_EVENTS) )
    {
        // the event will be propagated anyhow
        event.Skip();
    }
    else
    {
        wxWindow *parent = GetParent();

        if ( !parent || !parent->GetEventHandler()->ProcessEvent(event) )
        {
            event.Skip();
        }
    }

    if ( ( !m_wasModal ) &&
         event.IsAllowed() &&
         ( event.GetEventType() == wxEVT_WIZARD_FINISHED ||
           event.GetEventType() == wxEVT_WIZARD_CANCEL
         )
       )
    {
        Destroy();
    }
}

void wxWizard::SetBitmap(const wxBitmap& bitmap)
{
    m_bitmap = bitmap;
    if (m_statbmp)
        m_statbmp->SetBitmap(m_bitmap);
}

// ----------------------------------------------------------------------------
// wxWizardEvent
// ----------------------------------------------------------------------------

wxWizardEvent::wxWizardEvent(wxEventType type, int id, bool direction, wxWizardPage* page)
             : wxNotifyEvent(type, id)
{
    // Modified 10-20-2001 Robert Cavanaugh
    // add the active page to the event data
    m_direction = direction;
    m_page = page;
}

#endif // wxUSE_WIZARDDLG
