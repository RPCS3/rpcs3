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

enum PluginsEnum_t
{
	Plugin_CDVD = 0,
	Plugin_GS,
	Plugin_PAD1,
	Plugin_PAD2,
	Plugin_SPU2,
	Plugin_USB,
	Plugin_FW,
	Plugin_DEV9,
	Plugin_Count
};


//////////////////////////////////////////////////////////////////////////////////////////
// Pcsx2 Application Configuration.
//
// [TODO] : Rename this once we get to the point where the old Pcsx2Config stuff isn't in
// the way anymore. :)
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

		void LoadSave( IniInterface& conf );
	};

	// ------------------------------------------------------------------------
	struct FolderOptions
	{
		wxDirName Plugins;
		wxDirName Bios;
		wxDirName Snapshots;
		wxDirName Savestates;
		wxDirName MemoryCards;
		wxDirName Logs;
		wxDirName Dumps;

		void LoadSave( IniInterface& conf );
	};

	// ------------------------------------------------------------------------
	struct FilenameOptions
	{
		wxFileName Bios;
		wxFileName Plugins[Plugin_Count];

		void LoadSave( IniInterface& conf );

		const wxFileName& operator[]( PluginsEnum_t pluginidx ) const;
	};

	// ------------------------------------------------------------------------
	// Options struct for each memory card.
	//
	struct McdOptions
	{
		wxFileName Filename;	// user-configured location of this memory card
		bool Enabled;			// memory card enabled (if false, memcard will not show up in-game)
	};

	// ------------------------------------------------------------------------
	struct McdSysOptions
	{
		McdOptions Mcd[2];
		bool EnableNTFS;		// enables automatic ntfs compression of memory cards (Win32 only)
		bool EnableEjection;	// enables simulated ejection of memory cards when loading savestates

		void LoadSave( IniInterface& conf );
	};

	// ------------------------------------------------------------------------
	struct CpuRecompilerOptions
	{
		struct
		{
			bool
				Enabled:1,			// universal toggle for the profiler.
				RecBlocks_EE:1,		// Enables per-block profiling for the EE recompiler [unimplemented]
				RecBlocks_IOP:1,	// Enables per-block profiling for the IOP recompiler [unimplemented]
				RecBlocks_VU1:1;	// Enables per-block profiling for the VU1 recompiler [unimplemented]

		} Profiler;

		struct
		{
			bool
				EnableEE:1,
				EnableIOP:1,
				EnableVU0:1,
				EnableVU1:1;

		} Recompiler;

		void LoadSave( IniInterface& conf );
	};

	// ------------------------------------------------------------------------
	struct VideoOptions
	{
		bool MultithreadGS;		// Uses the multithreaded GS interface.
		bool closeOnEsc;		// Closes the GS/Video port on escape (good for fullscreen activity)
		bool UseFramelimiter;

		int RegionMode;			// 0=NTSC and 1=PAL
		int CustomFps;
		int CustomFrameSkip;
		int CustomConsecutiveFrames;
		int CustomConsecutiveSkip;

		void LoadSave( IniInterface& conf );
	};

	// ------------------------------------------------------------------------
	struct GamefixOptions
	{
		bool
			VuAddSubHack:1,		// Fix for Tri-ace games, they use an encryption algorithm that requires VU addi opcode to be bit-accurate.
			VuClipFlagHack:1,	// Fix for Digimon Rumble Arena 2, fixes spinning/hanging on intro-menu.
			FpuCompareHack:1,	// Fix for Persona games, maybe others. It's to do with the VU clip flag (again).
			FpuMulHack:1;		// Fix for Tales of Destiny hangs.

		void LoadSave();
	};

	// ------------------------------------------------------------------------
	struct SpeedhackOptions
	{
		int
			EECycleRate:3,		// EE cyclerate selector (1.0, 1.5, 2.0, 3.0)
			IopCycleRate_X2:1,	// enables the x2 multiplier of the IOP cyclerate
			ExtWaitcycles:1,	// enables extended waitcycles duration
			IntcStat:1;			// tells Pcsx2 to fast-forward through intc_stat waits.

		void LoadSave( IniInterface& conf );
	};

	// ------------------------------------------------------------------------
	// Helper functions for returning full pathnames of various Folders and files
	//
	struct FullpathHelpers
	{
		FullpathHelpers( const AppConfig& conf ) : m_conf( conf ) {}

		const AppConfig& m_conf;

		wxString operator[]( PluginsEnum_t pluginidx ) const;

		wxString Bios() const;
		wxString Mcd( uint mcdidx ) const;
	};

public:
	AppConfig() : Files( *this )
	,	Listbook_ImageSize( 32 )
	,	Toolbar_ImageSize( 32 )
	{
	}

	FullpathHelpers Files;

	wxPoint		MainGuiPosition;
	bool		CdvdVerboseReads;		// enables cdvd read activity verbosely dumped to the console

	// String value describing the desktop theme to use for pcsk2 (icons and background images)
	// The theme name is used to look up files in the themes folder (relative to the executable).
	wxString	DeskTheme;

	// Specifies the size of icons used in Listbooks; specifically the PCSX2 Properties dialog box.
	// Realisic values range from 96x96 to 24x24.
	int			Listbook_ImageSize;

	// Specifies the size of each toolbar icon, in pixels (any value >= 2 is valid, but realistically
	// values should be between 64 and 16 for usability reasons)
	int			Toolbar_ImageSize;

	// Enables display of toolbar text labels.
	bool		Toolbar_ShowLabels;

	CpuRecompilerOptions	Cpu;
	SpeedhackOptions		Speedhacks;
	GamefixOptions			Gamefixes;
	VideoOptions			Video;
	ConsoleLogOptions		ConLogBox;
	FolderOptions			Folders;
	FilenameOptions			BaseFilenames;
	McdSysOptions			MemoryCards;

public:
	void Load();
	void Save();

protected:
	void LoadSave( IniInterface& ini );
};

extern AppConfig g_Conf;
