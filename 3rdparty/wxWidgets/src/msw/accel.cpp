/////////////////////////////////////////////////////////////////////////////
// Name:        msw/accel.cpp
// Purpose:     wxAcceleratorTable
// Author:      Julian Smart
// Modified by:
// Created:     04/01/98
// RCS-ID:      $Id: accel.cpp 56049 2008-10-03 12:13:21Z VZ $
// Copyright:   (c) Julian Smart
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

#if wxUSE_ACCEL

#ifndef WX_PRECOMP
    #include "wx/window.h"
#endif

#include "wx/accel.h"

#include "wx/msw/private.h"

extern WXWORD wxCharCodeWXToMSW(int id, bool *isVirtual);

IMPLEMENT_DYNAMIC_CLASS(wxAcceleratorTable, wxObject)

// ----------------------------------------------------------------------------
// data defining wxAcceleratorTable
// ----------------------------------------------------------------------------

class WXDLLEXPORT wxAcceleratorRefData: public wxObjectRefData
{
    friend class WXDLLIMPEXP_FWD_CORE wxAcceleratorTable;
public:
    wxAcceleratorRefData();
    virtual ~wxAcceleratorRefData();

    inline HACCEL GetHACCEL() const { return m_hAccel; }
protected:
    HACCEL      m_hAccel;
    bool        m_ok;

    DECLARE_NO_COPY_CLASS(wxAcceleratorRefData)
};

// ============================================================================
// implementation
// ============================================================================

// ----------------------------------------------------------------------------
// wxAcceleratorRefData
// ----------------------------------------------------------------------------

#define M_ACCELDATA ((wxAcceleratorRefData *)m_refData)

wxAcceleratorRefData::wxAcceleratorRefData()
{
    m_ok = false;
    m_hAccel = 0;
}

wxAcceleratorRefData::~wxAcceleratorRefData()
{
    if (m_hAccel)
    {
        DestroyAcceleratorTable((HACCEL) m_hAccel);
    }
}

// ----------------------------------------------------------------------------
// wxAcceleratorTable
// ----------------------------------------------------------------------------

// Load from .rc resource
wxAcceleratorTable::wxAcceleratorTable(const wxString& resource)
{
    m_refData = new wxAcceleratorRefData;

    HACCEL hAccel = ::LoadAccelerators(wxGetInstance(), resource);
    M_ACCELDATA->m_hAccel = hAccel;
    M_ACCELDATA->m_ok = hAccel != 0;
}

// Create from an array
wxAcceleratorTable::wxAcceleratorTable(int n, const wxAcceleratorEntry entries[])
{
    m_refData = new wxAcceleratorRefData;

    ACCEL* arr = new ACCEL[n];
    for ( int i = 0; i < n; i++ )
    {
        int flags = entries[i].GetFlags();

        BYTE fVirt = 0;
        if ( flags & wxACCEL_ALT )
            fVirt |= FALT | FVIRTKEY;
        if ( flags & wxACCEL_SHIFT )
            fVirt |= FSHIFT | FVIRTKEY;
        if ( flags & wxACCEL_CTRL )
            fVirt |= FCONTROL | FVIRTKEY;

        bool isVirtual;

        WORD key = wxCharCodeWXToMSW(entries[i].GetKeyCode(), &isVirtual);
        if (isVirtual)
            fVirt |= FVIRTKEY;

        arr[i].fVirt = fVirt;
        arr[i].key = key;
        arr[i].cmd = (WORD)entries[i].GetCommand();
    }

    M_ACCELDATA->m_hAccel = ::CreateAcceleratorTable(arr, n);
    delete[] arr;

    M_ACCELDATA->m_ok = (M_ACCELDATA->m_hAccel != 0);
}

bool wxAcceleratorTable::IsOk() const
{
    return (M_ACCELDATA && (M_ACCELDATA->m_ok));
}

void wxAcceleratorTable::SetHACCEL(WXHACCEL hAccel)
{
    if (!M_ACCELDATA)
        m_refData = new wxAcceleratorRefData;

    M_ACCELDATA->m_hAccel = (HACCEL) hAccel;
}

WXHACCEL wxAcceleratorTable::GetHACCEL() const
{
    if (!M_ACCELDATA)
        return 0;
    return (WXHACCEL) M_ACCELDATA->m_hAccel;
}

bool wxAcceleratorTable::Translate(wxWindow *window, WXMSG *wxmsg) const
{
    MSG *msg = (MSG *)wxmsg;
    return Ok() && ::TranslateAccelerator(GetHwndOf(window), GetHaccel(), msg);
}

#endif // wxUSE_ACCEL

