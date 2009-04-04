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

#include <wx/wx.h>
#include <wx/fileconf.h>

#include "System.h"

class MainEmuFrame;

class IniInterface		// abstract base class!
{
protected:
	wxConfigBase& m_Config;

public:
	virtual ~IniInterface();
	explicit IniInterface();
	explicit IniInterface( wxConfigBase& config );

	void SetPath( const wxString& path );
	void Flush();
	
	virtual void Entry( const wxString& var, wxString& value, const wxString& defvalue=wxString() )=0;
	virtual void Entry( const wxString& var, int& value, const int defvalue=0 )=0;
	virtual void Entry( const wxString& var, uint& value, const uint defvalue=0 )=0;
	virtual void Entry( const wxString& var, bool& value, const bool defvalue=0 )=0;

	virtual void Entry( const wxString& var, wxPoint& value, const wxPoint& defvalue=wxDefaultPosition )=0;
	virtual void Entry( const wxString& var, wxSize& value, const wxSize& defvalue=wxDefaultSize )=0;
	virtual void Entry( const wxString& var, wxRect& value, const wxRect& defvalue=wxDefaultRect )=0;

	virtual void EnumEntry( const wxString& var, int& value, const char* const* enumArray, const int defvalue=0 )=0;
};

class IniLoader : public IniInterface
{
public:
	virtual ~IniLoader();
	explicit IniLoader();
	explicit IniLoader( wxConfigBase& config );

	void Entry( const wxString& var, wxString& value, const wxString& defvalue=wxString() );
	void Entry( const wxString& var, int& value, const int defvalue=0 );
	void Entry( const wxString& var, uint& value, const uint defvalue=0 );
	void Entry( const wxString& var, bool& value, const bool defvalue=false );

	void Entry( const wxString& var, wxPoint& value, const wxPoint& defvalue=wxDefaultPosition );
	void Entry( const wxString& var, wxSize& value, const wxSize& defvalue=wxDefaultSize );
	void Entry( const wxString& var, wxRect& value, const wxRect& defvalue=wxDefaultRect );

	void EnumEntry( const wxString& var, int& value, const char* const* enumArray, const int defvalue=0 );
};


class IniSaver : public IniInterface
{
public:
	virtual ~IniSaver();
	explicit IniSaver();
	explicit IniSaver( wxConfigBase& config );

	void Entry( const wxString& var, wxString& value, const wxString& defvalue=wxString() );
	void Entry( const wxString& var, int& value, const int defvalue=0 );
	void Entry( const wxString& var, uint& value, const uint defvalue=0 );
	void Entry( const wxString& var, bool& value, const bool defvalue=false );

	void Entry( const wxString& var, wxPoint& value, const wxPoint& defvalue=wxDefaultPosition );
	void Entry( const wxString& var, wxSize& value, const wxSize& defvalue=wxDefaultSize );
	void Entry( const wxString& var, wxRect& value, const wxRect& defvalue=wxDefaultRect );

	void EnumEntry( const wxString& var, int& value, const char* const* enumArray, const int defvalue=0 );
};

//////////////////////////////////////////////////////////////////////////////////////////
//
class AppConfig
{
public:
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

	struct FolderOptions
	{
		wxString Plugins;
		wxString Bios;
		wxString Snapshots;
		wxString Savestates;
		wxString MemoryCards;
	};

	// Options struct for each memory card.
	struct McdOptions
	{
		wxString Filename;		// user-configured location of this memory card
		bool Enabled;			// memory card enabled (if false, memcard will not show up in-game)
	};

	struct McdSysOptions
	{
		McdOptions Mcd[2];
		bool EnableNTFS;		// enables automatic ntfs compression of memory cards (Win32 only)
		bool EnableEjection;	// enables simulated ejection of memory cards when loading savestates

		void LoadSave( IniInterface& conf );
	};

	
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

	struct GamefixOptions
	{
		bool
			VuAddSubHack:1,		// Fix for Tri-ace games, they use an encryption algorithm that requires VU addi opcode to be bit-accurate.
			VuClipFlagHack:1,	// Fix for Digimon Rumble Arena 2, fixes spinning/hanging on intro-menu.
			FpuCompareHack:1,	// Fix for Persona games, maybe others. It's to do with the VU clip flag (again).
			FpuMulHack:1;		// Fix for Tales of Destiny hangs.

		void LoadSave();
	};
	
	struct SpeedhackOptions
	{
		int
			EECycleRate:3,		// EE cyclerate selector (1.0, 1.5, 2.0, 3.0)
			IopCycleRate_X2:1,	// enables the x2 multiplier of the IOP cyclerate
			ExtWaitcycles:1,	// enables extended waitcycles duration
			IntcStat:1;			// tells Pcsx2 to fast-forward through intc_stat waits.

		void LoadSave( IniInterface& conf );
	};

public:
	wxPoint MainGuiPosition;
	bool CdvdVerboseReads;		// enables cdvd read activity verbosely dumped to the console
	
	CpuRecompilerOptions Cpu;
	SpeedhackOptions Speedhacks;
	GamefixOptions Gamefixes;
	VideoOptions Video;
	ConsoleLogOptions ConLogBox;
	FolderOptions Folders;
	
public:
	void LoadSave( IniInterface& ini );
};

//////////////////////////////////////////////////////////////////////////////////////////
//
class ConsoleLogFrame : public wxFrame, public NoncopyableObject
{
protected:
	wxTextCtrl&	m_TextCtrl;

public:
	// ctor & dtor
	ConsoleLogFrame(MainEmuFrame *pParent, const wxString& szTitle);
	virtual ~ConsoleLogFrame();

	// menu callbacks
	virtual void OnClose(wxCommandEvent& event);
	virtual void OnCloseWindow(wxCloseEvent& event);

	virtual void OnSave (wxCommandEvent& event);
	virtual void OnClear(wxCommandEvent& event);

	virtual void Write( const wxChar* text );
	void Newline();

	void SetColor( Console::Colors color );
	void ClearColor();

protected:
	// use standard ids for our commands!
	enum
	{
		Menu_Close = wxID_CLOSE,
		Menu_Save  = wxID_SAVE,
		Menu_Clear = wxID_CLEAR
	};

	// common part of OnClose() and OnCloseWindow()
	virtual void DoClose();

	DECLARE_EVENT_TABLE()
	
	void OnMoveAround( wxMoveEvent& evt );
	void OnResize( wxSizeEvent& evt );
};

//////////////////////////////////////////////////////////////////////////////////////////
//
class Pcsx2App : public wxApp
{
protected:
	MainEmuFrame* m_MainFrame;
	ConsoleLogFrame* m_ConsoleFrame;
	wxBitmap* m_Bitmap_Logo;

public:
	Pcsx2App();

	bool OnInit();
	int OnExit();

	const wxBitmap& GetLogoBitmap() const;
	MainEmuFrame& GetMainFrame() const
	{
		wxASSERT( m_MainFrame != NULL );
		return *m_MainFrame;
	}

	ConsoleLogFrame* GetConsoleFrame() const { return m_ConsoleFrame; }
	void SetConsoleFrame( ConsoleLogFrame& frame ) { m_ConsoleFrame = &frame; }
	
	bool TryOpenConfigCwd();
};

DECLARE_APP(Pcsx2App)

extern AppConfig g_Conf;
