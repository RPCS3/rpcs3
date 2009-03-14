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

#include "PrecompiledHeader.h"
#include "Misc.h"
#include "frmMain.h"
#include "frmGameFixes.h"
#include "frmLogging.h"

wxMenu* frmMain::MakeLanguagesMenu() const
{
	wxMenu* menuLangs = new wxMenu();
	
	// TODO : Index languages out of the resources and Langs folder.

	menuLangs->Append(Menu_Language_Start, _T("English"), wxEmptyString, wxITEM_RADIO);

	return menuLangs;
}

wxMenu* frmMain::MakeStatesMenu()
{
	wxMenu* mnuStates = new wxMenu();

	m_LoadStatesSubmenu.Append( Menu_State_LoadOther, _T("Other..."), wxEmptyString, wxITEM_NORMAL );
	m_SaveStatesSubmenu.Append( Menu_State_SaveOther, _T("Other..."), wxEmptyString, wxITEM_NORMAL );

	mnuStates->Append( Menu_State_Load, _T("Load"), &m_LoadStatesSubmenu, wxEmptyString );
	mnuStates->Append( Menu_State_Save, _T("Save"), &m_SaveStatesSubmenu, wxEmptyString );
	return mnuStates;
}

wxMenu* frmMain::MakeStatesSubMenu( int baseid ) const
{
	wxMenu* mnuSubstates = new wxMenu();
	wxString slot( _T("Slot") );

	mnuSubstates->Append( baseid,    _T("Slot 0"), wxEmptyString, wxITEM_NORMAL );
	mnuSubstates->Append( baseid+1,  _T("Slot 1"), wxEmptyString, wxITEM_NORMAL );
	mnuSubstates->Append( baseid+2,  _T("Slot 2"), wxEmptyString, wxITEM_NORMAL );
	mnuSubstates->Append( baseid+3,  _T("Slot 3"), wxEmptyString, wxITEM_NORMAL );
	mnuSubstates->Append( baseid+4,  _T("Slot 4"), wxEmptyString, wxITEM_NORMAL );
	return mnuSubstates;
}

void frmMain::PopulateVideoMenu()
{
	m_menuVideo.Append( Menu_Video_Basics,	_T("Basic Settings..."),	wxEmptyString, wxITEM_CHECK );
	m_menuVideo.AppendSeparator();

	// Populate options from the plugin here.
	
	m_menuVideo.Append( Menu_Video_Advanced,	_T("Advanced..."),		wxEmptyString, wxITEM_NORMAL );
}

void frmMain::PopulateAudioMenu()
{
	// Populate options from the plugin here.

	m_menuAudio.Append( Menu_Audio_Advanced,	_T("Advanced..."),		wxEmptyString, wxITEM_NORMAL );
}

void frmMain::PopulatePadMenu()
{
	// Populate options from the plugin here.

	m_menuPad.Append( Menu_Pad_Advanced,	_T("Advanced..."),		wxEmptyString, wxITEM_NORMAL );
}

void frmMain::ConnectMenus()
{
	// This just seems a bit more flexable & intuitive to me, if overly verbose.
	
	Connect( Menu_QuickBootCD, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(frmMain::Menu_QuickBootCD_Click));
	Connect( Menu_FullBootCD, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(frmMain::Menu_BootCD_Click));
	Connect( Menu_BootNoCD, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(frmMain::Menu_BootNoCD_Click));
	
	Connect( Menu_RunELF, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(frmMain::Menu_OpenELF_Click));
	Connect( Menu_Run_Exit, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(frmMain::Menu_Exit_Click));
	
	Connect( Menu_SuspendExec, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(frmMain::Menu_Suspend_Click));
	Connect( Menu_ResumeExec, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(frmMain::Menu_Resume_Click));
	Connect( Menu_Reset, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(frmMain::Menu_Reset_Click));
	
	Connect( Menu_State_LoadOther, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(frmMain::Menu_LoadStateOther_Click));
	Connect( Menu_State_SaveOther, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(frmMain::Menu_SaveStateOther_Click));
	
	Connect( Menu_Config_Gamefixes, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(frmMain::Menu_Gamefixes_Click));
	
	Connect( Menu_Debug_Open, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(frmMain::Menu_Debug_Open_Click));
	Connect( Menu_Debug_MemoryDump, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(frmMain::Menu_Debug_MemoryDump_Click));
	Connect( Menu_Debug_Logging, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(frmMain::Menu_Debug_Logging_Click));
}

