///////////////////////////////////////////////////////////////////////////////
// Name:        src/common/statbar.cpp
// Purpose:     wxStatusBarBase implementation
// Author:      Vadim Zeitlin
// Modified by:
// Created:     14.10.01
// RCS-ID:      $Id: statbar.cpp 42171 2006-10-20 14:54:14Z VS $
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

#if wxUSE_STATUSBAR

#include "wx/statusbr.h"

#ifndef WX_PRECOMP
    #include "wx/frame.h"
#endif //WX_PRECOMP

#include "wx/listimpl.cpp"
WX_DEFINE_LIST(wxListString)

const wxChar wxStatusBarNameStr[] = wxT("statusBar");

// ============================================================================
// wxStatusBarBase implementation
// ============================================================================

IMPLEMENT_DYNAMIC_CLASS(wxStatusBar, wxWindow)

// ----------------------------------------------------------------------------
// ctor/dtor
// ----------------------------------------------------------------------------

wxStatusBarBase::wxStatusBarBase()
{
    m_nFields = 0;

    InitWidths();
    InitStacks();
    InitStyles();
}

wxStatusBarBase::~wxStatusBarBase()
{
    FreeWidths();
    FreeStacks();
    FreeStyles();

    // notify the frame that it doesn't have a status bar any longer to avoid
    // dangling pointers
    wxFrame *frame = wxDynamicCast(GetParent(), wxFrame);
    if ( frame && frame->GetStatusBar() == this )
    {
        frame->SetStatusBar(NULL);
    }
}

// ----------------------------------------------------------------------------
// widths array handling
// ----------------------------------------------------------------------------

void wxStatusBarBase::InitWidths()
{
    m_statusWidths = NULL;
}

void wxStatusBarBase::FreeWidths()
{
    delete [] m_statusWidths;
}

// ----------------------------------------------------------------------------
// styles array handling
// ----------------------------------------------------------------------------

void wxStatusBarBase::InitStyles()
{
    m_statusStyles = NULL;
}

void wxStatusBarBase::FreeStyles()
{
    delete [] m_statusStyles;
}

// ----------------------------------------------------------------------------
// field widths
// ----------------------------------------------------------------------------

void wxStatusBarBase::SetFieldsCount(int number, const int *widths)
{
    wxCHECK_RET( number > 0, _T("invalid field number in SetFieldsCount") );

    bool refresh = false;

    if ( number != m_nFields )
    {
        // copy stacks if present
        if(m_statusTextStacks)
        {
            wxListString **newStacks = new wxListString*[number];
            size_t i, j, max = wxMin(number, m_nFields);

            // copy old stacks
            for(i = 0; i < max; ++i)
                newStacks[i] = m_statusTextStacks[i];
            // free old stacks in excess
            for(j = i; j < (size_t)m_nFields; ++j)
            {
                if(m_statusTextStacks[j])
                {
                    m_statusTextStacks[j]->Clear();
                    delete m_statusTextStacks[j];
                }
            }
            // initialize new stacks to NULL
            for(j = i; j < (size_t)number; ++j)
                newStacks[j] = 0;

            m_statusTextStacks = newStacks;
        }

        // Resize styles array
        if (m_statusStyles)
        {
            int *oldStyles = m_statusStyles;
            m_statusStyles = new int[number];
            int i, max = wxMin(number, m_nFields);

            // copy old styles
            for (i = 0; i < max; ++i)
                m_statusStyles[i] = oldStyles[i];

            // initialize new styles to wxSB_NORMAL
            for (i = max; i < number; ++i)
                m_statusStyles[i] = wxSB_NORMAL;

            // free old styles
            delete [] oldStyles;
        }


        m_nFields = number;

        ReinitWidths();

        refresh = true;
    }
    //else: keep the old m_statusWidths if we had them

    if ( widths )
    {
        SetStatusWidths(number, widths);

        // already done from SetStatusWidths()
        refresh = false;
    }

    if ( refresh )
        Refresh();
}

void wxStatusBarBase::SetStatusWidths(int WXUNUSED_UNLESS_DEBUG(n),
                                      const int widths[])
{
    wxCHECK_RET( widths, _T("NULL pointer in SetStatusWidths") );

    wxASSERT_MSG( n == m_nFields, _T("field number mismatch") );

    if ( !m_statusWidths )
        m_statusWidths = new int[m_nFields];

    for ( int i = 0; i < m_nFields; i++ )
    {
        m_statusWidths[i] = widths[i];
    }

    // update the display after the widths changed
    Refresh();
}

