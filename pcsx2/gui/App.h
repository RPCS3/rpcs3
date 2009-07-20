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

#include "System.h"

using namespace Threading;

class MainEmuFrame;
class IniInterface;

extern wxFileHistory* g_RecentIsoList;

class LogWriteEvent;

DECLARE_EVENT_TYPE(wxEVT_DockConsole, -1);

//////////////////////////////////////////////////////////////////////////////////////////
//
class ConsoleLogFrame : public wxFrame, public NoncopyableObject
{
protected:
	class ColorArray : public NoncopyableObject
	{
	protected:
		SafeArray<wxTextAttr> m_table;
		wxTextAttr m_color_default;

	public:
		ColorArray();
		const wxTextAttr& operator[]( Console::Colors coloridx ) const
		{
			return m_table[(int)coloridx];
		}

		void SetFont( const wxFont& font );
		const wxTextAttr& Default() { return m_table[0]; }
	};

protected:
	wxTextCtrl&	m_TextCtrl;
	ColorArray	m_ColorTable;
	Console::Colors m_curcolor;
	volatile long m_msgcounter;		// used to track queued messages and throttle load placed on the gui message pump

public:
	// ctor & dtor
	ConsoleLogFrame( MainEmuFrame *pParent, const wxString& szTitle, const AppConfig::ConsoleLogOptions& options );
	virtual ~ConsoleLogFrame();

	virtual void Write( const wxString& text );
	virtual void SetColor( Console::Colors color );
	virtual void ClearColor();
	virtual void DockedMove();

	void Write( Console::Colors color, const wxString& text );
	void Newline();
	void CountMessage();
	void DoMessage();

protected:

	// menu callbacks
	virtual void OnOpen (wxMenuEvent& event);
	virtual void OnClose(wxMenuEvent& event);
	virtual void OnSave (wxMenuEvent& event);
	virtual void OnClear(wxMenuEvent& event);
	virtual void OnCloseWindow(wxCloseEvent& event);

	void OnWrite( wxCommandEvent& event );
	void OnNewline( wxCommandEvent& event );
	void OnSetTitle( wxCommandEvent& event );
	void OnDockedMove( wxCommandEvent& event );

	// common part of OnClose() and OnCloseWindow()
	virtual void DoClose();

	void OnMoveAround( wxMoveEvent& evt );
	void OnResize( wxSizeEvent& evt );
};


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
			Video;

		ConfigIds() :
			Paths( -1 )
		,	Plugins( -1 )
		,	Speedhacks( -1 )
		,	Gamefixes( -1 )
		,	Video( -1 )
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
class Pcsx2App : public wxApp
{
protected:
	MainEmuFrame* m_MainFrame;
	ConsoleLogFrame* m_ProgramLogBox;
	ConsoleLogFrame* m_Ps2ConLogBox;
	wxBitmap* m_Bitmap_Logo;

	wxImageList	m_ConfigImages;
	bool		m_ConfigImagesAreLoaded;

	wxImageList* m_ToolbarImages;		// dynamic (pointer) to allow for large/small redefinition.
	AppImageIds m_ImageId;

public:
	Pcsx2App();

	wxFrame* GetMainWindow() const;

	bool OnInit();
	int  OnExit();
	void OnInitCmdLine( wxCmdLineParser& parser );
	bool OnCmdLineParsed( wxCmdLineParser& parser );

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
};

DECLARE_APP(Pcsx2App)
