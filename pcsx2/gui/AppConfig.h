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

#include "AppForwardDefs.h"
#include "PathDefs.h"
#include "CDVD/CDVDaccess.h"

enum DocsModeType
{
	// uses /home/user or /cwd for the program data.  This is the default mode and is the most
	// friendly to modern computing security requirements; as it isolates all file modification
	// to a zone of the hard drive that has granted write permissions to the user.
	DocsFolder_User,
	
	// uses a custom location for program data. Typically the custom folder is either the
	// absolute or relative location of the program -- absolute is preferred because it is
	// considered more secure by MSW standards, due to DLL search rules.
	//
	// To enable PCSX2's "portable" mode, use this setting and specify "." for the custom
	// documents folder.
	DocsFolder_Custom,
};

namespace PathDefs
{
	// complete pathnames are returned by these functions.
	// These are used for initial default values when first configuring PCSX2, or when the
	// user checks "Use Default paths" option provided on most path selectors.  These are not
	// used otherwise, in favor of the user-configurable specifications in the ini files.

	extern wxDirName GetUserLocalDataDir();
	extern wxDirName GetProgramDataDir();
	extern wxDirName GetDocuments();
	extern wxDirName GetDocuments( DocsModeType mode );
	extern wxDirName GetThemes();
}

extern DocsModeType		DocsFolderMode;				// 
extern bool				UseDefaultSettingsFolder;	// when TRUE, pcsx2 derives the settings folder from the DocsFolderMode
extern bool				UseDefaultPluginsFolder;
extern bool				UseDefaultThemesFolder;

extern wxDirName		CustomDocumentsFolder;		// allows the specification of a custom home folder for PCSX2 documents files.
extern wxDirName		SettingsFolder;				// dictates where the settings folder comes from, *if* UseDefaultSettingsFolder is FALSE.

extern wxDirName		InstallFolder;
extern wxDirName		PluginsFolder;
extern wxDirName		ThemesFolder;

extern wxDirName GetSettingsFolder();
extern wxString  GetVmSettingsFilename();
extern wxString  GetUiSettingsFilename();
extern wxString  GetUiKeysFilename();

extern wxDirName GetLogFolder();

enum InstallationModeType
{
	// Use the user defined folder selections.  These can be anywhere on a user's hard drive,
	// though by default the binaries (plugins, themes) are located in Install_Dir (registered
	// by the installer), and the user files (screenshots, inis) are in the user's documents
	// folder.  All folders are changable within the GUI.
	InstallMode_Registered,

	// In this mode, both Install_Dir and UserDocuments folders default the directory containing
	// PCSX2.exe, or the current working directory (if the PCSX2 directory could not be determined).
	// Folders cannot be changed from within the gui, however the fixed defaults can be manually
	// specified in the portable.ini by power users/devs.
	//
	// This mode is typically enabled by the presence of a 'portable.ini' in the folder.
	InstallMode_Portable,
};
bool IsPortable();

extern InstallationModeType	InstallationMode;

enum AspectRatioType
{
	AspectRatio_Stretch,
	AspectRatio_4_3,
	AspectRatio_16_9,
	AspectRatio_MaxCount
};

