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
	CoreThread_Indeterminate,
	CoreThread_Started,
	CoreThread_Resumed,
	CoreThread_Suspended,
	CoreThread_Reset,
	CoreThread_Stopped,
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
	CorePlugins_Loaded,
	CorePlugins_Init,
	CorePlugins_Opening,		// dispatched prior to plugins being opened
	CorePlugins_Opened,		// dispatched after plugins are opened
	CorePlugins_Closing,		// dispatched prior to plugins being closed
	CorePlugins_Closed,		// dispatched after plugins are closed
	CorePlugins_Shutdown,
	CorePlugins_Unloaded,
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
	virtual ~IEventListener_CoreThread() throw() {}

	virtual void DispatchEvent( const CoreThreadStatus& status );

protected:
	virtual void CoreThread_OnStarted() {}
	virtual void CoreThread_OnResumed() {}
	virtual void CoreThread_OnSuspended() {}
	virtual void CoreThread_OnReset() {}
	virtual void CoreThread_OnStopped() {}
};

class EventListener_CoreThread : public IEventListener_CoreThread
{
public:
	EventListener_CoreThread();
	virtual ~EventListener_CoreThread() throw();
};

// --------------------------------------------------------------------------------------
//  IEventListener_Plugins
// --------------------------------------------------------------------------------------
class IEventListener_Plugins : public IEventDispatcher<PluginEventType>
{
public:
	typedef PluginEventType EvtParams;

public:
	virtual ~IEventListener_Plugins() throw() {}

	virtual void DispatchEvent( const PluginEventType& pevt );

protected:
	virtual void CorePlugins_OnLoaded() {}
	virtual void CorePlugins_OnInit() {}
	virtual void CorePlugins_OnOpening() {}		// dispatched prior to plugins being opened
	virtual void CorePlugins_OnOpened() {}		// dispatched after plugins are opened
	virtual void CorePlugins_OnClosing() {}		// dispatched prior to plugins being closed
	virtual void CorePlugins_OnClosed() {}		// dispatched after plugins are closed
	virtual void CorePlugins_OnShutdown() {}
	virtual void CorePlugins_OnUnloaded() {}
};

class EventListener_Plugins : public IEventListener_Plugins
{
public:
	EventListener_Plugins();
	virtual ~EventListener_Plugins() throw();
};

// --------------------------------------------------------------------------------------
//  IEventListener_AppStatus
// --------------------------------------------------------------------------------------
class IEventListener_AppStatus : public IEventDispatcher<AppEventInfo>
{
public:
	typedef AppEventInfo EvtParams;

public:
	virtual ~IEventListener_AppStatus() throw() {}

	virtual void DispatchEvent( const AppEventInfo& evtinfo );

protected:
	virtual void AppStatusEvent_OnSettingsLoadSave( const AppSettingsEventInfo& evtinfo ) {}
	virtual void AppStatusEvent_OnSettingsApplied() {}
	virtual void AppStatusEvent_OnExit() {}
};

class EventListener_AppStatus : public IEventListener_AppStatus
{
public:
	EventListener_AppStatus();
	virtual ~EventListener_AppStatus() throw();
};

// --------------------------------------------------------------------------------------
//  EventListenerHelpers (CoreThread / Plugins / AppStatus)
// --------------------------------------------------------------------------------------
// Welcome to the awkward world of C++ multi-inheritence.  wxWidgets' Connect() system is
// incompatible because of limitations in C++ class member function pointers, so we need
// this second layer class to act as a bridge between the event system and the class's
// handler implementations.
//

template< typename TypeToDispatchTo >
class EventListenerHelper_CoreThread : public EventListener_CoreThread
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
	void OnCoreThread_Indeterminate()	{ Owner.OnCoreThread_Indeterminate(); }
	void CoreThread_OnStarted()			{ Owner.OnCoreThread_Started(); }
	void CoreThread_OnResumed()			{ Owner.OnCoreThread_Resumed(); }
	void CoreThread_OnSuspended()		{ Owner.OnCoreThread_Suspended(); }
	void CoreThread_OnReset()			{ Owner.OnCoreThread_Reset(); }
	void CoreThread_OnStopped()			{ Owner.OnCoreThread_Stopped(); }
};

template< typename TypeToDispatchTo >
class EventListenerHelper_Plugins : public EventListener_Plugins
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
	void CorePlugins_OnLoaded()		{ Owner.OnCorePlugins_Loaded(); }
	void CorePlugins_OnInit()		{ Owner.OnCorePlugins_Init(); }
	void CorePlugins_OnOpening()		{ Owner.OnCorePlugins_Opening(); }
	void CorePlugins_OnOpened()		{ Owner.OnCorePlugins_Opened(); }
	void CorePlugins_OnClosing()		{ Owner.OnCorePlugins_Closing(); }
	void CorePlugins_OnClosed()		{ Owner.OnCorePlugins_Closed(); }
	void CorePlugins_OnShutdown()	{ Owner.OnCorePlugins_Shutdown(); }
	void CorePlugins_OnUnloaded()	{ Owner.OnCorePlugins_Unloaded(); }
};

template< typename TypeToDispatchTo >
class EventListenerHelper_AppStatus : public EventListener_AppStatus
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
