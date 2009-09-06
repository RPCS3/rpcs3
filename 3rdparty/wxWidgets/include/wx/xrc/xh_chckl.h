/////////////////////////////////////////////////////////////////////////////
// Name:        wx/xrc/xh_chckl.h
// Purpose:     XML resource handler for wxCheckListBox
// Author:      Bob Mitchell
// Created:     2000/03/21
// RCS-ID:      $Id: xh_chckl.h 41590 2006-10-03 14:53:40Z VZ $
// Copyright:   (c) 2000 Bob Mitchell and Verant Interactive
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef _WX_XH_CHCKL_H_
#define _WX_XH_CHCKL_H_

#include "wx/xrc/xmlres.h"

#if wxUSE_XRC && wxUSE_CHECKLISTBOX

class WXDLLIMPEXP_XRC wxCheckListBoxXmlHandler : public wxXmlResourceHandler
{
    DECLARE_DYNAMIC_CLASS(wxCheckListBoxXmlHandler)

public:
    wxCheckListBoxXmlHandler();
    virtual wxObject *DoCreateResource();
    virtual bool CanHandle(wxXmlNode *node);

private:
    bool m_insideBox;
    wxArrayString strList;
};

#endif // wxUSE_XRC && wxUSE_CHECKLISTBOX

#endif // _WX_XH_CHECKLIST_H_
