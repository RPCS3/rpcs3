///////////////////////////////////////////////////////////////////////////////
// Name:        src/common/containr.cpp
// Purpose:     implementation of wxControlContainer
// Author:      Vadim Zeitlin
// Modified by:
// Created:     06.08.01
// RCS-ID:      $Id: containr.cpp 44273 2007-01-21 01:21:45Z VZ $
// Copyright:   (c) 2001 Vadim Zeitlin <zeitlin@dptmaths.ens-cachan.fr>
// License:     wxWindows licence
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

#ifndef WX_PRECOMP
    #include "wx/log.h"
    #include "wx/event.h"
    #include "wx/window.h"
    #include "wx/scrolbar.h"
    #include "wx/radiobut.h"
    #include "wx/containr.h"
#endif //WX_PRECOMP

// trace mask for focus messages
#define TRACE_FOCUS _T("focus")

// ============================================================================
// implementation
// ============================================================================

wxControlContainer::wxControlContainer(wxWindow *winParent)
{
    m_winParent = winParent;
    m_winLastFocused = NULL;
    m_inSetFocus = false;
}

bool wxControlContainer::AcceptsFocus() const
{
    // if we're not shown or disabled, we can't accept focus
    if ( m_winParent->IsShown() && m_winParent->IsEnabled() )
    {
        // otherwise we can accept focus either if we have no children at all
        // (in this case we're probably not used as a container) or only when
        // at least one child will accept focus
        wxWindowList::compatibility_iterator node = m_winParent->GetChildren().GetFirst();
        if ( !node )
            return true;

#ifdef __WXMAC__
        // wxMac has eventually the two scrollbars as children, they don't count
        // as real children in the algorithm mentioned above
        bool hasRealChildren = false ;
#endif

        while ( node )
        {
            wxWindow *child = node->GetData();
            node = node->GetNext();

#ifdef __WXMAC__
            if ( m_winParent->MacIsWindowScrollbar( child ) )
                continue;
            hasRealChildren = true ;
#endif
            if ( child->AcceptsFocus() )
            {
                return true;
            }
        }

#ifdef __WXMAC__
        if ( !hasRealChildren )
            return true ;
#endif
    }

    return false;
}

void wxControlContainer::SetLastFocus(wxWindow *win)
{
    // the panel itself should never get the focus at all but if it does happen
    // temporarily (as it seems to do under wxGTK), at the very least don't
    // forget our previous m_winLastFocused
    if ( win != m_winParent )
    {
        // if we're setting the focus
        if ( win )
        {
            // find the last _immediate_ child which got focus
            wxWindow *winParent = win;
            while ( winParent != m_winParent )
            {
                win = winParent;
                winParent = win->GetParent();

                // Yes, this can happen, though in a totally pathological case.
                // like when detaching a menubar from a frame with a child
                // which has pushed itself as an event handler for the menubar.
                // (under wxGTK)

                wxASSERT_MSG( winParent,
                              _T("Setting last focus for a window that is not our child?") );
            }
        }

        m_winLastFocused = win;

        if ( win )
        {
            wxLogTrace(TRACE_FOCUS, _T("Set last focus to %s(%s)"),
                       win->GetClassInfo()->GetClassName(),
                       win->GetLabel().c_str());
        }
        else
        {
            wxLogTrace(TRACE_FOCUS, _T("No more last focus"));
        }
    }

    // propagate the last focus upwards so that our parent can set focus back
    // to us if it loses it now and regains later
    wxWindow *parent = m_winParent->GetParent();
    if ( parent )
    {
        wxChildFocusEvent eventFocus(m_winParent);
        parent->GetEventHandler()->ProcessEvent(eventFocus);
    }
}

// --------------------------------------------------------------------
// The following four functions are used to find other radio buttons
// within the same group. Used by wxSetFocusToChild on wxMSW
// --------------------------------------------------------------------

#ifdef __WXMSW__

