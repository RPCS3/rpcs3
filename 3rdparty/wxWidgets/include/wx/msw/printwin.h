/////////////////////////////////////////////////////////////////////////////
// Name:        printwin.h
// Purpose:     wxWindowsPrinter, wxWindowsPrintPreview classes
// Author:      Julian Smart
// Modified by:
// Created:     01/02/97
// RCS-ID:      $Id: printwin.h 42522 2006-10-27 13:07:40Z JS $
// Copyright:   (c) Julian Smart
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef _WX_PRINTWIN_H_
#define _WX_PRINTWIN_H_

#include "wx/prntbase.h"

// ---------------------------------------------------------------------------
// Represents the printer: manages printing a wxPrintout object
// ---------------------------------------------------------------------------

class WXDLLEXPORT wxWindowsPrinter : public wxPrinterBase
{
    DECLARE_DYNAMIC_CLASS(wxWindowsPrinter)

public:
    wxWindowsPrinter(wxPrintDialogData *data = NULL);
    virtual ~wxWindowsPrinter();

    virtual bool Print(wxWindow *parent,
                       wxPrintout *printout,
                       bool prompt = true);

    virtual wxDC *PrintDialog(wxWindow *parent);
    virtual bool Setup(wxWindow *parent);

private:
    WXFARPROC     m_lpAbortProc;

    DECLARE_NO_COPY_CLASS(wxWindowsPrinter)
};

// ---------------------------------------------------------------------------
// wxPrintPreview: programmer creates an object of this class to preview a
// wxPrintout.
// ---------------------------------------------------------------------------

class WXDLLEXPORT wxWindowsPrintPreview : public wxPrintPreviewBase
{
public:
    wxWindowsPrintPreview(wxPrintout *printout,
                          wxPrintout *printoutForPrinting = NULL,
                          wxPrintDialogData *data = NULL);
    wxWindowsPrintPreview(wxPrintout *printout,
                          wxPrintout *printoutForPrinting,
                          wxPrintData *data);
    virtual ~wxWindowsPrintPreview();

    virtual bool Print(bool interactive);
    virtual void DetermineScaling();

private:
    DECLARE_DYNAMIC_CLASS_NO_COPY(wxWindowsPrintPreview)
};

#endif
// _WX_PRINTWIN_H_
