/////////////////////////////////////////////////////////////////////////////
// Name:        wx/stockitem.h
// Purpose:     stock items helpers (privateh header)
// Author:      Vaclav Slavik
// Modified by:
// Created:     2004-08-15
// RCS-ID:      $Id: stockitem.h 42935 2006-11-02 09:51:49Z JS $
// Copyright:   (c) Vaclav Slavik, 2004
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef _WX_STOCKITEM_H_
#define _WX_STOCKITEM_H_

#include "wx/defs.h"
#include "wx/wxchar.h"
#include "wx/string.h"
#include "wx/accel.h"

// ----------------------------------------------------------------------------
// Helper functions for stock items handling:
// ----------------------------------------------------------------------------

// Returns true if the ID is in the list of recognized stock actions
WXDLLEXPORT bool wxIsStockID(wxWindowID id);

// Returns true of the label is empty or label of a stock button with
// given ID
WXDLLEXPORT bool wxIsStockLabel(wxWindowID id, const wxString& label);

enum wxStockLabelQueryFlag
{
    wxSTOCK_NOFLAGS = 0,

    wxSTOCK_WITH_MNEMONIC = 1,
    wxSTOCK_WITH_ACCELERATOR = 2
};

// Returns label that should be used for given stock UI element (e.g. "&OK"
// for wxSTOCK_OK); if wxSTOCK_WITH_MNEMONIC is given, the & character
// is included; if wxSTOCK_WITH_ACCELERATOR is given, the stock accelerator
// for given ID is concatenated to the label using \t as separator
WXDLLEXPORT wxString wxGetStockLabel(wxWindowID id,
                                     long flags = wxSTOCK_WITH_MNEMONIC);

#if wxUSE_ACCEL

    // Returns the accelerator that should be used for given stock UI element
    // (e.g. "Ctrl+x" for wxSTOCK_EXIT)
    WXDLLEXPORT wxAcceleratorEntry wxGetStockAccelerator(wxWindowID id);

#endif

// wxStockHelpStringClient conceptually works like wxArtClient: it gives a hint to
// wxGetStockHelpString() about the context where the help string is to be used
enum wxStockHelpStringClient
{
    wxSTOCK_MENU        // help string to use for menu items
};

// Returns an help string for the given stock UI element and for the given "context".
WXDLLEXPORT wxString wxGetStockHelpString(wxWindowID id,
                                          wxStockHelpStringClient client = wxSTOCK_MENU);


#ifdef __WXGTK20__

// Translates stock ID to GTK+'s stock item string indentifier:
WXDLLEXPORT const char *wxGetStockGtkID(wxWindowID id);

#endif

#endif // _WX_STOCKITEM_H_
