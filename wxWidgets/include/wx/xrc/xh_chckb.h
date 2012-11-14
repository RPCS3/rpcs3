/////////////////////////////////////////////////////////////////////////////
// Name:        wx/xrc/xh_chckb.h
// Purpose:     XML resource handler for wxCheckBox
// Author:      Bob Mitchell
// Created:     2000/03/21
// RCS-ID:      $Id: xh_chckb.h 41590 2006-10-03 14:53:40Z VZ $
// Copyright:   (c) 2000 Bob Mitchell and Verant Interactive
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef _WX_XH_CHCKB_H_
#define _WX_XH_CHCKB_H_

#include "wx/xrc/xmlres.h"

#if wxUSE_XRC && wxUSE_CHECKBOX

class WXDLLIMPEXP_XRC wxCheckBoxXmlHandler : public wxXmlResourceHandler
{
    DECLARE_DYNAMIC_CLASS(wxCheckBoxXmlHandler)

public:
    wxCheckBoxXmlHandler();
    virtual wxObject *DoCreateResource();
    virtual bool CanHandle(wxXmlNode *node);
};

#endif // wxUSE_XRC && wxUSE_CHECKBOX

#endif // _WX_XH_CHECKBOX_H_

