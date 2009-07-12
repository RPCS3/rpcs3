///////////////////////////////////////////////////////////////////////////////
// Name:        src/common/radiocmn.cpp
// Purpose:     wxRadioBox methods common to all ports
// Author:      Vadim Zeitlin
// Modified by:
// Created:     03.06.01
// RCS-ID:      $Id: radiocmn.cpp 54930 2008-08-02 19:45:23Z VZ $
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

#if wxUSE_RADIOBOX

#ifndef WX_PRECOMP
    #include "wx/radiobox.h"
#endif //WX_PRECOMP

#if wxUSE_TOOLTIPS
    #include "wx/tooltip.h"
#endif // wxUSE_TOOLTIPS

#if wxUSE_HELP
    #include "wx/cshelp.h"
#endif

// ============================================================================
// implementation
// ============================================================================

void wxRadioBoxBase::SetMajorDim(unsigned int majorDim, long style)
{
    wxCHECK_RET( majorDim != 0, _T("major radiobox dimension can't be 0") );

    m_majorDim = majorDim;

    int minorDim = (GetCount() + m_majorDim - 1) / m_majorDim;

    if ( style & wxRA_SPECIFY_COLS )
    {
        m_numCols = majorDim;
        m_numRows = minorDim;
    }
    else // wxRA_SPECIFY_ROWS
    {
        m_numCols = minorDim;
        m_numRows = majorDim;
    }
}

int wxRadioBoxBase::GetNextItem(int item, wxDirection dir, long style) const
{
    const int itemStart = item;

    int count = GetCount(),
        numCols = GetColumnCount(),
        numRows = GetRowCount();

    bool horz = (style & wxRA_SPECIFY_COLS) != 0;

    do
    {
        switch ( dir )
        {
            case wxUP:
                if ( horz )
                {
                    item -= numCols;
                }
                else // vertical layout
                {
                    if ( !item-- )
                        item = count - 1;
                }
                break;

            case wxLEFT:
                if ( horz )
                {
                    if ( !item-- )
                        item = count - 1;
                }
                else // vertical layout
                {
                    item -= numRows;
                }
                break;

            case wxDOWN:
                if ( horz )
                {
                    item += numCols;
                }
                else // vertical layout
                {
                    if ( ++item == count )
                        item = 0;
                }
                break;

            case wxRIGHT:
                if ( horz )
                {
                    if ( ++item == count )
                        item = 0;
                }
                else // vertical layout
                {
                    item += numRows;
                }
                break;

            default:
                wxFAIL_MSG( _T("unexpected wxDirection value") );
                return wxNOT_FOUND;
        }

        // ensure that the item is in range [0..count)
        if ( item < 0 )
        {
            // first map the item to the one in the same column but in the last
            // row
            item += count;

            // now there are 2 cases: either it is the first item of the last
            // row in which case we need to wrap again and get to the last item
            // or we can just go to the previous item
            if ( item % (horz ? numCols : numRows) )
                item--;
            else
                item = count - 1;
        }
        else if ( item >= count )
        {
            // same logic as above
            item -= count;

            // ... except that we need to check if this is not the last item,
            // not the first one
            if ( (item + 1) % (horz ? numCols : numRows) )
                item++;
            else
                item = 0;
        }

        wxASSERT_MSG( item < count && item >= 0,
                      _T("logic error in wxRadioBox::GetNextItem()") );
    }
    // we shouldn't select the non-active items, continue looking for a
    // visible and shown one unless we came back to the item we started from in
    // which case bail out to avoid infinite loop
    while ( !(IsItemShown(item) && IsItemEnabled(item)) && item != itemStart );

    return item;
}

#if wxUSE_TOOLTIPS

void wxRadioBoxBase::SetItemToolTip(unsigned int item, const wxString& text)
{
    wxASSERT_MSG( item < GetCount(), _T("Invalid item index") );

    // extend the array to have entries for all our items on first use
    if ( !m_itemsTooltips )
    {
        m_itemsTooltips = new wxToolTipArray;
        m_itemsTooltips->resize(GetCount());
    }

    wxToolTip *tooltip = (*m_itemsTooltips)[item];

    bool changed = true;
    if ( text.empty() )
    {
        if ( tooltip )
        {
            // delete the tooltip
            delete tooltip;
            tooltip = NULL;
        }
        else // nothing to do
        {
            changed = false;
        }
    }
    else // non empty tooltip text
    {
        if ( tooltip )
        {
            // just change the existing tooltip text, don't change the tooltip
            tooltip->SetTip(text);
            changed = false;
        }
        else // no tooltip yet
        {
            // create the new one
            tooltip = new wxToolTip(text);
        }
    }

    if ( changed )
    {
        (*m_itemsTooltips)[item] = tooltip;
        DoSetItemToolTip(item, tooltip);
    }
}

void
wxRadioBoxBase::DoSetItemToolTip(unsigned int WXUNUSED(item),
                                 wxToolTip * WXUNUSED(tooltip))
{
    // per-item tooltips not implemented by default
}

#endif // wxUSE_TOOLTIPS

wxRadioBoxBase::~wxRadioBoxBase()
{
#if wxUSE_TOOLTIPS
    if ( m_itemsTooltips )
    {
        const size_t n = m_itemsTooltips->size();
        for ( size_t i = 0; i < n; i++ )
            delete (*m_itemsTooltips)[i];

        delete m_itemsTooltips;
    }
#endif // wxUSE_TOOLTIPS
}

#if wxUSE_HELP

// set helptext for a particular item
void wxRadioBoxBase::SetItemHelpText(unsigned int n, const wxString& helpText)
{
    wxCHECK_RET( n < GetCount(), _T("Invalid item index") );

    if ( m_itemsHelpTexts.empty() )
    {
        // once-only initialization of the array: reserve space for all items
        m_itemsHelpTexts.Add(wxEmptyString, GetCount());
    }

    m_itemsHelpTexts[n] = helpText;
}

// retrieve helptext for a particular item
wxString wxRadioBoxBase::GetItemHelpText( unsigned int n ) const
{
    wxCHECK_MSG( n < GetCount(), wxEmptyString, _T("Invalid item index") );

    return m_itemsHelpTexts.empty() ? wxString() : m_itemsHelpTexts[n];
}

// return help text for the item for which wxEVT_HELP was generated.
wxString wxRadioBoxBase::DoGetHelpTextAtPoint(const wxWindow *derived,
                                              const wxPoint& pt,
                                              wxHelpEvent::Origin origin) const
{
    const int item = origin == wxHelpEvent::Origin_HelpButton
                        ? GetItemFromPoint(pt)
                        : GetSelection();

    if ( item != wxNOT_FOUND )
    {
        wxString text = GetItemHelpText(wx_static_cast(unsigned int, item));
        if( !text.empty() )
            return text;
    }

    return derived->wxWindowBase::GetHelpTextAtPoint(pt, origin);
}

#endif // wxUSE_HELP

#if WXWIN_COMPATIBILITY_2_4

// these functions are deprecated and don't do anything
int wxRadioBoxBase::GetNumberOfRowsOrCols() const
{
    return 1;
}

void wxRadioBoxBase::SetNumberOfRowsOrCols(int WXUNUSED(n))
{
}

#endif // WXWIN_COMPATIBILITY_2_4

#endif // wxUSE_RADIOBOX
