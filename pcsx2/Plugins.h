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

struct PluginInfo
{
	const char* shortname;
	PluginsEnum_t id;
	int typemask;
	int version;			// minimum version required / supported
};

extern const PluginInfo tbl_PluginInfo[];

namespace Exception
{
	class NotPcsxPlugin : public Stream
	{
	public:
		virtual ~NotPcsxPlugin() throw() {}
		explicit NotPcsxPlugin( const wxString& objname ) :
		Stream( objname, wxLt("File is not a PCSX2 plugin") ) {}

		explicit NotPcsxPlugin( const PluginsEnum_t& pid ) :
		Stream( wxString::FromUTF8( tbl_PluginInfo[pid].shortname ), wxLt("File is not a PCSX2 plugin") ) {}
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

void LoadPlugins();
void ReleasePlugins();

void OpenPlugins(const char* pTitleFilename);
void ClosePlugins( bool closegs );
void CloseGS();

void InitPlugins();
void ShutdownPlugins();

void PluginsResetGS();

#endif /* __PLUGINS_H__ */
