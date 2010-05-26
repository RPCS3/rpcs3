/*  PCSX2 - PS2 Emulator for PCs
 *  Copyright (C) 2002-2010  PCSX2 Dev Team
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

#include "App.h"
#include "SaveState.h"

// --------------------------------------------------------------------------------------
//  SysExecEvent_SaveSinglePlugin
// --------------------------------------------------------------------------------------
// fixme : Ideally this should use either Close or Pause depending on if the system is in
// Fullscreen Exclusive mode or regular mode.  But since we don't yet support Fullscreen
// Exclusive mode, and since I'm too lazy to make some third suspend class for that, we're
// just using CoreThreadPause.  --air
//
class SysExecEvent_SaveSinglePlugin : public BaseSysExecEvent_ScopedCore
{
	typedef BaseSysExecEvent_ScopedCore _parent;

protected:
	PluginsEnum_t			m_pid;

public:
	wxString GetEventName() const { return L"SaveSinglePlugin"; }

	virtual ~SysExecEvent_SaveSinglePlugin() throw() {}
	SysExecEvent_SaveSinglePlugin* Clone() const { return new SysExecEvent_SaveSinglePlugin( *this ); }

	SysExecEvent_SaveSinglePlugin( PluginsEnum_t pid=PluginId_GS )
	{
		m_pid = pid;
	}
	
	SysExecEvent_SaveSinglePlugin& SetPluginId( PluginsEnum_t pid )
	{
		m_pid = pid;
		return *this;
	}

protected:
	void InvokeEvent();
	void CleanupEvent();
};


extern void StateCopy_SaveToFile( const wxString& file );
extern void StateCopy_LoadFromFile( const wxString& file );
extern void StateCopy_SaveToSlot( uint num );
extern void StateCopy_LoadFromSlot( uint slot );
