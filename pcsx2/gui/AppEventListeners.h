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

#pragma once

#include "Utilities/EventSource.h"

enum CoreThreadStatus
{
	CoreStatus_Indeterminate,
	CoreStatus_Started,
	CoreStatus_Resumed,
	CoreStatus_Suspended,
	CoreStatus_Reset,
	CoreStatus_Stopped,
};

enum AppEventType
{
	AppStatus_SettingsLoaded,
	AppStatus_SettingsSaved,
	AppStatus_SettingsApplied,
	AppStatus_Exiting
};

enum PluginEventType
{
	PluginsEvt_Loaded,
	PluginsEvt_Init,
	PluginsEvt_Opening,		// dispatched prior to plugins being opened
	PluginsEvt_Opened,		// dispatched after plugins are opened
	PluginsEvt_Closing,		// dispatched prior to plugins being closed
	PluginsEvt_Closed,		// dispatched after plugins are closed
	PluginsEvt_Shutdown,
	PluginsEvt_Unloaded,
};

struct AppEventInfo
{
	AppEventType	evt_type;

	AppEventInfo( AppEventType type )
	{
		evt_type = type;
	}
};

struct AppSettingsEventInfo : AppEventInfo
{
	IniInterface&	m_ini;

	AppSettingsEventInfo( IniInterface&	ini );

	IniInterface& GetIni() const
	{
		return const_cast<IniInterface&>(m_ini);
	}
};

// --------------------------------------------------------------------------------------
//  IEventListener_CoreThread
// --------------------------------------------------------------------------------------
class IEventListener_CoreThread : public IEventDispatcher<CoreThreadStatus>
{
public:
	typedef CoreThreadStatus EvtParams;

public:
	IEventListener_CoreThread();
	virtual ~IEventListener_CoreThread() throw();

	virtual void DispatchEvent( const CoreThreadStatus& status )
	{
		switch( status )
		{
			case CoreStatus_Indeterminate:	OnCoreStatus_Indeterminate();	break;
			case CoreStatus_Started:		OnCoreStatus_Started();			break;
			case CoreStatus_Resumed:		OnCoreStatus_Resumed();			break;
			case CoreStatus_Suspended:		OnCoreStatus_Suspended();		break;
			case CoreStatus_Reset:			OnCoreStatus_Reset();			break;
			case CoreStatus_Stopped:		OnCoreStatus_Stopped();			break;
		}
	}

protected:
	virtual void OnCoreStatus_Indeterminate() {}
	virtual void OnCoreStatus_Started() {}
	virtual void OnCoreStatus_Resumed() {}
	virtual void OnCoreStatus_Suspended() {}
	virtual void OnCoreStatus_Reset() {}
	virtual void OnCoreStatus_Stopped() {}
};

// --------------------------------------------------------------------------------------
//  IEventListener_Plugins
// --------------------------------------------------------------------------------------
class IEventListener_Plugins : public IEventDispatcher<PluginEventType>
{
public:
	typedef PluginEventType EvtParams;

public:
	IEventListener_Plugins();
	virtual ~IEventListener_Plugins() throw();

	virtual void DispatchEvent( const PluginEventType& pevt )
	{
		switch( pevt )
		{
			case PluginsEvt_Loaded:		OnPluginsEvt_Loaded();		break;
			case PluginsEvt_Init:		OnPluginsEvt_Init();		break;
			case PluginsEvt_Opening:	OnPluginsEvt_Opening();		break;
			case PluginsEvt_Opened:		OnPluginsEvt_Opened();		break;
			case PluginsEvt_Closing:	OnPluginsEvt_Closing();		break;
			case PluginsEvt_Closed:		OnPluginsEvt_Closed();		break;
			case PluginsEvt_Shutdown:	OnPluginsEvt_Shutdown();	break;
			case PluginsEvt_Unloaded:	OnPluginsEvt_Unloaded();	break;
		}
	}

protected:
	virtual void OnPluginsEvt_Loaded() {}
	virtual void OnPluginsEvt_Init() {}
	virtual void OnPluginsEvt_Opening() {}		// dispatched prior to plugins being opened
	virtual void OnPluginsEvt_Opened() {}		// dispatched after plugins are opened
	virtual void OnPluginsEvt_Closing() {}		// dispatched prior to plugins being closed
	virtual void OnPluginsEvt_Closed() {}		// dispatched after plugins are closed
	virtual void OnPluginsEvt_Shutdown() {}
	virtual void OnPluginsEvt_Unloaded() {}

};

// --------------------------------------------------------------------------------------
//  IEventListener_AppStatus
// --------------------------------------------------------------------------------------
class IEventListener_AppStatus : public IEventDispatcher<AppEventInfo>
{
public:
	typedef AppEventInfo EvtParams;

public:
	IEventListener_AppStatus();
	virtual ~IEventListener_AppStatus() throw();

