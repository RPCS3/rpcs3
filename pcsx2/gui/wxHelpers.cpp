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
	// ------------------------------------------------------------------------
	// FlagsAccessors - Provides read-write copies of standard sizer flags for our interface.
	// These standard definitions provide a consistent and pretty interface for our GUI.
	// Without them things look compacted, misaligned, and yucky!
	//
	// Implementation Note: Accessors are all provisioned as dynamic (realtime) sizer calculations.
	// I've preferred this over cstatic const variables on the premise that spacing logic could
	// in the future become a dynamic value (currently it is affixed to 6 for most items).
	//
	namespace SizerFlags
	{
		wxSizerFlags StdSpace()
		{
			return wxSizerFlags().Border( wxALL, 6 );
		}

		wxSizerFlags StdCenter()
		{
			return wxSizerFlags().Align( wxALIGN_CENTER ).DoubleBorder();
		}

		wxSizerFlags StdExpand()
		{
			return StdSpace().Expand();
		}

		wxSizerFlags StdGroupie()
		{
			// Groups look better with a slightly smaller margin than standard.
			// (basically this accounts for the group's frame)
			return wxSizerFlags().Border( wxLEFT | wxBOTTOM | wxRIGHT, 4 ).Expand();
		}

		// This force-aligns the std button sizer to the right, where (at least) us win32 platform
		// users always expect it to be.  Most likely Mac platforms expect it on the left side
		// just because it's *not* where win32 sticks it.  Too bad!
		wxSizerFlags StdButton()
		{
			return wxSizerFlags().Align( wxALIGN_RIGHT ).Border();
		}

		wxSizerFlags Checkbox()
		{
			return StdExpand();
		}
	};

	// ------------------------------------------------------------------------
	// Creates a new checkbox and adds it to the specified sizer/parent combo.
	// Uses the default spacer setting for adding checkboxes.
	//
	wxCheckBox& AddCheckBoxTo( wxWindow* parent, wxSizer& sizer, const wxString& label, wxWindowID id )
	{
		wxCheckBox* retval = new wxCheckBox( parent, id, label );
		sizer.Add( retval, SizerFlags::Checkbox() );
		return *retval;
	}

	// ------------------------------------------------------------------------
	// Creates a new Radio Button and adds it to the specified sizer/parent combo.
	// The first item in a group should pass True for the isFisrt parameter.
	// Uses the default spacer setting for checkboxes.
	//
	wxRadioButton& AddRadioButtonTo( wxWindow* parent, wxSizer& sizer, const wxString& label, wxWindowID id, bool isFirst )
	{
		wxRadioButton* retval = new wxRadioButton( parent, id, label, wxDefaultPosition, wxDefaultSize, isFirst ? wxRB_GROUP : 0 );
		sizer.Add( retval, SizerFlags::Checkbox() );
		return *retval;
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
	wxStaticText& AddStaticTextTo(wxWindow* parent, wxSizer& sizer, const wxString& label, int size, int alignFlags )
	{
		// No reason to ever have AutoResize enabled, quite frankly.  It just causes layout and centering problems.
		alignFlags |= wxST_NO_AUTORESIZE;
        wxStaticText *temp = new wxStaticText(parent, wxID_ANY, label, wxDefaultPosition, wxDefaultSize, alignFlags );
        if (size > 0) temp->Wrap(size);

        sizer.Add(temp, SizerFlags::StdSpace().Align( alignFlags & wxALIGN_MASK ) );
        return *temp;
    }

	// ------------------------------------------------------------------------
	// Launches the specified file according to its mime type
	//
	void Launch( const wxString& filename )
	{
		if( !wxLaunchDefaultBrowser( filename ) )
		{
		}
	}

	void Launch(const char *filename)
	{
		Launch( wxString::FromAscii(filename) );
	}

	// ------------------------------------------------------------------------
	// Launches a file explorer window on the specified path.  If the given path is not
	// a qualified URI (with a prefix:// ), file:// is automatically prepended.
	//
	void Explore( const wxString& path )
	{
		if( wxLaunchDefaultBrowser(
			!path.Contains( L"://") ? L"file://" + path : path )
		)
		{
			// WARN_LOG
		}
	}

	void Explore(const char *path)
	{
		Explore( wxString::FromAscii(path) );
	}
}

//////////////////////////////////////////////////////////////////////////////////////////
//
wxDialogWithHelpers::wxDialogWithHelpers( wxWindow* parent, int id,  const wxString& title, bool hasContextHelp, const wxPoint& pos, const wxSize& size ) :
	wxDialog( parent, id, title, pos, size , wxDEFAULT_DIALOG_STYLE), //, (wxCAPTION | wxMAXIMIZE | wxCLOSE_BOX | wxRESIZE_BORDER) ),	// flags for resizable dialogs, currently unused.
	m_hasContextHelp( hasContextHelp )
{
	if( hasContextHelp )
		wxHelpProvider::Set( new wxSimpleHelpProvider() );

	// Note: currently the Close (X) button doesn't appear to work in dialogs.  Docs indicate
	// that it should, so I presume the problem is in wxWidgets and that (hopefully!) an updated
	// version will fix it later.  I tried to fix it using a manual Connect but it didn't do
	// any good.
}

wxCheckBox& wxDialogWithHelpers::AddCheckBox( wxSizer& sizer, const wxString& label, wxWindowID id )
{
	return wxHelpers::AddCheckBoxTo( this, sizer, label, id );
}

wxStaticText& wxDialogWithHelpers::AddStaticText(wxSizer& sizer, const wxString& label, int size )
{
	return wxHelpers::AddStaticTextTo( this, sizer, label, size );
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
		buttonSizer->Add( new wxContextHelpButton(this), wxHelpers::SizerFlags::StdButton().Align( wxALIGN_LEFT ) );
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
	buttonSizer->Add( &s_buttons, wxHelpers::SizerFlags::StdButton() );
}

//////////////////////////////////////////////////////////////////////////////////////////
//
wxPanelWithHelpers::wxPanelWithHelpers( wxWindow* parent, int id, const wxPoint& pos, const wxSize& size ) :
	wxPanel( parent, id, pos, size )
,	m_StartNewRadioGroup( true )
{
}

wxCheckBox& wxPanelWithHelpers::AddCheckBox( wxSizer& sizer, const wxString& label, wxWindowID id )
{
	return wxHelpers::AddCheckBoxTo( this, sizer, label, id );
}

wxStaticText& wxPanelWithHelpers::AddStaticText(wxSizer& sizer, const wxString& label, int size, int alignFlags )
{
	return wxHelpers::AddStaticTextTo( this, sizer, label, size, alignFlags );
}

wxRadioButton& wxPanelWithHelpers::AddRadioButton( wxSizer& sizer, const wxString& label, const wxString& subtext, wxWindowID id )
{
	wxRadioButton& retval = wxHelpers::AddRadioButtonTo( this, sizer, label, id, m_StartNewRadioGroup );
	m_StartNewRadioGroup = false;

	if( !subtext.IsEmpty() )
	{
		sizer.Add( new wxStaticText( this, wxID_ANY, subtext ), wxSizerFlags().Border( wxLEFT, 25 ) );
		sizer.AddSpacer( 4 );
	}
	return retval;
}

