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
#include <wx/imaglist.h>
#include <wx/docview.h>

#include <wx/apptrait.h>

#include "AppConfig.h"
#include "System.h"
#include "ConsoleLogger.h"
#include "ps2/CoreEmuThread.h"

class IniInterface;

//////////////////////////////////////////////////////////////////////////////////////////
//
struct AppImageIds
{
	struct ConfigIds
	{
		int	Paths,
			Plugins,
			Speedhacks,
			Gamefixes,
			Video,
			Cpu;

		ConfigIds() :
			Paths( -1 )
		,	Plugins( -1 )
		,	Speedhacks( -1 )
		,	Gamefixes( -1 )
		,	Video( -1 )
		,	Cpu( -1 )
		{
		}
	} Config;

	struct ToolbarIds
	{
		int Settings,
			Play,
			Resume,
			PluginVideo,
			PluginAudio,
			PluginPad;

		ToolbarIds() :
			Settings( -1 )
		,	Play( -1 )
		,	PluginVideo( -1 )
		,	PluginAudio( -1 )
		,	PluginPad( -1 )
		{
		}
	} Toolbars;
};

//////////////////////////////////////////////////////////////////////////////////////////
//
class pxAppTraits : public wxGUIAppTraits
{
#ifdef __WXDEBUG__
public:
	virtual bool ShowAssertDialog(const wxString& msg);

protected:
	virtual wxString GetAssertStackTrace();
#endif

};

//////////////////////////////////////////////////////////////////////////////////////////
//
class Pcsx2App : public wxApp
{
protected:
	MainEmuFrame*		m_MainFrame;
	ConsoleLogFrame*	m_ProgramLogBox;
	ConsoleLogFrame*	m_Ps2ConLogBox;
	wxBitmap*			m_Bitmap_Logo;

	wxImageList		m_ConfigImages;
	bool			m_ConfigImagesAreLoaded;

	wxImageList*	m_ToolbarImages;		// dynamic (pointer) to allow for large/small redefinition.
	AppImageIds		m_ImageId;

public:
	Pcsx2App();
	virtual ~Pcsx2App();

	wxFrame* GetMainWindow() const;

	bool OnInit();
	int  OnExit();
	void OnInitCmdLine( wxCmdLineParser& parser );
	bool OnCmdLineParsed( wxCmdLineParser& parser );

	bool PrepForExit();
	
#ifdef __WXDEBUG__
	void OnAssertFailure( const wxChar *file, int line, const wxChar *func, const wxChar *cond, const wxChar *msg );
#endif

	const wxBitmap& GetLogoBitmap();
	wxImageList& GetImgList_Config();
	wxImageList& GetImgList_Toolbars();

	const AppImageIds& GetImgId() const { return m_ImageId; }

	MainEmuFrame& GetMainFrame() const
	{
		wxASSERT( m_MainFrame != NULL );
		return *m_MainFrame;
	}

	ConsoleLogFrame* GetProgramLog()
	{
		return m_ProgramLogBox;
	}

	ConsoleLogFrame* GetConsoleLog()
	{
		return m_Ps2ConLogBox;
	}

	void CloseProgramLog()
	{
		m_ProgramLogBox->Close();

		// disable future console log messages from being sent to the window.
		m_ProgramLogBox = NULL;
	}

	void ProgramLog_CountMsg()
	{
		if( m_ProgramLogBox == NULL ) return;
		m_ProgramLogBox->CountMessage();
	}

	void ProgramLog_PostEvent( wxEvent& evt )
	{
		if( m_ProgramLogBox == NULL ) return;
		m_ProgramLogBox->GetEventHandler()->AddPendingEvent( evt );
	}

	void ConsoleLog_PostEvent( wxEvent& evt )
	{
		if( m_Ps2ConLogBox == NULL ) return;
		m_Ps2ConLogBox->GetEventHandler()->AddPendingEvent( evt );
	}

	//ConsoleLogFrame* GetConsoleFrame() const { return m_ProgramLogBox; }
	//void SetConsoleFrame( ConsoleLogFrame& frame ) { m_ProgramLogBox = &frame; }

protected:
	void ReadUserModeSettings();
	bool TryOpenConfigCwd();
	void OnMessageBox( pxMessageBoxEvent& evt );
	void CleanupMess();
};

//////////////////////////////////////////////////////////////////////////////////////////
//
class AppEmuThread : public CoreEmuThread
{
public:
	AppEmuThread( const wxString& elf_file=wxEmptyString );
	virtual ~AppEmuThread() { }
	
	virtual void Resume();

protected:
	sptr ExecuteTask();
};



DECLARE_APP(Pcsx2App)

extern int EnumeratePluginsInFolder( const wxDirName& searchPath, wxArrayString* dest );
extern void LoadPlugins();
extern void InitPlugins();
extern void OpenPlugins();

extern wxRect wxGetDisplayArea();
extern bool pxIsValidWindowPosition( const wxWindow& window, const wxPoint& windowPos );

extern wxFileHistory*	g_RecentIsoList;
