/////////////////////////////////////////////////////////////////////////////
// Name:        src/common/window.cpp
// Purpose:     common (to all ports) wxWindow functions
// Author:      Julian Smart, Vadim Zeitlin
// Modified by:
// Created:     13/07/98
// RCS-ID:      $Id: wincmn.cpp 63032 2009-12-31 13:37:24Z VZ $
// Copyright:   (c) wxWidgets team
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

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

#ifndef WX_PRECOMP
    #include "wx/string.h"
    #include "wx/log.h"
    #include "wx/intl.h"
    #include "wx/frame.h"
    #include "wx/window.h"
    #include "wx/control.h"
    #include "wx/checkbox.h"
    #include "wx/radiobut.h"
    #include "wx/statbox.h"
    #include "wx/textctrl.h"
    #include "wx/settings.h"
    #include "wx/dialog.h"
    #include "wx/msgdlg.h"
    #include "wx/statusbr.h"
    #include "wx/toolbar.h"
    #include "wx/dcclient.h"
    #include "wx/scrolbar.h"
    #include "wx/layout.h"
    #include "wx/sizer.h"
#endif //WX_PRECOMP

#if wxUSE_DRAG_AND_DROP
    #include "wx/dnd.h"
#endif // wxUSE_DRAG_AND_DROP

#if wxUSE_ACCESSIBILITY
    #include "wx/access.h"
#endif

#if wxUSE_HELP
    #include "wx/cshelp.h"
#endif // wxUSE_HELP

#if wxUSE_TOOLTIPS
    #include "wx/tooltip.h"
#endif // wxUSE_TOOLTIPS

#if wxUSE_CARET
    #include "wx/caret.h"
#endif // wxUSE_CARET

#if wxUSE_SYSTEM_OPTIONS
    #include "wx/sysopt.h"
#endif

// For reporting compile- and runtime version of GTK+ in the ctrl+alt+mclick dialog.
// The gtk includes don't pull any other headers in, at least not on my system - MR
#ifdef __WXGTK__
    #ifdef __WXGTK20__
        #include <gtk/gtkversion.h>
    #else
        #include <gtk/gtkfeatures.h>
    #endif
    extern const unsigned int gtk_major_version;
    extern const unsigned int gtk_minor_version;
    extern const unsigned int gtk_micro_version;
#endif

#include "wx/platinfo.h"

// Windows List
WXDLLIMPEXP_DATA_CORE(wxWindowList) wxTopLevelWindows;

// ----------------------------------------------------------------------------
// static data
// ----------------------------------------------------------------------------

#if defined(__WXPALMOS__)
int wxWindowBase::ms_lastControlId = 32767;
#elif defined(__WXPM__)
int wxWindowBase::ms_lastControlId = 2000;
#else
int wxWindowBase::ms_lastControlId = -200;
#endif

IMPLEMENT_ABSTRACT_CLASS(wxWindowBase, wxEvtHandler)

// ----------------------------------------------------------------------------
// event table
// ----------------------------------------------------------------------------

BEGIN_EVENT_TABLE(wxWindowBase, wxEvtHandler)
    EVT_SYS_COLOUR_CHANGED(wxWindowBase::OnSysColourChanged)
    EVT_INIT_DIALOG(wxWindowBase::OnInitDialog)
    EVT_MIDDLE_DOWN(wxWindowBase::OnMiddleClick)

#if wxUSE_HELP
    EVT_HELP(wxID_ANY, wxWindowBase::OnHelp)
#endif // wxUSE_HELP

END_EVENT_TABLE()

// ============================================================================
// implementation of the common functionality of the wxWindow class
// ============================================================================

// ----------------------------------------------------------------------------
// initialization
// ----------------------------------------------------------------------------

// the default initialization
wxWindowBase::wxWindowBase()
{
    // no window yet, no parent nor children
    m_parent = (wxWindow *)NULL;
    m_windowId = wxID_ANY;

    // no constraints on the minimal window size
    m_minWidth =
    m_maxWidth = wxDefaultCoord;
    m_minHeight =
    m_maxHeight = wxDefaultCoord;

    // invalidiated cache value
    m_bestSizeCache = wxDefaultSize;

    // window are created enabled and visible by default
    m_isShown =
    m_isEnabled = true;

    // the default event handler is just this window
    m_eventHandler = this;

#if wxUSE_VALIDATORS
    // no validator
    m_windowValidator = (wxValidator *) NULL;
#endif // wxUSE_VALIDATORS

    // the colours/fonts are default for now, so leave m_font,
    // m_backgroundColour and m_foregroundColour uninitialized and set those
    m_hasBgCol =
    m_hasFgCol =
    m_hasFont = false;
    m_inheritBgCol =
    m_inheritFgCol =
    m_inheritFont = false;

    // no style bits
    m_exStyle =
    m_windowStyle = 0;

    m_backgroundStyle = wxBG_STYLE_SYSTEM;

#if wxUSE_CONSTRAINTS
    // no constraints whatsoever
    m_constraints = (wxLayoutConstraints *) NULL;
    m_constraintsInvolvedIn = (wxWindowList *) NULL;
#endif // wxUSE_CONSTRAINTS

    m_windowSizer = (wxSizer *) NULL;
    m_containingSizer = (wxSizer *) NULL;
    m_autoLayout = false;

#if wxUSE_DRAG_AND_DROP
    m_dropTarget = (wxDropTarget *)NULL;
#endif // wxUSE_DRAG_AND_DROP

#if wxUSE_TOOLTIPS
    m_tooltip = (wxToolTip *)NULL;
#endif // wxUSE_TOOLTIPS

#if wxUSE_CARET
    m_caret = (wxCaret *)NULL;
#endif // wxUSE_CARET

#if wxUSE_PALETTE
    m_hasCustomPalette = false;
#endif // wxUSE_PALETTE

#if wxUSE_ACCESSIBILITY
    m_accessible = NULL;
#endif

    m_virtualSize = wxDefaultSize;

    m_scrollHelper = (wxScrollHelper *) NULL;

    m_minVirtualWidth =
    m_maxVirtualWidth = wxDefaultCoord;
    m_minVirtualHeight =
    m_maxVirtualHeight = wxDefaultCoord;

    m_windowVariant = wxWINDOW_VARIANT_NORMAL;
#if wxUSE_SYSTEM_OPTIONS
    if ( wxSystemOptions::HasOption(wxWINDOW_DEFAULT_VARIANT) )
    {
       m_windowVariant = (wxWindowVariant) wxSystemOptions::GetOptionInt( wxWINDOW_DEFAULT_VARIANT ) ;
    }
#endif

    // Whether we're using the current theme for this window (wxGTK only for now)
    m_themeEnabled = false;

    // VZ: this one shouldn't exist...
    m_isBeingDeleted = false;
}

// common part of window creation process
bool wxWindowBase::CreateBase(wxWindowBase *parent,
                              wxWindowID id,
                              const wxPoint& WXUNUSED(pos),
                              const wxSize& WXUNUSED(size),
                              long style,
                              const wxValidator& wxVALIDATOR_PARAM(validator),
                              const wxString& name)
{
#if wxUSE_STATBOX
    // wxGTK doesn't allow to create controls with static box as the parent so
    // this will result in a crash when the program is ported to wxGTK so warn
    // the user about it

    // if you get this assert, the correct solution is to create the controls
    // as siblings of the static box
    wxASSERT_MSG( !parent || !wxDynamicCast(parent, wxStaticBox),
                  _T("wxStaticBox can't be used as a window parent!") );
#endif // wxUSE_STATBOX

    // ids are limited to 16 bits under MSW so if you care about portability,
    // it's not a good idea to use ids out of this range (and negative ids are
    // reserved for wxWidgets own usage)
    wxASSERT_MSG( id == wxID_ANY || (id >= 0 && id < 32767),
                  _T("invalid id value") );

    // generate a new id if the user doesn't care about it
    m_windowId = id == wxID_ANY ? NewControlId() : id;

    // don't use SetWindowStyleFlag() here, this function should only be called
    // to change the flag after creation as it tries to reflect the changes in
    // flags by updating the window dynamically and we don't need this here
    m_windowStyle = style;

    SetName(name);
    SetParent(parent);

#if wxUSE_VALIDATORS
    SetValidator(validator);
#endif // wxUSE_VALIDATORS

    // if the parent window has wxWS_EX_VALIDATE_RECURSIVELY set, we want to
    // have it too - like this it's possible to set it only in the top level
    // dialog/frame and all children will inherit it by defult
    if ( parent && (parent->GetExtraStyle() & wxWS_EX_VALIDATE_RECURSIVELY) )
    {
        SetExtraStyle(GetExtraStyle() | wxWS_EX_VALIDATE_RECURSIVELY);
    }

    return true;
}

bool wxWindowBase::ToggleWindowStyle(int flag)
{
    wxASSERT_MSG( flag, _T("flags with 0 value can't be toggled") );

    bool rc;
    long style = GetWindowStyleFlag();
    if ( style & flag )
    {
        style &= ~flag;
        rc = false;
    }
    else // currently off
    {
        style |= flag;
        rc = true;
    }

    SetWindowStyleFlag(style);

    return rc;
}

// ----------------------------------------------------------------------------
// destruction
// ----------------------------------------------------------------------------

// common clean up
wxWindowBase::~wxWindowBase()
{
    wxASSERT_MSG( GetCapture() != this, wxT("attempt to destroy window with mouse capture") );

    // FIXME if these 2 cases result from programming errors in the user code
    //       we should probably assert here instead of silently fixing them

    // Just in case the window has been Closed, but we're then deleting
    // immediately: don't leave dangling pointers.
    wxPendingDelete.DeleteObject(this);

    // Just in case we've loaded a top-level window via LoadNativeDialog but
    // we weren't a dialog class
    wxTopLevelWindows.DeleteObject((wxWindow*)this);

    wxASSERT_MSG( GetChildren().GetCount() == 0, wxT("children not destroyed") );

    // reset the top-level parent's default item if it is this widget
    if ( m_parent )
    {
        wxTopLevelWindow *tlw = wxDynamicCast(wxGetTopLevelParent((wxWindow*)this),
                                              wxTopLevelWindow);

        if ( tlw )
        {
            wxWindow* tmpDefaultItem = tlw->GetTmpDefaultItem();
            if ( tmpDefaultItem == this )
                tlw->SetTmpDefaultItem(NULL);
            else if ( tmpDefaultItem )
            {
                // A temporary default item masks the real default item, so
                // temporarily unset the temporary default item so we can access the
                // real default item.
                tlw->SetTmpDefaultItem(NULL);

                if ( tlw->GetDefaultItem() == this )
                    tlw->SetDefaultItem(NULL);

                // Set the temporary default item back.
                tlw->SetTmpDefaultItem(tmpDefaultItem);
            }
            else if ( tlw->GetDefaultItem() == this )
                tlw->SetDefaultItem(NULL);
        }
    }

    // reset the dangling pointer our parent window may keep to us
    if ( m_parent )
    {
        m_parent->RemoveChild(this);
    }

#if wxUSE_CARET
    delete m_caret;
#endif // wxUSE_CARET

#if wxUSE_VALIDATORS
    delete m_windowValidator;
#endif // wxUSE_VALIDATORS

#if wxUSE_CONSTRAINTS
    // Have to delete constraints/sizer FIRST otherwise sizers may try to look
    // at deleted windows as they delete themselves.
    DeleteRelatedConstraints();

    if ( m_constraints )
    {
        // This removes any dangling pointers to this window in other windows'
        // constraintsInvolvedIn lists.
        UnsetConstraints(m_constraints);
        delete m_constraints;
        m_constraints = NULL;
    }
#endif // wxUSE_CONSTRAINTS

    if ( m_containingSizer )
        m_containingSizer->Detach( (wxWindow*)this );

    delete m_windowSizer;

#if wxUSE_DRAG_AND_DROP
    delete m_dropTarget;
#endif // wxUSE_DRAG_AND_DROP

#if wxUSE_TOOLTIPS
    delete m_tooltip;
#endif // wxUSE_TOOLTIPS

#if wxUSE_ACCESSIBILITY
    delete m_accessible;
#endif

#if wxUSE_HELP
    // NB: this has to be called unconditionally, because we don't
    //     know whether this window has associated help text or not
    wxHelpProvider *helpProvider = wxHelpProvider::Get();
    if ( helpProvider )
        helpProvider->RemoveHelp(this);
#endif
}

