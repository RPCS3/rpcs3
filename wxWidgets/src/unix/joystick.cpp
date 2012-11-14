/////////////////////////////////////////////////////////////////////////////
// Name:        src/unix/joystick.cpp
// Purpose:     wxJoystick class
// Author:      Ported to Linux by Guilhem Lavaux
// Modified by:
// Created:     05/23/98
// RCS-ID:      $Id: joystick.cpp 40530 2006-08-09 11:18:24Z MW $
// Copyright:   (c) Guilhem Lavaux
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

// for compilers that support precompilation, includes "wx.h".
#include "wx/wxprec.h"

#if wxUSE_JOYSTICK

#include "wx/joystick.h"

#ifndef WX_PRECOMP
    #include "wx/event.h"
    #include "wx/window.h"
#endif //WX_PRECOMP

#include <linux/joystick.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>

#ifdef HAVE_SYS_SELECT_H
#   include <sys/select.h>
#endif

#include "wx/unix/private.h"

enum {
    wxJS_AXIS_X = 0,
    wxJS_AXIS_Y,
    wxJS_AXIS_Z,
    wxJS_AXIS_RUDDER,
    wxJS_AXIS_U,
    wxJS_AXIS_V,

    wxJS_AXIS_MAX = 32767,
    wxJS_AXIS_MIN = -32767
};


IMPLEMENT_DYNAMIC_CLASS(wxJoystick, wxObject)


////////////////////////////////////////////////////////////////////////////
// Background thread for reading the joystick device
////////////////////////////////////////////////////////////////////////////

class wxJoystickThread : public wxThread
{
public:
    wxJoystickThread(int device, int joystick);
    void* Entry();

private:
    int       m_device;
    int       m_joystick;
    wxPoint   m_lastposition;
    int       m_axe[15];
    int       m_buttons;
    wxWindow* m_catchwin;
    int       m_polling;

    friend class wxJoystick;
};


wxJoystickThread::wxJoystickThread(int device, int joystick)
    : m_device(device),
      m_joystick(joystick),
      m_lastposition(wxDefaultPosition),
      m_buttons(0),
      m_catchwin(NULL),
      m_polling(0)
{
    for (int i=0; i<15; i++)
        m_axe[i] = 0;
}


void* wxJoystickThread::Entry()
{
    struct js_event j_evt;
    fd_set read_fds;
    struct timeval time_out = {0, 0};

    wxFD_ZERO(&read_fds);
    while (true)
    {
        if (TestDestroy())
            break;

        // We use select when either polling or 'blocking' as even in the
        // blocking case we need to check TestDestroy periodically
        if (m_polling)
            time_out.tv_usec = m_polling * 1000;
        else
            time_out.tv_usec = 10 * 1000; // check at least every 10 msec in blocking case

        wxFD_SET(m_device, &read_fds);
        select(m_device+1, &read_fds, NULL, NULL, &time_out);
        if (wxFD_ISSET(m_device, &read_fds))
        {
            memset(&j_evt, 0, sizeof(j_evt));
            read(m_device, &j_evt, sizeof(j_evt));

            //printf("time: %d\t value: %d\t type: %d\t number: %d\n",
            //       j_evt.time, j_evt.value, j_evt.type, j_evt.number);

            wxJoystickEvent jwx_event;

            if (j_evt.type & JS_EVENT_AXIS)
            {
                m_axe[j_evt.number] = j_evt.value;

                switch (j_evt.number)
                {
                    case wxJS_AXIS_X:
                        m_lastposition.x = j_evt.value;
                        jwx_event.SetEventType(wxEVT_JOY_MOVE);
                        break;
                    case wxJS_AXIS_Y:
                        m_lastposition.y = j_evt.value;
                        jwx_event.SetEventType(wxEVT_JOY_MOVE);
                        break;
                    case wxJS_AXIS_Z:
                        jwx_event.SetEventType(wxEVT_JOY_ZMOVE);
                        break;
                    default:
                        jwx_event.SetEventType(wxEVT_JOY_MOVE);
                        // TODO: There should be a way to indicate that the event
                        //       is for some other axes.
                        break;
                }
            }

            if (j_evt.type & JS_EVENT_BUTTON)
            {
                if (j_evt.value)
                {
                    m_buttons |= (1 << j_evt.number);
                    jwx_event.SetEventType(wxEVT_JOY_BUTTON_DOWN);
                }
                else
                {
                    m_buttons &= ~(1 << j_evt.number);
                    jwx_event.SetEventType(wxEVT_JOY_BUTTON_UP);
                }

                jwx_event.SetButtonChange(j_evt.number);

                jwx_event.SetTimestamp(j_evt.time);
                jwx_event.SetJoystick(m_joystick);
                jwx_event.SetButtonState(m_buttons);
                jwx_event.SetPosition(m_lastposition);
                jwx_event.SetZPosition(m_axe[3]);
                jwx_event.SetEventObject(m_catchwin);

                if (m_catchwin)
                    m_catchwin->AddPendingEvent(jwx_event);
            }
        }
    }

    close(m_device);
    return NULL;
}