wxRadioButton* wxGetPreviousButtonInGroup(wxRadioButton *btn)
{
    if ( btn->HasFlag(wxRB_GROUP) || btn->HasFlag(wxRB_SINGLE) )
        return NULL;

    const wxWindowList& siblings = btn->GetParent()->GetChildren();
    wxWindowList::compatibility_iterator nodeThis = siblings.Find(btn);
    wxCHECK_MSG( nodeThis, NULL, _T("radio button not a child of its parent?") );

    // Iterate over all previous siblings until we find the next radio button
    wxWindowList::compatibility_iterator nodeBefore = nodeThis->GetPrevious();
    wxRadioButton *prevBtn = 0;
    while (nodeBefore)
    {
        prevBtn = wxDynamicCast(nodeBefore->GetData(), wxRadioButton);
        if (prevBtn)
            break;

        nodeBefore = nodeBefore->GetPrevious();
    }

    if (!prevBtn || prevBtn->HasFlag(wxRB_SINGLE))
    {
        // no more buttons in group
        return NULL;
    }

    return prevBtn;
}

wxRadioButton* wxGetNextButtonInGroup(wxRadioButton *btn)
{
    if (btn->HasFlag(wxRB_SINGLE))
        return NULL;

    const wxWindowList& siblings = btn->GetParent()->GetChildren();
    wxWindowList::compatibility_iterator nodeThis = siblings.Find(btn);
    wxCHECK_MSG( nodeThis, NULL, _T("radio button not a child of its parent?") );

    // Iterate over all previous siblings until we find the next radio button
    wxWindowList::compatibility_iterator nodeNext = nodeThis->GetNext();
    wxRadioButton *nextBtn = 0;
    while (nodeNext)
    {
        nextBtn = wxDynamicCast(nodeNext->GetData(), wxRadioButton);
        if (nextBtn)
            break;

        nodeNext = nodeNext->GetNext();
    }

    if ( !nextBtn || nextBtn->HasFlag(wxRB_GROUP) || nextBtn->HasFlag(wxRB_SINGLE) )
    {
        // no more buttons or the first button of the next group
        return NULL;
    }

    return nextBtn;
}

wxRadioButton* wxGetFirstButtonInGroup(wxRadioButton *btn)
{
    while (true)
    {
        wxRadioButton* prevBtn = wxGetPreviousButtonInGroup(btn);
        if (!prevBtn)
            return btn;

        btn = prevBtn;
    }
}

wxRadioButton* wxGetLastButtonInGroup(wxRadioButton *btn)
{
    while (true)
    {
        wxRadioButton* nextBtn = wxGetNextButtonInGroup(btn);
        if (!nextBtn)
            return btn;

        btn = nextBtn;
    }
}

wxRadioButton* wxGetSelectedButtonInGroup(wxRadioButton *btn)
{
    // Find currently selected button
    if (btn->GetValue())
        return btn;

    if (btn->HasFlag(wxRB_SINGLE))
        return NULL;

    wxRadioButton *selBtn;

    // First check all previous buttons
    for (selBtn = wxGetPreviousButtonInGroup(btn); selBtn; selBtn = wxGetPreviousButtonInGroup(selBtn))
        if (selBtn->GetValue())
            return selBtn;

    // Now all following buttons
    for (selBtn = wxGetNextButtonInGroup(btn); selBtn; selBtn = wxGetNextButtonInGroup(selBtn))
        if (selBtn->GetValue())
            return selBtn;

    return NULL;
}

#endif // __WXMSW__

// ----------------------------------------------------------------------------
// Keyboard handling - this is the place where the TAB traversal logic is
// implemented. As this code is common to all ports, this ensures consistent
// behaviour even if we don't specify how exactly the wxNavigationKeyEvent are
// generated and this is done in platform specific code which also ensures that
// we can follow the given platform standards.
// ----------------------------------------------------------------------------

