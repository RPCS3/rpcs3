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

// Loads plugins as specified in the Config global.
int  LoadPlugins();

// Unloads all plugin DLLs.  To change plugins, call ReleasePlugins followed by
// changes to Config.Plugins filenames, and then call LoadPlugins.
void ReleasePlugins();

int  OpenPlugins(const char* pTitleFilename);
void ClosePlugins( bool closegs );

int InitPlugins();

// Completely shuts down all plugins and re-initializes them. (clean slate)
// Plugins are not unloaded, so changes to Config.Plugins values will not
// take effect.  Use a manual set oc alls to ReleasePlugins and LoadPlugins for that.
void ResetPlugins();

void PluginsResetGS();

#endif /* __PLUGINS_H__ */
