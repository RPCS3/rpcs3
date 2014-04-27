/////////////////////////////////////////////////////////////////////////////
// Name:        src/msw/joystick.cpp
// Purpose:     wxJoystick class
// Author:      Julian Smart
// Modified by:
// Created:     04/01/98
// RCS-ID:      $Id: joystick.cpp 39021 2006-05-04 07:57:04Z ABX $
// Copyright:   (c) Julian Smart
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

// For compilers that support precompilation, includes "wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#if wxUSE_JOYSTICK

#include "wx/joystick.h"

#ifndef WX_PRECOMP
    #include "wx/string.h"
    #include "wx/window.h"
#endif

#include "wx/msw/private.h"

#if !defined(__GNUWIN32_OLD__) || defined(__CYGWIN10__)
    #include <mmsystem.h>
#endif

// Why doesn't BC++ have joyGetPosEx?
#if !defined(__WIN32__) || defined(__BORLANDC__)
#define NO_JOYGETPOSEX
#endif

#include "wx/msw/registry.h"

#include <regstr.h>

IMPLEMENT_DYNAMIC_CLASS(wxJoystick, wxObject)

// Attributes
////////////////////////////////////////////////////////////////////////////

/**
    johan@linkdata.se 2002-08-20:
    Now returns only valid, functioning
    joysticks, counting from the first
    available and upwards.
*/
wxJoystick::wxJoystick(int joystick)
{
    JOYINFO joyInfo;
    int i, maxsticks;

    maxsticks = joyGetNumDevs();
    for( i=0; i<maxsticks; i++ )
    {
        if( joyGetPos(i, & joyInfo) == JOYERR_NOERROR )
        {
            if( !joystick )
            {
                /* Found the one we want, store actual OS id and return */
                m_joystick = i;
                return;
            }
            joystick --;
        }
    }

    /* No such joystick, return ID 0 */
    m_joystick = 0;
    return;
}

wxPoint wxJoystick::GetPosition() const
{
    JOYINFO joyInfo;
    MMRESULT res = joyGetPos(m_joystick, & joyInfo);
    if (res == JOYERR_NOERROR )
        return wxPoint(joyInfo.wXpos, joyInfo.wYpos);
    else
        return wxPoint(0,0);
}

int wxJoystick::GetZPosition() const
{
    JOYINFO joyInfo;
    MMRESULT res = joyGetPos(m_joystick, & joyInfo);
    if (res == JOYERR_NOERROR )
        return joyInfo.wZpos;
    else
        return 0;
}

/**
    johan@linkdata.se 2002-08-20:
    Return a bitmap with all button states in it,
    like the GTK version does and Win32 does.
*/
int wxJoystick::GetButtonState() const
{
    JOYINFO joyInfo;
    MMRESULT res = joyGetPos(m_joystick, & joyInfo);
    if (res == JOYERR_NOERROR )
    {
        return joyInfo.wButtons;
#if 0
        int buttons = 0;

        if (joyInfo.wButtons & JOY_BUTTON1)
            buttons |= wxJOY_BUTTON1;
        if (joyInfo.wButtons & JOY_BUTTON2)
            buttons |= wxJOY_BUTTON2;
        if (joyInfo.wButtons & JOY_BUTTON3)
            buttons |= wxJOY_BUTTON3;
        if (joyInfo.wButtons & JOY_BUTTON4)
            buttons |= wxJOY_BUTTON4;

        return buttons;
#endif
    }
    else
        return 0;
}

/**
    JLI 2002-08-20:
    Returns -1 to signify error.
*/
int wxJoystick::GetPOVPosition() const
{
#ifndef NO_JOYGETPOSEX
    JOYINFOEX joyInfo;
    joyInfo.dwFlags = JOY_RETURNPOV;
    joyInfo.dwSize = sizeof(joyInfo);
    MMRESULT res = joyGetPosEx(m_joystick, & joyInfo);
    if (res == JOYERR_NOERROR )
    {
        return joyInfo.dwPOV;
    }
    else
        return -1;
#else
    return -1;
#endif
}

/**
    johan@linkdata.se 2002-08-20:
    Returns -1 to signify error.
*/
int wxJoystick::GetPOVCTSPosition() const
{
#ifndef NO_JOYGETPOSEX
    JOYINFOEX joyInfo;
    joyInfo.dwFlags = JOY_RETURNPOVCTS;
    joyInfo.dwSize = sizeof(joyInfo);
    MMRESULT res = joyGetPosEx(m_joystick, & joyInfo);
    if (res == JOYERR_NOERROR )
    {
        return joyInfo.dwPOV;
    }
    else
        return -1;
#else
    return -1;
#endif
}