void wxWindowBase::SendDestroyEvent()
{
    wxWindowDestroyEvent event;
    event.SetEventObject(this);
    event.SetId(GetId());
    GetEventHandler()->ProcessEvent(event);
}

bool wxWindowBase::Destroy()
{
    delete this;

    return true;
}

bool wxWindowBase::Close(bool force)
{
    wxCloseEvent event(wxEVT_CLOSE_WINDOW, m_windowId);
    event.SetEventObject(this);
    event.SetCanVeto(!force);

    // return false if window wasn't closed because the application vetoed the
    // close event
    return GetEventHandler()->ProcessEvent(event) && !event.GetVeto();
}

bool wxWindowBase::DestroyChildren()
{
    wxWindowList::compatibility_iterator node;
    for ( ;; )
    {
        // we iterate until the list becomes empty
        node = GetChildren().GetFirst();
        if ( !node )
            break;

        wxWindow *child = node->GetData();

        // note that we really want to call delete and not ->Destroy() here
        // because we want to delete the child immediately, before we are
        // deleted, and delayed deletion would result in problems as our (top
        // level) child could outlive its parent
        delete child;

        wxASSERT_MSG( !GetChildren().Find(child),
                      wxT("child didn't remove itself using RemoveChild()") );
    }

    return true;
}

// ----------------------------------------------------------------------------
// size/position related methods
// ----------------------------------------------------------------------------

// centre the window with respect to its parent in either (or both) directions
void wxWindowBase::DoCentre(int dir)
{
    wxCHECK_RET( !(dir & wxCENTRE_ON_SCREEN) && GetParent(),
                 _T("this method only implements centering child windows") );

    SetSize(GetRect().CentreIn(GetParent()->GetClientSize(), dir));
}

// fits the window around the children
void wxWindowBase::Fit()
{
    if ( !GetChildren().empty() )
    {
        SetSize(GetBestSize());
    }
    //else: do nothing if we have no children
}

// fits virtual size (ie. scrolled area etc.) around children
void wxWindowBase::FitInside()
{
    if ( GetChildren().GetCount() > 0 )
    {
        SetVirtualSize( GetBestVirtualSize() );
    }
}

// On Mac, scrollbars are explicitly children.
#ifdef __WXMAC__
static bool wxHasRealChildren(const wxWindowBase* win)
{
    int realChildCount = 0;

    for ( wxWindowList::compatibility_iterator node = win->GetChildren().GetFirst();
          node;
          node = node->GetNext() )
    {
        wxWindow *win = node->GetData();
        if ( !win->IsTopLevel() && win->IsShown() && !win->IsKindOf(CLASSINFO(wxScrollBar)))
            realChildCount ++;
    }
    return (realChildCount > 0);
}
#endif

void wxWindowBase::InvalidateBestSize()
{
    m_bestSizeCache = wxDefaultSize;

    // parent's best size calculation may depend on its children's
    // as long as child window we are in is not top level window itself
    // (because the TLW size is never resized automatically)
    // so let's invalidate it as well to be safe:
    if (m_parent && !IsTopLevel())
        m_parent->InvalidateBestSize();
}

// return the size best suited for the current window
wxSize wxWindowBase::DoGetBestSize() const
{
    wxSize best;

    if ( m_windowSizer )
    {
        // Adjust to window size, since the return value of GetWindowSizeForVirtualSize is
        // expressed in window and not client size
        wxSize minSize = m_windowSizer->GetMinSize();
        wxSize size(GetSize());
        wxSize clientSize(GetClientSize());

        wxSize minWindowSize(minSize.x + size.x - clientSize.x,
                             minSize.y + size.y - clientSize.y);

        best = GetWindowSizeForVirtualSize(minWindowSize);

        return best;
    }
#if wxUSE_CONSTRAINTS
    else if ( m_constraints )
    {
        wxConstCast(this, wxWindowBase)->SatisfyConstraints();

        // our minimal acceptable size is such that all our windows fit inside
        int maxX = 0,
            maxY = 0;

        for ( wxWindowList::compatibility_iterator node = GetChildren().GetFirst();
              node;
              node = node->GetNext() )
        {
            wxLayoutConstraints *c = node->GetData()->GetConstraints();
            if ( !c )
            {
                // it's not normal that we have an unconstrained child, but
                // what can we do about it?
                continue;
            }

            int x = c->right.GetValue(),
                y = c->bottom.GetValue();

            if ( x > maxX )
                maxX = x;

            if ( y > maxY )
                maxY = y;

            // TODO: we must calculate the overlaps somehow, otherwise we
            //       will never return a size bigger than the current one :-(
        }

        best = wxSize(maxX, maxY);
    }
#endif // wxUSE_CONSTRAINTS
    else if ( !GetChildren().empty()
#ifdef __WXMAC__
              && wxHasRealChildren(this)
#endif
              )
    {
        // our minimal acceptable size is such that all our visible child
        // windows fit inside
        int maxX = 0,
            maxY = 0;

        for ( wxWindowList::compatibility_iterator node = GetChildren().GetFirst();
              node;
              node = node->GetNext() )
        {
            wxWindow *win = node->GetData();
            if ( win->IsTopLevel()
                    || !win->IsShown()
#if wxUSE_STATUSBAR
                        || wxDynamicCast(win, wxStatusBar)
#endif // wxUSE_STATUSBAR
               )
            {
                // dialogs and frames lie in different top level windows -
                // don't deal with them here; as for the status bars, they
                // don't lie in the client area at all
                continue;
            }

            int wx, wy, ww, wh;
            win->GetPosition(&wx, &wy);

            // if the window hadn't been positioned yet, assume that it is in
            // the origin
            if ( wx == wxDefaultCoord )
                wx = 0;
            if ( wy == wxDefaultCoord )
                wy = 0;

            win->GetSize(&ww, &wh);
            if ( wx + ww > maxX )
                maxX = wx + ww;
            if ( wy + wh > maxY )
                maxY = wy + wh;
        }

        best = wxSize(maxX, maxY);
    }
    else // ! has children
    {
        // for a generic window there is no natural best size so, if the
        // minimal size is not set, use the current size but take care to
        // remember it as minimal size for the next time because our best size
        // should be constant: otherwise we could get into a situation when the
        // window is initially at some size, then expanded to a larger size and
        // then, when the containing window is shrunk back (because our initial
        // best size had been used for computing the parent min size), we can't
        // be shrunk back any more because our best size is now bigger
        wxSize size = GetMinSize();
        if ( !size.IsFullySpecified() )
        {
            size.SetDefaults(GetSize());
            wxConstCast(this, wxWindowBase)->SetMinSize(size);
        }

        // return as-is, unadjusted by the client size difference.
        return size;
    }

    // Add any difference between size and client size
    wxSize diff = GetSize() - GetClientSize();
    best.x += wxMax(0, diff.x);
    best.y += wxMax(0, diff.y);

    return best;
}

// helper of GetWindowBorderSize(): as many ports don't implement support for
// wxSYS_BORDER/EDGE_X/Y metrics in their wxSystemSettings, use hard coded
// fallbacks in this case
static int wxGetMetricOrDefault(wxSystemMetric what)
{
    int rc = wxSystemSettings::GetMetric(what);
    if ( rc == -1 )
    {
        switch ( what )
        {
            case wxSYS_BORDER_X:
            case wxSYS_BORDER_Y:
                // 2D border is by default 1 pixel wide
                rc = 1;
                break;

            case wxSYS_EDGE_X:
            case wxSYS_EDGE_Y:
                // 3D borders are by default 2 pixels
                rc = 2;
                break;

            default:
                wxFAIL_MSG( _T("unexpected wxGetMetricOrDefault() argument") );
                rc = 0;
        }
    }

    return rc;
}

wxSize wxWindowBase::GetWindowBorderSize() const
{
    wxSize size;

    switch ( GetBorder() )
    {
        case wxBORDER_NONE:
            // nothing to do, size is already (0, 0)
            break;

        case wxBORDER_SIMPLE:
        case wxBORDER_STATIC:
            size.x = wxGetMetricOrDefault(wxSYS_BORDER_X);
            size.y = wxGetMetricOrDefault(wxSYS_BORDER_Y);
            break;

        case wxBORDER_SUNKEN:
        case wxBORDER_RAISED:
            size.x = wxMax(wxGetMetricOrDefault(wxSYS_EDGE_X),
                           wxGetMetricOrDefault(wxSYS_BORDER_X));
            size.y = wxMax(wxGetMetricOrDefault(wxSYS_EDGE_Y),
                           wxGetMetricOrDefault(wxSYS_BORDER_Y));
            break;

        case wxBORDER_DOUBLE:
            size.x = wxGetMetricOrDefault(wxSYS_EDGE_X) +
                        wxGetMetricOrDefault(wxSYS_BORDER_X);
            size.y = wxGetMetricOrDefault(wxSYS_EDGE_Y) +
                        wxGetMetricOrDefault(wxSYS_BORDER_Y);
            break;

        default:
            wxFAIL_MSG(_T("Unknown border style."));
            break;
    }

    // we have borders on both sides
    return size*2;
}

wxSize wxWindowBase::GetEffectiveMinSize() const
{
    // merge the best size with the min size, giving priority to the min size
    wxSize min = GetMinSize();
    if (min.x == wxDefaultCoord || min.y == wxDefaultCoord)
    {
        wxSize best = GetBestSize();
        if (min.x == wxDefaultCoord) min.x =  best.x;
        if (min.y == wxDefaultCoord) min.y =  best.y;
    }
    return min;
}


void wxWindowBase::SetInitialSize(const wxSize& size)
{
    // Set the min size to the size passed in.  This will usually either be
    // wxDefaultSize or the size passed to this window's ctor/Create function.
    SetMinSize(size);

    // Merge the size with the best size if needed
    wxSize best = GetEffectiveMinSize();

    // If the current size doesn't match then change it
    if (GetSize() != best)
        SetSize(best);
}


// by default the origin is not shifted
wxPoint wxWindowBase::GetClientAreaOrigin() const
{
    return wxPoint(0,0);
}

wxSize wxWindowBase::ClientToWindowSize(const wxSize& size) const
{
    const wxSize diff(GetSize() - GetClientSize());

    return wxSize(size.x == -1 ? -1 : size.x + diff.x,
                  size.y == -1 ? -1 : size.y + diff.y);
}

wxSize wxWindowBase::WindowToClientSize(const wxSize& size) const
{
    const wxSize diff(GetSize() - GetClientSize());

    return wxSize(size.x == -1 ? -1 : size.x - diff.x,
                  size.y == -1 ? -1 : size.y - diff.y);
}

void wxWindowBase::SetWindowVariant( wxWindowVariant variant )
{
    if ( m_windowVariant != variant )
    {
        m_windowVariant = variant;

        DoSetWindowVariant(variant);
    }
}

void wxWindowBase::DoSetWindowVariant( wxWindowVariant variant )
{
    // adjust the font height to correspond to our new variant (notice that
    // we're only called if something really changed)
    wxFont font = GetFont();
    int size = font.GetPointSize();
    switch ( variant )
    {
        case wxWINDOW_VARIANT_NORMAL:
            break;

        case wxWINDOW_VARIANT_SMALL:
            size *= 3;
            size /= 4;
            break;

        case wxWINDOW_VARIANT_MINI:
            size *= 2;
            size /= 3;
            break;

        case wxWINDOW_VARIANT_LARGE:
            size *= 5;
            size /= 4;
            break;

        default:
            wxFAIL_MSG(_T("unexpected window variant"));
            break;
    }

    font.SetPointSize(size);
    SetFont(font);
}

