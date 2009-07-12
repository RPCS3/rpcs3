/////////////////////////////////////////////////////////////////////////////
// Name:        src/generic/datectlg.cpp
// Purpose:     generic wxDatePickerCtrlGeneric implementation
// Author:      Andreas Pflug
// Modified by:
// Created:     2005-01-19
// RCS-ID:      $Id: datectlg.cpp 54407 2008-06-28 18:58:07Z VZ $
// Copyright:   (c) 2005 Andreas Pflug <pgadmin@pse-consulting.de>
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

// ============================================================================
// declarations
// ============================================================================

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#if wxUSE_DATEPICKCTRL

#include "wx/datectrl.h"

// use this version if we're explicitly requested to do it or if it's the only
// one we have
#if !defined(wxHAS_NATIVE_DATEPICKCTRL) || \
        (defined(wxUSE_DATEPICKCTRL_GENERIC) && wxUSE_DATEPICKCTRL_GENERIC)

#ifndef WX_PRECOMP
    #include "wx/dialog.h"
    #include "wx/dcmemory.h"
    #include "wx/panel.h"
    #include "wx/textctrl.h"
    #include "wx/valtext.h"
#endif

#ifdef wxHAS_NATIVE_DATEPICKCTRL
    // this header is not included from wx/datectrl.h if we have a native
    // version, but we do need it here
    #include "wx/generic/datectrl.h"
#else
    // we need to define _WX_DEFINE_DATE_EVENTS_ before including wx/dateevt.h to
    // define the event types we use if we're the only date picker control version
    // being compiled -- otherwise it's defined in the native version implementation
    #define _WX_DEFINE_DATE_EVENTS_
#endif

#include "wx/dateevt.h"

#include "wx/calctrl.h"
#include "wx/combo.h"

// ----------------------------------------------------------------------------
// constants
// ----------------------------------------------------------------------------

#if defined(__WXMSW__)
    #define CALBORDER         0
#else
    #define CALBORDER         4
#endif

// ----------------------------------------------------------------------------
// global variables
// ----------------------------------------------------------------------------


// ----------------------------------------------------------------------------
// local classes
// ----------------------------------------------------------------------------