int wxJoystick::GetRudderPosition() const
{
#ifndef NO_JOYGETPOSEX
    JOYINFOEX joyInfo;
    joyInfo.dwFlags = JOY_RETURNR;
    joyInfo.dwSize = sizeof(joyInfo);
    MMRESULT res = joyGetPosEx(m_joystick, & joyInfo);
    if (res == JOYERR_NOERROR )
    {
        return joyInfo.dwRpos;
    }
    else
        return 0;
#else
    return 0;
#endif
}

int wxJoystick::GetUPosition() const
{
#ifndef NO_JOYGETPOSEX
    JOYINFOEX joyInfo;
    joyInfo.dwFlags = JOY_RETURNU;
    joyInfo.dwSize = sizeof(joyInfo);
    MMRESULT res = joyGetPosEx(m_joystick, & joyInfo);
    if (res == JOYERR_NOERROR )
    {
        return joyInfo.dwUpos;
    }
    else
        return 0;
#else
    return 0;
#endif
}

int wxJoystick::GetVPosition() const
{
#ifndef NO_JOYGETPOSEX
    JOYINFOEX joyInfo;
    joyInfo.dwFlags = JOY_RETURNV;
    joyInfo.dwSize = sizeof(joyInfo);
    MMRESULT res = joyGetPosEx(m_joystick, & joyInfo);
    if (res == JOYERR_NOERROR )
    {
        return joyInfo.dwVpos;
    }
    else
        return 0;
#else
    return 0;
#endif
}

int wxJoystick::GetMovementThreshold() const
{
    UINT thresh = 0;
    MMRESULT res = joyGetThreshold(m_joystick, & thresh);
    if (res == JOYERR_NOERROR )
    {
        return thresh;
    }
    else
        return 0;
}

void wxJoystick::SetMovementThreshold(int threshold)
{
    UINT thresh = threshold;
    joySetThreshold(m_joystick, thresh);
}

// Capabilities
////////////////////////////////////////////////////////////////////////////

/**
    johan@linkdata.se 2002-08-20:
    Now returns the number of connected, functioning
    joysticks, as intended.
*/
int wxJoystick::GetNumberJoysticks()
{
    JOYINFO joyInfo;
    int i, maxsticks, actualsticks;
    maxsticks = joyGetNumDevs();
    actualsticks = 0;
    for( i=0; i<maxsticks; i++ )
    {
        if( joyGetPos( i, & joyInfo ) == JOYERR_NOERROR )
        {
            actualsticks ++;
        }
    }
    return actualsticks;
}

/**
    johan@linkdata.se 2002-08-20:
    The old code returned true if there were any
    joystick capable drivers loaded (=always).
*/
bool wxJoystick::IsOk() const
{
    JOYINFO joyInfo;
    return (joyGetPos(m_joystick, & joyInfo) == JOYERR_NOERROR);
}

int wxJoystick::GetManufacturerId() const
{
    JOYCAPS joyCaps;
    if (joyGetDevCaps(m_joystick, & joyCaps, sizeof(JOYCAPS)) != JOYERR_NOERROR)
        return 0;
    else
        return joyCaps.wMid;
}

int wxJoystick::GetProductId() const
{
    JOYCAPS joyCaps;
    if (joyGetDevCaps(m_joystick, & joyCaps, sizeof(JOYCAPS)) != JOYERR_NOERROR)
        return 0;
    else
        return joyCaps.wPid;
}

wxString wxJoystick::GetProductName() const
{
    wxString str;
#ifndef __WINE__
    JOYCAPS joyCaps;
    if (joyGetDevCaps(m_joystick, &joyCaps, sizeof(joyCaps)) != JOYERR_NOERROR)
        return wxEmptyString;

    wxRegKey key1(wxString::Format(wxT("HKEY_LOCAL_MACHINE\\%s\\%s\\%s"),
                   REGSTR_PATH_JOYCONFIG, joyCaps.szRegKey, REGSTR_KEY_JOYCURR));

    key1.QueryValue(wxString::Format(wxT("Joystick%d%s"),
                                     m_joystick + 1, REGSTR_VAL_JOYOEMNAME),
                    str);

    wxRegKey key2(wxString::Format(wxT("HKEY_LOCAL_MACHINE\\%s\\%s"),
                                        REGSTR_PATH_JOYOEM, str.c_str()));
    key2.QueryValue(REGSTR_VAL_JOYOEMNAME, str);
#endif
    return str;
}

