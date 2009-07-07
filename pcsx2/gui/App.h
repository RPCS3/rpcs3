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
class IniInterface;

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
	virtual void Write( const char* text );
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

struct AppImageIds
{
	struct ConfigIds
	{
		int	Paths,
			Speedhacks,
			Gamefixes,
			Video;
			
		ConfigIds() : 
			Paths( -1 )
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
	ConsoleLogFrame* m_ConsoleFrame;
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

	ConsoleLogFrame* GetConsoleFrame() const { return m_ConsoleFrame; }
	void SetConsoleFrame( ConsoleLogFrame& frame ) { m_ConsoleFrame = &frame; }
	
	bool TryOpenConfigCwd();
};

DECLARE_APP(Pcsx2App)
