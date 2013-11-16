/////////////////////////////////////////////////////////////////////////////
// Name:        wx/xrc/xh_choicbk.h
// Purpose:     XML resource handler for wxChoicebook
// Author:      Vaclav Slavik
// Copyright:   (c) 2000 Vaclav Slavik
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef _WX_XH_CHOICEBK_H_
#define _WX_XH_CHOICEBK_H_

#include "wx/xrc/xmlres.h"

#if wxUSE_XRC && wxUSE_CHOICEBOOK

class WXDLLIMPEXP_FWD_CORE wxChoicebook;

class WXDLLIMPEXP_XRC wxChoicebookXmlHandler : public wxXmlResourceHandler
{
    DECLARE_DYNAMIC_CLASS(wxChoicebookXmlHandler)

public:
    wxChoicebookXmlHandler();
    virtual wxObject *DoCreateResource();
    virtual bool CanHandle(wxXmlNode *node);

private:
    bool m_isInside;
    wxChoicebook *m_choicebook;
};

#endif // wxUSE_XRC && wxUSE_CHOICEBOOK

#endif // _WX_XH_CHOICEBK_H_
