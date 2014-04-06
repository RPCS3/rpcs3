/*  PCSX2 - PS2 Emulator for PCs
 *  Copyright (C) 2002-2014  PCSX2 Dev Team
 *
 *  PCSX2 is free software: you can redistribute it and/or modify it under the terms
 *  of the GNU Lesser General Public License as published by the Free Software Found-
 *  ation, either version 3 of the License, or (at your option) any later version.
 *
 *  PCSX2 is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 *  without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 *  PURPOSE.  See the GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along with PCSX2.
 *  If not, see <http://www.gnu.org/licenses/>.
 */

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
