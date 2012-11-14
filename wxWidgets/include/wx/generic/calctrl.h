///////////////////////////////////////////////////////////////////////////////
// Name:        generic/calctrl.h
// Purpose:     generic implementation of date-picker control
// Author:      Vadim Zeitlin
// Modified by:
// Created:     29.12.99
// RCS-ID:      $Id: calctrl.h 61872 2009-09-09 22:37:05Z VZ $
// Copyright:   (c) 1999 Vadim Zeitlin <zeitlin@dptmaths.ens-cachan.fr>
// Licence:     wxWindows licence
///////////////////////////////////////////////////////////////////////////////

#ifndef _WX_GENERIC_CALCTRL_H
#define _WX_GENERIC_CALCTRL_H

#include "wx/control.h"         // the base class
#include "wx/dcclient.h"        // for wxPaintDC

class WXDLLIMPEXP_FWD_CORE wxComboBox;
class WXDLLIMPEXP_FWD_CORE wxStaticText;
class WXDLLIMPEXP_FWD_CORE wxSpinCtrl;

#define wxCalendarNameStr wxT("CalendarCtrl")

// ----------------------------------------------------------------------------
// wxCalendarCtrl: a control allowing the user to pick a date interactively
// ----------------------------------------------------------------------------

class WXDLLIMPEXP_ADV wxCalendarCtrl : public wxControl
{
public:
    // construction
    wxCalendarCtrl() { Init(); }
    wxCalendarCtrl(wxWindow *parent,
                   wxWindowID id,
                   const wxDateTime& date = wxDefaultDateTime,
                   const wxPoint& pos = wxDefaultPosition,
                   const wxSize& size = wxDefaultSize,
                   long style = wxCAL_SHOW_HOLIDAYS | wxWANTS_CHARS,
                   const wxString& name = wxCalendarNameStr);

    bool Create(wxWindow *parent,
                wxWindowID id,
                const wxDateTime& date = wxDefaultDateTime,
                const wxPoint& pos = wxDefaultPosition,
                const wxSize& size = wxDefaultSize,
                long style = wxCAL_SHOW_HOLIDAYS | wxWANTS_CHARS,
                const wxString& name = wxCalendarNameStr);

    virtual ~wxCalendarCtrl();

    virtual bool Destroy();

    // set/get the current date
    // ------------------------

    bool SetDate(const wxDateTime& date); // we need to be able to control if the event should be sent in SetDateAndNotify(...)
    const wxDateTime& GetDate() const { return m_date; }

    // set/get the range in which selection can occur
    // ---------------------------------------------

    bool SetLowerDateLimit(const wxDateTime& date = wxDefaultDateTime);
    const wxDateTime& GetLowerDateLimit() const { return m_lowdate; }
    bool SetUpperDateLimit(const wxDateTime& date = wxDefaultDateTime);
    const wxDateTime& GetUpperDateLimit() const { return m_highdate; }

    bool SetDateRange(const wxDateTime& lowerdate = wxDefaultDateTime, const wxDateTime& upperdate = wxDefaultDateTime);

    // calendar mode
    // -------------

    // some calendar styles can't be changed after the control creation by
    // just using SetWindowStyle() and Refresh() and the functions below
    // should be used instead for them

    // corresponds to wxCAL_NO_YEAR_CHANGE bit
    void EnableYearChange(bool enable = true);

    // corresponds to wxCAL_NO_MONTH_CHANGE bit
    void EnableMonthChange(bool enable = true);

    // corresponds to wxCAL_SHOW_HOLIDAYS bit
    void EnableHolidayDisplay(bool display = true);

    // customization
    // -------------

    // header colours are used for painting the weekdays at the top
    void SetHeaderColours(const wxColour& colFg, const wxColour& colBg)
    {
        m_colHeaderFg = colFg;
        m_colHeaderBg = colBg;
    }

    const wxColour& GetHeaderColourFg() const { return m_colHeaderFg; }
    const wxColour& GetHeaderColourBg() const { return m_colHeaderBg; }

    // highlight colour is used for the currently selected date
    void SetHighlightColours(const wxColour& colFg, const wxColour& colBg)
    {
        m_colHighlightFg = colFg;
        m_colHighlightBg = colBg;
    }

    const wxColour& GetHighlightColourFg() const { return m_colHighlightFg; }
    const wxColour& GetHighlightColourBg() const { return m_colHighlightBg; }

    // holiday colour is used for the holidays (if style & wxCAL_SHOW_HOLIDAYS)
    void SetHolidayColours(const wxColour& colFg, const wxColour& colBg)
    {
        m_colHolidayFg = colFg;
        m_colHolidayBg = colBg;
    }

    const wxColour& GetHolidayColourFg() const { return m_colHolidayFg; }
    const wxColour& GetHolidayColourBg() const { return m_colHolidayBg; }

    // an item without custom attributes is drawn with the default colours and
    // font and without border, setting custom attributes allows to modify this
    //
    // the day parameter should be in 1..31 range, for days 29, 30, 31 the
    // corresponding attribute is just unused if there is no such day in the
    // current month

    wxCalendarDateAttr *GetAttr(size_t day) const
    {
        wxCHECK_MSG( day > 0 && day < 32, NULL, wxT("invalid day") );

        return m_attrs[day - 1];
    }

    void SetAttr(size_t day, wxCalendarDateAttr *attr)
    {
        wxCHECK_RET( day > 0 && day < 32, wxT("invalid day") );

        delete m_attrs[day - 1];
        m_attrs[day - 1] = attr;
    }

    void SetHoliday(size_t day);

    void ResetAttr(size_t day) { SetAttr(day, (wxCalendarDateAttr *)NULL); }

