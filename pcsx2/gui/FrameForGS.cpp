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

#include "PrecompiledHeader.h"
#include "MainFrame.h"
#include "GS.h"

#include "wx/utils.h"


void GSFrame::InitDefaultAccelerators()
{
	typedef KeyAcceleratorCode AAC;

	m_Accels.Map( AAC( WXK_F1 ),				"States_FreezeCurrentSlot" );
	m_Accels.Map( AAC( WXK_F3 ),				"States_DefrostCurrentSlot");
	m_Accels.Map( AAC( WXK_F2 ),				"States_CycleSlotForward" );
	m_Accels.Map( AAC( WXK_F2 ).Shift(),		"States_CycleSlotBackward" );
	
	m_Accels.Map( AAC( WXK_F4 ),				"Frameskip_Toggle" );
	m_Accels.Map( AAC( WXK_TAB ),				"Framelimiter_TurboToggle" );
	m_Accels.Map( AAC( WXK_TAB ).Shift(),		"Framelimiter_MasterToggle" );
	
	m_Accels.Map( AAC( WXK_ESCAPE ),			"Sys_Suspend" );
	m_Accels.Map( AAC( WXK_F8 ),				"Sys_TakeSnapshot" );
	m_Accels.Map( AAC( WXK_F9 ),				"Sys_RenderswitchToggle" );
	
	m_Accels.Map( AAC( WXK_F10 ),				"Sys_LoggingToggle" );
	m_Accels.Map( AAC( WXK_F11 ),				"Sys_FreezeGS" );
	m_Accels.Map( AAC( WXK_F12 ),				"Sys_RecordingToggle" );
}

GSFrame::GSFrame(wxWindow* parent, const wxString& title):
	wxFrame(parent, wxID_ANY, title, wxDefaultPosition, wxSize( 640, 480 ), wxDEFAULT_FRAME_STYLE )
{
	InitDefaultAccelerators();
	//new wxStaticText( "" );

	Connect( wxEVT_CLOSE_WINDOW,	wxCloseEventHandler(GSFrame::OnCloseWindow) );
	Connect( wxEVT_KEY_DOWN,		wxKeyEventHandler(GSFrame::OnKeyDown) );
}

GSFrame::~GSFrame() throw()
{
	if( HasCoreThread() )
		GetCoreThread().Suspend();		// Just in case...!
}

void GSFrame::OnCloseWindow(wxCloseEvent& evt)
{
	wxGetApp().OnGsFrameClosed();
	evt.Skip();		// and close it.
}

void GSFrame::OnKeyDown( wxKeyEvent& evt )
{
	const GlobalCommandDescriptor* cmd = NULL;
	m_Accels.TryGetValue( KeyAcceleratorCode( evt ).val32, cmd );
	if( cmd == NULL )
	{
		evt.Skip();		// Let the global APP handle it if it wants
		return;
	}
	
	if( cmd != NULL )
	{
		DbgCon.WriteLn( "(gsFrame) Invoking command: %s", cmd->Id );
		cmd->Invoke();
	}
}