class wxCalendarComboPopup : public wxCalendarCtrl,
                             public wxComboPopup
{
public:

    wxCalendarComboPopup() : wxCalendarCtrl(),
                             wxComboPopup()
    {
    }

    virtual void Init()
    {
    }

    // NB: Don't create lazily since it didn't work that way before
    //     wxComboCtrl was used, and changing behaviour would almost
    //     certainly introduce new bugs.
    virtual bool Create(wxWindow* parent)
    {
        if ( !wxCalendarCtrl::Create(parent, wxID_ANY, wxDefaultDateTime,
                              wxPoint(0, 0), wxDefaultSize,
                              wxCAL_SHOW_HOLIDAYS | wxBORDER_SUNKEN) )
            return false;

        wxWindow *yearControl = wxCalendarCtrl::GetYearControl();

        wxClientDC dc(yearControl);
        dc.SetFont(yearControl->GetFont());
        wxCoord width, dummy;
        dc.GetTextExtent(wxT("2000"), &width, &dummy);
        width += ConvertDialogToPixels(wxSize(20, 0)).x;

        wxSize calSize = wxCalendarCtrl::GetBestSize();
        wxSize yearSize = yearControl->GetSize();
        yearSize.x = width;

        wxPoint yearPosition = yearControl->GetPosition();

        SetFormat(wxT("%x"));

        width = yearPosition.x + yearSize.x+2+CALBORDER/2;
        if (width < calSize.x-4)
            width = calSize.x-4;

        int calPos = (width-calSize.x)/2;
        if (calPos == -1)
        {
            calPos = 0;
            width += 2;
        }
        wxCalendarCtrl::SetSize(calPos, 0, calSize.x, calSize.y);
        yearControl->SetSize(width-yearSize.x-CALBORDER/2, yearPosition.y,
                             yearSize.x, yearSize.y);
        wxCalendarCtrl::GetMonthControl()->Move(0, 0);

        m_useSize.x = width+CALBORDER/2;
        m_useSize.y = calSize.y-2+CALBORDER;

        wxWindow* tx = m_combo->GetTextCtrl();
        if ( !tx )
            tx = m_combo;

        tx->Connect(wxEVT_KILL_FOCUS,
                    wxFocusEventHandler(wxCalendarComboPopup::OnKillTextFocus),
                    NULL, this);

        return true;
    }

    virtual wxSize GetAdjustedSize(int WXUNUSED(minWidth),
                                   int WXUNUSED(prefHeight),
                                   int WXUNUSED(maxHeight))
    {
        return m_useSize;
    }

    virtual wxWindow *GetControl() { return this; }

    void SetDateValue(const wxDateTime& date)
    {
        if ( date.IsValid() )
        {
            m_combo->SetText(date.Format(m_format));
            SetDate(date);
        }
        else // invalid date
        {
            wxASSERT_MSG( HasDPFlag(wxDP_ALLOWNONE),
                            _T("this control must have a valid date") );

            m_combo->SetText(wxEmptyString);
        }
    }

    bool IsTextEmpty() const
    {
        return m_combo->GetTextCtrl()->IsEmpty();
    }

    bool ParseDateTime(const wxString& s, wxDateTime* pDt)
    {
        wxASSERT(pDt);

        if ( !s.empty() )
        {
            pDt->ParseFormat(s, m_format);
            if ( !pDt->IsValid() )
                return false;
        }

        return true;
    }

    void SendDateEvent(const wxDateTime& dt)
    {
        //
        // Sends both wxCalendarEvent and wxDateEvent
        wxWindow* datePicker = m_combo->GetParent();

        wxCalendarEvent cev((wxCalendarCtrl*) this, wxEVT_CALENDAR_SEL_CHANGED);
        cev.SetEventObject(datePicker);
        cev.SetId(datePicker->GetId());
        cev.SetDate(dt);
        datePicker->GetEventHandler()->ProcessEvent(cev);

        wxDateEvent event(datePicker, dt, wxEVT_DATE_CHANGED);
        datePicker->GetEventHandler()->ProcessEvent(event);
    }

private:

    void OnCalKey(wxKeyEvent & ev)
    {
        if (ev.GetKeyCode() == WXK_ESCAPE && !ev.HasModifiers())
            Dismiss();
        else
            ev.Skip();
    }

    void OnSelChange(wxCalendarEvent &ev)
    {
        m_combo->SetText(GetDate().Format(m_format));

        if ( ev.GetEventType() == wxEVT_CALENDAR_DOUBLECLICKED )
        {
            Dismiss();
        }

        SendDateEvent(GetDate());
    }

    void OnKillTextFocus(wxFocusEvent &ev)
    {
        ev.Skip();

        const wxDateTime& dtOld = GetDate();

        wxDateTime dt;
        wxString value = m_combo->GetValue();
        if ( !ParseDateTime(value, &dt) )
        {
            if ( !HasDPFlag(wxDP_ALLOWNONE) )
                dt = dtOld;
        }

        m_combo->SetText(GetStringValueFor(dt));

        if ( !dt.IsValid() && HasDPFlag(wxDP_ALLOWNONE) )
            return;
        
        // notify that we had to change the date after validation
        if ( (dt.IsValid() && (!dtOld.IsValid() || dt != dtOld)) ||
                (!dt.IsValid() && dtOld.IsValid()) )
        {
            SetDate(dt);
            SendDateEvent(dt);
        }
    }

    bool HasDPFlag(int flag)
    {
        return m_combo->GetParent()->HasFlag(flag);
    }

    bool SetFormat(const wxChar *fmt)
    {
        m_format.clear();

        wxDateTime dt;
        dt.ParseFormat(wxT("2003-10-13"), wxT("%Y-%m-%d"));
        wxString str(dt.Format(fmt));

        const wxChar *p = str.c_str();
        while ( *p )
        {
            int n=wxAtoi(p);
            if (n == dt.GetDay())
            {
                m_format.Append(wxT("%d"));
                p += 2;
            }
            else if (n == (int)dt.GetMonth()+1)
            {
                m_format.Append(wxT("%m"));
                p += 2;
            }
            else if (n == dt.GetYear())
            {
                m_format.Append(wxT("%Y"));
                p += 4;
            }
            else if (n == (dt.GetYear() % 100))
            {
                if ( HasDPFlag(wxDP_SHOWCENTURY) )
                    m_format.Append(wxT("%Y"));
                else
                    m_format.Append(wxT("%y"));
                p += 2;
            }
            else
                m_format.Append(*p++);
        }

        if ( m_combo )
        {
            wxArrayString allowedChars;
            for ( wxChar c = _T('0'); c <= _T('9'); c++ )
                allowedChars.Add(wxString(c, 1));

            const wxChar *p2 = m_format.c_str();
            while ( *p2 )
            {
                if ( *p2 == '%')
                    p2 += 2;
                else
                    allowedChars.Add(wxString(*p2++, 1));
            }

    #if wxUSE_VALIDATORS
            wxTextValidator tv(wxFILTER_INCLUDE_CHAR_LIST);
            tv.SetIncludes(allowedChars);
            m_combo->SetValidator(tv);
    #endif

            if ( GetDate().IsValid() )
                m_combo->SetText(GetDate().Format(m_format));
        }

        return true;
    }

    virtual void SetStringValue(const wxString& s)
    {
        wxDateTime dt;
        if ( !s.empty() && ParseDateTime(s, &dt) )
            SetDate(dt);
        //else: keep the old value
    }

    virtual wxString GetStringValue() const
    {
        return GetStringValueFor(GetDate());
    }

private:
    // returns either the given date representation using the current format or
    // an empty string if it's invalid
    wxString GetStringValueFor(const wxDateTime& dt) const
    {
        wxString val;
        if ( dt.IsValid() )
            val = dt.Format(m_format);

        return val;
    }

    wxSize          m_useSize;
    wxString        m_format;

    DECLARE_EVENT_TABLE()
};