    // returns one of wxCAL_HITTEST_XXX constants and fills either date or wd
    // with the corresponding value (none for NOWHERE, the date for DAY and wd
    // for HEADER)
    wxCalendarHitTestResult HitTest(const wxPoint& pos,
                                    wxDateTime *date = NULL,
                                    wxDateTime::WeekDay *wd = NULL);

    // implementation only from now on
    // -------------------------------

    // forward these functions to all subcontrols
    virtual bool Enable(bool enable = true);
    virtual bool Show(bool show = true);

    virtual void SetWindowStyleFlag(long style);

    virtual wxVisualAttributes GetDefaultAttributes() const
        { return GetClassDefaultAttributes(GetWindowVariant()); }

    static wxVisualAttributes
    GetClassDefaultAttributes(wxWindowVariant variant = wxWINDOW_VARIANT_NORMAL);

    void OnSysColourChanged(wxSysColourChangedEvent& event);

protected:
    // override some base class virtuals
    virtual wxSize DoGetBestSize() const;
    virtual void DoGetPosition(int *x, int *y) const;
    virtual void DoGetSize(int *width, int *height) const;
    virtual void DoSetSize(int x, int y, int width, int height, int sizeFlags);
    virtual void DoMoveWindow(int x, int y, int width, int height);

private:
    // common part of all ctors
    void Init();

    // startup colours and reinitialization after colour changes in system
    void InitColours();

    // event handlers
    void OnPaint(wxPaintEvent& event);
    void OnClick(wxMouseEvent& event);
    void OnDClick(wxMouseEvent& event);
    void OnChar(wxKeyEvent& event);
    void OnMonthChange(wxCommandEvent& event);
    void OnYearChange(wxCommandEvent& event);
    void OnYearTextChange(wxCommandEvent& event);

    // (re)calc m_widthCol and m_heightRow
    void RecalcGeometry();

    // set the date and send the notification
    void SetDateAndNotify(const wxDateTime& date);

    // get the week (row, in range 1..6) for the given date
    size_t GetWeek(const wxDateTime& date) const;

    // get the date from which we start drawing days
    wxDateTime GetStartDate() const;

    // is this date shown?
    bool IsDateShown(const wxDateTime& date) const;

    // is this date in the given range?
    bool IsDateInRange(const wxDateTime& date) const;

    // range helpers
    bool ChangeYear(wxDateTime* target) const;
    bool ChangeMonth(wxDateTime* target) const;

    // redraw the given date
    void RefreshDate(const wxDateTime& date);

    // change the date inside the same month/year
    void ChangeDay(const wxDateTime& date);

    // set the attributes for the holidays if needed
    void SetHolidayAttrs();

    // reset all holidays
    void ResetHolidayAttrs();

    // generate the given calendar event(s)
    void GenerateEvent(wxEventType type)
    {
        wxCalendarEvent event(this, type);
        (void)GetEventHandler()->ProcessEvent(event);
    }

    void GenerateEvents(wxEventType type1, wxEventType type2)
    {
        GenerateEvent(type1);
        GenerateEvent(type2);
    }

    // do we allow changing the month/year?
    bool AllowMonthChange() const
    {
        return (GetWindowStyle() & wxCAL_NO_MONTH_CHANGE)
                != wxCAL_NO_MONTH_CHANGE;
    }
    bool AllowYearChange() const
    {
        return !(GetWindowStyle() & wxCAL_NO_YEAR_CHANGE);
    }

    // show the correct controls
    void ShowCurrentControls();

    // create the month combo and year spin controls
    void CreateMonthComboBox();
    void CreateYearSpinCtrl();

public:
    // get the currently shown control for month/year
    wxControl *GetMonthControl() const;
    wxControl *GetYearControl() const;

private:
    // OnPaint helper-methods

    // Highlight the [fromdate : todate] range using pen and brush
    void HighlightRange(wxPaintDC* dc, const wxDateTime& fromdate, const wxDateTime& todate, const wxPen* pen, const wxBrush* brush);

    // Get the "coordinates" for the date relative to the month currently displayed.
    // using (day, week): upper left coord is (1, 1), lower right coord is (7, 6)
    // if the date isn't visible (-1, -1) is put in (day, week) and false is returned
    bool GetDateCoord(const wxDateTime& date, int *day, int *week) const;

    // Set the flag for SetDate(): otherwise it would overwrite the year
    // typed in by the user
    void SetUserChangedYear() { m_userChangedYear = true; }

    // the subcontrols
    wxStaticText *m_staticMonth;
    wxComboBox *m_comboMonth;

    wxStaticText *m_staticYear;
    wxSpinCtrl *m_spinYear;

    // the current selection
    wxDateTime m_date;

    // the date-range
    wxDateTime m_lowdate;
    wxDateTime m_highdate;

    // default attributes
    wxColour m_colHighlightFg,
             m_colHighlightBg,
             m_colHolidayFg,
             m_colHolidayBg,
             m_colHeaderFg,
             m_colHeaderBg,
             m_colBackground,
             m_colSorrounding;

    // the attributes for each of the month days
    wxCalendarDateAttr *m_attrs[31];

    // the width and height of one column/row in the calendar
    wxCoord m_widthCol,
            m_heightRow,
            m_rowOffset;

    wxRect m_leftArrowRect,
           m_rightArrowRect;

    // the week day names
    wxString m_weekdays[7];

    // true if SetDate() is being called as the result of changing the year in
    // the year control
    bool m_userChangedYear;

    DECLARE_DYNAMIC_CLASS(wxCalendarCtrl)
    DECLARE_EVENT_TABLE()
    DECLARE_NO_COPY_CLASS(wxCalendarCtrl)
};

#endif // _WX_GENERIC_CALCTRL_H