void wxControlContainer::HandleOnNavigationKey( wxNavigationKeyEvent& event )
{
    wxWindow *parent = m_winParent->GetParent();

    // the event is propagated downwards if the event emitter was our parent
    bool goingDown = event.GetEventObject() == parent;

    const wxWindowList& children = m_winParent->GetChildren();

    // if we have exactly one notebook-like child window (actually it could be
    // any window that returns true from its HasMultiplePages()), then
    // [Shift-]Ctrl-Tab and Ctrl-PageUp/Down keys should iterate over its pages
    // even if the focus is outside of the control because this is how the
    // standard MSW properties dialogs behave and we do it under other platforms
    // as well because it seems like a good idea -- but we can always put this
    // block inside "#ifdef __WXMSW__" if it's not suitable there
    if ( event.IsWindowChange() && !goingDown )
    {
        // check if we have a unique notebook-like child
        wxWindow *bookctrl = NULL;
        for ( wxWindowList::const_iterator i = children.begin(),
                                         end = children.end();
              i != end;
              ++i )
        {
            wxWindow * const window = *i;
            if ( window->HasMultiplePages() )
            {
                if ( bookctrl )
                {
                    // this is the second book-like control already so don't do
                    // anything as we don't know which one should have its page
                    // changed
                    bookctrl = NULL;
                    break;
                }

                bookctrl = window;
            }
        }

        if ( bookctrl )
        {
            // make sure that we don't bubble up the event again from the book
            // control resulting in infinite recursion
            wxNavigationKeyEvent eventCopy(event);
            eventCopy.SetEventObject(m_winParent);
            if ( bookctrl->GetEventHandler()->ProcessEvent(eventCopy) )
                return;
        }
    }

    // there is not much to do if we don't have children and we're not
    // interested in "notebook page change" events here
    if ( !children.GetCount() || event.IsWindowChange() )
    {
        // let the parent process it unless it already comes from our parent
        // of we don't have any
        if ( goingDown ||
             !parent || !parent->GetEventHandler()->ProcessEvent(event) )
        {
            event.Skip();
        }

        return;
    }

    // where are we going?
    const bool forward = event.GetDirection();

    // the node of the children list from which we should start looking for the
    // next acceptable child
    wxWindowList::compatibility_iterator node, start_node;

    // we should start from the first/last control and not from the one which
    // had focus the last time if we're propagating the event downwards because
    // for our parent we look like a single control
    if ( goingDown )
    {
        // just to be sure it's not used (normally this is not necessary, but
        // doesn't hurt neither)
        m_winLastFocused = (wxWindow *)NULL;

        // start from first or last depending on where we're going
        node = forward ? children.GetFirst() : children.GetLast();
    }
    else // going up
    {
        // try to find the child which has the focus currently

        // the event emitter might have done this for us
        wxWindow *winFocus = event.GetCurrentFocus();

        // but if not, we might know where the focus was ourselves
        if (!winFocus)
            winFocus = m_winLastFocused;

        // if still no luck, do it the hard way
        if (!winFocus)
            winFocus = wxWindow::FindFocus();

        if ( winFocus )
        {
#ifdef __WXMSW__
            // If we are in a radio button group, start from the first item in the
            // group
            if ( event.IsFromTab() && wxIsKindOf(winFocus, wxRadioButton ) )
                winFocus = wxGetFirstButtonInGroup((wxRadioButton*)winFocus);
#endif
            // ok, we found the focus - now is it our child?
            start_node = children.Find( winFocus );
        }

        if ( !start_node && m_winLastFocused )
        {
            // window which has focus isn't our child, fall back to the one
            // which had the focus the last time
            start_node = children.Find( m_winLastFocused );
        }

        // if we still didn't find anything, we should start with the first one
        if ( !start_node )
        {
            start_node = children.GetFirst();
        }

        // and the first child which we can try setting focus to is the next or
        // the previous one
        node = forward ? start_node->GetNext() : start_node->GetPrevious();
    }

    // we want to cycle over all elements passing by NULL
    for ( ;; )
    {
        // don't go into infinite loop
        if ( start_node && node && node == start_node )
            break;

        // Have we come to the last or first item on the panel?
        if ( !node )
        {
            if ( !start_node )
            {
                // exit now as otherwise we'd loop forever
                break;
            }

            if ( !goingDown )
            {
                // Check if our (maybe grand) parent is another panel: if this
                // is the case, they will know what to do with this navigation
                // key and so give them the chance to process it instead of
                // looping inside this panel (normally, the focus will go to
                // the next/previous item after this panel in the parent
                // panel).
                wxWindow *focussed_child_of_parent = m_winParent;
                while ( parent )
                {
                    // we don't want to tab into a different dialog or frame
                    if ( focussed_child_of_parent->IsTopLevel() )
                        break;

                    event.SetCurrentFocus( focussed_child_of_parent );
                    if ( parent->GetEventHandler()->ProcessEvent( event ) )
                        return;

                    focussed_child_of_parent = parent;

                    parent = parent->GetParent();
                }
            }
            //else: as the focus came from our parent, we definitely don't want
            //      to send it back to it!

            // no, we are not inside another panel so process this ourself
            node = forward ? children.GetFirst() : children.GetLast();

            continue;
        }

        wxWindow *child = node->GetData();

#ifdef __WXMSW__
        if ( event.IsFromTab() )
        {
            if ( wxIsKindOf(child, wxRadioButton) )
            {
                // only radio buttons with either wxRB_GROUP or wxRB_SINGLE
                // can be tabbed to
                if ( child->HasFlag(wxRB_GROUP) )
                {
                    // need to tab into the active button within a group
                    wxRadioButton *rb = wxGetSelectedButtonInGroup((wxRadioButton*)child);
                    if ( rb )
                        child = rb;
                }
                else if ( !child->HasFlag(wxRB_SINGLE) )
                {
                    node = forward ? node->GetNext() : node->GetPrevious();
                    continue;
                }
            }
        }
        else if ( m_winLastFocused &&
                  wxIsKindOf(m_winLastFocused, wxRadioButton) &&
                  !m_winLastFocused->HasFlag(wxRB_SINGLE) )
        {
            // cursor keys don't navigate out of a radio button group so
            // find the correct radio button to focus
            if ( forward )
            {
                child = wxGetNextButtonInGroup((wxRadioButton*)m_winLastFocused);
                if ( !child )
                {
                    // no next button in group, set it to the first button
                    child = wxGetFirstButtonInGroup((wxRadioButton*)m_winLastFocused);
                }
            }
            else
            {
                child = wxGetPreviousButtonInGroup((wxRadioButton*)m_winLastFocused);
                if ( !child )
                {
                    // no previous button in group, set it to the last button
                    child = wxGetLastButtonInGroup((wxRadioButton*)m_winLastFocused);
                }
            }

            if ( child == m_winLastFocused )
            {
                // must be a group consisting of only one button therefore
                // no need to send a navigation event
                event.Skip(false);
                return;
            }
        }
#endif // __WXMSW__

        if ( child->AcceptsFocusFromKeyboard() )
        {
            // if we're setting the focus to a child panel we should prevent it
            // from giving it to the child which had the focus the last time
            // and instead give it to the first/last child depending from which
            // direction we're coming
            event.SetEventObject(m_winParent);

            // disable propagation for this call as otherwise the event might
            // bounce back to us.
            wxPropagationDisabler disableProp(event);
            if ( !child->GetEventHandler()->ProcessEvent(event) )
            {
                // set it first in case SetFocusFromKbd() results in focus
                // change too
                m_winLastFocused = child;

                // everything is simple: just give focus to it
                child->SetFocusFromKbd();
            }
            //else: the child manages its focus itself

            event.Skip( false );

            return;
        }

        node = forward ? node->GetNext() : node->GetPrevious();
    }

    // we cycled through all of our children and none of them wanted to accept
    // focus
    event.Skip();
}