void wxWindowBase::DoSetSizeHints( int minW, int minH,
                                   int maxW, int maxH,
                                   int WXUNUSED(incW), int WXUNUSED(incH) )
{
    wxCHECK_RET( (minW == wxDefaultCoord || maxW == wxDefaultCoord || minW <= maxW) &&
                    (minH == wxDefaultCoord || maxH == wxDefaultCoord || minH <= maxH),
                 _T("min width/height must be less than max width/height!") );

    m_minWidth = minW;
    m_maxWidth = maxW;
    m_minHeight = minH;
    m_maxHeight = maxH;
}


void wxWindowBase::SetVirtualSizeHints( int minW, int minH,
                                        int maxW, int maxH )
{
    m_minVirtualWidth = minW;
    m_maxVirtualWidth = maxW;
    m_minVirtualHeight = minH;
    m_maxVirtualHeight = maxH;
}

void wxWindowBase::DoSetVirtualSize( int x, int y )
{
    if ( m_minVirtualWidth != wxDefaultCoord && m_minVirtualWidth > x )
        x = m_minVirtualWidth;
    if ( m_maxVirtualWidth != wxDefaultCoord && m_maxVirtualWidth < x )
        x = m_maxVirtualWidth;
    if ( m_minVirtualHeight != wxDefaultCoord && m_minVirtualHeight > y )
        y = m_minVirtualHeight;
    if ( m_maxVirtualHeight != wxDefaultCoord && m_maxVirtualHeight < y )
        y = m_maxVirtualHeight;

    m_virtualSize = wxSize(x, y);
}

wxSize wxWindowBase::DoGetVirtualSize() const
{
    // we should use the entire client area so if it is greater than our
    // virtual size, expand it to fit (otherwise if the window is big enough we
    // wouldn't be using parts of it)
    wxSize size = GetClientSize();
    if ( m_virtualSize.x > size.x )
        size.x = m_virtualSize.x;

    if ( m_virtualSize.y >= size.y )
        size.y = m_virtualSize.y;

    return size;
}

void wxWindowBase::DoGetScreenPosition(int *x, int *y) const
{
    // screen position is the same as (0, 0) in client coords for non TLWs (and
    // TLWs override this method)
    if ( x )
        *x = 0;
    if ( y )
        *y = 0;

    ClientToScreen(x, y);
}

// ----------------------------------------------------------------------------
// show/hide/enable/disable the window
// ----------------------------------------------------------------------------

bool wxWindowBase::Show(bool show)
{
    if ( show != m_isShown )
    {
        m_isShown = show;

        return true;
    }
    else
    {
        return false;
    }
}

bool wxWindowBase::Enable(bool enable)
{
    if ( enable != m_isEnabled )
    {
        m_isEnabled = enable;

        return true;
    }
    else
    {
        return false;
    }
}

bool wxWindowBase::IsShownOnScreen() const
{
    // A window is shown on screen if it itself is shown and so are all its
    // parents. But if a window is toplevel one, then its always visible on
    // screen if IsShown() returns true, even if it has a hidden parent.
    return IsShown() &&
           (IsTopLevel() || !GetParent() || GetParent()->IsShownOnScreen());
}

// ----------------------------------------------------------------------------
// RTTI
// ----------------------------------------------------------------------------

bool wxWindowBase::IsTopLevel() const
{
    return false;
}

// ----------------------------------------------------------------------------
// reparenting the window
// ----------------------------------------------------------------------------

void wxWindowBase::AddChild(wxWindowBase *child)
{
    wxCHECK_RET( child, wxT("can't add a NULL child") );

    // this should never happen and it will lead to a crash later if it does
    // because RemoveChild() will remove only one node from the children list
    // and the other(s) one(s) will be left with dangling pointers in them
    wxASSERT_MSG( !GetChildren().Find((wxWindow*)child), _T("AddChild() called twice") );

    GetChildren().Append((wxWindow*)child);
    child->SetParent(this);
}

void wxWindowBase::RemoveChild(wxWindowBase *child)
{
    wxCHECK_RET( child, wxT("can't remove a NULL child") );

    GetChildren().DeleteObject((wxWindow *)child);
    child->SetParent(NULL);
}

bool wxWindowBase::Reparent(wxWindowBase *newParent)
{
    wxWindow *oldParent = GetParent();
    if ( newParent == oldParent )
    {
        // nothing done
        return false;
    }

    // unlink this window from the existing parent.
    if ( oldParent )
    {
        oldParent->RemoveChild(this);
    }
    else
    {
        wxTopLevelWindows.DeleteObject((wxWindow *)this);
    }

    // add it to the new one
    if ( newParent )
    {
        newParent->AddChild(this);
    }
    else
    {
        wxTopLevelWindows.Append((wxWindow *)this);
    }

    return true;
}

// ----------------------------------------------------------------------------
// event handler stuff
// ----------------------------------------------------------------------------

void wxWindowBase::PushEventHandler(wxEvtHandler *handler)
{
    wxEvtHandler *handlerOld = GetEventHandler();

    handler->SetNextHandler(handlerOld);

    if ( handlerOld )
        GetEventHandler()->SetPreviousHandler(handler);

    SetEventHandler(handler);
}

wxEvtHandler *wxWindowBase::PopEventHandler(bool deleteHandler)
{
    wxEvtHandler *handlerA = GetEventHandler();
    if ( handlerA )
    {
        wxEvtHandler *handlerB = handlerA->GetNextHandler();
        handlerA->SetNextHandler((wxEvtHandler *)NULL);

        if ( handlerB )
            handlerB->SetPreviousHandler((wxEvtHandler *)NULL);
        SetEventHandler(handlerB);

        if ( deleteHandler )
        {
            delete handlerA;
            handlerA = (wxEvtHandler *)NULL;
        }
    }

    return handlerA;
}

bool wxWindowBase::RemoveEventHandler(wxEvtHandler *handler)
{
    wxCHECK_MSG( handler, false, _T("RemoveEventHandler(NULL) called") );

    wxEvtHandler *handlerPrev = NULL,
                 *handlerCur = GetEventHandler();
    while ( handlerCur )
    {
        wxEvtHandler *handlerNext = handlerCur->GetNextHandler();

        if ( handlerCur == handler )
        {
            if ( handlerPrev )
            {
                handlerPrev->SetNextHandler(handlerNext);
            }
            else
            {
                SetEventHandler(handlerNext);
            }

            if ( handlerNext )
            {
                handlerNext->SetPreviousHandler ( handlerPrev );
            }

            handler->SetNextHandler(NULL);
            handler->SetPreviousHandler(NULL);

            return true;
        }

        handlerPrev = handlerCur;
        handlerCur = handlerNext;
    }

    wxFAIL_MSG( _T("where has the event handler gone?") );

    return false;
}

// ----------------------------------------------------------------------------
// colours, fonts &c
// ----------------------------------------------------------------------------

void wxWindowBase::InheritAttributes()
{
    const wxWindowBase * const parent = GetParent();
    if ( !parent )
        return;

    // we only inherit attributes which had been explicitly set for the parent
    // which ensures that this only happens if the user really wants it and
    // not by default which wouldn't make any sense in modern GUIs where the
    // controls don't all use the same fonts (nor colours)
    if ( parent->m_inheritFont && !m_hasFont )
        SetFont(parent->GetFont());

    // in addition, there is a possibility to explicitly forbid inheriting
    // colours at each class level by overriding ShouldInheritColours()
    if ( ShouldInheritColours() )
    {
        if ( parent->m_inheritFgCol && !m_hasFgCol )
            SetForegroundColour(parent->GetForegroundColour());

        // inheriting (solid) background colour is wrong as it totally breaks
        // any kind of themed backgrounds
        //
        // instead, the controls should use the same background as their parent
        // (ideally by not drawing it at all)
#if 0
        if ( parent->m_inheritBgCol && !m_hasBgCol )
            SetBackgroundColour(parent->GetBackgroundColour());
#endif // 0
    }
}

/* static */ wxVisualAttributes
wxWindowBase::GetClassDefaultAttributes(wxWindowVariant WXUNUSED(variant))
{
    // it is important to return valid values for all attributes from here,
    // GetXXX() below rely on this
    wxVisualAttributes attrs;
    attrs.font = wxSystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT);
    attrs.colFg = wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOWTEXT);

    // On Smartphone/PocketPC, wxSYS_COLOUR_WINDOW is a better reflection of
    // the usual background colour than wxSYS_COLOUR_BTNFACE.
    // It's a pity that wxSYS_COLOUR_WINDOW isn't always a suitable background
    // colour on other platforms.

#if defined(__WXWINCE__) && (defined(__SMARTPHONE__) || defined(__POCKETPC__))
    attrs.colBg = wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW);
#else
    attrs.colBg = wxSystemSettings::GetColour(wxSYS_COLOUR_BTNFACE);
#endif
    return attrs;
}

wxColour wxWindowBase::GetBackgroundColour() const
{
    if ( !m_backgroundColour.Ok() )
    {
        wxASSERT_MSG( !m_hasBgCol, _T("we have invalid explicit bg colour?") );

        // get our default background colour
        wxColour colBg = GetDefaultAttributes().colBg;

        // we must return some valid colour to avoid redoing this every time
        // and also to avoid surprizing the applications written for older
        // wxWidgets versions where GetBackgroundColour() always returned
        // something -- so give them something even if it doesn't make sense
        // for this window (e.g. it has a themed background)
        if ( !colBg.Ok() )
            colBg = GetClassDefaultAttributes().colBg;

        return colBg;
    }
    else
        return m_backgroundColour;
}

wxColour wxWindowBase::GetForegroundColour() const
{
    // logic is the same as above
    if ( !m_hasFgCol && !m_foregroundColour.Ok() )
    {
        wxASSERT_MSG( !m_hasFgCol, _T("we have invalid explicit fg colour?") );

        wxColour colFg = GetDefaultAttributes().colFg;

        if ( !colFg.Ok() )
            colFg = GetClassDefaultAttributes().colFg;

        return colFg;
    }
    else
        return m_foregroundColour;
}

bool wxWindowBase::SetBackgroundColour( const wxColour &colour )
{
    if ( colour == m_backgroundColour )
        return false;

    m_hasBgCol = colour.Ok();
    if ( m_backgroundStyle != wxBG_STYLE_CUSTOM )
        m_backgroundStyle = m_hasBgCol ? wxBG_STYLE_COLOUR : wxBG_STYLE_SYSTEM;

    m_inheritBgCol = m_hasBgCol;
    m_backgroundColour = colour;
    SetThemeEnabled( !m_hasBgCol && !m_foregroundColour.Ok() );
    return true;
}

bool wxWindowBase::SetForegroundColour( const wxColour &colour )
{
    if (colour == m_foregroundColour )
        return false;

    m_hasFgCol = colour.Ok();
    m_inheritFgCol = m_hasFgCol;
    m_foregroundColour = colour;
    SetThemeEnabled( !m_hasFgCol && !m_backgroundColour.Ok() );
    return true;
}

bool wxWindowBase::SetCursor(const wxCursor& cursor)
{
    // setting an invalid cursor is ok, it means that we don't have any special
    // cursor
    if ( m_cursor.IsSameAs(cursor) )
    {
        // no change
        return false;
    }

    m_cursor = cursor;

    return true;
}

wxFont wxWindowBase::GetFont() const
{
    // logic is the same as in GetBackgroundColour()
    if ( !m_font.Ok() )
    {
        wxASSERT_MSG( !m_hasFont, _T("we have invalid explicit font?") );

        wxFont font = GetDefaultAttributes().font;
        if ( !font.Ok() )
            font = GetClassDefaultAttributes().font;

        return font;
    }
    else
        return m_font;
}

bool wxWindowBase::SetFont(const wxFont& font)
{
    if ( font == m_font )
    {
        // no change
        return false;
    }

    m_font = font;
    m_hasFont = font.Ok();
    m_inheritFont = m_hasFont;

    InvalidateBestSize();

    return true;
}

