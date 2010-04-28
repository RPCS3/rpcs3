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

#include "App.h"
#include "SaveState.h"

// --------------------------------------------------------------------------------------
//  SaveSinglePluginHelper
// --------------------------------------------------------------------------------------
// A scoped convenience class for closing a single plugin and saving its state to memory.
// Emulation is suspended as needed, and is restored when the object leaves scope.  Within
// the scope of the object, code is free to call plugin re-configurations or even unload
// a plugin entirely and re-load a different plugin in its place.
//
class SaveSinglePluginHelper
{
protected:
	VmStateBuffer			m_plugstore;
	bool					m_validstate;
	PluginsEnum_t			m_pid;
	
	ScopedCoreThreadPause	m_scoped_pause;

public:
	SaveSinglePluginHelper( PluginsEnum_t pid );
	virtual ~SaveSinglePluginHelper() throw();
};


extern VmStateBuffer& StateCopy_GetBuffer();
extern bool StateCopy_IsValid();

extern void StateCopy_FreezeToMem();

extern void StateCopy_SaveToFile( const wxString& file );
extern void StateCopy_LoadFromFile( const wxString& file );
extern void StateCopy_SaveToSlot( uint num );
extern void StateCopy_LoadFromSlot( uint slot );
extern void StateCopy_Clear();
