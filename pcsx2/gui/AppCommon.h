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

#include "Utilities/SafeArray.h"
#include "Utilities/EventSource.h"
#include "Utilities/Threading.h"

#include "Utilities/wxGuiTools.h"
#include "Utilities/pxRadioPanel.h"
#include "Utilities/pxCheckBox.h"
#include "Utilities/pxStaticText.h"
#include "Utilities/CheckedStaticBox.h"

class MainEmuFrame;
class GSFrame;
class ConsoleLogFrame;
class PipeRedirectionBase;
class AppCoreThread;

enum AppEventType
{
	// Maybe this will be expanded upon later..?
	AppStatus_Exiting
};

enum PluginEventType
{
	PluginsEvt_Loaded,
	PluginsEvt_Init,
	PluginsEvt_Open,
	PluginsEvt_Close,
	PluginsEvt_Shutdown,
	PluginsEvt_Unloaded,
};

#include "AppConfig.h"
#include "Panels/BaseConfigPanel.h"