#if wxUSE_PALETTE

void wxWindowBase::SetPalette(const wxPalette& pal)
{
    m_hasCustomPalette = true;
    m_palette = pal;

    // VZ: can anyone explain me what do we do here?
    wxWindowDC d((wxWindow *) this);
    d.SetPalette(pal);
}

wxWindow *wxWindowBase::GetAncestorWithCustomPalette() const
{
    wxWindow *win = (wxWindow *)this;
    while ( win && !win->HasCustomPalette() )
    {
        win = win->GetParent();
    }

    return win;
}

#endif // wxUSE_PALETTE

#if wxUSE_CARET
void wxWindowBase::SetCaret(wxCaret *caret)
{
    if ( m_caret )
    {
        delete m_caret;
    }

    m_caret = caret;

    if ( m_caret )
    {
        wxASSERT_MSG( m_caret->GetWindow() == this,
                      wxT("caret should be created associated to this window") );
    }
}
#endif // wxUSE_CARET

#if wxUSE_VALIDATORS
// ----------------------------------------------------------------------------
// validators
// ----------------------------------------------------------------------------

void wxWindowBase::SetValidator(const wxValidator& validator)
{
    if ( m_windowValidator )
        delete m_windowValidator;

    m_windowValidator = (wxValidator *)validator.Clone();

    if ( m_windowValidator )
        m_windowValidator->SetWindow(this);
}
#endif // wxUSE_VALIDATORS

// ----------------------------------------------------------------------------
// update region stuff
// ----------------------------------------------------------------------------

wxRect wxWindowBase::GetUpdateClientRect() const
{
    wxRegion rgnUpdate = GetUpdateRegion();
    rgnUpdate.Intersect(GetClientRect());
    wxRect rectUpdate = rgnUpdate.GetBox();
    wxPoint ptOrigin = GetClientAreaOrigin();
    rectUpdate.x -= ptOrigin.x;
    rectUpdate.y -= ptOrigin.y;

    return rectUpdate;
}

bool wxWindowBase::DoIsExposed(int x, int y) const
{
    return m_updateRegion.Contains(x, y) != wxOutRegion;
}

bool wxWindowBase::DoIsExposed(int x, int y, int w, int h) const
{
    return m_updateRegion.Contains(x, y, w, h) != wxOutRegion;
}

void wxWindowBase::ClearBackground()
{
    // wxGTK uses its own version, no need to add never used code
#ifndef __WXGTK__
    wxClientDC dc((wxWindow *)this);
    wxBrush brush(GetBackgroundColour(), wxSOLID);
    dc.SetBackground(brush);
    dc.Clear();
#endif // __WXGTK__
}

// ----------------------------------------------------------------------------
// find child window by id or name
// ----------------------------------------------------------------------------

wxWindow *wxWindowBase::FindWindow(long id) const
{
    if ( id == m_windowId )
        return (wxWindow *)this;

    wxWindowBase *res = (wxWindow *)NULL;
    wxWindowList::compatibility_iterator node;
    for ( node = m_children.GetFirst(); node && !res; node = node->GetNext() )
    {
        wxWindowBase *child = node->GetData();
        res = child->FindWindow( id );
    }

    return (wxWindow *)res;
}

wxWindow *wxWindowBase::FindWindow(const wxString& name) const
{
    if ( name == m_windowName )
        return (wxWindow *)this;

    wxWindowBase *res = (wxWindow *)NULL;
    wxWindowList::compatibility_iterator node;
    for ( node = m_children.GetFirst(); node && !res; node = node->GetNext() )
    {
        wxWindow *child = node->GetData();
        res = child->FindWindow(name);
    }

    return (wxWindow *)res;
}


// find any window by id or name or label: If parent is non-NULL, look through
// children for a label or title matching the specified string. If NULL, look
// through all top-level windows.
//
// to avoid duplicating code we reuse the same helper function but with
// different comparators

typedef bool (*wxFindWindowCmp)(const wxWindow *win,
                                const wxString& label, long id);

static
bool wxFindWindowCmpLabels(const wxWindow *win, const wxString& label,
                           long WXUNUSED(id))
{
    return win->GetLabel() == label;
}

static
bool wxFindWindowCmpNames(const wxWindow *win, const wxString& label,
                          long WXUNUSED(id))
{
    return win->GetName() == label;
}

static
bool wxFindWindowCmpIds(const wxWindow *win, const wxString& WXUNUSED(label),
                        long id)
{
    return win->GetId() == id;
}

// recursive helper for the FindWindowByXXX() functions
static
wxWindow *wxFindWindowRecursively(const wxWindow *parent,
                                  const wxString& label,
                                  long id,
                                  wxFindWindowCmp cmp)
{
    if ( parent )
    {
        // see if this is the one we're looking for
        if ( (*cmp)(parent, label, id) )
            return (wxWindow *)parent;

        // It wasn't, so check all its children
        for ( wxWindowList::compatibility_iterator node = parent->GetChildren().GetFirst();
              node;
              node = node->GetNext() )
        {
            // recursively check each child
            wxWindow *win = (wxWindow *)node->GetData();
            wxWindow *retwin = wxFindWindowRecursively(win, label, id, cmp);
            if (retwin)
                return retwin;
        }
    }

    // Not found
    return NULL;
}

// helper for FindWindowByXXX()
static
wxWindow *wxFindWindowHelper(const wxWindow *parent,
                             const wxString& label,
                             long id,
                             wxFindWindowCmp cmp)
{
    if ( parent )
    {
        // just check parent and all its children
        return wxFindWindowRecursively(parent, label, id, cmp);
    }

    // start at very top of wx's windows
    for ( wxWindowList::compatibility_iterator node = wxTopLevelWindows.GetFirst();
          node;
          node = node->GetNext() )
    {
        // recursively check each window & its children
        wxWindow *win = node->GetData();
        wxWindow *retwin = wxFindWindowRecursively(win, label, id, cmp);
        if (retwin)
            return retwin;
    }

    return NULL;
}

/* static */
wxWindow *
wxWindowBase::FindWindowByLabel(const wxString& title, const wxWindow *parent)
{
    return wxFindWindowHelper(parent, title, 0, wxFindWindowCmpLabels);
}

/* static */
wxWindow *
wxWindowBase::FindWindowByName(const wxString& title, const wxWindow *parent)
{
    wxWindow *win = wxFindWindowHelper(parent, title, 0, wxFindWindowCmpNames);

    if ( !win )
    {
        // fall back to the label
        win = FindWindowByLabel(title, parent);
    }

    return win;
}

/* static */
wxWindow *
wxWindowBase::FindWindowById( long id, const wxWindow* parent )
{
    return wxFindWindowHelper(parent, wxEmptyString, id, wxFindWindowCmpIds);
}

// ----------------------------------------------------------------------------
// dialog oriented functions
// ----------------------------------------------------------------------------

void wxWindowBase::MakeModal(bool modal)
{
    // Disable all other windows
    if ( IsTopLevel() )
    {
        wxWindowList::compatibility_iterator node = wxTopLevelWindows.GetFirst();
        while (node)
        {
            wxWindow *win = node->GetData();
            if (win != this)
                win->Enable(!modal);

            node = node->GetNext();
        }
    }
}

bool wxWindowBase::Validate()
{
#if wxUSE_VALIDATORS
    bool recurse = (GetExtraStyle() & wxWS_EX_VALIDATE_RECURSIVELY) != 0;

    wxWindowList::compatibility_iterator node;
    for ( node = m_children.GetFirst(); node; node = node->GetNext() )
    {
        wxWindowBase *child = node->GetData();
        wxValidator *validator = child->GetValidator();
        if ( validator && !validator->Validate((wxWindow *)this) )
        {
            return false;
        }

        if ( recurse && !child->Validate() )
        {
            return false;
        }
    }
#endif // wxUSE_VALIDATORS

    return true;
}

bool wxWindowBase::TransferDataToWindow()
{
#if wxUSE_VALIDATORS
    bool recurse = (GetExtraStyle() & wxWS_EX_VALIDATE_RECURSIVELY) != 0;

    wxWindowList::compatibility_iterator node;
    for ( node = m_children.GetFirst(); node; node = node->GetNext() )
    {
        wxWindowBase *child = node->GetData();
        wxValidator *validator = child->GetValidator();
        if ( validator && !validator->TransferToWindow() )
        {
            wxLogWarning(_("Could not transfer data to window"));
#if wxUSE_LOG
            wxLog::FlushActive();
#endif // wxUSE_LOG

            return false;
        }

        if ( recurse )
        {
            if ( !child->TransferDataToWindow() )
            {
                // warning already given
                return false;
            }
        }
    }
#endif // wxUSE_VALIDATORS

    return true;
}

bool wxWindowBase::TransferDataFromWindow()
{
#if wxUSE_VALIDATORS
    bool recurse = (GetExtraStyle() & wxWS_EX_VALIDATE_RECURSIVELY) != 0;

    wxWindowList::compatibility_iterator node;
    for ( node = m_children.GetFirst(); node; node = node->GetNext() )
    {
        wxWindow *child = node->GetData();
        wxValidator *validator = child->GetValidator();
        if ( validator && !validator->TransferFromWindow() )
        {
            // nop warning here because the application is supposed to give
            // one itself - we don't know here what might have gone wrongly

            return false;
        }

        if ( recurse )
        {
            if ( !child->TransferDataFromWindow() )
            {
                // warning already given
                return false;
            }
        }
    }
#endif // wxUSE_VALIDATORS

    return true;
}

void wxWindowBase::InitDialog()
{
    wxInitDialogEvent event(GetId());
    event.SetEventObject( this );
    GetEventHandler()->ProcessEvent(event);
}

// ----------------------------------------------------------------------------
// context-sensitive help support
// ----------------------------------------------------------------------------

#if wxUSE_HELP

// associate this help text with this window
void wxWindowBase::SetHelpText(const wxString& text)
{
    wxHelpProvider *helpProvider = wxHelpProvider::Get();
    if ( helpProvider )
    {
        helpProvider->AddHelp(this, text);
    }
}

// associate this help text with all windows with the same id as this
// one
void wxWindowBase::SetHelpTextForId(const wxString& text)
{
    wxHelpProvider *helpProvider = wxHelpProvider::Get();
    if ( helpProvider )
    {
        helpProvider->AddHelp(GetId(), text);
    }
}

// get the help string associated with this window (may be empty)
// default implementation forwards calls to the help provider
wxString
wxWindowBase::GetHelpTextAtPoint(const wxPoint & WXUNUSED(pt),
                                 wxHelpEvent::Origin WXUNUSED(origin)) const
{
    wxString text;
    wxHelpProvider *helpProvider = wxHelpProvider::Get();
    if ( helpProvider )
    {
        text = helpProvider->GetHelp(this);
    }

    return text;
}

// show help for this window
void wxWindowBase::OnHelp(wxHelpEvent& event)
{
    wxHelpProvider *helpProvider = wxHelpProvider::Get();
    if ( helpProvider )
    {
        wxPoint pos = event.GetPosition();
        const wxHelpEvent::Origin origin = event.GetOrigin();
        if ( origin == wxHelpEvent::Origin_Keyboard )
        {
            // if the help event was generated from keyboard it shouldn't
            // appear at the mouse position (which is still the only position
            // associated with help event) if the mouse is far away, although
            // we still do use the mouse position if it's over the window
            // because we suppose the user looks approximately at the mouse
            // already and so it would be more convenient than showing tooltip
            // at some arbitrary position which can be quite far from it
            const wxRect rectClient = GetClientRect();
            if ( !rectClient.Contains(ScreenToClient(pos)) )
            {
                // position help slightly under and to the right of this window
                pos = ClientToScreen(wxPoint(
                        2*GetCharWidth(),
                        rectClient.height + GetCharHeight()
                      ));
            }
        }

        if ( helpProvider->ShowHelpAtPoint(this, pos, origin) )
        {
            // skip the event.Skip() below
            return;
        }
    }

    event.Skip();
}

