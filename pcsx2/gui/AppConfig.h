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

class IniInterface;

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
		bool Visible;
		// if true, DisplayPos is ignored and the console is automatically docked to the main window.
		bool AutoDock;
		// Display position used if AutoDock is false (ignored otherwise)
		wxPoint DisplayPosition;
		wxSize DisplaySize;
		
		// Size of the font in points.
		int FontSize;

		void LoadSave( IniInterface& conf, const wxChar* title );
	};

	// ------------------------------------------------------------------------
	struct FolderOptions
	{
		wxDirName
			Plugins,
			Settings,
			Bios,
			Snapshots,
			Savestates,
			MemoryCards,
			Logs;

		wxDirName RunIso;		// last used location for Iso loading.

		bool
			UseDefaultPlugins:1,
			UseDefaultSettings:1,
			UseDefaultBios:1,
			UseDefaultSnapshots:1,
			UseDefaultSavestates:1,
			UseDefaultMemoryCards:1,
			UseDefaultLogs:1;

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
	bool		UseAdminMode;			// dictates if the program uses /home/user or /cwd for the program data
	wxPoint		MainGuiPosition;

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

	McdOptions				Mcd[2];
	ConsoleLogOptions		ProgLogBox;
	ConsoleLogOptions		Ps2ConBox;
	FolderOptions			Folders;
	FilenameOptions			BaseFilenames;

	// PCSX2-core emulation options, which are passed to the emu core prior to initiating
	// an emulation session.  Note these are the options saved into the GUI ini file and
	// which are shown as options in the gui preferences, but *not* necessarily the options
	// used by emulation.  The gui allows temporary per-game and commandline level overrides.
	Pcsx2Config				EmuOptions;

protected:
	// indicates if the main AppConfig settings are valid (excludes the status of UseAdminMode,
	// which is a special value that's initialized independently of the rest of the config)
	bool m_IsLoaded;

public:
	AppConfig() :
		Listbook_ImageSize( 32 )
	,	Toolbar_ImageSize( 24 )
	,	m_IsLoaded( false )
	{
	}

	wxString FullpathToBios() const;
	wxString FullpathToMcd( uint mcdidx ) const;
	wxString FullpathTo( PluginsEnum_t pluginId ) const;

	void Load();
	void Save();
	void Apply();
	void LoadSaveUserMode( IniInterface& ini );

protected:
	void LoadSave( IniInterface& ini );
};

extern AppConfig* g_Conf;