frmMain::frmMain(wxWindow* parent, int id, const wxString& title, const wxPoint& pos, const wxSize& size, long style):
    wxFrame(parent, id, title, pos, size, wxCAPTION|wxCLOSE_BOX|wxSYSTEM_MENU|wxBORDER_THEME),

	m_menubar( *new wxMenuBar() ),
	m_statusbar( *CreateStatusBar(2, 0) ),
	m_background( *new wxStaticBitmap(this, wxID_ANY, wxBitmap(_T("./pcsxAbout.png"), wxBITMAP_TYPE_PNG) ) ),

	m_menuRun( *new wxMenu() ),
	m_menuConfig( *new wxMenu() ),
	m_menuMisc( *new wxMenu() ),

	m_menuVideo( *new wxMenu() ),
	m_menuAudio( *new wxMenu() ),
	m_menuPad( *new wxMenu() ),
	m_menuDebug( *new wxMenu() ),
    
	m_LoadStatesSubmenu( *MakeStatesSubMenu( Menu_State_Load01 ) ),
	m_SaveStatesSubmenu( *MakeStatesSubMenu( Menu_State_Save01 ) )
{
    
	// ------------------------------------------------------------------------
	
	m_menuRun.Append(Menu_QuickBootCD,	_T("Boot CDVD (Quick)"),	wxEmptyString, wxITEM_NORMAL);
	m_menuRun.Append(Menu_FullBootCD,	_T("Boot CDVD (Full)"),		wxEmptyString, wxITEM_NORMAL);
	m_menuRun.Append(Menu_BootNoCD,		_T("Boot without CDVD"),	wxEmptyString, wxITEM_NORMAL);
	m_menuRun.Append(Menu_RunELF,		_T("Run ELF File..."),		wxEmptyString, wxITEM_NORMAL);
	m_menuRun.AppendSeparator();

	m_menuRun.Append(Menu_SuspendExec,	_T("Suspend"),	_T("Suspends emulation progress."), wxITEM_NORMAL);
	m_menuRun.Append(Menu_ResumeExec,	_T("Resume"),	_T("Resumes emulation progress."), wxITEM_NORMAL);
	m_menuRun.Append(Menu_States,		_T("States"),	MakeStatesMenu(), wxEmptyString);
	m_menuRun.Append(Menu_Reset,		_T("Reset"),	_T("Resets emulation state and reloads plugins."), wxITEM_NORMAL);

	m_menuRun.AppendSeparator();
	m_menuRun.Append(Menu_Run_Exit,	_T("Exit"), _T("Closing Pcsx2 may be hazardous to your health"), wxITEM_NORMAL);

    // ------------------------------------------------------------------------

	m_menuConfig.Append(Menu_Config_CDVD,		_T("Cdvdrom"),	wxEmptyString, wxITEM_NORMAL);
	m_menuConfig.Append(Menu_Config_DEV9,		_T("Dev9"),		wxEmptyString, wxITEM_NORMAL);
	m_menuConfig.Append(Menu_Config_USB,		_T("USB"),		wxEmptyString, wxITEM_NORMAL);
	m_menuConfig.Append(Menu_Config_FireWire,	_T("Firewire"),	wxEmptyString, wxITEM_NORMAL);

	PopulateVideoMenu();
	PopulateAudioMenu();
	PopulatePadMenu();

	m_menuConfig.AppendSeparator();
	m_menuConfig.Append(Menu_SelectPlugins,	_T("Plugin Selector..."), wxEmptyString, wxITEM_NORMAL);

	m_menuConfig.AppendSeparator();
	m_menuConfig.Append(Menu_Config_Memcards,	_T("Memcards"),		_T("Memory card file locations and options."), wxITEM_NORMAL);
	m_menuConfig.Append(Menu_Config_Gamefixes,	_T("Gamefixes"),	_T("God of War and TriAce fixes are found here."), wxITEM_NORMAL);
	m_menuConfig.Append(Menu_Config_SpeedHacks,	_T("Speed Hacks"),	_T("Options to make Pcsx2 emulate faster, but less accurately."), wxITEM_NORMAL);
	m_menuConfig.Append(Menu_Config_Patches,	_T("Patches"),		wxEmptyString, wxITEM_NORMAL);
	m_menuConfig.Append(Menu_Config_Advanced,	_T("Advanced"),		_T("Cpu, Fpu, and Recompiler options."), wxITEM_NORMAL);

	// ------------------------------------------------------------------------

	m_menuMisc.Append(Menu_Console,	_T("Enable Console"), wxEmptyString, wxITEM_CHECK);
	m_menuMisc.Append(Menu_Patches,	_T("Enable Patches"), wxEmptyString, wxITEM_CHECK);
	m_menuMisc.Append(Menu_Profiler,_T("Enable Profiler"), wxEmptyString, wxITEM_CHECK);
	m_menuMisc.AppendSeparator();

	// No dialogs implemented for these yet...
	//m_menuMisc.Append(41, "Patch Browser...", wxEmptyString, wxITEM_NORMAL);
	//m_menuMisc.Append(42, "Patch Finder...", wxEmptyString, wxITEM_NORMAL);

	// Ref will want this re-added eventually.
	//m_menuMisc.Append(47, _T("Print CDVD Info..."), wxEmptyString, wxITEM_CHECK);

	m_menuMisc.Append(Menu_Languages,	_T("Languages"),	MakeLanguagesMenu(), wxEmptyString);
	m_menuMisc.AppendSeparator();

	m_menuMisc.Append(Menu_About,		_T("About..."), wxEmptyString, wxITEM_NORMAL);
	m_menuMisc.Append(Menu_Website,		_T("Pcsx2 Website..."), _T("Opens your web-browser!"), wxITEM_NORMAL);


	m_menuDebug.Append(Menu_Debug_Open,		_T("Open Debug Window..."), wxEmptyString, wxITEM_NORMAL);
	m_menuDebug.Append(Menu_Debug_MemoryDump,		_T("Memory Dump..."), wxEmptyString, wxITEM_NORMAL);
	m_menuDebug.Append(Menu_Debug_Logging,		_T("Logging..."), wxEmptyString, wxITEM_NORMAL);
	
	m_menubar.Append( &m_menuRun,		_T("Run" ));
	m_menubar.Append( &m_menuConfig,	_T("Config" ));
	m_menubar.Append( &m_menuVideo,		_T("Video" ));
	m_menubar.Append( &m_menuAudio,		_T("Audio" ));
	m_menubar.Append( &m_menuPad,		_T("Pad" ));
	m_menubar.Append( &m_menuMisc,		_T("Misc" ));
	m_menubar.Append( &m_menuDebug,		_T("Debug" ));

	SetMenuBar( &m_menubar );
	ConnectMenus();
	// The many other fancy portions of our main window!

	set_properties();
}

