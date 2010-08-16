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

#include <wx/wx.h>

#include "Threading.h"
#include "wxGuiTools.h"
#include "pxEvents.h"

using namespace Threading;

class pxSynchronousCommandEvent;

// --------------------------------------------------------------------------------------
//  pxAppLog / ConsoleLogSource_App
// --------------------------------------------------------------------------------------

class ConsoleLogSource_App : public ConsoleLogSource
{
	typedef ConsoleLogSource _parent;

public:
	ConsoleLogSource_App();
};

extern ConsoleLogSource_App pxConLog_App;

#define pxAppLog pxConLog_App.IsActive() && pxConLog_App


// --------------------------------------------------------------------------------------
//  ModalButtonPanel
// --------------------------------------------------------------------------------------
class ModalButtonPanel : public wxPanelWithHelpers
{
public:
	ModalButtonPanel( wxWindow* window, const MsgButtons& buttons );
	virtual ~ModalButtonPanel() throw() { }

	virtual void AddActionButton( wxWindowID id );
	virtual void AddCustomButton( wxWindowID id, const wxString& label );

	virtual void OnActionButtonClicked( wxCommandEvent& evt );
};

#ifdef _MSC_VER
	typedef std::list< wxEvent*, wxObjectAllocator<wxEvent*> > wxEventList;
#else
	typedef std::list< wxEvent* > wxEventList;
#endif

// --------------------------------------------------------------------------------------
//  wxAppWithHelpers
// --------------------------------------------------------------------------------------
class wxAppWithHelpers : public wxApp
{
	typedef wxApp _parent;
	
	DECLARE_DYNAMIC_CLASS(wxAppWithHelpers)

protected:
	wxEventList					m_IdleEventQueue;
	Threading::MutexRecursive	m_IdleEventMutex;
	wxTimer						m_IdleEventTimer;

public:
	wxAppWithHelpers();
	virtual ~wxAppWithHelpers() {}

	void CleanUp();
	
	void DeleteObject( BaseDeletableObject& obj );
	void DeleteObject( BaseDeletableObject* obj )
	{
		if( obj == NULL ) return;
		DeleteObject( *obj );
	}

	void DeleteThread( Threading::pxThread& obj );
	void DeleteThread( Threading::pxThread* obj )
	{
		if( obj == NULL ) return;
		DeleteThread( *obj );
	}

	void PostCommand( void* clientData, int evtType, int intParam=0, long longParam=0, const wxString& stringParam=wxEmptyString );
	void PostCommand( int evtType, int intParam=0, long longParam=0, const wxString& stringParam=wxEmptyString );
	void PostMethod( FnType_Void* method );
	void PostIdleMethod( FnType_Void* method );
	void ProcessMethod( FnType_Void* method );

	bool Rpc_TryInvoke( FnType_Void* method );
	bool Rpc_TryInvokeAsync( FnType_Void* method );

	sptr ProcessCommand( void* clientData, int evtType, int intParam=0, long longParam=0, const wxString& stringParam=wxEmptyString );
	sptr ProcessCommand( int evtType, int intParam=0, long longParam=0, const wxString& stringParam=wxEmptyString );

	void ProcessAction( pxActionEvent& evt );
	void PostAction( const pxActionEvent& evt );

	void Ping();
	bool OnInit();
	//int  OnExit();

	void AddIdleEvent( const wxEvent& evt );

	void PostEvent( const wxEvent& evt );
	bool ProcessEvent( wxEvent& evt );
	bool ProcessEvent( wxEvent* evt );
	
	bool ProcessEvent( pxActionEvent& evt );
	bool ProcessEvent( pxActionEvent* evt );

protected:
	void IdleEventDispatcher( const wxChar* action=wxEmptyString );

	void OnIdleEvent( wxIdleEvent& evt );
	void OnStartIdleEventTimer( wxEvent& evt );
	void OnIdleEventTimeout( wxTimerEvent& evt );
	void OnDeleteObject( wxCommandEvent& evt );
	void OnDeleteThread( wxCommandEvent& evt );
	void OnSynchronousCommand( pxSynchronousCommandEvent& evt );
	void OnInvokeAction( pxActionEvent& evt );

};

namespace Msgbox
{
	extern int	ShowModal( BaseMessageBoxEvent& evt );
	extern int	ShowModal( const wxString& title, const wxString& content, const MsgButtons& buttons );
}