#endif // wxUSE_HELP

// ----------------------------------------------------------------------------
// tooltips
// ----------------------------------------------------------------------------

#if wxUSE_TOOLTIPS

void wxWindowBase::SetToolTip( const wxString &tip )
{
    // don't create the new tooltip if we already have one
    if ( m_tooltip )
    {
        m_tooltip->SetTip( tip );
    }
    else
    {
        SetToolTip( new wxToolTip( tip ) );
    }

    // setting empty tooltip text does not remove the tooltip any more - use
    // SetToolTip((wxToolTip *)NULL) for this
}

void wxWindowBase::DoSetToolTip(wxToolTip *tooltip)
{
    if ( m_tooltip != tooltip )
    {
        if ( m_tooltip )
            delete m_tooltip;

        m_tooltip = tooltip;
    }
}

#endif // wxUSE_TOOLTIPS

// ----------------------------------------------------------------------------
// constraints and sizers
// ----------------------------------------------------------------------------

#if wxUSE_CONSTRAINTS

void wxWindowBase::SetConstraints( wxLayoutConstraints *constraints )
{
    if ( m_constraints )
    {
        UnsetConstraints(m_constraints);
        delete m_constraints;
    }
    m_constraints = constraints;
    if ( m_constraints )
    {
        // Make sure other windows know they're part of a 'meaningful relationship'
        if ( m_constraints->left.GetOtherWindow() && (m_constraints->left.GetOtherWindow() != this) )
            m_constraints->left.GetOtherWindow()->AddConstraintReference(this);
        if ( m_constraints->top.GetOtherWindow() && (m_constraints->top.GetOtherWindow() != this) )
            m_constraints->top.GetOtherWindow()->AddConstraintReference(this);
        if ( m_constraints->right.GetOtherWindow() && (m_constraints->right.GetOtherWindow() != this) )
            m_constraints->right.GetOtherWindow()->AddConstraintReference(this);
        if ( m_constraints->bottom.GetOtherWindow() && (m_constraints->bottom.GetOtherWindow() != this) )
            m_constraints->bottom.GetOtherWindow()->AddConstraintReference(this);
        if ( m_constraints->width.GetOtherWindow() && (m_constraints->width.GetOtherWindow() != this) )
            m_constraints->width.GetOtherWindow()->AddConstraintReference(this);
        if ( m_constraints->height.GetOtherWindow() && (m_constraints->height.GetOtherWindow() != this) )
            m_constraints->height.GetOtherWindow()->AddConstraintReference(this);
        if ( m_constraints->centreX.GetOtherWindow() && (m_constraints->centreX.GetOtherWindow() != this) )
            m_constraints->centreX.GetOtherWindow()->AddConstraintReference(this);
        if ( m_constraints->centreY.GetOtherWindow() && (m_constraints->centreY.GetOtherWindow() != this) )
            m_constraints->centreY.GetOtherWindow()->AddConstraintReference(this);
    }
}

// This removes any dangling pointers to this window in other windows'
// constraintsInvolvedIn lists.
void wxWindowBase::UnsetConstraints(wxLayoutConstraints *c)
{
    if ( c )
    {
        if ( c->left.GetOtherWindow() && (c->top.GetOtherWindow() != this) )
            c->left.GetOtherWindow()->RemoveConstraintReference(this);
        if ( c->top.GetOtherWindow() && (c->top.GetOtherWindow() != this) )
            c->top.GetOtherWindow()->RemoveConstraintReference(this);
        if ( c->right.GetOtherWindow() && (c->right.GetOtherWindow() != this) )
            c->right.GetOtherWindow()->RemoveConstraintReference(this);
        if ( c->bottom.GetOtherWindow() && (c->bottom.GetOtherWindow() != this) )
            c->bottom.GetOtherWindow()->RemoveConstraintReference(this);
        if ( c->width.GetOtherWindow() && (c->width.GetOtherWindow() != this) )
            c->width.GetOtherWindow()->RemoveConstraintReference(this);
        if ( c->height.GetOtherWindow() && (c->height.GetOtherWindow() != this) )
            c->height.GetOtherWindow()->RemoveConstraintReference(this);
        if ( c->centreX.GetOtherWindow() && (c->centreX.GetOtherWindow() != this) )
            c->centreX.GetOtherWindow()->RemoveConstraintReference(this);
        if ( c->centreY.GetOtherWindow() && (c->centreY.GetOtherWindow() != this) )
            c->centreY.GetOtherWindow()->RemoveConstraintReference(this);
    }
}

// Back-pointer to other windows we're involved with, so if we delete this
// window, we must delete any constraints we're involved with.
void wxWindowBase::AddConstraintReference(wxWindowBase *otherWin)
{
    if ( !m_constraintsInvolvedIn )
        m_constraintsInvolvedIn = new wxWindowList;
    if ( !m_constraintsInvolvedIn->Find((wxWindow *)otherWin) )
        m_constraintsInvolvedIn->Append((wxWindow *)otherWin);
}

// REMOVE back-pointer to other windows we're involved with.
void wxWindowBase::RemoveConstraintReference(wxWindowBase *otherWin)
{
    if ( m_constraintsInvolvedIn )
        m_constraintsInvolvedIn->DeleteObject((wxWindow *)otherWin);
}

// Reset any constraints that mention this window
void wxWindowBase::DeleteRelatedConstraints()
{
    if ( m_constraintsInvolvedIn )
    {
        wxWindowList::compatibility_iterator node = m_constraintsInvolvedIn->GetFirst();
        while (node)
        {
            wxWindow *win = node->GetData();
            wxLayoutConstraints *constr = win->GetConstraints();

            // Reset any constraints involving this window
            if ( constr )
            {
                constr->left.ResetIfWin(this);
                constr->top.ResetIfWin(this);
                constr->right.ResetIfWin(this);
                constr->bottom.ResetIfWin(this);
                constr->width.ResetIfWin(this);
                constr->height.ResetIfWin(this);
                constr->centreX.ResetIfWin(this);
                constr->centreY.ResetIfWin(this);
            }

            wxWindowList::compatibility_iterator next = node->GetNext();
            m_constraintsInvolvedIn->Erase(node);
            node = next;
        }

        delete m_constraintsInvolvedIn;
        m_constraintsInvolvedIn = (wxWindowList *) NULL;
    }
}

#endif // wxUSE_CONSTRAINTS

void wxWindowBase::SetSizer(wxSizer *sizer, bool deleteOld)
{
    if ( sizer == m_windowSizer)
        return;

    if ( m_windowSizer )
    {
        m_windowSizer->SetContainingWindow(NULL);

        if ( deleteOld )
            delete m_windowSizer;
    }

    m_windowSizer = sizer;
    if ( m_windowSizer )
    {
        m_windowSizer->SetContainingWindow((wxWindow *)this);
    }

    SetAutoLayout(m_windowSizer != NULL);
}

void wxWindowBase::SetSizerAndFit(wxSizer *sizer, bool deleteOld)
{
    SetSizer( sizer, deleteOld );
    sizer->SetSizeHints( (wxWindow*) this );
}


void wxWindowBase::SetContainingSizer(wxSizer* sizer)
{
    // adding a window to a sizer twice is going to result in fatal and
    // hard to debug problems later because when deleting the second
    // associated wxSizerItem we're going to dereference a dangling
    // pointer; so try to detect this as early as possible
    wxASSERT_MSG( !sizer || m_containingSizer != sizer,
                  _T("Adding a window to the same sizer twice?") );

    m_containingSizer = sizer;
}

#if wxUSE_CONSTRAINTS

void wxWindowBase::SatisfyConstraints()
{
    wxLayoutConstraints *constr = GetConstraints();
    bool wasOk = constr && constr->AreSatisfied();

    ResetConstraints();   // Mark all constraints as unevaluated

    int noChanges = 1;

    // if we're a top level panel (i.e. our parent is frame/dialog), our
    // own constraints will never be satisfied any more unless we do it
    // here
    if ( wasOk )
    {
        while ( noChanges > 0 )
        {
            LayoutPhase1(&noChanges);
        }
    }

    LayoutPhase2(&noChanges);
}

#endif // wxUSE_CONSTRAINTS

bool wxWindowBase::Layout()
{
    // If there is a sizer, use it instead of the constraints
    if ( GetSizer() )
    {
        int w = 0, h = 0;
        GetVirtualSize(&w, &h);
        GetSizer()->SetDimension( 0, 0, w, h );
    }
#if wxUSE_CONSTRAINTS
    else
    {
        SatisfyConstraints(); // Find the right constraints values
        SetConstraintSizes(); // Recursively set the real window sizes
    }
#endif

    return true;
}

#if wxUSE_CONSTRAINTS

// first phase of the constraints evaluation: set our own constraints
bool wxWindowBase::LayoutPhase1(int *noChanges)
{
    wxLayoutConstraints *constr = GetConstraints();

    return !constr || constr->SatisfyConstraints(this, noChanges);
}

// second phase: set the constraints for our children
bool wxWindowBase::LayoutPhase2(int *noChanges)
{
    *noChanges = 0;

    // Layout children
    DoPhase(1);

    // Layout grand children
    DoPhase(2);

    return true;
}

// Do a phase of evaluating child constraints
bool wxWindowBase::DoPhase(int phase)
{
    // the list containing the children for which the constraints are already
    // set correctly
    wxWindowList succeeded;

    // the max number of iterations we loop before concluding that we can't set
    // the constraints
    static const int maxIterations = 500;

    for ( int noIterations = 0; noIterations < maxIterations; noIterations++ )
    {
        int noChanges = 0;

        // loop over all children setting their constraints
        for ( wxWindowList::compatibility_iterator node = GetChildren().GetFirst();
              node;
              node = node->GetNext() )
        {
            wxWindow *child = node->GetData();
            if ( child->IsTopLevel() )
            {
                // top level children are not inside our client area
                continue;
            }

            if ( !child->GetConstraints() || succeeded.Find(child) )
            {
                // this one is either already ok or nothing we can do about it
                continue;
            }

            int tempNoChanges = 0;
            bool success = phase == 1 ? child->LayoutPhase1(&tempNoChanges)
                                      : child->LayoutPhase2(&tempNoChanges);
            noChanges += tempNoChanges;

            if ( success )
            {
                succeeded.Append(child);
            }
        }

        if ( !noChanges )
        {
            // constraints are set
            break;
        }
    }

    return true;
}

void wxWindowBase::ResetConstraints()
{
    wxLayoutConstraints *constr = GetConstraints();
    if ( constr )
    {
        constr->left.SetDone(false);
        constr->top.SetDone(false);
        constr->right.SetDone(false);
        constr->bottom.SetDone(false);
        constr->width.SetDone(false);
        constr->height.SetDone(false);
        constr->centreX.SetDone(false);
        constr->centreY.SetDone(false);
    }

    wxWindowList::compatibility_iterator node = GetChildren().GetFirst();
    while (node)
    {
        wxWindow *win = node->GetData();
        if ( !win->IsTopLevel() )
            win->ResetConstraints();
        node = node->GetNext();
    }
}

// Need to distinguish between setting the 'fake' size for windows and sizers,
// and setting the real values.
void wxWindowBase::SetConstraintSizes(bool recurse)
{
    wxLayoutConstraints *constr = GetConstraints();
    if ( constr && constr->AreSatisfied() )
    {
        int x = constr->left.GetValue();
        int y = constr->top.GetValue();
        int w = constr->width.GetValue();
        int h = constr->height.GetValue();

        if ( (constr->width.GetRelationship() != wxAsIs ) ||
             (constr->height.GetRelationship() != wxAsIs) )
        {
            SetSize(x, y, w, h);
        }
        else
        {
            // If we don't want to resize this window, just move it...
            Move(x, y);
        }
    }
    else if ( constr )
    {
        wxLogDebug(wxT("Constraints not satisfied for %s named '%s'."),
                   GetClassInfo()->GetClassName(),
                   GetName().c_str());
    }

    if ( recurse )
    {
        wxWindowList::compatibility_iterator node = GetChildren().GetFirst();
        while (node)
        {
            wxWindow *win = node->GetData();
            if ( !win->IsTopLevel() && win->GetConstraints() )
                win->SetConstraintSizes();
            node = node->GetNext();
        }
    }
}

