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

#include "PrecompiledHeader.h"
#include "HashMap.h"
#include "wxGuiTools.h"
#include "pxStaticText.h"
#include "Threading.h"

#include <wx/cshelp.h>
#include <wx/tooltip.h>
#include <wx/spinctrl.h>

using namespace pxSizerFlags;

// --------------------------------------------------------------------------------------
//  BaseDeletableObject Implementation
// --------------------------------------------------------------------------------------
// This code probably deserves a better home.  It's general purpose non-GUI code (the single
// wxApp/Gui dependency is in wxGuiTools.cpp for now).
//
bool BaseDeletableObject::MarkForDeletion()
{
	return !_InterlockedExchange( &m_IsBeingDeleted, true );
}

void BaseDeletableObject::DeleteSelf()
{
	if( MarkForDeletion() )
		DoDeletion();
}

BaseDeletableObject::BaseDeletableObject()
{
	#ifdef _MSC_VER
	// Bleh, this fails because _CrtIsValidHeapPointer calls HeapValidate on the
	// pointer, but the pointer is a virtual base class, so it's not a valid block. >_<
	//pxAssertDev( _CrtIsValidHeapPointer( this ), "BaseDeletableObject types cannot be created on the stack or as temporaries!" );
	#endif

	m_IsBeingDeleted = false;
}

BaseDeletableObject::~BaseDeletableObject() throw()
{
	AffinityAssert_AllowFrom_MainUI();
}


// --------------------------------------------------------------------------------------


// Creates a text control which is right-justified and has it's minimum width configured to suit
// the number of digits requested.
wxTextCtrl* CreateNumericalTextCtrl( wxWindow* parent, int digits )
{
	wxTextCtrl* ctrl = new wxTextCtrl( parent, wxID_ANY );
	ctrl->SetWindowStyleFlag( wxTE_RIGHT );
	pxFitToDigits( ctrl, digits );
	return ctrl;
}

void pxFitToDigits( wxWindow* win, int digits )
{
	int ex;
	win->GetTextExtent( wxString( L'0', digits+1 ), &ex, NULL );
	win->SetMinSize( wxSize( ex+10, wxDefaultCoord ) );		// +10 for text control borders/insets and junk.
}

void pxFitToDigits( wxSpinCtrl* win, int digits )
{
	// HACK!!  The better way would be to create a pxSpinCtrl class that extends wxSpinCtrl and thus
	// have access to wxSpinButton::DoGetBestSize().  But since I don't want to do that, we'll just
	// make/fake it with a value it's pretty common to Win32/GTK/Mac:

	static const int MagicSpinnerSize = 18;

	int ex;
	win->GetTextExtent( wxString( L'0', digits+1 ), &ex, NULL );
	win->SetMinSize( wxSize( ex+10+MagicSpinnerSize, wxDefaultCoord ) );		// +10 for text control borders/insets and junk.
}

bool pxDialogExists( const wxString& name )
{
	return wxFindWindowByName( name ) != NULL;
}

// =====================================================================================================
//  wxDialogWithHelpers Class Implementations
// =====================================================================================================

DEFINE_EVENT_TYPE( pxEvt_OnDialogCreated )

IMPLEMENT_DYNAMIC_CLASS(wxDialogWithHelpers, wxDialog)

wxDialogWithHelpers::wxDialogWithHelpers()
{
	m_hasContextHelp	= false;
	m_extraButtonSizer	= NULL;

	Init();
}

wxDialogWithHelpers::wxDialogWithHelpers( wxWindow* parent, const wxString& title, bool hasContextHelp, bool resizable )
	: wxDialog( parent, wxID_ANY, title, wxDefaultPosition, wxDefaultSize,
		wxDEFAULT_DIALOG_STYLE | (resizable ? wxRESIZE_BORDER : 0)
	)
{
	m_hasContextHelp	= hasContextHelp;
	Init();
}

wxDialogWithHelpers::wxDialogWithHelpers(wxWindow* parent, const wxString& title, wxOrientation orient)
	: wxDialog( parent, wxID_ANY, title )
{
	m_hasContextHelp	= false;
	SetSizer( new wxBoxSizer( orient ) );
	Init();

	SetMinWidth( 500 );
	*this += StdPadding;
}

wxDialogWithHelpers::~wxDialogWithHelpers() throw()
{
}

