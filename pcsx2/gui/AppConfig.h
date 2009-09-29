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

#include "PathDefs.h"
#include "CDVD/CDVDaccess.h"

class IniInterface;
class wxFileConfig;

extern bool			UseAdminMode;			// dictates if the program uses /home/user or /cwd for the program data
extern wxDirName	SettingsFolder;			// dictates where the settings folder comes from, *if* UseDefaultSettingsFolder is FALSE.
extern bool			UseDefaultSettingsFolder;	// when TRUE, pcsx2 derives the settings folder from the UseAdminMode

wxDirName GetSettingsFolder();
wxString  GetSettingsFilename();

//////////////////////////////////////////////////////////////////////////////////////////
// Pcsx2 Application Configuration.
//
//
class AppConfig
{
public:
	// ------------------------------------------------------------------------
	struct ConsoleLogOptions
	{
		bool		Visible;
		// if true, DisplayPos is ignored and the console is automatically docked to the main window.
		bool		AutoDock;
		// Display position used if AutoDock is false (ignored otherwise)
		wxPoint		DisplayPosition;
		wxSize		DisplaySize;
		
		// Size of the font in points.
		int			FontSize;

		ConsoleLogOptions();
		void LoadSave( IniInterface& conf, const wxChar* title );
	};

	// ------------------------------------------------------------------------
	struct FolderOptions
	{
		BITFIELD32()
			bool
				UseDefaultPlugins:1,
				UseDefaultSettings:1,
				UseDefaultBios:1,
				UseDefaultSnapshots:1,
				UseDefaultSavestates:1,
				UseDefaultMemoryCards:1,
				UseDefaultLogs:1;
		BITFIELD_END

		wxDirName
			Plugins,
			Bios,
			Snapshots,
			Savestates,
			MemoryCards,
			Logs;

		wxDirName RunIso;		// last used location for Iso loading.

		FolderOptions();
		void LoadSave( IniInterface& conf );
		void ApplyDefaults();

		void Set( FoldersEnum_t folderidx, const wxString& src, bool useDefault );

		const wxDirName& operator[]( FoldersEnum_t folderidx ) const;
		const bool IsDefault( FoldersEnum_t folderidx ) const;
	};

	// ------------------------------------------------------------------------
	struct FilenameOptions
	{
		wxFileName Bios;
		wxFileName Plugins[PluginId_Count];

		void LoadSave( IniInterface& conf );

		const wxFileName& operator[]( PluginsEnum_t pluginidx ) const;
	};

	// ------------------------------------------------------------------------
	// Options struct for each memory card.
	//
	struct McdOptions
	{
		wxFileName	Filename;	// user-configured location of this memory card
		bool		Enabled;	// memory card enabled (if false, memcard will not show up in-game)
	};

public:
	wxPoint		MainGuiPosition;

	// Because remembering the last used tab on the settings panel is cool (tab is remembered
	// by it's UTF/ASCII name).
	wxString	SettingsTabName;

	// Current language in use (correlates to a wxWidgets wxLANGUAGE specifier)
	wxLanguage	LanguageId;
	
	int			RecentFileCount;		// number of files displayed in the Recent Isos list.
	
	// String value describing the desktop theme to use for pcsk2 (icons and background images)
	// The theme name is used to look up files in the themes folder (relative to the executable).
	wxString	DeskTheme;

	// Specifies the size of icons used in Listbooks; specifically the PCSX2 Properties dialog box.
	// Realistic values range from 96x96 to 24x24.
	int			Listbook_ImageSize;

	// Specifies the size of each toolbar icon, in pixels (any value >= 2 is valid, but realistically
	// values should be between 64 and 16 for usability reasons)
	int			Toolbar_ImageSize;

	// Enables display of toolbar text labels.
	bool		Toolbar_ShowLabels;

	// enables automatic ntfs compression of memory cards (Win32 only)
	bool		McdEnableNTFS;
	
	// Closes the GS/Video port on escape (good for fullscreen activity)
	bool		CloseGSonEsc;

	wxString		CurrentIso;
	CDVD_SourceType CdvdSource;

	McdOptions				Mcd[2][4];
	ConsoleLogOptions		ProgLogBox;
	ConsoleLogOptions		Ps2ConBox;
	FolderOptions			Folders;
	FilenameOptions			BaseFilenames;

	// PCSX2-core emulation options, which are passed to the emu core prior to initiating
	// an emulation session.  Note these are the options saved into the GUI ini file and
	// which are shown as options in the gui preferences, but *not* necessarily the options
	// used by emulation.  The gui allows temporary per-game and commandline level overrides.
	Pcsx2Config				EmuOptions;

public:
	AppConfig();

	wxString FullpathToBios() const;
	wxString FullpathToMcd( uint port, uint slot ) const;
	wxString FullpathTo( PluginsEnum_t pluginId ) const;

	void LoadSaveUserMode( IniInterface& ini, const wxString& cwdhash );

	void LoadSave( IniInterface& ini );
	void LoadSaveMemcards( IniInterface& ini );
};

struct ConfigOverrides
{
	AppConfig::FilenameOptions	Filenames;
	wxString		SettingsFolder;
};

extern ConfigOverrides OverrideOptions;

extern wxFileConfig* OpenFileConfig( const wxString& filename );
extern void AppConfig_OnChangedSettingsFolder( bool overwrite =  false );

extern ScopedPtr<AppConfig> g_Conf;