// Only set the size/position of the constraint (if any)
void wxWindowBase::SetSizeConstraint(int x, int y, int w, int h)
{
    wxLayoutConstraints *constr = GetConstraints();
    if ( constr )
    {
        if ( x != wxDefaultCoord )
        {
            constr->left.SetValue(x);
            constr->left.SetDone(true);
        }
        if ( y != wxDefaultCoord )
        {
            constr->top.SetValue(y);
            constr->top.SetDone(true);
        }
        if ( w != wxDefaultCoord )
        {
            constr->width.SetValue(w);
            constr->width.SetDone(true);
        }
        if ( h != wxDefaultCoord )
        {
            constr->height.SetValue(h);
            constr->height.SetDone(true);
        }
    }
}

void wxWindowBase::MoveConstraint(int x, int y)
{
    wxLayoutConstraints *constr = GetConstraints();
    if ( constr )
    {
        if ( x != wxDefaultCoord )
        {
            constr->left.SetValue(x);
            constr->left.SetDone(true);
        }
        if ( y != wxDefaultCoord )
        {
            constr->top.SetValue(y);
            constr->top.SetDone(true);
        }
    }
}

void wxWindowBase::GetSizeConstraint(int *w, int *h) const
{
    wxLayoutConstraints *constr = GetConstraints();
    if ( constr )
    {
        *w = constr->width.GetValue();
        *h = constr->height.GetValue();
    }
    else
        GetSize(w, h);
}

void wxWindowBase::GetClientSizeConstraint(int *w, int *h) const
{
    wxLayoutConstraints *constr = GetConstraints();
    if ( constr )
    {
        *w = constr->width.GetValue();
        *h = constr->height.GetValue();
    }
    else
        GetClientSize(w, h);
}

void wxWindowBase::GetPositionConstraint(int *x, int *y) const
{
    wxLayoutConstraints *constr = GetConstraints();
    if ( constr )
    {
        *x = constr->left.GetValue();
        *y = constr->top.GetValue();
    }
    else
        GetPosition(x, y);
}

#endif // wxUSE_CONSTRAINTS

void wxWindowBase::AdjustForParentClientOrigin(int& x, int& y, int sizeFlags) const
{
    // don't do it for the dialogs/frames - they float independently of their
    // parent
    if ( !IsTopLevel() )
    {
        wxWindow *parent = GetParent();
        if ( !(sizeFlags & wxSIZE_NO_ADJUSTMENTS) && parent )
        {
            wxPoint pt(parent->GetClientAreaOrigin());
            x += pt.x;
            y += pt.y;
        }
    }
}

// ----------------------------------------------------------------------------
// Update UI processing
// ----------------------------------------------------------------------------

void wxWindowBase::UpdateWindowUI(long flags)
{
    wxUpdateUIEvent event(GetId());
    event.SetEventObject(this);

    if ( GetEventHandler()->ProcessEvent(event) )
    {
        DoUpdateWindowUI(event);
    }

    if (flags & wxUPDATE_UI_RECURSE)
    {
        wxWindowList::compatibility_iterator node = GetChildren().GetFirst();
        while (node)
        {
            wxWindow* child = (wxWindow*) node->GetData();
            child->UpdateWindowUI(flags);
            node = node->GetNext();
        }
    }
}

// do the window-specific processing after processing the update event
void wxWindowBase::DoUpdateWindowUI(wxUpdateUIEvent& event)
{
    if ( event.GetSetEnabled() )
        Enable(event.GetEnabled());

    if ( event.GetSetShown() )
        Show(event.GetShown());
}

// ----------------------------------------------------------------------------
// dialog units translations
// ----------------------------------------------------------------------------

wxPoint wxWindowBase::ConvertPixelsToDialog(const wxPoint& pt)
{
    int charWidth = GetCharWidth();
    int charHeight = GetCharHeight();
    wxPoint pt2 = wxDefaultPosition;
    if (pt.x != wxDefaultCoord)
        pt2.x = (int) ((pt.x * 4) / charWidth);
    if (pt.y != wxDefaultCoord)
        pt2.y = (int) ((pt.y * 8) / charHeight);

    return pt2;
}

wxPoint wxWindowBase::ConvertDialogToPixels(const wxPoint& pt)
{
    int charWidth = GetCharWidth();
    int charHeight = GetCharHeight();
    wxPoint pt2 = wxDefaultPosition;
    if (pt.x != wxDefaultCoord)
        pt2.x = (int) ((pt.x * charWidth) / 4);
    if (pt.y != wxDefaultCoord)
        pt2.y = (int) ((pt.y * charHeight) / 8);

    return pt2;
}

// ----------------------------------------------------------------------------
// event handlers
// ----------------------------------------------------------------------------

// propagate the colour change event to the subwindows
void wxWindowBase::OnSysColourChanged(wxSysColourChangedEvent& event)
{
    wxWindowList::compatibility_iterator node = GetChildren().GetFirst();
    while ( node )
    {
        // Only propagate to non-top-level windows
        wxWindow *win = node->GetData();
        if ( !win->IsTopLevel() )
        {
            wxSysColourChangedEvent event2;
            event.SetEventObject(win);
            win->GetEventHandler()->ProcessEvent(event2);
        }

        node = node->GetNext();
    }

    Refresh();
}

// the default action is to populate dialog with data when it's created,
// and nudge the UI into displaying itself correctly in case
// we've turned the wxUpdateUIEvents frequency down low.
void wxWindowBase::OnInitDialog( wxInitDialogEvent &WXUNUSED(event) )
{
    TransferDataToWindow();

    // Update the UI at this point
    UpdateWindowUI(wxUPDATE_UI_RECURSE);
}

// methods for drawing the sizers in a visible way
#ifdef __WXDEBUG__

static void DrawSizers(wxWindowBase *win);

static void DrawBorder(wxWindowBase *win, const wxRect& rect, bool fill = false)
{
    wxClientDC dc((wxWindow *)win);
    dc.SetPen(*wxRED_PEN);
    dc.SetBrush(fill ? wxBrush(*wxRED, wxCROSSDIAG_HATCH): *wxTRANSPARENT_BRUSH);
    dc.DrawRectangle(rect.Deflate(1, 1));
}

static void DrawSizer(wxWindowBase *win, wxSizer *sizer)
{
    const wxSizerItemList& items = sizer->GetChildren();
    for ( wxSizerItemList::const_iterator i = items.begin(),
                                        end = items.end();
          i != end;
          ++i )
    {
        wxSizerItem *item = *i;
        if ( item->IsSizer() )
        {
            DrawBorder(win, item->GetRect().Deflate(2));
            DrawSizer(win, item->GetSizer());
        }
        else if ( item->IsSpacer() )
        {
            DrawBorder(win, item->GetRect().Deflate(2), true);
        }
        else if ( item->IsWindow() )
        {
            DrawSizers(item->GetWindow());
        }
    }
}

static void DrawSizers(wxWindowBase *win)
{
    wxSizer *sizer = win->GetSizer();
    if ( sizer )
    {
        DrawBorder(win, win->GetClientSize());
        DrawSizer(win, sizer);
    }
    else // no sizer, still recurse into the children
    {
        const wxWindowList& children = win->GetChildren();
        for ( wxWindowList::const_iterator i = children.begin(),
                                         end = children.end();
              i != end;
              ++i )
        {
            DrawSizers(*i);
        }
    }
}

#endif // __WXDEBUG__

// process special middle clicks
void wxWindowBase::OnMiddleClick( wxMouseEvent& event )
{
    if ( event.ControlDown() && event.AltDown() )
    {
#ifdef __WXDEBUG__
        // Ctrl-Alt-Shift-mclick makes the sizers visible in debug builds
        if ( event.ShiftDown() )
        {
            DrawSizers(this);
            return;
        }
#endif // __WXDEBUG__

#if wxUSE_MSGDLG
        // don't translate these strings, they're for diagnostics purposes only
        wxString msg;
        msg.Printf(_T("wxWidgets Library (%s port)\n")
                   _T("Version %d.%d.%d%s%s, compiled at %s %s\n")
                   _T("Runtime version of toolkit used is %d.%d.%s\n")
                   _T("Copyright (c) 1995-2010 wxWidgets team"),
                   wxPlatformInfo::Get().GetPortIdName().c_str(),
                   wxMAJOR_VERSION,
                   wxMINOR_VERSION,
                   wxRELEASE_NUMBER,
#if wxUSE_UNICODE
                   L" (Unicode)",
#else
                   wxEmptyString,
#endif
#ifdef __WXDEBUG__
                   _T(" Debug build"),
#else
                   wxEmptyString,
#endif
                   __TDATE__,
                   __TTIME__,
                   wxPlatformInfo::Get().GetToolkitMajorVersion(),
                   wxPlatformInfo::Get().GetToolkitMinorVersion(),
#ifdef __WXGTK__
                   wxString::Format(_T("\nThe compile-time GTK+ version is %d.%d.%d."), GTK_MAJOR_VERSION, GTK_MINOR_VERSION, GTK_MICRO_VERSION).c_str()
#else
                   wxEmptyString
#endif
                   );

        wxMessageBox(msg, _T("wxWidgets information"),
                     wxICON_INFORMATION | wxOK,
                     (wxWindow *)this);
    }
    else
#endif // wxUSE_MSGDLG
    {
        event.Skip();
    }
}

// ----------------------------------------------------------------------------
// accessibility
// ----------------------------------------------------------------------------

#if wxUSE_ACCESSIBILITY
void wxWindowBase::SetAccessible(wxAccessible* accessible)
{
    if (m_accessible && (accessible != m_accessible))
        delete m_accessible;
    m_accessible = accessible;
    if (m_accessible)
        m_accessible->SetWindow((wxWindow*) this);
}

// Returns the accessible object, creating if necessary.
wxAccessible* wxWindowBase::GetOrCreateAccessible()
{
    if (!m_accessible)
        m_accessible = CreateAccessible();
    return m_accessible;
}

// Override to create a specific accessible object.
wxAccessible* wxWindowBase::CreateAccessible()
{
    return new wxWindowAccessible((wxWindow*) this);
}

#endif

// ----------------------------------------------------------------------------
// list classes implementation
// ----------------------------------------------------------------------------

#if wxUSE_STL

#include "wx/listimpl.cpp"
WX_DEFINE_LIST(wxWindowList)

#else // !wxUSE_STL

void wxWindowListNode::DeleteData()
{
    delete (wxWindow *)GetData();
}

#endif // wxUSE_STL/!wxUSE_STL

// ----------------------------------------------------------------------------
// borders
// ----------------------------------------------------------------------------

wxBorder wxWindowBase::GetBorder(long flags) const
{
    wxBorder border = (wxBorder)(flags & wxBORDER_MASK);
    if ( border == wxBORDER_DEFAULT )
    {
        border = GetDefaultBorder();
    }

    return border;
}

wxBorder wxWindowBase::GetDefaultBorder() const
{
    return wxBORDER_NONE;
}

// ----------------------------------------------------------------------------
// hit testing
// ----------------------------------------------------------------------------

wxHitTest wxWindowBase::DoHitTest(wxCoord x, wxCoord y) const
{
    // here we just check if the point is inside the window or not

    // check the top and left border first
    bool outside = x < 0 || y < 0;
    if ( !outside )
    {
        // check the right and bottom borders too
        wxSize size = GetSize();
        outside = x >= size.x || y >= size.y;
    }

    return outside ? wxHT_WINDOW_OUTSIDE : wxHT_WINDOW_INSIDE;
}

