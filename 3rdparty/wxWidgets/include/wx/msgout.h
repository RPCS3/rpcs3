///////////////////////////////////////////////////////////////////////////////
// Name:        wx/msgout.h
// Purpose:     wxMessageOutput class. Shows a message to the user
// Author:      Mattia Barbon
// Modified by:
// Created:     17.07.02
// RCS-ID:      $Id: msgout.h 35690 2005-09-25 20:23:30Z VZ $
// Copyright:   (c) Mattia Barbon
// Licence:     wxWindows licence
///////////////////////////////////////////////////////////////////////////////

#ifndef _WX_MSGOUT_H_
#define _WX_MSGOUT_H_

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

#include "wx/defs.h"
#include "wx/wxchar.h"

// ----------------------------------------------------------------------------
// wxMessageOutput is a class abstracting formatted output target, i.e.
// something you can printf() to
// ----------------------------------------------------------------------------

class WXDLLIMPEXP_BASE wxMessageOutput
{
public:
    virtual ~wxMessageOutput() { }

    // show a message to the user
    virtual void Printf(const wxChar* format, ...)  ATTRIBUTE_PRINTF_2 = 0;

    // gets the current wxMessageOutput object (may be NULL during
    // initialization or shutdown)
    static wxMessageOutput* Get();

    // sets the global wxMessageOutput instance; returns the previous one
    static wxMessageOutput* Set(wxMessageOutput* msgout);

private:
    static wxMessageOutput* ms_msgOut;
};

// ----------------------------------------------------------------------------
// implementation showing the message to the user in "best" possible way: uses
// native message box if available (currently only under Windows) and stderr
// otherwise; unlike wxMessageOutputMessageBox this class is always safe to use
// ----------------------------------------------------------------------------

class WXDLLIMPEXP_BASE wxMessageOutputBest : public wxMessageOutput
{
public:
    wxMessageOutputBest() { }

    virtual void Printf(const wxChar* format, ...) ATTRIBUTE_PRINTF_2;
};

// ----------------------------------------------------------------------------
// implementation which sends output to stderr
// ----------------------------------------------------------------------------

class WXDLLIMPEXP_BASE wxMessageOutputStderr : public wxMessageOutput
{
public:
    wxMessageOutputStderr() { }

    virtual void Printf(const wxChar* format, ...) ATTRIBUTE_PRINTF_2;
};

// ----------------------------------------------------------------------------
// implementation which shows output in a message box
// ----------------------------------------------------------------------------

#if wxUSE_GUI

class WXDLLIMPEXP_CORE wxMessageOutputMessageBox : public wxMessageOutput
{
public:
    wxMessageOutputMessageBox() { }

    virtual void Printf(const wxChar* format, ...) ATTRIBUTE_PRINTF_2;
};

#endif // wxUSE_GUI

// ----------------------------------------------------------------------------
// implementation using the native way of outputting debug messages
// ----------------------------------------------------------------------------

class WXDLLIMPEXP_BASE wxMessageOutputDebug : public wxMessageOutput
{
public:
    wxMessageOutputDebug() { }

    virtual void Printf(const wxChar* format, ...) ATTRIBUTE_PRINTF_2;
};

// ----------------------------------------------------------------------------
// implementation using wxLog (mainly for backwards compatibility)
// ----------------------------------------------------------------------------

class WXDLLIMPEXP_BASE wxMessageOutputLog : public wxMessageOutput
{
public:
    wxMessageOutputLog() { }

    virtual void Printf(const wxChar* format, ...) ATTRIBUTE_PRINTF_2;
};

#endif
    // _WX_MSGOUT_H_