BEGIN_EVENT_TABLE(wxCalendarComboPopup, wxCalendarCtrl)
    EVT_KEY_DOWN(wxCalendarComboPopup::OnCalKey)
    EVT_CALENDAR_SEL_CHANGED(wxID_ANY, wxCalendarComboPopup::OnSelChange)
    EVT_CALENDAR_DAY(wxID_ANY, wxCalendarComboPopup::OnSelChange)
    EVT_CALENDAR_MONTH(wxID_ANY, wxCalendarComboPopup::OnSelChange)
    EVT_CALENDAR_YEAR(wxID_ANY, wxCalendarComboPopup::OnSelChange)
    EVT_CALENDAR(wxID_ANY, wxCalendarComboPopup::OnSelChange)
END_EVENT_TABLE()


// ============================================================================
// wxDatePickerCtrlGeneric implementation
// ============================================================================

BEGIN_EVENT_TABLE(wxDatePickerCtrlGeneric, wxDatePickerCtrlBase)
    EVT_TEXT(wxID_ANY, wxDatePickerCtrlGeneric::OnText)
    EVT_SIZE(wxDatePickerCtrlGeneric::OnSize)
    EVT_SET_FOCUS(wxDatePickerCtrlGeneric::OnFocus)
END_EVENT_TABLE()

#ifndef wxHAS_NATIVE_DATEPICKCTRL
    IMPLEMENT_DYNAMIC_CLASS(wxDatePickerCtrl, wxControl)
#endif

// ----------------------------------------------------------------------------
// creation
// ----------------------------------------------------------------------------

