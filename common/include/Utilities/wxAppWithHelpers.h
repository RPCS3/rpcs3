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

#pragma once

#include <wx/wx.h>

#include "Threading.h"
#include "wxGuiTools.h"
#include "pxEvents.h"

using namespace Threading;

class pxSynchronousCommandEvent;


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

typedef void FnType_Void();

// --------------------------------------------------------------------------------------
//  wxAppWithHelpers
// --------------------------------------------------------------------------------------
class wxAppWithHelpers : public wxApp
{
	typedef wxApp _parent;
	
	DECLARE_DYNAMIC_CLASS(wxAppWithHelpers)

protected:
	std::vector<wxEvent*>			m_IdleEventQueue;
	Threading::Mutex				m_IdleEventMutex;
	wxTimer							m_IdleEventTimer;

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

	void DeleteThread( Threading::PersistentThread& obj );
	void DeleteThread( Threading::PersistentThread* obj )
	{
		if( obj == NULL ) return;
		DeleteThread( *obj );
	}

	void PostCommand( void* clientData, int evtType, int intParam=0, long longParam=0, const wxString& stringParam=wxEmptyString );
	void PostCommand( int evtType, int intParam=0, long longParam=0, const wxString& stringParam=wxEmptyString );
	void PostMethod( FnType_Void* method );
	void PostIdleMethod( FnType_Void* method );
	void ProcessMethod( void (*method)() );

	sptr ProcessCommand( void* clientData, int evtType, int intParam=0, long longParam=0, const wxString& stringParam=wxEmptyString );
	sptr ProcessCommand( int evtType, int intParam=0, long longParam=0, const wxString& stringParam=wxEmptyString );

	void ProcessAction( pxInvokeActionEvent& evt );
	void PostAction( const pxInvokeActionEvent& evt );

	bool PostMethodMyself( void (*method)() );

	void Ping();
	bool OnInit();
	//int  OnExit();

	void AddIdleEvent( const wxEvent& evt );

	void PostEvent( const wxEvent& evt );
	bool ProcessEvent( wxEvent& evt );
	bool ProcessEvent( wxEvent* evt );
	
	bool ProcessEvent( pxInvokeActionEvent& evt );
	bool ProcessEvent( pxInvokeActionEvent* evt );

protected:
	void IdleEventDispatcher( const char* action );

	void OnIdleEvent( wxIdleEvent& evt );
	void OnStartIdleEventTimer( wxEvent& evt );
	void OnIdleEventTimeout( wxTimerEvent& evt );
	void OnDeleteObject( wxCommandEvent& evt );
	void OnDeleteThread( wxCommandEvent& evt );
	void OnSynchronousCommand( pxSynchronousCommandEvent& evt );
	void OnInvokeAction( pxInvokeActionEvent& evt );

};

namespace Msgbox
{
	extern int	ShowModal( BaseMessageBoxEvent& evt );
}
