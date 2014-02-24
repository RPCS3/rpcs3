#pragma once
#include <wx/wx.h>
#include "DebugTools/DebugInterface.h"

DECLARE_LOCAL_EVENT_TYPE( debEVT_SETSTATUSBARTEXT, wxNewEventType() )
DECLARE_LOCAL_EVENT_TYPE( debEVT_UPDATELAYOUT, wxNewEventType() )
DECLARE_LOCAL_EVENT_TYPE( debEVT_GOTOINMEMORYVIEW, wxNewEventType() )
DECLARE_LOCAL_EVENT_TYPE( debEVT_GOTOINDISASM, wxNewEventType() )
DECLARE_LOCAL_EVENT_TYPE( debEVT_RUNTOPOS, wxNewEventType() )
DECLARE_LOCAL_EVENT_TYPE( debEVT_MAPLOADED, wxNewEventType() )
DECLARE_LOCAL_EVENT_TYPE( debEVT_STEPOVER, wxNewEventType() )
DECLARE_LOCAL_EVENT_TYPE( debEVT_STEPINTO, wxNewEventType() )
DECLARE_LOCAL_EVENT_TYPE( debEVT_UPDATE, wxNewEventType() )

bool executeExpressionWindow(wxWindow* parent, DebugInterface* cpu, u64& dest, const wxString& defaultValue = wxEmptyString);
