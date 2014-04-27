/////////////////////////////////////////////////////////////////////////////
// Name:        wx/xrc/xh_radbx.h
// Purpose:     XML resource handler for wxRadioBox
// Author:      Bob Mitchell
// Created:     2000/03/21
// RCS-ID:      $Id: xh_radbx.h 41590 2006-10-03 14:53:40Z VZ $
// Copyright:   (c) 2000 Bob Mitchell and Verant Interactive
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef _WX_XH_RADBX_H_
#define _WX_XH_RADBX_H_

#include "wx/xrc/xmlres.h"

#if wxUSE_XRC && wxUSE_RADIOBOX

class WXDLLIMPEXP_XRC wxRadioBoxXmlHandler : public wxXmlResourceHandler
{
    DECLARE_DYNAMIC_CLASS(wxRadioBoxXmlHandler)

public:
    wxRadioBoxXmlHandler();
    virtual wxObject *DoCreateResource();
    virtual bool CanHandle(wxXmlNode *node);

private:
    bool m_insideBox;

    // the items labels
    wxArrayString labels;

    // the items tooltips
    wxArrayString tooltips;

    // the item help text
    wxArrayString helptexts;
    wxArrayInt    helptextSpecified;
};

#endif // wxUSE_XRC && wxUSE_RADIOBOX

#endif // _WX_XH_RADBX_H_
