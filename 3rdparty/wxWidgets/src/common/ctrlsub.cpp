///////////////////////////////////////////////////////////////////////////////
// Name:        src/common/ctrlsub.cpp
// Purpose:     wxItemContainer implementation
// Author:      Vadim Zeitlin
// Modified by:
// Created:     22.10.99
// RCS-ID:      $Id: ctrlsub.cpp 39077 2006-05-06 19:05:50Z VZ $
// Copyright:   (c) wxWidgets team
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

#if wxUSE_CONTROLS

#ifndef WX_PRECOMP
    #include "wx/ctrlsub.h"
    #include "wx/arrstr.h"
#endif

IMPLEMENT_ABSTRACT_CLASS(wxControlWithItems, wxControl)

// ============================================================================
// wxItemContainerImmutable implementation
// ============================================================================

wxItemContainerImmutable::~wxItemContainerImmutable()
{
    // this destructor is required for Darwin
}

// ----------------------------------------------------------------------------
// selection
// ----------------------------------------------------------------------------

wxString wxItemContainerImmutable::GetStringSelection() const
{
    wxString s;

    int sel = GetSelection();
    if ( sel != wxNOT_FOUND )
        s = GetString((unsigned int)sel);

    return s;
}

bool wxItemContainerImmutable::SetStringSelection(const wxString& s)
{
    const int sel = FindString(s);
    if ( sel == wxNOT_FOUND )
        return false;

    SetSelection(sel);

    return true;
}

wxArrayString wxItemContainerImmutable::GetStrings() const
{
    wxArrayString result;

    const unsigned int count = GetCount();
    result.Alloc(count);
    for ( unsigned int n = 0; n < count; n++ )
        result.Add(GetString(n));

    return result;
}

// ============================================================================
// wxItemContainer implementation
// ============================================================================

wxItemContainer::~wxItemContainer()
{
    // this destructor is required for Darwin
}

// ----------------------------------------------------------------------------
// appending items
// ----------------------------------------------------------------------------

void wxItemContainer::Append(const wxArrayString& strings)
{
    const size_t count = strings.GetCount();
    for ( size_t n = 0; n < count; n++ )
    {
        Append(strings[n]);
    }
}

int wxItemContainer::Insert(const wxString& item, unsigned int pos, void *clientData)
{
    int n = DoInsert(item, pos);
    if ( n != wxNOT_FOUND )
        SetClientData(n, clientData);

    return n;
}

int wxItemContainer::Insert(const wxString& item, unsigned int pos, wxClientData *clientData)
{
    int n = DoInsert(item, pos);
    if ( n != wxNOT_FOUND )
        SetClientObject(n, clientData);

    return n;
}

// ----------------------------------------------------------------------------
// client data
// ----------------------------------------------------------------------------

void wxItemContainer::SetClientObject(unsigned int n, wxClientData *data)
{
    wxASSERT_MSG( m_clientDataItemsType != wxClientData_Void,
                  wxT("can't have both object and void client data") );

    // when we call SetClientObject() for the first time, m_clientDataItemsType
    // is still wxClientData_None and so calling DoGetItemClientObject() would
    // fail (in addition to being useless) - don't do it
    if ( m_clientDataItemsType == wxClientData_Object )
    {
        wxClientData *clientDataOld = DoGetItemClientObject(n);
        if ( clientDataOld )
            delete clientDataOld;
    }
    else // m_clientDataItemsType == wxClientData_None
    {
        // now we have object client data
        m_clientDataItemsType = wxClientData_Object;
    }

    DoSetItemClientObject(n, data);
}

wxClientData *wxItemContainer::GetClientObject(unsigned int n) const
{
    wxASSERT_MSG( m_clientDataItemsType == wxClientData_Object,
                  wxT("this window doesn't have object client data") );

    return DoGetItemClientObject(n);
}

void wxItemContainer::SetClientData(unsigned int n, void *data)
{
    wxASSERT_MSG( m_clientDataItemsType != wxClientData_Object,
                  wxT("can't have both object and void client data") );

    DoSetItemClientData(n, data);
    m_clientDataItemsType = wxClientData_Void;
}

void *wxItemContainer::GetClientData(unsigned int n) const
{
    wxASSERT_MSG( m_clientDataItemsType == wxClientData_Void,
                  wxT("this window doesn't have void client data") );

    return DoGetItemClientData(n);
}

// ============================================================================
// wxControlWithItems implementation
// ============================================================================

void wxControlWithItems::InitCommandEventWithItems(wxCommandEvent& event, int n)
{
    InitCommandEvent(event);

    if ( n != wxNOT_FOUND )
    {
        if ( HasClientObjectData() )
            event.SetClientObject(GetClientObject(n));
        else if ( HasClientUntypedData() )
            event.SetClientData(GetClientData(n));
    }
}

wxControlWithItems::~wxControlWithItems()
{
    // this destructor is required for Darwin
}

#endif // wxUSE_CONTROLS
