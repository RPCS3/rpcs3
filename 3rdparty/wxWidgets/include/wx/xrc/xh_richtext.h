/////////////////////////////////////////////////////////////////////////////
// Name:        wx/xrc/xh_text.h
// Purpose:     XML resource handler for wxRichTextCtrl
// Author:      Julian Smart
// Created:     2006-11-08
// RCS-ID:      $Id: xh_richtext.h 43199 2006-11-08 15:45:11Z JS $
// Copyright:   (c) 2006 Julian Smart
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef _WX_XH_RICHTEXT_H_
#define _WX_XH_RICHTEXT_H_

#include "wx/xrc/xmlres.h"

#if wxUSE_XRC && wxUSE_RICHTEXT

class WXDLLIMPEXP_XRC wxRichTextCtrlXmlHandler : public wxXmlResourceHandler
{
    DECLARE_DYNAMIC_CLASS(wxRichTextCtrlXmlHandler)

public:
    wxRichTextCtrlXmlHandler();
    virtual wxObject *DoCreateResource();
    virtual bool CanHandle(wxXmlNode *node);
};

#endif // wxUSE_XRC && wxUSE_RICHTEXT

#endif // _WX_XH_RICHTEXT_H_
