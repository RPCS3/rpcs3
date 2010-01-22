/*  PCSX2 - PS2 Emulator for PCs
 *  Copyright (C) 2002-2009  PCSX2 Dev Team
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

#include "PrecompiledHeader.h"
#include "App.h"
#include "IniInterface.h"
#include "Utilities/EventSource.inl"

template class EventSource< IEventListener_CoreThread >;
template class EventSource< IEventListener_Plugins >;
template class EventSource< IEventListener_AppStatus >;

AppSettingsEventInfo::AppSettingsEventInfo( IniInterface& ini )
	: AppEventInfo( ini.IsSaving() ? AppStatus_SettingsSaved : AppStatus_SettingsLoaded )
	, m_ini( ini )
{
}

IEventListener_CoreThread::IEventListener_CoreThread()
{
	wxGetApp().AddListener( this );
}

IEventListener_CoreThread::~IEventListener_CoreThread() throw()
{
	wxGetApp().RemoveListener( this );
}

IEventListener_Plugins::IEventListener_Plugins()
{
	wxGetApp().AddListener( this );
}

IEventListener_Plugins::~IEventListener_Plugins() throw()
{
	wxGetApp().RemoveListener( this );
}

IEventListener_AppStatus::IEventListener_AppStatus()
{
	wxGetApp().AddListener( this );
}

IEventListener_AppStatus::~IEventListener_AppStatus() throw()
{
	wxGetApp().RemoveListener( this );
}

void IEventListener_AppStatus::DispatchEvent( const AppEventInfo& evtinfo )
{
	switch( evtinfo.evt_type )
	{
		case AppStatus_SettingsLoaded:
		case AppStatus_SettingsSaved:
			AppStatusEvent_OnSettingsLoadSave( (const AppSettingsEventInfo&)evtinfo );
		break;

		case AppStatus_SettingsApplied:	AppStatusEvent_OnSettingsApplied();	break;
		case AppStatus_Exiting:			AppStatusEvent_OnExit();				break;
	}
}