int wxJoystick::GetXMin() const
{
    JOYCAPS joyCaps;
    if (joyGetDevCaps(m_joystick, & joyCaps, sizeof(JOYCAPS)) != JOYERR_NOERROR)
        return 0;
    else
        return joyCaps.wXmin;
}

int wxJoystick::GetYMin() const
{
    JOYCAPS joyCaps;
    if (joyGetDevCaps(m_joystick, & joyCaps, sizeof(JOYCAPS)) != JOYERR_NOERROR)
        return 0;
    else
        return joyCaps.wYmin;
}

int wxJoystick::GetZMin() const
{
    JOYCAPS joyCaps;
    if (joyGetDevCaps(m_joystick, & joyCaps, sizeof(JOYCAPS)) != JOYERR_NOERROR)
        return 0;
    else
        return joyCaps.wZmin;
}

int wxJoystick::GetXMax() const
{
    JOYCAPS joyCaps;
    if (joyGetDevCaps(m_joystick, & joyCaps, sizeof(JOYCAPS)) != JOYERR_NOERROR)
        return 0;
    else
        return joyCaps.wXmax;
}

int wxJoystick::GetYMax() const
{
    JOYCAPS joyCaps;
    if (joyGetDevCaps(m_joystick, & joyCaps, sizeof(JOYCAPS)) != JOYERR_NOERROR)
        return 0;
    else
        return joyCaps.wYmax;
}

int wxJoystick::GetZMax() const
{
    JOYCAPS joyCaps;
    if (joyGetDevCaps(m_joystick, & joyCaps, sizeof(JOYCAPS)) != JOYERR_NOERROR)
        return 0;
    else
        return joyCaps.wZmax;
}

int wxJoystick::GetNumberButtons() const
{
    JOYCAPS joyCaps;
    if (joyGetDevCaps(m_joystick, & joyCaps, sizeof(JOYCAPS)) != JOYERR_NOERROR)
        return 0;
    else
        return joyCaps.wNumButtons;
}

int wxJoystick::GetNumberAxes() const
{
#if defined(__WIN32__)
    JOYCAPS joyCaps;
    if (joyGetDevCaps(m_joystick, & joyCaps, sizeof(JOYCAPS)) != JOYERR_NOERROR)
        return 0;
    else
        return joyCaps.wNumAxes;
#else
    return 0;
#endif
}

int wxJoystick::GetMaxButtons() const
{
#if defined(__WIN32__)
    JOYCAPS joyCaps;
    if (joyGetDevCaps(m_joystick, & joyCaps, sizeof(JOYCAPS)) != JOYERR_NOERROR)
        return 0;
    else
        return joyCaps.wMaxButtons;
#else
    return 0;
#endif
}

int wxJoystick::GetMaxAxes() const
{
#if defined(__WIN32__)
    JOYCAPS joyCaps;
    if (joyGetDevCaps(m_joystick, & joyCaps, sizeof(JOYCAPS)) != JOYERR_NOERROR)
        return 0;
    else
        return joyCaps.wMaxAxes;
#else
    return 0;
#endif
}

int wxJoystick::GetPollingMin() const
{
    JOYCAPS joyCaps;
    if (joyGetDevCaps(m_joystick, & joyCaps, sizeof(JOYCAPS)) != JOYERR_NOERROR)
        return 0;
    else
        return joyCaps.wPeriodMin;
}

int wxJoystick::GetPollingMax() const
{
    JOYCAPS joyCaps;
    if (joyGetDevCaps(m_joystick, & joyCaps, sizeof(JOYCAPS)) != JOYERR_NOERROR)
        return 0;
    else
        return joyCaps.wPeriodMax;
}

int wxJoystick::GetRudderMin() const
{
#if defined(__WIN32__)
    JOYCAPS joyCaps;
    if (joyGetDevCaps(m_joystick, & joyCaps, sizeof(JOYCAPS)) != JOYERR_NOERROR)
        return 0;
    else
        return joyCaps.wRmin;
#else
    return 0;
#endif
}

int wxJoystick::GetRudderMax() const
{
#if defined(__WIN32__)
    JOYCAPS joyCaps;
    if (joyGetDevCaps(m_joystick, & joyCaps, sizeof(JOYCAPS)) != JOYERR_NOERROR)
        return 0;
    else
        return joyCaps.wRmax;
#else
    return 0;
#endif
}

int wxJoystick::GetUMin() const
{
#if defined(__WIN32__)
    JOYCAPS joyCaps;
    if (joyGetDevCaps(m_joystick, & joyCaps, sizeof(JOYCAPS)) != JOYERR_NOERROR)
        return 0;
    else
        return joyCaps.wUmin;
#else
    return 0;
#endif
}

