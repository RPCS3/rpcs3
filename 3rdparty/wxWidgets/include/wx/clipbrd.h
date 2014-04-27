/////////////////////////////////////////////////////////////////////////////
// Name:        wx/clipbrd.h
// Purpose:     wxClipboad class and clipboard functions
// Author:      Vadim Zeitlin
// Modified by:
// Created:     19.10.99
// RCS-ID:      $Id: clipbrd.h 49563 2007-10-31 20:46:21Z VZ $
// Copyright:   (c) wxWidgets Team
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef _WX_CLIPBRD_H_BASE_
#define _WX_CLIPBRD_H_BASE_

#include "wx/defs.h"

#if wxUSE_CLIPBOARD


#include "wx/object.h"
#include "wx/wxchar.h"

class WXDLLIMPEXP_FWD_CORE wxDataFormat;
class WXDLLIMPEXP_FWD_CORE wxDataObject;
class WXDLLIMPEXP_FWD_CORE wxClipboard;

// ----------------------------------------------------------------------------
// wxClipboard represents the system clipboard. Normally, you should use
// wxTheClipboard which is a global pointer to the (unique) clipboard.
//
// Clipboard can be used to copy data to/paste data from. It works together
// with wxDataObject.
// ----------------------------------------------------------------------------

class WXDLLEXPORT wxClipboardBase : public wxObject
{
public:
    wxClipboardBase() {}

    // open the clipboard before Add/SetData() and GetData()
    virtual bool Open() = 0;

    // close the clipboard after Add/SetData() and GetData()
    virtual void Close() = 0;

    // query whether the clipboard is opened
    virtual bool IsOpened() const = 0;

    // add to the clipboard data
    //
    // NB: the clipboard owns the pointer and will delete it, so data must be
    //     allocated on the heap
    virtual bool AddData( wxDataObject *data ) = 0;

    // set the clipboard data, this is the same as Clear() followed by
    // AddData()
    virtual bool SetData( wxDataObject *data ) = 0;

    // ask if data in correct format is available
    virtual bool IsSupported( const wxDataFormat& format ) = 0;

    // fill data with data on the clipboard (if available)
    virtual bool GetData( wxDataObject& data ) = 0;

    // clears wxTheClipboard and the system's clipboard if possible
    virtual void Clear() = 0;

    // flushes the clipboard: this means that the data which is currently on
    // clipboard will stay available even after the application exits (possibly
    // eating memory), otherwise the clipboard will be emptied on exit
    virtual bool Flush() { return false; }

    // X11 has two clipboards which get selected by this call. Empty on MSW.
    virtual void UsePrimarySelection( bool WXUNUSED(primary) = false ) { }

    // Returns global instance (wxTheClipboard) of the object:
    static wxClipboard *Get();
};

// ----------------------------------------------------------------------------
// globals
// ----------------------------------------------------------------------------

// The global clipboard object - backward compatible access macro:
#define wxTheClipboard   (wxClipboard::Get())

// ----------------------------------------------------------------------------
// include platform-specific class declaration
// ----------------------------------------------------------------------------

#if defined(__WXMSW__)
    #include "wx/msw/clipbrd.h"
#elif defined(__WXMOTIF__)
    #include "wx/motif/clipbrd.h"
#elif defined(__WXGTK20__)
    #include "wx/gtk/clipbrd.h"
#elif defined(__WXGTK__)
    #include "wx/gtk1/clipbrd.h"
#elif defined(__WXX11__)
    #include "wx/x11/clipbrd.h"
#elif defined(__WXMGL__)
    #include "wx/mgl/clipbrd.h"
#elif defined(__WXMAC__)
    #include "wx/mac/clipbrd.h"
#elif defined(__WXCOCOA__)
    #include "wx/cocoa/clipbrd.h"
#elif defined(__WXPM__)
    #include "wx/os2/clipbrd.h"
#endif

// ----------------------------------------------------------------------------
// helpful class for opening the clipboard and automatically closing it
// ----------------------------------------------------------------------------

class WXDLLEXPORT wxClipboardLocker
{
public:
    wxClipboardLocker(wxClipboard *clipboard = (wxClipboard *)NULL)
    {
        m_clipboard = clipboard ? clipboard : wxTheClipboard;
        if ( m_clipboard )
        {
            m_clipboard->Open();
        }
    }

    bool operator!() const { return !m_clipboard->IsOpened(); }

    ~wxClipboardLocker()
    {
        if ( m_clipboard )
        {
            m_clipboard->Close();
        }
    }

private:
    wxClipboard *m_clipboard;

    DECLARE_NO_COPY_CLASS(wxClipboardLocker)
};

#endif // wxUSE_CLIPBOARD

#endif // _WX_CLIPBRD_H_BASE_