void wxControlContainer::HandleOnWindowDestroy(wxWindowBase *child)
{
    if ( child == m_winLastFocused )
        m_winLastFocused = NULL;
}

// ----------------------------------------------------------------------------
// focus handling
// ----------------------------------------------------------------------------

bool wxControlContainer::DoSetFocus()
{
    wxLogTrace(TRACE_FOCUS, _T("SetFocus on wxPanel 0x%p."),
               m_winParent->GetHandle());

    if (m_inSetFocus)
        return true;

    // when the panel gets the focus we move the focus to either the last
    // window that had the focus or the first one that can get it unless the
    // focus had been already set to some other child

    wxWindow *win = wxWindow::FindFocus();
    while ( win )
    {
        if ( win == m_winParent )
        {
            // our child already has focus, don't take it away from it
            return true;
        }

        if ( win->IsTopLevel() )
        {
            // don't look beyond the first top level parent - useless and
            // unnecessary
            break;
        }

        win = win->GetParent();
    }

    // protect against infinite recursion:
    m_inSetFocus = true;

    bool ret = SetFocusToChild();

    m_inSetFocus = false;

    return ret;
}

void wxControlContainer::HandleOnFocus(wxFocusEvent& event)
{
    wxLogTrace(TRACE_FOCUS, _T("OnFocus on wxPanel 0x%p, name: %s"),
               m_winParent->GetHandle(),
               m_winParent->GetName().c_str() );

    DoSetFocus();

    event.Skip();
}