void wxStatusBarBase::SetStatusStyles(int WXUNUSED_UNLESS_DEBUG(n),
                                      const int styles[])
{
    wxCHECK_RET( styles, _T("NULL pointer in SetStatusStyles") );

    wxASSERT_MSG( n == m_nFields, _T("field number mismatch") );

    if ( !m_statusStyles )
        m_statusStyles = new int[m_nFields];

    for ( int i = 0; i < m_nFields; i++ )
    {
        m_statusStyles[i] = styles[i];
    }

    // update the display after the widths changed
    Refresh();
}

wxArrayInt wxStatusBarBase::CalculateAbsWidths(wxCoord widthTotal) const
{
    wxArrayInt widths;

    if ( m_statusWidths == NULL )
    {
        if ( m_nFields )
        {
            // Default: all fields have the same width. This is not always
            // possible to do exactly (if widthTotal is not divisible by
            // m_nFields) - if that happens, we distribute the extra pixels
            // among all fields:
            int widthToUse = widthTotal;

            for ( int i = m_nFields; i > 0; i-- )
            {
                // divide the unassigned width evently between the
                // not yet processed fields:
                int w = widthToUse / i;
                widths.Add(w);
                widthToUse -= w;
            }

        }
        //else: we're empty anyhow
    }
    else // have explicit status widths
    {
        // calculate the total width of all the fixed width fields and the
        // total number of var field widths counting with multiplicity
        int nTotalWidth = 0,
            nVarCount = 0,
            i;
        for ( i = 0; i < m_nFields; i++ )
        {
            if ( m_statusWidths[i] >= 0 )
            {
                nTotalWidth += m_statusWidths[i];
            }
            else
            {
                nVarCount += -m_statusWidths[i];
            }
        }

        // the amount of extra width we have per each var width field
        int widthExtra = widthTotal - nTotalWidth;

        // do fill the array
        for ( i = 0; i < m_nFields; i++ )
        {
            if ( m_statusWidths[i] >= 0 )
            {
                widths.Add(m_statusWidths[i]);
            }
            else
            {
                int nVarWidth = widthExtra > 0 ? (widthExtra * -m_statusWidths[i]) / nVarCount : 0;
                nVarCount += m_statusWidths[i];
                widthExtra -= nVarWidth;
                widths.Add(nVarWidth);
            }
        }
    }

    return widths;
}

// ----------------------------------------------------------------------------
// text stacks handling
// ----------------------------------------------------------------------------

void wxStatusBarBase::InitStacks()
{
    m_statusTextStacks = NULL;
}

void wxStatusBarBase::FreeStacks()
{
    if ( !m_statusTextStacks )
        return;

    for ( size_t i = 0; i < (size_t)m_nFields; ++i )
    {
        if ( m_statusTextStacks[i] )
        {
            wxListString& t = *m_statusTextStacks[i];
            WX_CLEAR_LIST(wxListString, t);
            delete m_statusTextStacks[i];
        }
    }

    delete[] m_statusTextStacks;
}

// ----------------------------------------------------------------------------
// text stacks
// ----------------------------------------------------------------------------

void wxStatusBarBase::PushStatusText(const wxString& text, int number)
{
    wxListString* st = GetOrCreateStatusStack(number);
    // This long-winded way around avoids an internal compiler error
    // in VC++ 6 with RTTI enabled
    wxString tmp1(GetStatusText(number));
    wxString* tmp = new wxString(tmp1);
    st->Insert(tmp);
    SetStatusText(text, number);
}

void wxStatusBarBase::PopStatusText(int number)
{
    wxListString *st = GetStatusStack(number);
    wxCHECK_RET( st, _T("Unbalanced PushStatusText/PopStatusText") );
    wxListString::compatibility_iterator top = st->GetFirst();

    SetStatusText(*top->GetData(), number);
    delete top->GetData();
    st->Erase(top);
    if(st->GetCount() == 0)
    {
        delete st;
        m_statusTextStacks[number] = 0;
    }
}

wxListString *wxStatusBarBase::GetStatusStack(int i) const
{
    if(!m_statusTextStacks)
        return 0;
    return m_statusTextStacks[i];
}

wxListString *wxStatusBarBase::GetOrCreateStatusStack(int i)
{
    if(!m_statusTextStacks)
    {
        m_statusTextStacks = new wxListString*[m_nFields];

        size_t j;
        for(j = 0; j < (size_t)m_nFields; ++j) m_statusTextStacks[j] = 0;
    }

    if(!m_statusTextStacks[i])
    {
        m_statusTextStacks[i] = new wxListString();
    }

    return m_statusTextStacks[i];
}

#endif // wxUSE_STATUSBAR
