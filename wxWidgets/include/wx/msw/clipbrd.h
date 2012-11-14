/////////////////////////////////////////////////////////////////////////////
// Name:        wx/msw/clipbrd.h
// Purpose:     wxClipboad class and clipboard functions for MSW
// Author:      Julian Smart
// Modified by:
// Created:     01/02/97
// RCS-ID:      $Id: clipbrd.h 49563 2007-10-31 20:46:21Z VZ $
// Copyright:   (c) Julian Smart
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef _WX_CLIPBRD_H_
#define _WX_CLIPBRD_H_

#if wxUSE_CLIPBOARD

#include "wx/list.h"
#include "wx/module.h"
#include "wx/dataobj.h"     // for wxDataFormat

// These functions superceded by wxClipboard, but retained in order to
// implement wxClipboard, and for compatibility.

// open/close the clipboard
WXDLLEXPORT bool wxOpenClipboard();
WXDLLEXPORT bool wxIsClipboardOpened();
#define wxClipboardOpen wxIsClipboardOpened
WXDLLEXPORT bool wxCloseClipboard();

// get/set data
WXDLLEXPORT bool wxEmptyClipboard();
WXDLLEXPORT bool wxSetClipboardData(wxDataFormat dataFormat,
                                    const void *data,
                                    int width = 0, int height = 0);
WXDLLEXPORT void* wxGetClipboardData(wxDataFormat dataFormat,
                                     long *len = NULL);

// clipboard formats
WXDLLEXPORT bool wxIsClipboardFormatAvailable(wxDataFormat dataFormat);
WXDLLEXPORT wxDataFormat wxEnumClipboardFormats(wxDataFormat dataFormat);
WXDLLEXPORT int  wxRegisterClipboardFormat(wxChar *formatName);
WXDLLEXPORT bool wxGetClipboardFormatName(wxDataFormat dataFormat,
                                          wxChar *formatName,
                                          int maxCount);

//-----------------------------------------------------------------------------
// wxClipboard
//-----------------------------------------------------------------------------

class WXDLLIMPEXP_FWD_CORE wxDataObject;
class WXDLLEXPORT wxClipboard : public wxClipboardBase
{
    DECLARE_DYNAMIC_CLASS(wxClipboard)

public:
    wxClipboard();
    virtual ~wxClipboard();

    // open the clipboard before SetData() and GetData()
    virtual bool Open();

    // close the clipboard after SetData() and GetData()
    virtual void Close();

    // query whether the clipboard is opened
    virtual bool IsOpened() const;

    // set the clipboard data. all other formats will be deleted.
    virtual bool SetData( wxDataObject *data );

    // add to the clipboard data.
    virtual bool AddData( wxDataObject *data );

    // ask if data in correct format is available
    virtual bool IsSupported( const wxDataFormat& format );

    // fill data with data on the clipboard (if available)
    virtual bool GetData( wxDataObject& data );

    // clears wxTheClipboard and the system's clipboard if possible
    virtual void Clear();

    // flushes the clipboard: this means that the data which is currently on
    // clipboard will stay available even after the application exits (possibly
    // eating memory), otherwise the clipboard will be emptied on exit
    virtual bool Flush();

    // X11 has two clipboards which get selected by this call. Empty on MSW.
    void UsePrimarySelection( bool WXUNUSED(primary) = false ) { }

private:
    IDataObject *m_lastDataObject;
    bool m_isOpened;
};

#endif // wxUSE_CLIPBOARD
#endif
    // _WX_CLIPBRD_H_
