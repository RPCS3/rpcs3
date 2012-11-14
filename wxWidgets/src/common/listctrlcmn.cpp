////////////////////////////////////////////////////////////////////////////////
// Name:        src/common/listctrlcmn.cpp
// Purpose:     Common defines for wxListCtrl and wxListCtrl-based classes.
// Author:      Kevin Ollivier
// Created:     09/15/06
// RCS-ID:      $Id: listctrlcmn.cpp 41568 2006-10-02 17:38:30Z PC $
// Copyright:   (c) Kevin Ollivier
// Licence:     wxWindows licence
////////////////////////////////////////////////////////////////////////////////

// =============================================================================
// declarations
// =============================================================================

// -----------------------------------------------------------------------------
// headers
// -----------------------------------------------------------------------------

// For compilers that support precompilation, includes "wx.h".
#include "wx/wxprec.h"

#if wxUSE_LISTCTRL

#include "wx/listctrl.h"

const wxChar wxListCtrlNameStr[] = wxT("listCtrl");

// ListCtrl events
DEFINE_EVENT_TYPE(wxEVT_COMMAND_LIST_BEGIN_DRAG)
DEFINE_EVENT_TYPE(wxEVT_COMMAND_LIST_BEGIN_RDRAG)
DEFINE_EVENT_TYPE(wxEVT_COMMAND_LIST_BEGIN_LABEL_EDIT)
DEFINE_EVENT_TYPE(wxEVT_COMMAND_LIST_END_LABEL_EDIT)
DEFINE_EVENT_TYPE(wxEVT_COMMAND_LIST_DELETE_ITEM)
DEFINE_EVENT_TYPE(wxEVT_COMMAND_LIST_DELETE_ALL_ITEMS)
#if WXWIN_COMPATIBILITY_2_4
DEFINE_EVENT_TYPE(wxEVT_COMMAND_LIST_GET_INFO)
DEFINE_EVENT_TYPE(wxEVT_COMMAND_LIST_SET_INFO)
#endif
DEFINE_EVENT_TYPE(wxEVT_COMMAND_LIST_ITEM_SELECTED)
DEFINE_EVENT_TYPE(wxEVT_COMMAND_LIST_ITEM_DESELECTED)
DEFINE_EVENT_TYPE(wxEVT_COMMAND_LIST_KEY_DOWN)
DEFINE_EVENT_TYPE(wxEVT_COMMAND_LIST_INSERT_ITEM)
DEFINE_EVENT_TYPE(wxEVT_COMMAND_LIST_COL_CLICK)
DEFINE_EVENT_TYPE(wxEVT_COMMAND_LIST_COL_RIGHT_CLICK)
DEFINE_EVENT_TYPE(wxEVT_COMMAND_LIST_COL_BEGIN_DRAG)
DEFINE_EVENT_TYPE(wxEVT_COMMAND_LIST_COL_DRAGGING)
DEFINE_EVENT_TYPE(wxEVT_COMMAND_LIST_COL_END_DRAG)
DEFINE_EVENT_TYPE(wxEVT_COMMAND_LIST_ITEM_RIGHT_CLICK)
DEFINE_EVENT_TYPE(wxEVT_COMMAND_LIST_ITEM_MIDDLE_CLICK)
DEFINE_EVENT_TYPE(wxEVT_COMMAND_LIST_ITEM_ACTIVATED)
DEFINE_EVENT_TYPE(wxEVT_COMMAND_LIST_ITEM_FOCUSED)
DEFINE_EVENT_TYPE(wxEVT_COMMAND_LIST_CACHE_HINT)

#endif // wxUSE_LISTCTRL