////////////////////////////////////////////////////////////////////////////

wxJoystick::wxJoystick(int joystick)
    : m_device(-1),
      m_joystick(joystick),
      m_thread(NULL)
{
    wxString dev_name;

     // old /dev structure
    dev_name.Printf( wxT("/dev/js%d"), joystick);
    m_device = open(dev_name.fn_str(), O_RDONLY);

    // new /dev structure with "input" subdirectory
    if (m_device == -1)
    {
        dev_name.Printf( wxT("/dev/input/js%d"), joystick);
        m_device = open(dev_name.fn_str(), O_RDONLY);
    }

    if (m_device != -1)
    {
        m_thread = new wxJoystickThread(m_device, m_joystick);
        m_thread->Create();
        m_thread->Run();
    }
}


wxJoystick::~wxJoystick()
{
    ReleaseCapture();
    if (m_thread)
        m_thread->Delete();  // It's detached so it will delete itself
    m_device = -1;
}


////////////////////////////////////////////////////////////////////////////
// State
////////////////////////////////////////////////////////////////////////////

wxPoint wxJoystick::GetPosition() const
{
    wxPoint pos(wxDefaultPosition);
    if (m_thread) pos = m_thread->m_lastposition;
    return pos;
}

int wxJoystick::GetZPosition() const
{
    if (m_thread)
        return m_thread->m_axe[wxJS_AXIS_Z];
    return 0;
}

int wxJoystick::GetButtonState() const
{
    if (m_thread)
        return m_thread->m_buttons;
    return 0;
}

int wxJoystick::GetPOVPosition() const
{
    return -1;
}

int wxJoystick::GetPOVCTSPosition() const
{
    return -1;
}

int wxJoystick::GetRudderPosition() const
{
    if (m_thread)
        return m_thread->m_axe[wxJS_AXIS_RUDDER];
    return 0;
}

int wxJoystick::GetUPosition() const
{
    if (m_thread)
        return m_thread->m_axe[wxJS_AXIS_U];
    return 0;
}

int wxJoystick::GetVPosition() const
{
    if (m_thread)
        return m_thread->m_axe[wxJS_AXIS_V];
    return 0;
}

int wxJoystick::GetMovementThreshold() const
{
    return 0;
}

void wxJoystick::SetMovementThreshold(int threshold)
{
}

////////////////////////////////////////////////////////////////////////////
// Capabilities
////////////////////////////////////////////////////////////////////////////

bool wxJoystick::IsOk() const
{
    return (m_device != -1);
}