// =====================================================================================================
//  Pcsx2 Application Configuration. 
// =====================================================================================================

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

		// Color theme by name!
		wxString	Theme;

		ConsoleLogOptions();
		void LoadSave( IniInterface& conf, const wxChar* title );
	};

	// ------------------------------------------------------------------------
	struct FolderOptions
	{
		BITFIELD32()
			bool
				UseDefaultBios:1,
				UseDefaultSnapshots:1,
				UseDefaultSavestates:1,
				UseDefaultMemoryCards:1,
				UseDefaultLogs:1,
				UseDefaultLangs:1,
				UseDefaultCheats:1,
				UseDefaultCheatsWS:1;
		BITFIELD_END

		wxDirName
			Bios,
			Snapshots,
			Savestates,
			MemoryCards,
			Langs,
			Logs,
			Cheats,
			CheatsWS;

		wxDirName RunIso;		// last used location for Iso loading.
		wxDirName RunELF;		// last used location for ELF loading.

		FolderOptions();
		void LoadSave( IniInterface& conf );
		void ApplyDefaults();

		void Set( FoldersEnum_t folderidx, const wxString& src, bool useDefault );

		const wxDirName& operator[]( FoldersEnum_t folderidx ) const;
		wxDirName& operator[]( FoldersEnum_t folderidx );
		bool IsDefault( FoldersEnum_t folderidx ) const;
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

	// ------------------------------------------------------------------------
	// The GS window receives much love from the land of Options and Settings.
	//
	struct GSWindowOptions
	{
		// Closes the GS/Video port on escape (good for fullscreen activity)
		bool		CloseOnEsc;
		
		bool		DefaultToFullscreen;
		bool		AlwaysHideMouse;
		bool		DisableResizeBorders;
		bool		DisableScreenSaver;

		AspectRatioType AspectRatio;
		Fixed100	Zoom;
		Fixed100	StretchY;
		Fixed100	OffsetX;
		Fixed100	OffsetY;


		wxSize		WindowSize;
		wxPoint		WindowPos;
		bool		IsMaximized;
		bool		IsFullscreen;

		bool		IsToggleFullscreenOnDoubleClick;

		GSWindowOptions();

		void LoadSave( IniInterface& conf );
		void SanityCheck();
	};

	struct FramerateOptions
	{
		bool		SkipOnLimit;
		bool		SkipOnTurbo;

		Fixed100	NominalScalar;
		Fixed100	TurboScalar;
		Fixed100	SlomoScalar;

		FramerateOptions();
		
		void LoadSave( IniInterface& conf );
		void SanityCheck();
	};

public:
	wxPoint		MainGuiPosition;

	// Because remembering the last used tab on the settings panel is cool (tab is remembered
	// by it's UTF/ASCII name).
	wxString	SysSettingsTabName;
	wxString	McdSettingsTabName;
	wxString	ComponentsTabName;
	wxString	AppSettingsTabName;
	wxString	GameDatabaseTabName;

	// Currently selected language ID -- wxWidgets version-specific identifier.  This is one side of
	// a two-part configuration that also includes LanguageCode.
	wxLanguage	LanguageId;

	// Current language in use (correlates to the universal language codes, such as "en_US", "de_DE", etc).
	// This code is not always unique, which is why we use the language ID also.
	wxString	LanguageCode;

	int			RecentIsoCount;		// number of files displayed in the Recent Isos list.

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

	// uses automatic ntfs compression when creating new memory cards (Win32 only)
#ifdef __WXMSW__
	bool		McdCompressNTFS;
#endif

	// Master toggle for enabling or disabling all speedhacks in one fail-free swoop.
	// (the toggle is applied when a new EmuConfig is sent through AppCoreThread::ApplySettings)
	bool		EnableSpeedHacks;
	bool		EnableGameFixes;

	// Presets try to prevent users from overwhelming when they want to change settings (usually to make a game run faster).
	// The presets allow to modify the balance between emulation accuracy and emulation speed using a pseudo-linear control.
	// It's pseudo since there's no way to arrange groups of all of pcsx2's settings such that each next group makes it slighty faster and slightly less compatiible for all games.
	//However, By carefully selecting these preset config groups, it's hopefully possible to achieve this goal for a reasonable percentage (hopefully above 50%) of the games.
	//when presets are enabled, the user has practically no control over the emulation settings, and can only choose the preset to use.

	// The next 2 vars enable/disable presets alltogether, and select/reflect current preset, respectively.
	bool		EnablePresets;
	int			PresetIndex;

	wxString				CurrentIso;
	wxString				CurrentELF;
	CDVD_SourceType			CdvdSource;

	// Memorycard options - first 2 are default slots, last 6 are multitap 1 and 2
	// slots (3 each)
	McdOptions				Mcd[8];
	
	ConsoleLogOptions		ProgLogBox;
	FolderOptions			Folders;
	FilenameOptions			BaseFilenames;
	GSWindowOptions			GSWindow;
	FramerateOptions		Framerate;
	
	// PCSX2-core emulation options, which are passed to the emu core prior to initiating
	// an emulation session.  Note these are the options saved into the GUI ini file and
	// which are shown as options in the gui preferences, but *not* necessarily the options
	// used by emulation.  The gui allows temporary per-game and commandline level overrides.
	Pcsx2Config				EmuOptions;

public:
	AppConfig();

	wxString FullpathToBios() const;
	wxString FullpathToMcd( uint slot ) const;
	wxString FullpathTo( PluginsEnum_t pluginId ) const;

	bool FullpathMatchTest( PluginsEnum_t pluginId, const wxString& cmpto ) const;

	void LoadSave( IniInterface& ini );
	void LoadSaveRootItems( IniInterface& ini );
	void LoadSaveMemcards( IniInterface& ini );

	static int  GetMaxPresetIndex();
    static bool isOkGetPresetTextAndColor(int n, wxString& label, wxColor& c);
	
	bool        IsOkApplyPreset(int n);


	//The next 2 flags are used with ApplyConfigToGui which the presets system use:
	
	//Indicates that the scope is only for preset-related items.
	static const int APPLY_FLAG_FROM_PRESET			= 0x01;

	//Indicates that the change should manually propagate to sub items because it's called directly and not as an event.
	//Currently used by some panels which contain sub-panels which are affected by presets.
	static const int APPLY_FLAG_MANUALLY_PROPAGATE	= 0x02;

};

extern void AppLoadSettings();
extern void AppSaveSettings();
extern void AppApplySettings( const AppConfig* oldconf=NULL );

extern void App_LoadSaveInstallSettings( IniInterface& ini );
extern void App_SaveInstallSettings( wxConfigBase* ini );
extern void App_LoadInstallSettings( wxConfigBase* ini );

extern void ConLog_LoadSaveSettings( IniInterface& ini );
extern void SysTraceLog_LoadSaveSettings( IniInterface& ini );


extern wxFileConfig* OpenFileConfig( const wxString& filename );
extern void RelocateLogfile();
extern void AppConfig_OnChangedSettingsFolder( bool overwrite =  false );
extern wxConfigBase* GetAppConfig();

extern ScopedPtr<AppConfig> g_Conf;