bool wxDatePickerCtrlGeneric::Create(wxWindow *parent,
                                     wxWindowID id,
                                     const wxDateTime& date,
                                     const wxPoint& pos,
                                     const wxSize& size,
                                     long style,
                                     const wxValidator& validator,
                                     const wxString& name)
{
    wxASSERT_MSG( !(style & wxDP_SPIN),
                  _T("wxDP_SPIN style not supported, use wxDP_DEFAULT") );

    if ( !wxControl::Create(parent, id, pos, size,
                            style | wxCLIP_CHILDREN | wxWANTS_CHARS | wxBORDER_NONE,
                            validator, name) )
    {
        return false;
    }

    InheritAttributes();

    m_combo = new wxComboCtrl(this, -1, wxEmptyString,
                              wxDefaultPosition, wxDefaultSize);

    m_combo->SetCtrlMainWnd(this);

    m_popup = new wxCalendarComboPopup();
    m_cal = m_popup;

#if defined(__WXMSW__)
    // without this keyboard navigation in month control doesn't work
    m_combo->UseAltPopupWindow();
#endif
    m_combo->SetPopupControl(m_popup);

    m_popup->SetDateValue(date.IsValid() ? date : wxDateTime::Today());

    SetInitialSize(size);

    return true;
}


void wxDatePickerCtrlGeneric::Init()
{
    m_combo = NULL;
    m_popup = NULL;
    m_cal = NULL;
}

wxDatePickerCtrlGeneric::~wxDatePickerCtrlGeneric()
{
}

bool wxDatePickerCtrlGeneric::Destroy()
{
    if ( m_combo )
        m_combo->Destroy();

    m_combo = NULL;
    m_popup = NULL;
    m_cal = NULL;

    return wxControl::Destroy();
}

// ----------------------------------------------------------------------------
// overridden base class methods
// ----------------------------------------------------------------------------

wxSize wxDatePickerCtrlGeneric::DoGetBestSize() const
{
    return m_combo->GetBestSize();
}

// ----------------------------------------------------------------------------
// wxDatePickerCtrlGeneric API
// ----------------------------------------------------------------------------

bool
wxDatePickerCtrlGeneric::SetDateRange(const wxDateTime& lowerdate,
                                      const wxDateTime& upperdate)
{
    return m_popup->SetDateRange(lowerdate, upperdate);
}


wxDateTime wxDatePickerCtrlGeneric::GetValue() const
{
    if ( HasFlag(wxDP_ALLOWNONE) && m_popup->IsTextEmpty() )
        return wxInvalidDateTime;
    return m_popup->GetDate();
}


void wxDatePickerCtrlGeneric::SetValue(const wxDateTime& date)
{
    m_popup->SetDateValue(date);
}


bool wxDatePickerCtrlGeneric::GetRange(wxDateTime *dt1, wxDateTime *dt2) const
{
    if (dt1)
        *dt1 = m_popup->GetLowerDateLimit();
    if (dt2)
        *dt2 = m_popup->GetUpperDateLimit();
    return true;
}


void
wxDatePickerCtrlGeneric::SetRange(const wxDateTime &dt1, const wxDateTime &dt2)
{
    m_popup->SetDateRange(dt1, dt2);
}

// ----------------------------------------------------------------------------
// event handlers
// ----------------------------------------------------------------------------


void wxDatePickerCtrlGeneric::OnSize(wxSizeEvent& event)
{
    if ( m_combo )
        m_combo->SetSize(GetClientSize());

    event.Skip();
}


void wxDatePickerCtrlGeneric::OnText(wxCommandEvent &ev)
{
    ev.SetEventObject(this);
    ev.SetId(GetId());
    GetParent()->ProcessEvent(ev);

    // We'll create an additional event if the date is valid.
    // If the date isn't valid, the user's probably in the middle of typing
    wxDateTime dt;
    if ( !m_popup->ParseDateTime(m_combo->GetValue(), &dt) )
        return;

    m_popup->SendDateEvent(dt);
}


void wxDatePickerCtrlGeneric::OnFocus(wxFocusEvent& WXUNUSED(event))
{
    m_combo->SetFocus();
}


#endif // wxUSE_DATEPICKCTRL_GENERIC

#endif // wxUSE_DATEPICKCTRL

