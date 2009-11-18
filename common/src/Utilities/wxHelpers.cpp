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
#include "HashMap.h"
#include "wxHelpers.h"
#include "pxStaticText.h"

#include <wx/cshelp.h>
#include <wx/tooltip.h>


// ------------------------------------------------------------------------
// Creates a static text box that generally "makes sense" in a free-flowing layout.  Specifically, this
// ensures that that auto resizing is disabled, and that the sizer flags match the alignment specified
// for the textbox.
//
// Parameters:
//  Size - allows forcing the control to wrap text at a specific pre-defined pixel width;
//      or specify zero to let wxWidgets layout the text as it deems appropriate (recommended)
//
// alignFlags - Either wxALIGN_LEFT, RIGHT, or CENTRE.  All other wxStaticText flags are ignored
//      or overridden.  [default is left alignment]
//
pxStaticText& wxHelpers::AddStaticTextTo(wxWindow* parent, wxSizer& sizer, const wxString& label, int alignFlags )
{
    pxStaticText& temp( *new pxStaticText( parent, label, alignFlags ) );
    temp.AddTo( sizer );
    return temp;
}

pxStaticText& wxHelpers::InsertStaticTextAt(wxWindow* parent, wxSizer& sizer, int position, const wxString& label, int alignFlags )
{
	pxStaticText& temp( *new pxStaticText(parent, label, alignFlags ) );
	temp.InsertAt( sizer, position );
	return temp;
}

// =====================================================================================================
//  wxDialogWithHelpers Class Implementations
// =====================================================================================================

HashTools::HashMap< wxWindowID, int > m_DialogIdents( 0, wxID_ANY );

bool pxDialogExists( wxWindowID id )
{
	int dest = 0;
	m_DialogIdents.TryGetValue( id, dest );
	return (dest > 0);
}

// --------------------------------------------------------------------------------------
//  wxDialogWithHelpers Implementation
// --------------------------------------------------------------------------------------
IMPLEMENT_DYNAMIC_CLASS(wxDialogWithHelpers, wxDialog)

wxDialogWithHelpers::wxDialogWithHelpers()
{
	m_idealWidth		= wxDefaultCoord;
	m_hasContextHelp	= false;
}

wxDialogWithHelpers::wxDialogWithHelpers( wxWindow* parent, int id,  const wxString& title, bool hasContextHelp, const wxPoint& pos, const wxSize& size )
	: wxDialog( parent, id, title, pos, size , wxDEFAULT_DIALOG_STYLE) //, (wxCAPTION | wxMAXIMIZE | wxCLOSE_BOX | wxRESIZE_BORDER) ),	// flags for resizable dialogs, currently unused.
{
	++m_DialogIdents[GetId()];

	m_idealWidth = wxDefaultCoord;

	m_hasContextHelp = hasContextHelp;
	if( m_hasContextHelp )
		delete wxHelpProvider::Set( new wxSimpleHelpProvider() );

	// GTK/Linux Note: currently the Close (X) button doesn't appear to work in dialogs.  Docs
	// indicate that it should, so I presume the problem is in wxWidgets and that (hopefully!)
	// an updated version will fix it later.  I tried to fix it using a manual Connect but it
	// didn't do any good.  (problem could also be my Co-Linux / x-window manager)
	
	//Connect( wxEVT_ACTIVATE, wxActivateEventHandler(wxDialogWithHelpers::OnActivate) );
}

wxDialogWithHelpers::~wxDialogWithHelpers() throw()
{
	--m_DialogIdents[GetId()];
	pxAssert( m_DialogIdents[GetId()] >= 0 );
}

void wxDialogWithHelpers::OnActivate(wxActivateEvent& evt)
{
	//evt.Skip();
}

wxStaticText& wxDialogWithHelpers::AddStaticText(wxSizer& sizer, const wxString& label, int alignFlags )
{
	return wxHelpers::AddStaticTextTo( this, sizer, label, alignFlags );
}

