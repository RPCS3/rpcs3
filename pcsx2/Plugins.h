/*  Pcsx2 - Pc Ps2 Emulator
 *  Copyright (C) 2002-2009  Pcsx2 Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
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
	class NotPcsxPlugin : public Stream
	{
	public:
		virtual ~NotPcsxPlugin() throw() {}
		explicit NotPcsxPlugin( const wxString& objname );
		explicit NotPcsxPlugin( const PluginsEnum_t& pid );
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

//////////////////////////////////////////////////////////////////////////////////////////
//
class PluginManager
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
	~PluginManager();

	void Init();
	void Shutdown();
	void Open( PluginsEnum_t pid );
	void Close( PluginsEnum_t pid );
	void Close();

	void Freeze( PluginsEnum_t pid, int mode, freezeData* data );
	void Freeze( PluginsEnum_t pid, SaveState& state );
	void Freeze( SaveState& state );

	friend PluginManager* PluginManager_Create( const wxString (&folders)[PluginId_Count] );
	friend PluginManager* PluginManager_Create( const wxChar* (&folders)[PluginId_Count] );

protected:
	// Internal constructor, should be called by Create only.
	PluginManager( const wxString (&folders)[PluginId_Count] );

	void BindCommon( PluginsEnum_t pid );
	void BindRequired( PluginsEnum_t pid );
	void BindOptional( PluginsEnum_t pid );
};

extern const PluginInfo tbl_PluginInfo[];
extern PluginManager* g_plugins;

extern int EnumeratePluginsInFolder( const wxDirName& searchPath, wxArrayString* dest );


extern void LoadPlugins();
extern void ReleasePlugins();

extern void InitPlugins();
extern void ShutdownPlugins();

extern void OpenPlugins();
extern void ClosePlugins( bool closegs );

