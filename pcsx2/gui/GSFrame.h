
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

#include "AppCommon.h"
#include "CpuUsageProvider.h"


enum LimiterModeType
{
	Limit_Nominal,
	Limit_Turbo,
	Limit_Slomo,
};

extern LimiterModeType g_LimiterMode;

// --------------------------------------------------------------------------------------
//  GSPanel
// --------------------------------------------------------------------------------------
class GSPanel : public wxWindow
	, public EventListener_AppStatus
{
	typedef wxWindow _parent;

protected:
	ScopedPtr<AcceleratorDictionary>	m_Accels;

	wxTimer					m_HideMouseTimer;
	bool					m_CursorShown;
	bool					m_HasFocus;

public:
	GSPanel( wxWindow* parent );
	virtual ~GSPanel() throw();

	void DoResize();
	void DoShowMouse();

protected:
	void AppStatusEvent_OnSettingsApplied();

#ifdef __WXMSW__
	virtual WXLRESULT MSWWindowProc(WXUINT message, WXWPARAM wParam, WXLPARAM lParam);
#endif

	void InitDefaultAccelerators();

	void OnCloseWindow( wxCloseEvent& evt );
	void OnResize(wxSizeEvent& event);
	void OnShowMouse( wxMouseEvent& evt );
	void OnHideMouseTimeout( wxTimerEvent& evt );
	void OnKeyDown( wxKeyEvent& evt );
	void OnFocus( wxFocusEvent& evt );
	void OnFocusLost( wxFocusEvent& evt );
};


// --------------------------------------------------------------------------------------
//  GSFrame
// --------------------------------------------------------------------------------------
class GSFrame : public wxFrame
	, public EventListener_AppStatus
	, public EventListener_CoreThread
{
	typedef wxFrame _parent;

protected:
	wxTimer					m_timer_UpdateTitle;
	wxWindowID				m_id_gspanel;
	wxWindowID				m_id_OutputDisabled;
	wxStaticText*			m_label_Disabled;
	wxStatusBar*			m_statusbar;

	CpuUsageProvider		m_CpuUsage;

public:
	GSFrame(wxWindow* parent, const wxString& title);
	virtual ~GSFrame() throw();

	GSPanel* GetViewport();
	void SetFocus();
	bool Show( bool shown=true );
	wxStaticText* GetLabel_OutputDisabled() const;

	bool ShowFullScreen(bool show, long style = wxFULLSCREEN_ALL);

protected:
	void OnCloseWindow( wxCloseEvent& evt );
	void OnMove( wxMoveEvent& evt );
	void OnResize( wxSizeEvent& evt );
	void OnActivate( wxActivateEvent& evt );
	void OnUpdateTitle( wxTimerEvent& evt );

	void AppStatusEvent_OnSettingsApplied();
	void CoreThread_OnResumed();
	void CoreThread_OnSuspended();
};

// --------------------------------------------------------------------------------------
//  s* macros!  ['s' stands for 'shortcut']
// --------------------------------------------------------------------------------------
// Use these for "silent fail" invocation of PCSX2 Application-related constructs.  If the
// construct (albeit wxApp, MainFrame, CoreThread, etc) is null, the requested method will
// not be invoked, and an optional "else" clause can be affixed for handling the end case.
//
// See App.h (sApp) for more details.
//
#define sGSFrame \
	if( GSFrame* __gsframe_ = wxGetApp().GetGsFramePtr() ) (*__gsframe_)