	virtual void DispatchEvent( const AppEventInfo& evtinfo );

protected:
	virtual void AppStatusEvent_OnSettingsLoadSave( const AppSettingsEventInfo& evtinfo ) {}
	virtual void AppStatusEvent_OnSettingsApplied() {}
	virtual void AppStatusEvent_OnExit() {}
};

// --------------------------------------------------------------------------------------
//  EventListenerHelper_AppStatus
// --------------------------------------------------------------------------------------
// Welcome to the awkward world of C++ multi-inheritence.  wxWidgets' Connect() system is
// incompatible because of limitations in C++ class member function pointers, so we need
// this second layer class to act as a bridge between the event system and the class's
// handler implementations.
//
template< typename TypeToDispatchTo >
class EventListenerHelper_CoreThread : public IEventListener_CoreThread
{
public:
	TypeToDispatchTo&	Owner;

public:
	EventListenerHelper_CoreThread( TypeToDispatchTo& dispatchTo )
		: Owner( dispatchTo ) { }

	EventListenerHelper_CoreThread( TypeToDispatchTo* dispatchTo )
		: Owner( *dispatchTo )
	{
		pxAssume(dispatchTo != NULL);
	}

	virtual ~EventListenerHelper_CoreThread() throw() {}

protected:
	void OnCoreStatus_Indeterminate()	{ Owner.OnCoreStatus_Indeterminate(); }
	void OnCoreStatus_Started()			{ Owner.OnCoreStatus_Started(); }
	void OnCoreStatus_Resumed()			{ Owner.OnCoreStatus_Resumed(); }
	void OnCoreStatus_Suspended()		{ Owner.OnCoreStatus_Suspended(); }
	void OnCoreStatus_Reset()			{ Owner.OnCoreStatus_Reset(); }
	void OnCoreStatus_Stopped()			{ Owner.OnCoreStatus_Stopped(); }
};

// --------------------------------------------------------------------------------------
//  EventListenerHelper_AppStatus
// --------------------------------------------------------------------------------------
// Welcome to the awkward world of C++ multi-inheritence.  wxWidgets' Connect() system is
// incompatible because of limitations in C++ class member function pointers, so we need
// this second layer class to act as a bridge between the event system and the class's
// handler implementations.
//
template< typename TypeToDispatchTo >
class EventListenerHelper_Plugins : public IEventListener_Plugins
{
public:
	TypeToDispatchTo&	Owner;

public:
	EventListenerHelper_Plugins( TypeToDispatchTo& dispatchTo )
		: Owner( dispatchTo ) { }

	EventListenerHelper_Plugins( TypeToDispatchTo* dispatchTo )
		: Owner( *dispatchTo )
	{
		pxAssume(dispatchTo != NULL);
	}

	virtual ~EventListenerHelper_Plugins() throw() {}

protected:
	void OnPluginsEvt_Loaded()		{ Owner.OnPluginsEvt_Loaded(); }
	void OnPluginsEvt_Init()		{ Owner.OnPluginsEvt_Init(); }
	void OnPluginsEvt_Opening()		{ Owner.OnPluginsEvt_Opening(); }
	void OnPluginsEvt_Opened()		{ Owner.OnPluginsEvt_Opened(); }
	void OnPluginsEvt_Closing()		{ Owner.OnPluginsEvt_Closing(); }
	void OnPluginsEvt_Closed()		{ Owner.OnPluginsEvt_Closed(); }
	void OnPluginsEvt_Shutdown()	{ Owner.OnPluginsEvt_Shutdown(); }
	void OnPluginsEvt_Unloaded()	{ Owner.OnPluginsEvt_Unloaded(); }
};

// --------------------------------------------------------------------------------------
//  EventListenerHelper_AppStatus
// --------------------------------------------------------------------------------------
// Welcome to the awkward world of C++ multi-inheritence.  wxWidgets' Connect() system is
// incompatible because of limitations in C++ class member function pointers, so we need
// this second layer class to act as a bridge between the event system and the class's
// handler implementations.
//
template< typename TypeToDispatchTo >
class EventListenerHelper_AppStatus : public IEventListener_AppStatus
{
public:
	TypeToDispatchTo&	Owner;

public:
	EventListenerHelper_AppStatus( TypeToDispatchTo& dispatchTo )
		: Owner( dispatchTo ) { }

	EventListenerHelper_AppStatus( TypeToDispatchTo* dispatchTo )
		: Owner( *dispatchTo )
	{
		pxAssume(dispatchTo != NULL);
	}

	virtual ~EventListenerHelper_AppStatus() throw() {}

protected:
	virtual void AppStatusEvent_OnSettingsLoadSave( const AppSettingsEventInfo& evtinfo ) { Owner.AppStatusEvent_OnSettingsLoadSave( evtinfo ); }
	virtual void AppStatusEvent_OnSettingsApplied() { Owner.AppStatusEvent_OnSettingsApplied(); }
	virtual void AppStatusEvent_OnExit() { Owner.AppStatusEvent_OnExit(); }
};