// ----------------------------------------------------------------------------
// mouse capture
// ----------------------------------------------------------------------------

struct WXDLLEXPORT wxWindowNext
{
    wxWindow *win;
    wxWindowNext *next;
} *wxWindowBase::ms_winCaptureNext = NULL;
wxWindow *wxWindowBase::ms_winCaptureCurrent = NULL;
bool wxWindowBase::ms_winCaptureChanging = false;

void wxWindowBase::CaptureMouse()
{
    wxLogTrace(_T("mousecapture"), _T("CaptureMouse(%p)"), wx_static_cast(void*, this));

    wxASSERT_MSG( !ms_winCaptureChanging, _T("recursive CaptureMouse call?") );

    ms_winCaptureChanging = true;

    wxWindow *winOld = GetCapture();
    if ( winOld )
    {
        ((wxWindowBase*) winOld)->DoReleaseMouse();

        // save it on stack
        wxWindowNext *item = new wxWindowNext;
        item->win = winOld;
        item->next = ms_winCaptureNext;
        ms_winCaptureNext = item;
    }
    //else: no mouse capture to save

    DoCaptureMouse();
    ms_winCaptureCurrent = (wxWindow*)this;

    ms_winCaptureChanging = false;
}

void wxWindowBase::ReleaseMouse()
{
    wxLogTrace(_T("mousecapture"), _T("ReleaseMouse(%p)"), wx_static_cast(void*, this));

    wxASSERT_MSG( !ms_winCaptureChanging, _T("recursive ReleaseMouse call?") );

    wxASSERT_MSG( GetCapture() == this, wxT("attempt to release mouse, but this window hasn't captured it") );

    ms_winCaptureChanging = true;

    DoReleaseMouse();
    ms_winCaptureCurrent = NULL;

    if ( ms_winCaptureNext )
    {
        ((wxWindowBase*)ms_winCaptureNext->win)->DoCaptureMouse();
        ms_winCaptureCurrent = ms_winCaptureNext->win;

        wxWindowNext *item = ms_winCaptureNext;
        ms_winCaptureNext = item->next;
        delete item;
    }
    //else: stack is empty, no previous capture

    ms_winCaptureChanging = false;

    wxLogTrace(_T("mousecapture"),
        (const wxChar *) _T("After ReleaseMouse() mouse is captured by %p"),
        wx_static_cast(void*, GetCapture()));
}

static void DoNotifyWindowAboutCaptureLost(wxWindow *win)
{
    wxMouseCaptureLostEvent event(win->GetId());
    event.SetEventObject(win);
    if ( !win->GetEventHandler()->ProcessEvent(event) )
    {
        // windows must handle this event, otherwise the app wouldn't behave
        // correctly if it loses capture unexpectedly; see the discussion here:
        // http://sourceforge.net/tracker/index.php?func=detail&aid=1153662&group_id=9863&atid=109863
        // http://article.gmane.org/gmane.comp.lib.wxwidgets.devel/82376
        wxFAIL_MSG( _T("window that captured the mouse didn't process wxEVT_MOUSE_CAPTURE_LOST") );
    }
}

/* static */
void wxWindowBase::NotifyCaptureLost()
{
    // don't do anything if capture lost was expected, i.e. resulted from
    // a wx call to ReleaseMouse or CaptureMouse:
    if ( ms_winCaptureChanging )
        return;

    // if the capture was lost unexpectedly, notify every window that has
    // capture (on stack or current) about it and clear the stack:

    if ( ms_winCaptureCurrent )
    {
        DoNotifyWindowAboutCaptureLost(ms_winCaptureCurrent);
        ms_winCaptureCurrent = NULL;
    }

    while ( ms_winCaptureNext )
    {
        wxWindowNext *item = ms_winCaptureNext;
        ms_winCaptureNext = item->next;

        DoNotifyWindowAboutCaptureLost(item->win);

        delete item;
    }
}

#if wxUSE_HOTKEY

bool
wxWindowBase::RegisterHotKey(int WXUNUSED(hotkeyId),
                             int WXUNUSED(modifiers),
                             int WXUNUSED(keycode))
{
    // not implemented
    return false;
}

bool wxWindowBase::UnregisterHotKey(int WXUNUSED(hotkeyId))
{
    // not implemented
    return false;
}

#endif // wxUSE_HOTKEY

// ----------------------------------------------------------------------------
// event processing
// ----------------------------------------------------------------------------

bool wxWindowBase::TryValidator(wxEvent& wxVALIDATOR_PARAM(event))
{
#if wxUSE_VALIDATORS
    // Can only use the validator of the window which
    // is receiving the event
    if ( event.GetEventObject() == this )
    {
        wxValidator *validator = GetValidator();
        if ( validator && validator->ProcessEvent(event) )
        {
            return true;
        }
    }
#endif // wxUSE_VALIDATORS

    return false;
}

bool wxWindowBase::TryParent(wxEvent& event)
{
    // carry on up the parent-child hierarchy if the propagation count hasn't
    // reached zero yet
    if ( event.ShouldPropagate() )
    {
        // honour the requests to stop propagation at this window: this is
        // used by the dialogs, for example, to prevent processing the events
        // from the dialog controls in the parent frame which rarely, if ever,
        // makes sense
        if ( !(GetExtraStyle() & wxWS_EX_BLOCK_EVENTS) )
        {
            wxWindow *parent = GetParent();
            if ( parent && !parent->IsBeingDeleted() )
            {
                wxPropagateOnce propagateOnce(event);

                return parent->GetEventHandler()->ProcessEvent(event);
            }
        }
    }

    return wxEvtHandler::TryParent(event);
}

// ----------------------------------------------------------------------------
// window relationships
// ----------------------------------------------------------------------------

wxWindow *wxWindowBase::DoGetSibling(MoveKind order) const
{
    wxCHECK_MSG( GetParent(), NULL,
                    _T("GetPrev/NextSibling() don't work for TLWs!") );

    wxWindowList& siblings = GetParent()->GetChildren();
    wxWindowList::compatibility_iterator i = siblings.Find((wxWindow *)this);
    wxCHECK_MSG( i, NULL, _T("window not a child of its parent?") );

    if ( order == MoveBefore )
        i = i->GetPrevious();
    else // MoveAfter
        i = i->GetNext();

    return i ? i->GetData() : NULL;
}

// ----------------------------------------------------------------------------
// keyboard navigation
// ----------------------------------------------------------------------------

// Navigates in the specified direction.
bool wxWindowBase::Navigate(int flags)
{
    wxNavigationKeyEvent eventNav;
    eventNav.SetFlags(flags);
    eventNav.SetEventObject(this);
    if ( GetParent()->GetEventHandler()->ProcessEvent(eventNav) )
    {
        return true;
    }
    return false;
}

void wxWindowBase::DoMoveInTabOrder(wxWindow *win, MoveKind move)
{
    // check that we're not a top level window
    wxCHECK_RET( GetParent(),
                    _T("MoveBefore/AfterInTabOrder() don't work for TLWs!") );

    // detect the special case when we have nothing to do anyhow and when the
    // code below wouldn't work
    if ( win == this )
        return;

    // find the target window in the siblings list
    wxWindowList& siblings = GetParent()->GetChildren();
    wxWindowList::compatibility_iterator i = siblings.Find(win);
    wxCHECK_RET( i, _T("MoveBefore/AfterInTabOrder(): win is not a sibling") );

    // unfortunately, when wxUSE_STL == 1 DetachNode() is not implemented so we
    // can't just move the node around
    wxWindow *self = (wxWindow *)this;
    siblings.DeleteObject(self);
    if ( move == MoveAfter )
    {
        i = i->GetNext();
    }

    if ( i )
    {
        siblings.Insert(i, self);
    }
    else // MoveAfter and win was the last sibling
    {
        siblings.Append(self);
    }
}

// ----------------------------------------------------------------------------
// focus handling
// ----------------------------------------------------------------------------

/*static*/ wxWindow* wxWindowBase::FindFocus()
{
    wxWindowBase *win = DoFindFocus();
    return win ? win->GetMainWindowOfCompositeControl() : NULL;
}

// ----------------------------------------------------------------------------
// drag and drop
// ----------------------------------------------------------------------------

#if wxUSE_DRAG_AND_DROP && !defined(__WXMSW__)

class wxDragAcceptFilesImplTarget : public wxFileDropTarget
{
public:
    wxDragAcceptFilesImplTarget(wxWindowBase *win) : m_win(win) {}

    virtual bool OnDropFiles(wxCoord x, wxCoord y,
                             const wxArrayString& filenames)
    {
        wxDropFilesEvent event(wxEVT_DROP_FILES,
                               filenames.size(),
                               wxCArrayString(filenames).Release());
        event.SetEventObject(m_win);
        event.m_pos.x = x;
        event.m_pos.y = y;

        return m_win->GetEventHandler()->ProcessEvent(event);
    }

private:
    wxWindowBase * const m_win;

    DECLARE_NO_COPY_CLASS(wxDragAcceptFilesImplTarget)
};


// Generic version of DragAcceptFiles(). It works by installing a simple
// wxFileDropTarget-to-EVT_DROP_FILES adaptor and therefore cannot be used
// together with explicit SetDropTarget() calls.
void wxWindowBase::DragAcceptFiles(bool accept)
{
    if ( accept )
    {
        wxASSERT_MSG( !GetDropTarget(),
                      _T("cannot use DragAcceptFiles() and SetDropTarget() together") );
        SetDropTarget(new wxDragAcceptFilesImplTarget(this));
    }
    else
    {
        SetDropTarget(NULL);
    }
}

#endif // wxUSE_DRAG_AND_DROP && !defined(__WXMSW__)

// ----------------------------------------------------------------------------
// global functions
// ----------------------------------------------------------------------------

wxWindow* wxGetTopLevelParent(wxWindow *win)
{
    while ( win && !win->IsTopLevel() )
         win = win->GetParent();

    return win;
}

#if wxUSE_ACCESSIBILITY
// ----------------------------------------------------------------------------
// accessible object for windows
// ----------------------------------------------------------------------------

// Can return either a child object, or an integer
// representing the child element, starting from 1.
wxAccStatus wxWindowAccessible::HitTest(const wxPoint& WXUNUSED(pt), int* WXUNUSED(childId), wxAccessible** WXUNUSED(childObject))
{
    wxASSERT( GetWindow() != NULL );
    if (!GetWindow())
        return wxACC_FAIL;

    return wxACC_NOT_IMPLEMENTED;
}

// Returns the rectangle for this object (id = 0) or a child element (id > 0).
wxAccStatus wxWindowAccessible::GetLocation(wxRect& rect, int elementId)
{
    wxASSERT( GetWindow() != NULL );
    if (!GetWindow())
        return wxACC_FAIL;

    wxWindow* win = NULL;
    if (elementId == 0)
    {
        win = GetWindow();
    }
    else
    {
        if (elementId <= (int) GetWindow()->GetChildren().GetCount())
        {
            win = GetWindow()->GetChildren().Item(elementId-1)->GetData();
        }
        else
            return wxACC_FAIL;
    }
    if (win)
    {
        rect = win->GetRect();
        if (win->GetParent() && !win->IsKindOf(CLASSINFO(wxTopLevelWindow)))
            rect.SetPosition(win->GetParent()->ClientToScreen(rect.GetPosition()));
        return wxACC_OK;
    }

    return wxACC_NOT_IMPLEMENTED;
}

