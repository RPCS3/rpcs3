/////////////////////////////////////////////////////////////////////////////
// Name:        wx/xrc/xh_propdlg.h
// Purpose:     XML resource handler for wxPropertySheetDialog
// Author:      Sander Berents
// Created:     2007/07/12
// RCS-ID:      $Id: xh_propdlg.h 53135 2008-04-12 02:31:04Z VZ $
// Copyright:   (c) 2007 Sander Berents
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef _WX_XH_PROPDLG_H_
#define _WX_XH_PROPDLG_H_

#include "wx/xrc/xmlres.h"

#if wxUSE_XRC

#if wxABI_VERSION >= 20805
class WXDLLIMPEXP_FWD_ADV wxPropertySheetDialog;

class WXDLLIMPEXP_XRC wxPropertySheetDialogXmlHandler : public wxXmlResourceHandler
{
    DECLARE_DYNAMIC_CLASS(wxPropertySheetDialogXmlHandler)

public:
    wxPropertySheetDialogXmlHandler();
    virtual wxObject *DoCreateResource();
    virtual bool CanHandle(wxXmlNode *node);

private:
    bool m_isInside;
    wxPropertySheetDialog *m_dialog;
};
#endif // wxABI_VERSION >= 20805

#endif // wxUSE_XRC

#endif // _WX_XH_PROPDLG_H_