void wxDialogWithHelpers::AddOkCancel( wxSizer &sizer, bool hasApply )
{
	wxSizer* buttonSizer = &sizer;
	if( m_hasContextHelp )
	{
		// Add the context-sensitive help button on the caption for the platforms
		// which support it (currently MSW only)
		SetExtraStyle( wxDIALOG_EX_CONTEXTHELP );

#ifndef __WXMSW__
		// create a sizer to hold the help and ok/cancel buttons, for platforms
		// that need a custom help icon.  [fixme: help icon prolly better off somewhere else]
		buttonSizer = new wxBoxSizer( wxHORIZONTAL );
		buttonSizer->Add( new wxContextHelpButton(this), pxSizerFlags::StdButton().Align( wxALIGN_LEFT ) );
		sizer.Add( buttonSizer, wxSizerFlags().Center() );
#endif
	}

	wxStdDialogButtonSizer& s_buttons = *new wxStdDialogButtonSizer();

	s_buttons.AddButton( new wxButton( this, wxID_OK ) );
	s_buttons.AddButton( new wxButton( this, wxID_CANCEL ) );

	if( hasApply )
	{
		s_buttons.AddButton( new wxButton( this, wxID_APPLY ) );
	}

	s_buttons.Realize();
	buttonSizer->Add( &s_buttons, pxSizerFlags::StdButton() );
}

// --------------------------------------------------------------------------------------
//  wxPanelWithHelpers Implementations
// --------------------------------------------------------------------------------------

IMPLEMENT_DYNAMIC_CLASS(wxPanelWithHelpers, wxPanel)

void wxPanelWithHelpers::Init()
{
	m_idealWidth = wxDefaultCoord;

	// Find the first parent with a fixed width:
	wxWindow* millrun = this->GetParent();
	while( (m_idealWidth == wxDefaultCoord) && millrun != NULL )
	{
		if( wxIsKindOf( millrun, wxPanelWithHelpers ) )
			m_idealWidth = ((wxPanelWithHelpers*)millrun)->GetIdealWidth();

		else if( wxIsKindOf( millrun, wxDialogWithHelpers ) )
			m_idealWidth = ((wxDialogWithHelpers*)millrun)->GetIdealWidth();

		millrun = millrun->GetParent();
	}

	if( m_idealWidth == wxDefaultCoord || GetParent() == NULL )
		return;

	// Check for a StaticBox -- if we belong to one then we'll want to "downgrade" the
	// inherited textbox width automatically.

	wxSizer* guess = GetSizer();
	if( guess == NULL ) guess = GetParent()->GetSizer();
	if( guess == NULL ) guess = GetParent()->GetContainingSizer();
	
	if( guess != NULL )
	{
		int top=0, others=0;
		if( wxIsKindOf( guess, wxStaticBoxSizer ) )
			((wxStaticBoxSizer*)guess)->GetStaticBox()->GetBordersForSizer( &top, &others );

		m_idealWidth -= (others*2);
		m_idealWidth -= 2;				// generic padding compensation (no exact sciences to be found here)
	}
}

wxPanelWithHelpers* wxPanelWithHelpers::AddStaticBox( const wxString& label, wxOrientation orient )
{
	wxSizer* oldSizer = GetSizer();

	SetSizer( new wxStaticBoxSizer( orient, this, label ), false );
	Init();

	if( oldSizer )
		GetSizer()->Add( oldSizer );
	
	return this;
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

// ------------------------------------------------------------------------
// Creates a static text box that generally "makes sense" in a free-flowing layout.  Specifically, this
// ensures that that auto resizing is disabled, and that the sizer flags match the alignment specified
// for the textbox.
//
// Parameters:
//  Size - allows forcing the control to wrap text at a specific pre-defined pixel width;
//      or specify zero to let wxWidgets layout the text as it deems appropriate (recommended)
//
// alignFlags - Either wxALIGN_LEFT, RIGHT, or CENTRE.  All other wxStaticText flags are ignored
//      or overridden.  [default is left alignment]
//
pxStaticText& wxPanelWithHelpers::AddStaticText(wxSizer& sizer, const wxString& label, int alignFlags )
{
	return wxHelpers::AddStaticTextTo( this, sizer, label, alignFlags );
}
