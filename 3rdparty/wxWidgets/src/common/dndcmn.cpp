///////////////////////////////////////////////////////////////////////////////
// Name:        common/dndcmn.cpp
// Author:      Robert Roebling
// Modified by:
// Created:     19.10.99
// RCS-ID:      $Id: dndcmn.cpp 43664 2006-11-26 21:50:51Z JS $
// Copyright:   (c) wxWidgets Team
// Licence:     wxWindows licence
///////////////////////////////////////////////////////////////////////////////

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#include "wx/dnd.h"

#if wxUSE_DRAG_AND_DROP

bool wxIsDragResultOk(wxDragResult res)
{
    return res == wxDragCopy || res == wxDragMove || res == wxDragLink;
}

#endif

