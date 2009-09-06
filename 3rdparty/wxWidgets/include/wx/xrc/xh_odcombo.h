/////////////////////////////////////////////////////////////////////////////
// Name:        wx/xrc/xh_odcombo.h
// Purpose:     XML resource handler for wxOwnerDrawnComboBox
// Author:      Alex Bligh - based on wx/xrc/xh_combo.h
// Created:     2006/06/19
// RCS-ID:      $Id: xh_odcombo.h 41590 2006-10-03 14:53:40Z VZ $
// Copyright:   (c) 2006 Alex Bligh
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef _WX_XH_ODCOMBO_H_
#define _WX_XH_ODCOMBO_H_

#include "wx/xrc/xmlres.h"

#if wxUSE_XRC && wxUSE_ODCOMBOBOX

class WXDLLIMPEXP_XRC wxOwnerDrawnComboBoxXmlHandler : public wxXmlResourceHandler
{
    DECLARE_DYNAMIC_CLASS(wxOwnerDrawnComboBoxXmlHandler)

public:
    wxOwnerDrawnComboBoxXmlHandler();
    virtual wxObject *DoCreateResource();
    virtual bool CanHandle(wxXmlNode *node);

private:
    bool m_insideBox;
    wxArrayString strList;
};

#endif // wxUSE_XRC && wxUSE_ODCOMBOBOX

#endif // _WX_XH_ODCOMBO_H_

