/////////////////////////////////////////////////////////////////////////////
// Name:        wx/xrc/xh_notbk.h
// Purpose:     XML resource handler for wxNotebook
// Author:      Vaclav Slavik
// RCS-ID:      $Id: xh_notbk.h 53135 2008-04-12 02:31:04Z VZ $
// Copyright:   (c) 2000 Vaclav Slavik
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef _WX_XH_NOTBK_H_
#define _WX_XH_NOTBK_H_

#include "wx/xrc/xmlres.h"

#if wxUSE_XRC && wxUSE_NOTEBOOK

class WXDLLIMPEXP_FWD_CORE wxNotebook;

class WXDLLIMPEXP_XRC wxNotebookXmlHandler : public wxXmlResourceHandler
{
    DECLARE_DYNAMIC_CLASS(wxNotebookXmlHandler)

public:
    wxNotebookXmlHandler();
    virtual wxObject *DoCreateResource();
    virtual bool CanHandle(wxXmlNode *node);

private:
    bool m_isInside;
    wxNotebook *m_notebook;
};

#endif // wxUSE_XRC && wxUSE_NOTEBOOK

#endif // _WX_XH_NOTBK_H_