void wxDialogWithHelpers::Init()
{
	m_extraButtonSizer	= NULL;

	if( m_hasContextHelp )
		delete wxHelpProvider::Set( new wxSimpleHelpProvider() );

	// GTK/Linux Note: currently the Close (X) button doesn't appear to work in dialogs.  Docs
	// indicate that it should, so I presume the problem is in wxWidgets and that (hopefully!)
	// an updated version will fix it later.  I tried to fix it using a manual Connect but it
	// didn't do any good.  (problem could also be my Co-Linux / x-window manager)

	Connect( pxEvt_OnDialogCreated,	wxCommandEventHandler	(wxDialogWithHelpers::OnDialogCreated) );

	Connect( wxID_OK,		wxEVT_COMMAND_BUTTON_CLICKED,	wxCommandEventHandler	(wxDialogWithHelpers::OnOkCancel) );
	Connect( wxID_CANCEL,	wxEVT_COMMAND_BUTTON_CLICKED,	wxCommandEventHandler	(wxDialogWithHelpers::OnOkCancel) );
	Connect(				wxEVT_CLOSE_WINDOW,				wxCloseEventHandler		(wxDialogWithHelpers::OnCloseWindow) );

	wxCommandEvent createEvent( pxEvt_OnDialogCreated );
	createEvent.SetId( GetId() );
	AddPendingEvent( createEvent );
}

void wxDialogWithHelpers::OnDialogCreated( wxCommandEvent& evt )
{
	evt.Skip();
	if( (evt.GetId() == GetId()) && !GetDialogName().IsEmpty() )
		SetName( L"Dialog:" + GetDialogName() );
}

wxString wxDialogWithHelpers::GetDialogName() const
{
	return wxEmptyString;
}

void wxDialogWithHelpers::SmartCenterFit()
{
	Fit();

	// Smart positioning logic!  If our parent window is larger than our window by some
	// good amount, then we center on that.  If not, center relative to the screen.  This
	// avoids the popup automatically eclipsing the parent window (which happens in PCSX2
	// a lot since the main window is small).

	bool centerfail = true;
	if( wxWindow* parent = GetParent() )
	{
		const wxSize parentSize( parent->GetSize() );

		if( (parentSize.x > ((int)GetSize().x * 1.75)) && (parentSize.y > ((int)GetSize().y * 1.75)) )
		{
			CenterOnParent();
			centerfail = false;
		}
	}

	if( centerfail ) CenterOnScreen();
}

// Overrides wxDialog behavior to include automatic Fit() and CenterOnParent/Screen.  The centering
// is based on a heuristic the centers against the parent window if the parent window is at least
// 75% larger than the fitted dialog.
int wxDialogWithHelpers::ShowModal()
{
	SmartCenterFit();
	return wxDialog::ShowModal();
}

// Overrides wxDialog behavior to include automatic Fit() and CenterOnParent/Screen.  The centering
// is based on a heuristic the centers against the parent window if the parent window is at least
// 75% larger than the fitted dialog.
bool wxDialogWithHelpers::Show( bool show )
{
	if( show ) SmartCenterFit();
	return wxDialog::Show( show );
}

wxStaticText& wxDialogWithHelpers::Label( const wxString& label )
{
	return *new wxStaticText( this, wxID_ANY, label, wxDefaultPosition, wxDefaultSize, wxALIGN_CENTER_VERTICAL );
}

pxStaticText& wxDialogWithHelpers::Text( const wxString& label )
{
	return *new pxStaticText( this, label );
}

pxStaticText& wxDialogWithHelpers::Heading( const wxString& label )
{
	return *new pxStaticHeading( this, label );
}

void wxDialogWithHelpers::OnCloseWindow( wxCloseEvent& evt )
{
	if( !IsModal() ) Destroy();
	evt.Skip();
}

void wxDialogWithHelpers::OnOkCancel( wxCommandEvent& evt )
{
	Close();
	evt.Skip();
}