int wxJoystick::GetNumberJoysticks()
{
    wxString dev_name;
    int fd, j;

    for (j=0; j<4; j++) {
        dev_name.Printf(wxT("/dev/js%d"), j);
        fd = open(dev_name.fn_str(), O_RDONLY);
        if (fd == -1)
            break;
        close(fd);
    }

    if (j == 0) {
        for (j=0; j<4; j++) {
            dev_name.Printf(wxT("/dev/input/js%d"), j);
            fd = open(dev_name.fn_str(), O_RDONLY);
            if (fd == -1)
                return j;
            close(fd);
        }
    }

    return j;
}

int wxJoystick::GetManufacturerId() const
{
    return 0;
}

int wxJoystick::GetProductId() const
{
    return 0;
}

wxString wxJoystick::GetProductName() const
{
    char name[128];

    if (ioctl(m_device, JSIOCGNAME(sizeof(name)), name) < 0)
        strcpy(name, "Unknown");
    return wxString(name, wxConvLibc);
}

int wxJoystick::GetXMin() const
{
    return wxJS_AXIS_MIN;
}

int wxJoystick::GetYMin() const
{
    return wxJS_AXIS_MIN;
}

int wxJoystick::GetZMin() const
{
    return wxJS_AXIS_MIN;
}

int wxJoystick::GetXMax() const
{
    return wxJS_AXIS_MAX;
}

int wxJoystick::GetYMax() const
{
    return wxJS_AXIS_MAX;
}

int wxJoystick::GetZMax() const
{
    return wxJS_AXIS_MAX;
}

int wxJoystick::GetNumberButtons() const
{
    char nb=0;

    if (m_device != -1)
        ioctl(m_device, JSIOCGBUTTONS, &nb);

    return nb;
}

int wxJoystick::GetNumberAxes() const
{
    char nb=0;

    if (m_device != -1)
        ioctl(m_device, JSIOCGAXES, &nb);

    return nb;
}

int wxJoystick::GetMaxButtons() const
{
    return 15; // internal
}

int wxJoystick::GetMaxAxes() const
{
    return 15; // internal
}

int wxJoystick::GetPollingMin() const
{
    return 10;
}

int wxJoystick::GetPollingMax() const
{
    return 1000;
}

int wxJoystick::GetRudderMin() const
{
    return wxJS_AXIS_MIN;
}

int wxJoystick::GetRudderMax() const
{
    return wxJS_AXIS_MAX;
}

int wxJoystick::GetUMin() const
{
    return wxJS_AXIS_MIN;
}

int wxJoystick::GetUMax() const
{
    return wxJS_AXIS_MAX;
}

int wxJoystick::GetVMin() const
{
    return wxJS_AXIS_MIN;
}

int wxJoystick::GetVMax() const
{
    return wxJS_AXIS_MAX;
}

bool wxJoystick::HasRudder() const
{
    return GetNumberAxes() >= wxJS_AXIS_RUDDER;
}

bool wxJoystick::HasZ() const
{
    return GetNumberAxes() >= wxJS_AXIS_Z;
}

bool wxJoystick::HasU() const
{
    return GetNumberAxes() >= wxJS_AXIS_U;
}

bool wxJoystick::HasV() const
{
    return GetNumberAxes() >= wxJS_AXIS_V;
}

bool wxJoystick::HasPOV() const
{
    return false;
}

bool wxJoystick::HasPOV4Dir() const
{
    return false;
}

bool wxJoystick::HasPOVCTS() const
{
    return false;
}

////////////////////////////////////////////////////////////////////////////
// Operations
////////////////////////////////////////////////////////////////////////////

bool wxJoystick::SetCapture(wxWindow* win, int pollingFreq)
{
    if (m_thread)
    {
        m_thread->m_catchwin = win;
        m_thread->m_polling = pollingFreq;
        return true;
    }
    return false;
}

bool wxJoystick::ReleaseCapture()
{
    if (m_thread)
    {
        m_thread->m_catchwin = NULL;
        m_thread->m_polling = 0;
        return true;
    }
    return false;
}
#endif  // wxUSE_JOYSTICK
