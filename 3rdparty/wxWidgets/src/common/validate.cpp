/////////////////////////////////////////////////////////////////////////////
// Name:        src/common/validate.cpp
// Purpose:     wxValidator
// Author:      Julian Smart
// Modified by:
// Created:     04/01/98
// RCS-ID:      $Id: validate.cpp 39656 2006-06-09 21:21:53Z ABX $
// Copyright:   (c) Julian Smart
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

// For compilers that support precompilation, includes "wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#if wxUSE_VALIDATORS

#include "wx/validate.h"

#ifndef WX_PRECOMP
    #include "wx/window.h"
#endif

const wxValidator wxDefaultValidator;

IMPLEMENT_DYNAMIC_CLASS(wxValidator, wxEvtHandler)

// VZ: personally, I think true would be more appropriate - these bells are
//     _annoying_
bool wxValidator::ms_isSilent = false;

wxValidator::wxValidator()
{
  m_validatorWindow = (wxWindow *) NULL;
}

wxValidator::~wxValidator()
{
}

#endif
  // wxUSE_VALIDATORS
