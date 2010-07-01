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

#include "AppCommon.h"

// --------------------------------------------------------------------------------------
//  AppCorePlugins
// --------------------------------------------------------------------------------------
// This extension of SysCorePlugins provides event listener sources for plugins -- loading,
// unloading, open, close, shutdown, etc.
//
// FIXME : Should this be made part of the PCSX2 core emulation? (integrated into SysCorePlugins)
//   I'm undecided on if it makes sense more in that context or in this one (interface).
//
class AppCorePlugins : public SysCorePlugins
{
	typedef SysCorePlugins _parent;

public:
	AppCorePlugins();
	virtual ~AppCorePlugins() throw();

	void Load( const wxString (&folders)[PluginId_Count] );
	void Load( PluginsEnum_t pid, const wxString& srcfile );
	void Unload( PluginsEnum_t pid );
	void Unload();

	bool Init();
	void Init( PluginsEnum_t pid );
	void Shutdown( PluginsEnum_t pid );
	bool Shutdown();
	void Close();	
	void Open();

protected:
	bool OpenPlugin_GS();
	void ClosePlugin_GS();
};