bool wxControlContainer::SetFocusToChild()
{
    return wxSetFocusToChild(m_winParent, &m_winLastFocused);
}

// ----------------------------------------------------------------------------
// SetFocusToChild(): this function is used by wxPanel but also by wxFrame in
// wxMSW, this is why it is outside of wxControlContainer class
// ----------------------------------------------------------------------------

bool wxSetFocusToChild(wxWindow *win, wxWindow **childLastFocused)
{
    wxCHECK_MSG( win, false, _T("wxSetFocusToChild(): invalid window") );
    wxCHECK_MSG( childLastFocused, false,
                 _T("wxSetFocusToChild(): NULL child poonter") );

    if ( *childLastFocused )
    {
        // It might happen that the window got reparented
        if ( (*childLastFocused)->GetParent() == win )
        {
            wxLogTrace(TRACE_FOCUS,
                       _T("SetFocusToChild() => last child (0x%p)."),
                       (*childLastFocused)->GetHandle());

            // not SetFocusFromKbd(): we're restoring focus back to the old
            // window and not setting it as the result of a kbd action
            (*childLastFocused)->SetFocus();
            return true;
        }
        else
        {
            // it doesn't count as such any more
            *childLastFocused = (wxWindow *)NULL;
        }
    }

    // set the focus to the first child who wants it
    wxWindowList::compatibility_iterator node = win->GetChildren().GetFirst();
    while ( node )
    {
        wxWindow *child = node->GetData();
        node = node->GetNext();

#ifdef __WXMAC__
        if ( child->GetParent()->MacIsWindowScrollbar( child ) )
            continue;
#endif
        
        if ( child->AcceptsFocusFromKeyboard() && !child->IsTopLevel() )
        {
#ifdef __WXMSW__
            // If a radiobutton is the first focusable child, search for the
            // selected radiobutton in the same group
            wxRadioButton* btn = wxDynamicCast(child, wxRadioButton);
            if (btn)
            {
                wxRadioButton* selected = wxGetSelectedButtonInGroup(btn);
                if (selected)
                    child = selected;
            }
#endif

            wxLogTrace(TRACE_FOCUS,
                       _T("SetFocusToChild() => first child (0x%p)."),
                       child->GetHandle());

            *childLastFocused = child;
            child->SetFocusFromKbd();
            return true;
        }
    }

    return false;
}
