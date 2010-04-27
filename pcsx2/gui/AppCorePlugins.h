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

#include "AppCommon.h"

// --------------------------------------------------------------------------------------
//  AppPluginManager
// --------------------------------------------------------------------------------------
// This extension of PluginManager provides event listener sources for plugins -- loading,
// unloading, open, close, shutdown, etc.
//
// FIXME : Should this be made part of the PCSX2 core emulation? (integrated into PluginManager)
//   I'm undecided on if it makes sense more in that context or in this one (interface).
//
class AppPluginManager : public PluginManager
{
	typedef PluginManager _parent;

public:
	AppPluginManager();
	virtual ~AppPluginManager() throw();

	void Load( const wxString (&folders)[PluginId_Count] );
	void Unload();
	
	void Init();
	void Shutdown();
	void Close();	
	void Open();

protected:
	bool OpenPlugin_GS();
	void ClosePlugin_GS();
};
