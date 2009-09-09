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

#define PLUGINtypedefs
#define PLUGINfuncs
#include "PS2Edefs.h"

#include <wx/dynlib.h>

struct PluginInfo
{
	const char* shortname;
	PluginsEnum_t id;
	int typemask;
	int version;			// minimum version required / supported
	
	wxString GetShortname() const
	{
		return wxString::FromUTF8( shortname );
	}
};

namespace Exception
{
	class PluginError : public virtual RuntimeError
	{
	public:
		PluginsEnum_t PluginId;

	public:
		DEFINE_EXCEPTION_COPYTORS( PluginError )
		PluginError() {}
		PluginError( PluginsEnum_t pid, const char* msg="Generic plugin error" )
		{
			BaseException::InitBaseEx( msg );
			PluginId = pid;
		}

		virtual wxString FormatDiagnosticMessage() const;
		virtual wxString FormatDisplayMessage() const;
	};

	// Plugin load errors occur when initially trying to load plugins during the
	// creation of a PluginManager object.  The error may either be due to non-existence,
	// corruption, or incompatible versioning.
	class PluginLoadError : public virtual PluginError, public virtual BadStream
	{
	public:
		DEFINE_EXCEPTION_COPYTORS( PluginLoadError )

		PluginLoadError( PluginsEnum_t pid, const wxString& objname, const char* eng );

		PluginLoadError( PluginsEnum_t pid, const wxString& objname,
			const wxString& eng_msg, const wxString& xlt_msg );

		virtual wxString FormatDiagnosticMessage() const;
		virtual wxString FormatDisplayMessage() const;
	};

	// Thrown when a plugin fails it's init() callback.  The meaning of this error is entirely
	// dependent on the plugin and, in most cases probably never happens (most plugins do little
	// more than a couple basic memory reservations during init)
	class PluginInitError : public virtual PluginError
	{
	public:
		DEFINE_EXCEPTION_COPYTORS( PluginInitError )

		explicit PluginInitError( PluginsEnum_t pid,
			const char* msg=wxLt("%s plugin failed to initialize.  Your system may have insufficient memory or resources needed.") )
		{
			BaseException::InitBaseEx( msg );
			PluginId = pid;
		}
	};

	// Plugin failed to open.  Typically this is a non-critical error that means the plugin has
	// not been configured properly by the user, but may also be indicative of a system
	class PluginOpenError : public virtual PluginError
	{
	public:
		DEFINE_EXCEPTION_COPYTORS( PluginOpenError )

		explicit PluginOpenError( PluginsEnum_t pid,
			const char* msg=wxLt("%s plugin failed to open.  Your computer may have insufficient resources, or incompatible hardware/drivers.") )
		{
			BaseException::InitBaseEx( msg );
			PluginId = pid;
		}
	};
};

//////////////////////////////////////////////////////////////////////////////////////////
// Important: Contents of this structure must match the order of the contents of the
// s_MethMessCommon[] array defined in Plugins.cpp.
//
// Note: Open is excluded from this list because the GS and CDVD have custom signatures >_<
//
struct LegacyPluginAPI_Common
{
	s32  (CALLBACK* Init)();
	void (CALLBACK* Close)();
	void (CALLBACK* Shutdown)();

	void (CALLBACK* KeyEvent)( keyEvent* evt );
	s32  (CALLBACK* Freeze)(int mode, freezeData *data);
	s32  (CALLBACK* Test)();
	void (CALLBACK* Configure)();
	void (CALLBACK* About)();
	
	LegacyPluginAPI_Common() :
		Init	( NULL )
	,	Close	( NULL )
	,	Shutdown( NULL )
	,	Freeze	( NULL )
	,	Test	( NULL )
	,	Configure( NULL )
	,	About	( NULL )
	{
	}
};

class SaveState;
class mtgsThreadObject;

//////////////////////////////////////////////////////////////////////////////////////////
// IPluginManager
// Provides a basic placebo "do-nothing" interface for plugin management.  This is used
// to avoid NULL pointer exceptions/segfaults when referencing the plugin manager global
// handle.
//
// Note: The Init and Freeze methods of this class will cause debug assertions, but Close
// methods fail silently, on the premise that Close and Shutdown are typically run from
// exception handlers or cleanup code, and null pointers should be silently ignored in
// favor of continuing cleanup.
//
class PluginManagerBase
{
	DeclareNoncopyableObject( PluginManagerBase )

public:
	PluginManagerBase() {}
	virtual ~PluginManagerBase() {}

	virtual void Init() { wxASSERT_MSG( false, L"Null PluginManager!" ); }
	virtual void Shutdown() {}
	virtual void Open() { }
	virtual void Open( PluginsEnum_t pid ) { wxASSERT_MSG( false, L"Null PluginManager!" ); }
	virtual void Close( PluginsEnum_t pid ) {}
	virtual void Close( bool closegs=true ) {}

	virtual void Freeze( PluginsEnum_t pid, int mode, freezeData* data ) { wxASSERT_MSG( false, L"Null PluginManager!" ); }
	virtual void Freeze( PluginsEnum_t pid, SaveState& state ) { wxASSERT_MSG( false, L"Null PluginManager!" ); }
	virtual void Freeze( SaveState& state ) { wxASSERT_MSG( false, L"Null PluginManager!" ); }
	
	virtual bool KeyEvent( const keyEvent& evt ) { return false; }
};

//////////////////////////////////////////////////////////////////////////////////////////
//
class PluginManager : public PluginManagerBase
{
	DeclareNoncopyableObject( PluginManager )

protected:
	struct PluginStatus_t
	{
		bool		IsInitialized;
		bool		IsOpened;
		wxString	Filename;

		LegacyPluginAPI_Common	CommonBindings;
		wxDynamicLibrary		Lib;
		
		PluginStatus_t() :
			IsInitialized( false )
		,	IsOpened( false )
		,	Filename()
		,	CommonBindings()
		,	Lib()
		{
		}
	};

	bool m_initialized;

	PluginStatus_t m_info[PluginId_Count];

public:
	virtual ~PluginManager();

	void Init();
	void Shutdown();
	void Open();
	void Open( PluginsEnum_t pid );
	void Close( PluginsEnum_t pid );
	void Close( bool closegs=true );

	void Freeze( PluginsEnum_t pid, int mode, freezeData* data );
	void Freeze( PluginsEnum_t pid, SaveState& state );
	void Freeze( SaveState& state );

	bool KeyEvent( const keyEvent& evt );
	void Configure( PluginsEnum_t pid );

	friend PluginManager* PluginManager_Create( const wxString (&folders)[PluginId_Count] );
	friend PluginManager* PluginManager_Create( const wxChar* (&folders)[PluginId_Count] );

protected:
	// Internal constructor, should be called by Create only.
	PluginManager( const wxString (&folders)[PluginId_Count] );

	void BindCommon( PluginsEnum_t pid );
	void BindRequired( PluginsEnum_t pid );
	void BindOptional( PluginsEnum_t pid );
	
	friend class mtgsThreadObject;
};

extern const PluginInfo tbl_PluginInfo[];
extern PluginManager* g_plugins;

extern PluginManager* PluginManager_Create( const wxString (&folders)[PluginId_Count] );
extern PluginManager* PluginManager_Create( const wxChar* (&folders)[PluginId_Count] );

extern PluginManagerBase& GetPluginManager();

