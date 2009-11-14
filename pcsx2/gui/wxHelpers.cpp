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
#include "Utilities/HashMap.h"
#include "wxHelpers.h"

#include <wx/cshelp.h>

#if wxUSE_TOOLTIPS
#   include <wx/tooltip.h>
#endif


// This method is used internally to create multi line checkboxes and radio buttons.
static wxStaticText* _appendStaticSubtext( wxWindow* parent, wxSizer& sizer, const wxString& subtext, const wxString& tooltip, int wrapLen )
{
	static const int Indentation = 23;

	if( subtext.IsEmpty() ) return NULL;

	wxStaticText* joe = new wxStaticText( parent, wxID_ANY, subtext );
	if( wrapLen > 0 ) joe->Wrap( wrapLen-Indentation );
	if( !tooltip.IsEmpty() )
		pxSetToolTip( joe, tooltip );
	sizer.Add( joe, wxSizerFlags().Border( wxLEFT, Indentation ) );
	sizer.AddSpacer( 9 );

	return joe;
}

pxCheckBox::pxCheckBox(wxPanelWithHelpers* parent, const wxString& label, const wxString& subtext)
	: wxPanel( parent )
{
	m_idealWidth	= parent->GetIdealWidth() - 24;
	Init( label, subtext );
}

pxCheckBox::pxCheckBox(wxDialogWithHelpers* parent, const wxString& label, const wxString& subtext)
	: wxPanel( parent )
{
	m_idealWidth	= parent->GetIdealWidth() - 24;
	Init( label, subtext );
}

void pxCheckBox::Init(const wxString& label, const wxString& subtext)
{
	m_checkbox		= new wxCheckBox( this, wxID_ANY, label );

	wxBoxSizer&	mySizer( *new wxBoxSizer(wxVERTICAL) );
	mySizer.Add( m_checkbox, pxSizerFlags::StdExpand() );
	m_subtext = _appendStaticSubtext( this, mySizer, subtext, wxEmptyString, m_idealWidth );

	SetSizer( &mySizer );
}

// applies the tooltip to both both the checkbox and it's static subtext (if present), and
// performs word wrapping on platforms that need it (eg mswindows).
pxCheckBox& pxCheckBox::SetToolTip( const wxString& tip )
{
	const wxString wrapped( pxFormatToolTipText(this, tip) );
	pxSetToolTip( m_checkbox, wrapped );
	pxSetToolTip( m_subtext, wrapped );
	return *this;
}

pxCheckBox& pxCheckBox::SetValue( bool val )
{
	m_checkbox->SetValue( val );
	return *this;
}

bool pxCheckBox::GetValue() const
{
	return m_checkbox->GetValue();
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
wxStaticText& wxHelpers::AddStaticTextTo(wxWindow* parent, wxSizer& sizer, const wxString& label, int alignFlags, int size )
{
	// No reason to ever have AutoResize enabled, quite frankly.  It just causes layout and centering problems.
	alignFlags |= wxST_NO_AUTORESIZE;
    wxStaticText& temp( *new wxStaticText(parent, wxID_ANY, label, wxDefaultPosition, wxDefaultSize, alignFlags ) );
    if( size > 0 ) temp.Wrap( size );

    sizer.Add( &temp, pxSizerFlags::StdSpace().Align( alignFlags & wxALIGN_MASK ) );
    return temp;
}

wxStaticText& wxHelpers::InsertStaticTextAt(wxWindow* parent, wxSizer& sizer, int position, const wxString& label, int alignFlags, int size )
{
	// No reason to ever have AutoResize enabled, quite frankly.  It just causes layout and centering problems.
	alignFlags |= wxST_NO_AUTORESIZE;
	wxStaticText& temp( *new wxStaticText(parent, wxID_ANY, label, wxDefaultPosition, wxDefaultSize, alignFlags ) );
	if( size > 0 ) temp.Wrap( size );

	sizer.Insert( position, &temp, pxSizerFlags::StdSpace().Align( alignFlags & wxALIGN_MASK ) );
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

wxDialogWithHelpers::wxDialogWithHelpers( wxWindow* parent, int id,  const wxString& title, bool hasContextHelp, const wxPoint& pos, const wxSize& size )
	: wxDialog( parent, id, title, pos, size , wxDEFAULT_DIALOG_STYLE) //, (wxCAPTION | wxMAXIMIZE | wxCLOSE_BOX | wxRESIZE_BORDER) ),	// flags for resizable dialogs, currently unused.
{
	++m_DialogIdents[GetId()];

	m_idealWidth = wxDefaultCoord;

	m_hasContextHelp = hasContextHelp;
	if( m_hasContextHelp )
		delete wxHelpProvider::Set( new wxSimpleHelpProvider() );

	// Note: currently the Close (X) button doesn't appear to work in dialogs.  Docs indicate
	// that it should, so I presume the problem is in wxWidgets and that (hopefully!) an updated
	// version will fix it later.  I tried to fix it using a manual Connect but it didn't do
	// any good.
}

wxDialogWithHelpers::~wxDialogWithHelpers() throw()
{
	--m_DialogIdents[GetId()];
	pxAssert( m_DialogIdents[GetId()] >= 0 );
}

wxStaticText& wxDialogWithHelpers::AddStaticText(wxSizer& sizer, const wxString& label, int size, int alignFlags )
{
	return wxHelpers::AddStaticTextTo( this, sizer, label, size, alignFlags );
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
		buttonSizer->Add( new wxContextHelpButton(this), wxHelpers::pxSizerFlags::StdButton().Align( wxALIGN_LEFT ) );
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

//////////////////////////////////////////////////////////////////////////////////////////
//
wxPanelWithHelpers::wxPanelWithHelpers( wxWindow* parent, int idealWidth ) :
	wxPanel( parent )
{
	m_idealWidth			= idealWidth;
	m_StartNewRadioGroup	= true;
}

wxPanelWithHelpers::wxPanelWithHelpers( wxWindow* parent, const wxPoint& pos, const wxSize& size ) :
	wxPanel( parent, wxID_ANY, pos, size )
{
	m_idealWidth			= wxDefaultCoord;
	m_StartNewRadioGroup	= true;
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
wxStaticText& wxPanelWithHelpers::AddStaticText(wxSizer& sizer, const wxString& label, int alignFlags, int size )
{
	return wxHelpers::AddStaticTextTo( this, sizer, label, alignFlags, (size > 0) ? size : GetIdealWidth()-24 );
}

// ------------------------------------------------------------------------
// Creates a new Radio button and adds it to the specified sizer, with optional tooltip.  Uses the
// default spacer setting for adding checkboxes, and the tooltip (if specified) is applied to both
// the radio button and it's static subtext (if present).
//
// Static subtext, if specified, is displayed below the checkbox and is indented accordingly.
// The first item in a group should pass True for the isFirst parameter.
//
/*wxRadioButton& wxPanelWithHelpers::AddRadioButton( wxSizer& sizer, const wxString& label, const wxString& subtext, const wxString& tooltip )
{
	wxRadioButton& result = wxHelpers::AddRadioButtonTo( this, sizer, label, subtext, tooltip, GetIdealWidth()-8, m_StartNewRadioGroup );
	m_StartNewRadioGroup = false;
	return result;
}*/
