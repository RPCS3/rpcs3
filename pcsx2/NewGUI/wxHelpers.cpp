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
#include "wxHelpers.h"

#include <wx/cshelp.h>

#if wxUSE_TOOLTIPS
#   include <wx/tooltip.h>
#endif

namespace wxHelpers
{
	wxSizerFlags stdCenteredFlags( wxSizerFlags().Align(wxALIGN_CENTER).DoubleBorder() );
	wxSizerFlags stdSpacingFlags( wxSizerFlags().Border( wxALL, 6 ) );
	wxSizerFlags stdButtonSizerFlags( wxSizerFlags().Align(wxALIGN_RIGHT).Border() );
	wxSizerFlags CheckboxFlags( wxSizerFlags().Border( wxALL, 6 ).Expand() );
	
	wxCheckBox& AddCheckBoxTo( wxWindow* parent, wxBoxSizer& sizer, const wxString& label, wxWindowID id )
	{
		wxCheckBox* retval = new wxCheckBox( parent, id, label );
		sizer.Add( retval, CheckboxFlags );
		return *retval;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////
//
wxDialogWithHelpers::wxDialogWithHelpers( wxWindow* parent, int id,  const wxString& title, bool hasContextHelp, const wxPoint& pos, const wxSize& size ) :
	wxDialog( parent, id, title, pos, size ),
	m_hasContextHelp( hasContextHelp )
{
	if( hasContextHelp )
		wxHelpProvider::Set( new wxSimpleHelpProvider() );
}

wxCheckBox& wxDialogWithHelpers::AddCheckBox( wxBoxSizer& sizer, const wxString& label, wxWindowID id )
{
	return wxHelpers::AddCheckBoxTo( this, sizer, label, id );
}

void wxDialogWithHelpers::AddOkCancel( wxBoxSizer &sizer )
{
	wxBoxSizer* buttonSizer = &sizer;
	if( m_hasContextHelp )
	{
		// Add the context-sensitive help button on the caption for the platforms
		// which support it (currently MSW only)
		SetExtraStyle( wxDIALOG_EX_CONTEXTHELP );

#ifndef __WXMSW__
		// create a sizer to hold the help and ok/cancel buttons.
		buttonSizer = new wxBoxSizer( wxHORIZONTAL );
		buttonSizer->Add( new wxContextHelpButton(this), wxHelpers::stdButtonSizerFlags.Align( wxALIGN_LEFT ) );
#endif
	}
	buttonSizer->Add( CreateStdDialogButtonSizer( wxOK | wxCANCEL ), wxHelpers::stdButtonSizerFlags );
}

