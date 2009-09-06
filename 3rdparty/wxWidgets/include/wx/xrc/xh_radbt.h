/////////////////////////////////////////////////////////////////////////////
// Name:        wx/xrc/xh_radbt.h
// Purpose:     XML resource handler for wxRadioButton
// Author:      Bob Mitchell
// Created:     2000/03/21
// RCS-ID:      $Id: xh_radbt.h 41590 2006-10-03 14:53:40Z VZ $
// Copyright:   (c) 2000 Bob Mitchell and Verant Interactive
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef _WX_XH_RADBT_H_
#define _WX_XH_RADBT_H_

#include "wx/xrc/xmlres.h"

#if wxUSE_XRC && wxUSE_RADIOBTN

class WXDLLIMPEXP_XRC wxRadioButtonXmlHandler : public wxXmlResourceHandler
{
    DECLARE_DYNAMIC_CLASS(wxRadioButtonXmlHandler)

public:
    wxRadioButtonXmlHandler();
    virtual wxObject *DoCreateResource();
    virtual bool CanHandle(wxXmlNode *node);
};

#endif // wxUSE_XRC && wxUSE_RADIOBOX

#endif // _WX_XH_RADBT_H_