int wxJoystick::GetUMax() const
{
#if defined(__WIN32__)
    JOYCAPS joyCaps;
    if (joyGetDevCaps(m_joystick, & joyCaps, sizeof(JOYCAPS)) != JOYERR_NOERROR)
        return 0;
    else
        return joyCaps.wUmax;
#else
    return 0;
#endif
}

int wxJoystick::GetVMin() const
{
#if defined(__WIN32__)
    JOYCAPS joyCaps;
    if (joyGetDevCaps(m_joystick, & joyCaps, sizeof(JOYCAPS)) != JOYERR_NOERROR)
        return 0;
    else
        return joyCaps.wVmin;
#else
    return 0;
#endif
}

int wxJoystick::GetVMax() const
{
#if defined(__WIN32__)
    JOYCAPS joyCaps;
    if (joyGetDevCaps(m_joystick, & joyCaps, sizeof(JOYCAPS)) != JOYERR_NOERROR)
        return 0;
    else
        return joyCaps.wVmax;
#else
    return 0;
#endif
}


bool wxJoystick::HasRudder() const
{
#if defined(__WIN32__)
    JOYCAPS joyCaps;
    if (joyGetDevCaps(m_joystick, & joyCaps, sizeof(JOYCAPS)) != JOYERR_NOERROR)
        return false;
    else
        return ((joyCaps.wCaps & JOYCAPS_HASR) == JOYCAPS_HASR);
#else
    return false;
#endif
}

bool wxJoystick::HasZ() const
{
#if defined(__WIN32__)
    JOYCAPS joyCaps;
    if (joyGetDevCaps(m_joystick, & joyCaps, sizeof(JOYCAPS)) != JOYERR_NOERROR)
        return false;
    else
        return ((joyCaps.wCaps & JOYCAPS_HASZ) == JOYCAPS_HASZ);
#else
    return false;
#endif
}

bool wxJoystick::HasU() const
{
#if defined(__WIN32__)
    JOYCAPS joyCaps;
    if (joyGetDevCaps(m_joystick, & joyCaps, sizeof(JOYCAPS)) != JOYERR_NOERROR)
        return false;
    else
        return ((joyCaps.wCaps & JOYCAPS_HASU) == JOYCAPS_HASU);
#else
    return false;
#endif
}

bool wxJoystick::HasV() const
{
#if defined(__WIN32__)
    JOYCAPS joyCaps;
    if (joyGetDevCaps(m_joystick, & joyCaps, sizeof(JOYCAPS)) != JOYERR_NOERROR)
        return false;
    else
        return ((joyCaps.wCaps & JOYCAPS_HASV) == JOYCAPS_HASV);
#else
    return false;
#endif
}

bool wxJoystick::HasPOV() const
{
#if defined(__WIN32__)
    JOYCAPS joyCaps;
    if (joyGetDevCaps(m_joystick, & joyCaps, sizeof(JOYCAPS)) != JOYERR_NOERROR)
        return false;
    else
        return ((joyCaps.wCaps & JOYCAPS_HASPOV) == JOYCAPS_HASPOV);
#else
    return false;
#endif
}

bool wxJoystick::HasPOV4Dir() const
{
#if defined(__WIN32__)
    JOYCAPS joyCaps;
    if (joyGetDevCaps(m_joystick, & joyCaps, sizeof(JOYCAPS)) != JOYERR_NOERROR)
        return false;
    else
        return ((joyCaps.wCaps & JOYCAPS_POV4DIR) == JOYCAPS_POV4DIR);
#else
    return false;
#endif
}

bool wxJoystick::HasPOVCTS() const
{
#if defined(__WIN32__)
    JOYCAPS joyCaps;
    if (joyGetDevCaps(m_joystick, & joyCaps, sizeof(JOYCAPS)) != JOYERR_NOERROR)
        return false;
    else
        return ((joyCaps.wCaps & JOYCAPS_POVCTS) == JOYCAPS_POVCTS);
#else
    return false;
#endif
}

// Operations
////////////////////////////////////////////////////////////////////////////

bool wxJoystick::SetCapture(wxWindow* win, int pollingFreq)
{
    BOOL changed = (pollingFreq == 0);
    MMRESULT res = joySetCapture((HWND) win->GetHWND(), m_joystick, pollingFreq, changed);
    return (res == JOYERR_NOERROR);
}

bool wxJoystick::ReleaseCapture()
{
    MMRESULT res = joyReleaseCapture(m_joystick);
    return (res == JOYERR_NOERROR);
}

#endif // wxUSE_JOYSTICK
