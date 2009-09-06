/////////////////////////////////////////////////////////////////////////////
// Name:        wx/xrc/xh_wizrd.h
// Purpose:     XML resource handler for wxWizard
// Author:      Vaclav Slavik
// Created:     2003/03/02
// RCS-ID:      $Id: xh_wizrd.h 53135 2008-04-12 02:31:04Z VZ $
// Copyright:   (c) 2000 Vaclav Slavik
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef _WX_XH_WIZRD_H_
#define _WX_XH_WIZRD_H_

#include "wx/xrc/xmlres.h"

#if wxUSE_XRC && wxUSE_WIZARDDLG

class WXDLLIMPEXP_FWD_ADV wxWizard;
class WXDLLIMPEXP_FWD_ADV wxWizardPageSimple;

class WXDLLIMPEXP_XRC wxWizardXmlHandler : public wxXmlResourceHandler
{
    DECLARE_DYNAMIC_CLASS(wxWizardXmlHandler)

public:
    wxWizardXmlHandler();
    virtual wxObject *DoCreateResource();
    virtual bool CanHandle(wxXmlNode *node);

private:
    wxWizard *m_wizard;
    wxWizardPageSimple *m_lastSimplePage;
};

#endif // wxUSE_XRC && wxUSE_WIZARDDLG

#endif // _WX_XH_WIZRD_H_
