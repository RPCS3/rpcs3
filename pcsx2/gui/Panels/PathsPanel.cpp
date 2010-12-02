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

#include "PrecompiledHeader.h"
#include "ConfigurationPanels.h"

#include <wx/stdpaths.h>

using namespace pxSizerFlags;

static const int BetweenFolderSpace = 5;

// ------------------------------------------------------------------------
Panels::BasePathsPanel::BasePathsPanel( wxWindow* parent )
	: wxPanelWithHelpers( parent, wxVERTICAL )
{
}

// ------------------------------------------------------------------------
Panels::StandardPathsPanel::StandardPathsPanel( wxWindow* parent ) :
	BasePathsPanel( parent )
{
	*this += BetweenFolderSpace;
	*this += (new DirPickerPanel( this, FolderId_Savestates,
		_("Savestates:"),
		_("Select folder for Savestates") ))->
		SetToolTip( pxEt( "!ContextTip:Folders:Savestates",
			L"This folder is where PCSX2 records savestates; which are recorded either by using "
			L"menus/toolbars, or by pressing F1/F3 (load/save)."
		)
	) | SubGroup();

	*this += BetweenFolderSpace;
	*this += (new DirPickerPanel( this, FolderId_Snapshots,
		_("Snapshots:"),
		_("Select a folder for Snapshots") ))->
		SetToolTip( pxEt( "!ContextTip:Folders:Snapshots",
			L"This folder is where PCSX2 saves screenshots.  Actual screenshot image format and style "
			L"may vary depending on the GS plugin being used."
		)
	) | SubGroup();

	*this += BetweenFolderSpace;
	*this += (new DirPickerPanel( this, FolderId_Logs,
		_("Logs/Dumps:" ),
		_("Select a folder for logs/dumps") ))->
		SetToolTip( pxEt( "!ContextTip:Folders:Logs",
			L"This folder is where PCSX2 saves its logfiles and diagnostic dumps.  Most plugins will "
			L"also adhere to this folder, however some older plugins may ignore it."
		)
	) | SubGroup();

	/*
	*this += BetweenFolderSpace;
	*this += (new DirPickerPanel( this, FolderId_MemoryCards,
		_("Memorycards:"),
		_("Select a default Memorycards folder") ))->
		SetToolTip( pxE( "!Tooltip:Folders:Memorycards",
			L"This is the default path where PCSX2 loads or creates its memory cards, and can be "
			L"overridden in the MemoryCard Configuration by using absolute filenames."
		)
	) | SubGroup();*/

	*this += BetweenFolderSpace;

	GetSizer()->SetMinSize( wxSize( 475, GetSizer()->GetMinSize().GetHeight() ) );
}