void frmMain::Menu_QuickBootCD_Click(wxCommandEvent &event)
{
}

void frmMain::Menu_BootCD_Click(wxCommandEvent &event)
{
}

void frmMain::Menu_BootNoCD_Click(wxCommandEvent &event)
{
}


void frmMain::Menu_OpenELF_Click(wxCommandEvent &event)
{
}

void frmMain::Menu_LoadStateOther_Click(wxCommandEvent &event)
{
}

void frmMain::Menu_SaveStateOther_Click(wxCommandEvent &event)
{
}

void frmMain::Menu_Exit_Click(wxCommandEvent &event)
{
	Close();
}

void frmMain::Menu_Suspend_Click(wxCommandEvent &event)
{
}

void frmMain::Menu_Resume_Click(wxCommandEvent &event)
{
}

void frmMain::Menu_Reset_Click(wxCommandEvent &event)
{
}

void frmMain::Menu_Gamefixes_Click( wxCommandEvent& event )
{
	frmGameFixes joe( NULL, wxID_ANY );
	joe.ShowModal();
}

void frmMain::Menu_Debug_Open_Click(wxCommandEvent &event)
{
}

void frmMain::Menu_Debug_MemoryDump_Click(wxCommandEvent &event)
{
}

void frmMain::Menu_Debug_Logging_Click(wxCommandEvent &event)
{
	frmLogging joe( NULL, wxID_ANY );
	joe.ShowModal();
}

void frmMain::set_properties()
{
	wxSize backsize( m_background.GetSize() );

	SetTitle(_("Pcsx2"));
	wxIcon _icon( _T("./cdrom02.png"), wxBITMAP_TYPE_PNG );
	SetIcon(_icon);
	SetClientSize( backsize );
	int m_statusbar_widths[] = { (int)(backsize.GetWidth()*0.73), (int)(backsize.GetWidth()*0.25) };
	m_statusbar.SetStatusWidths(2, m_statusbar_widths);
	m_statusbar.SetStatusText( _T("The Status is Good!"), 0);
	m_statusbar.SetStatusText( _T("Good Status"), 1);

	wxBoxSizer& joe( *new wxBoxSizer( wxVERTICAL ) );
	
	joe.Add( &m_background );
	SetSizerAndFit( &joe );

	//Layout();
}
