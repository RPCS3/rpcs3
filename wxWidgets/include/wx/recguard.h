///////////////////////////////////////////////////////////////////////////////
// Name:        wx/recguard.h
// Purpose:     declaration and implementation of wxRecursionGuard class
// Author:      Vadim Zeitlin
// Modified by:
// Created:     14.08.2003
// RCS-ID:      $Id: recguard.h 61872 2009-09-09 22:37:05Z VZ $
// Copyright:   (c) 2003 Vadim Zeitlin <vadim@wxwidgets.org>
// Licence:     wxWindows licence
///////////////////////////////////////////////////////////////////////////////

#ifndef _WX_RECGUARD_H_
#define _WX_RECGUARD_H_

#include "wx/defs.h"

// ----------------------------------------------------------------------------
// wxRecursionGuardFlag is used with wxRecursionGuard
// ----------------------------------------------------------------------------

typedef int wxRecursionGuardFlag;

// ----------------------------------------------------------------------------
// wxRecursionGuard is the simplest way to protect a function from reentrancy
// ----------------------------------------------------------------------------

class WXDLLIMPEXP_BASE wxRecursionGuard
{
public:
    wxRecursionGuard(wxRecursionGuardFlag& flag)
        : m_flag(flag)
    {
        m_isInside = flag++ != 0;
    }

    ~wxRecursionGuard()
    {
        wxASSERT_MSG( m_flag > 0, wxT("unbalanced wxRecursionGuards!?") );

        m_flag--;
    }

    bool IsInside() const { return m_isInside; }

private:
    wxRecursionGuardFlag& m_flag;

    // true if the flag had been already set when we were created
    bool m_isInside;
};

#endif // _WX_RECGUARD_H_