void wxDialogWithHelpers::AddOkCancel( wxSizer &sizer, bool hasApply )
{
	wxStdDialogButtonSizer& s_buttons( *new wxStdDialogButtonSizer() );

	s_buttons.AddButton( new wxButton( this, wxID_OK ) );
	s_buttons.AddButton( new wxButton( this, wxID_CANCEL ) );

	if( hasApply )
		s_buttons.AddButton( new wxButton( this, wxID_APPLY ) );

	m_extraButtonSizer = new wxBoxSizer( wxHORIZONTAL );

	// Add the context-sensitive help button on the caption for the platforms
	// which support it (currently MSW only)
	if( m_hasContextHelp )
	{
		SetExtraStyle( wxDIALOG_EX_CONTEXTHELP );
#ifndef __WXMSW__
		*m_extraButtonSizer += new wxContextHelpButton(this) | StdButton();
#endif
	}

	// create a sizer to hold the help and ok/cancel buttons, for platforms
	// that need a custom help icon.  [fixme: help icon prolly better off somewhere else]
	wxFlexGridSizer& flex( *new wxFlexGridSizer( 2 ) );
	flex.AddGrowableCol( 0, 1 );
	flex.AddGrowableCol( 1, 15 );

	flex	+= m_extraButtonSizer	| pxAlignLeft;
	flex	+= s_buttons			| (pxExpand & pxCenter);

	sizer	+= flex	| StdExpand();

	s_buttons.Realize();
}

void wxDialogWithHelpers::AddOkCancel( wxSizer *sizer, bool hasApply )
{
	if( sizer == NULL ) sizer = GetSizer();
	pxAssume( sizer );
	AddOkCancel( *sizer, hasApply );
}

wxDialogWithHelpers& wxDialogWithHelpers::SetMinWidth( int newWidth )
{
	SetMinSize( wxSize( newWidth, GetMinHeight() ) );
	if( wxSizer* sizer = GetSizer() )
		sizer->SetMinSize( wxSize( newWidth, sizer->GetMinSize().GetHeight() ) );
	return *this;
}

// --------------------------------------------------------------------------------------
//  wxPanelWithHelpers Implementations
// --------------------------------------------------------------------------------------

IMPLEMENT_DYNAMIC_CLASS(wxPanelWithHelpers, wxPanel)

void wxPanelWithHelpers::Init()
{
}

// Creates a Static Box container for this panel.  the static box sizer becomes the default
// sizer for this panel.  If the panel already has a sizer set, then that sizer will be
// transfered to the new StaticBoxSizer (and will be the first item in it's list, retaining
// consistent and expected layout)
wxPanelWithHelpers* wxPanelWithHelpers::AddFrame( const wxString& label, wxOrientation orient )
{
	wxSizer* oldSizer = GetSizer();

	SetSizer( new wxStaticBoxSizer( orient, this, label ), false );
	Init();

	if( oldSizer )
		*this += oldSizer | pxExpand;

	return this;
}

wxStaticText& wxPanelWithHelpers::Label( const wxString& label )
{
	return *new wxStaticText( this, wxID_ANY, label );
}

pxStaticText& wxPanelWithHelpers::Text( const wxString& label )
{
	return *new pxStaticText( this, label );
}

pxStaticText& wxPanelWithHelpers::Heading( const wxString& label )
{
	return *new pxStaticHeading( this, label );
}

wxPanelWithHelpers::wxPanelWithHelpers( wxWindow* parent, wxOrientation orient, const wxString& staticBoxLabel )
	: wxPanel( parent )
{
	SetSizer( new wxStaticBoxSizer( orient, this, staticBoxLabel ) );
	Init();
}

wxPanelWithHelpers::wxPanelWithHelpers( wxWindow* parent, wxOrientation orient )
	: wxPanel( parent )
{
	SetSizer( new wxBoxSizer( orient ) );
	Init();
}

wxPanelWithHelpers::wxPanelWithHelpers( wxWindow* parent )
	: wxPanel( parent )
{
	Init();
}

wxPanelWithHelpers::wxPanelWithHelpers( wxWindow* parent, const wxPoint& pos, const wxSize& size )
	: wxPanel( parent, wxID_ANY, pos, size )
{
	Init();
}

wxPanelWithHelpers& wxPanelWithHelpers::SetMinWidth( int newWidth )
{
	SetMinSize( wxSize( newWidth, GetMinHeight() ) );
	if( wxSizer* sizer = GetSizer() )
		sizer->SetMinSize( wxSize( newWidth, sizer->GetMinSize().GetHeight() ) );
	return *this;
}