// Navigates from fromId to toId/toObject.
wxAccStatus wxWindowAccessible::Navigate(wxNavDir navDir, int fromId,
                             int* WXUNUSED(toId), wxAccessible** toObject)
{
    wxASSERT( GetWindow() != NULL );
    if (!GetWindow())
        return wxACC_FAIL;

    switch (navDir)
    {
    case wxNAVDIR_FIRSTCHILD:
        {
            if (GetWindow()->GetChildren().GetCount() == 0)
                return wxACC_FALSE;
            wxWindow* childWindow = (wxWindow*) GetWindow()->GetChildren().GetFirst()->GetData();
            *toObject = childWindow->GetOrCreateAccessible();

            return wxACC_OK;
        }
    case wxNAVDIR_LASTCHILD:
        {
            if (GetWindow()->GetChildren().GetCount() == 0)
                return wxACC_FALSE;
            wxWindow* childWindow = (wxWindow*) GetWindow()->GetChildren().GetLast()->GetData();
            *toObject = childWindow->GetOrCreateAccessible();

            return wxACC_OK;
        }
    case wxNAVDIR_RIGHT:
    case wxNAVDIR_DOWN:
    case wxNAVDIR_NEXT:
        {
            wxWindowList::compatibility_iterator node =
                wxWindowList::compatibility_iterator();
            if (fromId == 0)
            {
                // Can't navigate to sibling of this window
                // if we're a top-level window.
                if (!GetWindow()->GetParent())
                    return wxACC_NOT_IMPLEMENTED;

                node = GetWindow()->GetParent()->GetChildren().Find(GetWindow());
            }
            else
            {
                if (fromId <= (int) GetWindow()->GetChildren().GetCount())
                    node = GetWindow()->GetChildren().Item(fromId-1);
            }

            if (node && node->GetNext())
            {
                wxWindow* nextWindow = node->GetNext()->GetData();
                *toObject = nextWindow->GetOrCreateAccessible();
                return wxACC_OK;
            }
            else
                return wxACC_FALSE;
        }
    case wxNAVDIR_LEFT:
    case wxNAVDIR_UP:
    case wxNAVDIR_PREVIOUS:
        {
            wxWindowList::compatibility_iterator node =
                wxWindowList::compatibility_iterator();
            if (fromId == 0)
            {
                // Can't navigate to sibling of this window
                // if we're a top-level window.
                if (!GetWindow()->GetParent())
                    return wxACC_NOT_IMPLEMENTED;

                node = GetWindow()->GetParent()->GetChildren().Find(GetWindow());
            }
            else
            {
                if (fromId <= (int) GetWindow()->GetChildren().GetCount())
                    node = GetWindow()->GetChildren().Item(fromId-1);
            }

            if (node && node->GetPrevious())
            {
                wxWindow* previousWindow = node->GetPrevious()->GetData();
                *toObject = previousWindow->GetOrCreateAccessible();
                return wxACC_OK;
            }
            else
                return wxACC_FALSE;
        }
    }

    return wxACC_NOT_IMPLEMENTED;
}

// Gets the name of the specified object.
wxAccStatus wxWindowAccessible::GetName(int childId, wxString* name)
{
    wxASSERT( GetWindow() != NULL );
    if (!GetWindow())
        return wxACC_FAIL;

    wxString title;

    // If a child, leave wxWidgets to call the function on the actual
    // child object.
    if (childId > 0)
        return wxACC_NOT_IMPLEMENTED;

    // This will eventually be replaced by specialised
    // accessible classes, one for each kind of wxWidgets
    // control or window.
#if wxUSE_BUTTON
    if (GetWindow()->IsKindOf(CLASSINFO(wxButton)))
        title = ((wxButton*) GetWindow())->GetLabel();
    else
#endif
        title = GetWindow()->GetName();

    if (!title.empty())
    {
        *name = title;
        return wxACC_OK;
    }
    else
        return wxACC_NOT_IMPLEMENTED;
}

// Gets the number of children.
wxAccStatus wxWindowAccessible::GetChildCount(int* childId)
{
    wxASSERT( GetWindow() != NULL );
    if (!GetWindow())
        return wxACC_FAIL;

    *childId = (int) GetWindow()->GetChildren().GetCount();
    return wxACC_OK;
}

// Gets the specified child (starting from 1).
// If *child is NULL and return value is wxACC_OK,
// this means that the child is a simple element and
// not an accessible object.
wxAccStatus wxWindowAccessible::GetChild(int childId, wxAccessible** child)
{
    wxASSERT( GetWindow() != NULL );
    if (!GetWindow())
        return wxACC_FAIL;

    if (childId == 0)
    {
        *child = this;
        return wxACC_OK;
    }

    if (childId > (int) GetWindow()->GetChildren().GetCount())
        return wxACC_FAIL;

    wxWindow* childWindow = GetWindow()->GetChildren().Item(childId-1)->GetData();
    *child = childWindow->GetOrCreateAccessible();
    if (*child)
        return wxACC_OK;
    else
        return wxACC_FAIL;
}

// Gets the parent, or NULL.
wxAccStatus wxWindowAccessible::GetParent(wxAccessible** parent)
{
    wxASSERT( GetWindow() != NULL );
    if (!GetWindow())
        return wxACC_FAIL;

    wxWindow* parentWindow = GetWindow()->GetParent();
    if (!parentWindow)
    {
        *parent = NULL;
        return wxACC_OK;
    }
    else
    {
        *parent = parentWindow->GetOrCreateAccessible();
        if (*parent)
            return wxACC_OK;
        else
            return wxACC_FAIL;
    }
}

// Performs the default action. childId is 0 (the action for this object)
// or > 0 (the action for a child).
// Return wxACC_NOT_SUPPORTED if there is no default action for this
// window (e.g. an edit control).
wxAccStatus wxWindowAccessible::DoDefaultAction(int WXUNUSED(childId))
{
    wxASSERT( GetWindow() != NULL );
    if (!GetWindow())
        return wxACC_FAIL;

    return wxACC_NOT_IMPLEMENTED;
}

// Gets the default action for this object (0) or > 0 (the action for a child).
// Return wxACC_OK even if there is no action. actionName is the action, or the empty
// string if there is no action.
// The retrieved string describes the action that is performed on an object,
// not what the object does as a result. For example, a toolbar button that prints
// a document has a default action of "Press" rather than "Prints the current document."
wxAccStatus wxWindowAccessible::GetDefaultAction(int WXUNUSED(childId), wxString* WXUNUSED(actionName))
{
    wxASSERT( GetWindow() != NULL );
    if (!GetWindow())
        return wxACC_FAIL;

    return wxACC_NOT_IMPLEMENTED;
}

// Returns the description for this object or a child.
wxAccStatus wxWindowAccessible::GetDescription(int WXUNUSED(childId), wxString* description)
{
    wxASSERT( GetWindow() != NULL );
    if (!GetWindow())
        return wxACC_FAIL;

    wxString ht(GetWindow()->GetHelpTextAtPoint(wxDefaultPosition, wxHelpEvent::Origin_Keyboard));
    if (!ht.empty())
    {
        *description = ht;
        return wxACC_OK;
    }
    return wxACC_NOT_IMPLEMENTED;
}

// Returns help text for this object or a child, similar to tooltip text.
wxAccStatus wxWindowAccessible::GetHelpText(int WXUNUSED(childId), wxString* helpText)
{
    wxASSERT( GetWindow() != NULL );
    if (!GetWindow())
        return wxACC_FAIL;

    wxString ht(GetWindow()->GetHelpTextAtPoint(wxDefaultPosition, wxHelpEvent::Origin_Keyboard));
    if (!ht.empty())
    {
        *helpText = ht;
        return wxACC_OK;
    }
    return wxACC_NOT_IMPLEMENTED;
}

// Returns the keyboard shortcut for this object or child.
// Return e.g. ALT+K
wxAccStatus wxWindowAccessible::GetKeyboardShortcut(int WXUNUSED(childId), wxString* WXUNUSED(shortcut))
{
    wxASSERT( GetWindow() != NULL );
    if (!GetWindow())
        return wxACC_FAIL;

    return wxACC_NOT_IMPLEMENTED;
}

// Returns a role constant.
wxAccStatus wxWindowAccessible::GetRole(int childId, wxAccRole* role)
{
    wxASSERT( GetWindow() != NULL );
    if (!GetWindow())
        return wxACC_FAIL;

    // If a child, leave wxWidgets to call the function on the actual
    // child object.
    if (childId > 0)
        return wxACC_NOT_IMPLEMENTED;

    if (GetWindow()->IsKindOf(CLASSINFO(wxControl)))
        return wxACC_NOT_IMPLEMENTED;
#if wxUSE_STATUSBAR
    if (GetWindow()->IsKindOf(CLASSINFO(wxStatusBar)))
        return wxACC_NOT_IMPLEMENTED;
#endif
#if wxUSE_TOOLBAR
    if (GetWindow()->IsKindOf(CLASSINFO(wxToolBar)))
        return wxACC_NOT_IMPLEMENTED;
#endif

    //*role = wxROLE_SYSTEM_CLIENT;
    *role = wxROLE_SYSTEM_CLIENT;
    return wxACC_OK;

    #if 0
    return wxACC_NOT_IMPLEMENTED;
    #endif
}

// Returns a state constant.
wxAccStatus wxWindowAccessible::GetState(int childId, long* state)
{
    wxASSERT( GetWindow() != NULL );
    if (!GetWindow())
        return wxACC_FAIL;

    // If a child, leave wxWidgets to call the function on the actual
    // child object.
    if (childId > 0)
        return wxACC_NOT_IMPLEMENTED;

    if (GetWindow()->IsKindOf(CLASSINFO(wxControl)))
        return wxACC_NOT_IMPLEMENTED;

#if wxUSE_STATUSBAR
    if (GetWindow()->IsKindOf(CLASSINFO(wxStatusBar)))
        return wxACC_NOT_IMPLEMENTED;
#endif
#if wxUSE_TOOLBAR
    if (GetWindow()->IsKindOf(CLASSINFO(wxToolBar)))
        return wxACC_NOT_IMPLEMENTED;
#endif

    *state = 0;
    return wxACC_OK;

    #if 0
    return wxACC_NOT_IMPLEMENTED;
    #endif
}

// Returns a localized string representing the value for the object
// or child.
wxAccStatus wxWindowAccessible::GetValue(int WXUNUSED(childId), wxString* WXUNUSED(strValue))
{
    wxASSERT( GetWindow() != NULL );
    if (!GetWindow())
        return wxACC_FAIL;

    return wxACC_NOT_IMPLEMENTED;
}

// Selects the object or child.
wxAccStatus wxWindowAccessible::Select(int WXUNUSED(childId), wxAccSelectionFlags WXUNUSED(selectFlags))
{
    wxASSERT( GetWindow() != NULL );
    if (!GetWindow())
        return wxACC_FAIL;

    return wxACC_NOT_IMPLEMENTED;
}

// Gets the window with the keyboard focus.
// If childId is 0 and child is NULL, no object in
// this subhierarchy has the focus.
// If this object has the focus, child should be 'this'.
wxAccStatus wxWindowAccessible::GetFocus(int* WXUNUSED(childId), wxAccessible** WXUNUSED(child))
{
    wxASSERT( GetWindow() != NULL );
    if (!GetWindow())
        return wxACC_FAIL;

    return wxACC_NOT_IMPLEMENTED;
}

#if wxUSE_VARIANT
// Gets a variant representing the selected children
// of this object.
// Acceptable values:
// - a null variant (IsNull() returns true)
// - a list variant (GetType() == wxT("list")
// - an integer representing the selected child element,
//   or 0 if this object is selected (GetType() == wxT("long")
// - a "void*" pointer to a wxAccessible child object
wxAccStatus wxWindowAccessible::GetSelections(wxVariant* WXUNUSED(selections))
{
    wxASSERT( GetWindow() != NULL );
    if (!GetWindow())
        return wxACC_FAIL;

    return wxACC_NOT_IMPLEMENTED;
}
#endif // wxUSE_VARIANT

#endif // wxUSE_ACCESSIBILITY

// ----------------------------------------------------------------------------
// RTL support
// ----------------------------------------------------------------------------

wxCoord
wxWindowBase::AdjustForLayoutDirection(wxCoord x,
                                       wxCoord width,
                                       wxCoord widthTotal) const
{
    if ( GetLayoutDirection() == wxLayout_RightToLeft )
    {
        x = widthTotal - x - width;
    }

    return x;
}

