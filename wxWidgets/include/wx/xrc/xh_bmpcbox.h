/////////////////////////////////////////////////////////////////////////////
// Name:        wx/xrc/xh_bmpcbox.h
// Purpose:     XML resource handler for wxBitmapComboBox
// Author:      Jaakko Salli
// Created:     Sep-10-2006
// RCS-ID:      $Id: xh_bmpcbox.h 53135 2008-04-12 02:31:04Z VZ $
// Copyright:   (c) 2006 Jaakko Salli
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef _WX_XH_BMPCBOX_H_
#define _WX_XH_BMPCBOX_H_

#include "wx/xrc/xmlres.h"

#if wxUSE_XRC && wxUSE_BITMAPCOMBOBOX

class WXDLLIMPEXP_FWD_ADV wxBitmapComboBox;

class WXDLLIMPEXP_XRC wxBitmapComboBoxXmlHandler : public wxXmlResourceHandler
{
    DECLARE_DYNAMIC_CLASS(wxBitmapComboBoxXmlHandler)

public:
    wxBitmapComboBoxXmlHandler();
    virtual wxObject *DoCreateResource();
    virtual bool CanHandle(wxXmlNode *node);

private:
    wxBitmapComboBox*    m_combobox;
    bool                m_isInside;
};

#endif // wxUSE_XRC && wxUSE_BITMAPCOMBOBOX

#endif // _WX_XH_BMPCBOX_H_
