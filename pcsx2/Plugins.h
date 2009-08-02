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

#ifndef __PLUGINS_H__
#define __PLUGINS_H__

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
};

class SaveState;

//////////////////////////////////////////////////////////////////////////////////////////
//
class PluginManager
{
protected:
	bool m_initialized;
	bool m_loaded;
	
	bool m_IsInitialized[PluginId_Count];
	bool m_IsOpened[PluginId_Count];

	LegacyPluginAPI_Common m_CommonBindings[PluginId_Count];
	wxDynamicLibrary m_libs[PluginId_Count];

public:
	~PluginManager();
	PluginManager() :
		m_initialized( false )
	,	m_loaded( false )
	{
		memzero_obj( m_IsInitialized );
		memzero_obj( m_IsOpened );
	}

	void LoadPlugins();
	void UnloadPlugins();
	
	void Init( PluginsEnum_t pid );
	void Shutdown( PluginsEnum_t pid );
	void Open( PluginsEnum_t pid );
	void Close( PluginsEnum_t pid );
	
	void Freeze( PluginsEnum_t pid, int mode, freezeData* data );
	void Freeze( PluginsEnum_t pid, SaveState& state );
	void Freeze( SaveState& state );
	
protected:
	void BindCommon( PluginsEnum_t pid );
	void BindRequired( PluginsEnum_t pid );
	void BindOptional( PluginsEnum_t pid );
};

extern const PluginInfo tbl_PluginInfo[];
extern PluginManager* g_plugins;


void LoadPlugins();
void ReleasePlugins();

void OpenPlugins(const char* pTitleFilename);
void ClosePlugins( bool closegs );
void CloseGS();

void InitPlugins();
void ShutdownPlugins();

void PluginsResetGS();

#endif /* __PLUGINS_H__ */
