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
#include "wxHelpers.h"

#include <wx/cshelp.h>

#if wxUSE_TOOLTIPS
#   include <wx/tooltip.h>
#endif

// Retrieves the area of the screen, which can be used to enforce a valid zone for
// window positioning. (top/left coords are almost always (0,0) and bottom/right
// is the resolution of the desktop).
wxRect wxGetDisplayArea()
{
	return wxRect( wxPoint(), wxGetDisplaySize() );
}

// Returns FALSE if the window position is considered invalid, which means that it's title
// bar is most likely not easily grabble.  Such a window should be moved to a valid or
// default position.
bool pxIsValidWindowPosition( const wxWindow& window, const wxPoint& windowPos )
{
	// The height of the window is only revlevant to the height of a title bar, which is
	// all we need visible for the user to be able to drag the window into view.  But
	// there's no way to get that info from wx, so we'll just have to guesstimate...

	wxSize sizeMatters( window.GetSize().GetWidth(), 32 );		// if some gui has 32 pixels of undraggable title bar, the user deserves to suffer.
	return wxGetDisplayArea().Contains( wxRect( windowPos, sizeMatters ) );
}

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

		// A good sizer flags setting for top-level static boxes or top-level picture boxes.
		// Gives a generous border to the left, right, and bottom.  Top border can be configured
		// manually by using a spacer.
		wxSizerFlags TopLevelBox()
		{
			return wxSizerFlags().Border( wxLEFT | wxBOTTOM | wxRIGHT, 6 ).Expand();
		}

		// Flags intended for use on grouped StaticBox controls.  These flags are ideal for
		// StaticBoxes that are part of sub-panels or children of other static boxes, but may
		// not be best for parent StaticBoxes on dialogs (left and right borders feel a bit
		// "tight").
		wxSizerFlags SubGroup()
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

	// This method is used internally to create multi line checkboxes and radio buttons.
	void _appendStaticSubtext( wxWindow* parent, wxSizer& sizer, const wxString& subtext, const wxString& tooltip, int wrapLen )
	{
		static const int Indentation = 23;

		if( subtext.IsEmpty() ) return;

		wxStaticText* joe = new wxStaticText( parent, wxID_ANY, subtext );
		if( wrapLen > 0 ) joe->Wrap( wrapLen-Indentation );
		if( !tooltip.IsEmpty() )
			joe->SetToolTip( tooltip );
		sizer.Add( joe, wxSizerFlags().Border( wxLEFT, Indentation ) );
		sizer.AddSpacer( 9 );
	}


	// ------------------------------------------------------------------------
	// Creates a new checkbox and adds it to the specified sizer/parent combo, with optional tooltip.
	// Uses the default spacer setting for adding checkboxes, and the tooltip (if specified) is applied
	// to both the checkbox and it's static subtext (if present).
	//
	wxCheckBox& AddCheckBoxTo( wxWindow* parent, wxSizer& sizer, const wxString& label, const wxString& subtext, const wxString& tooltip, int wrapLen )
	{
		wxCheckBox* retval = new wxCheckBox( parent, wxID_ANY, label );
		sizer.Add( retval, SizerFlags::Checkbox() );

		if( !tooltip.IsEmpty() )
			retval->SetToolTip( tooltip );

		_appendStaticSubtext( parent, sizer, subtext, tooltip, wrapLen );

		return *retval;
	}

	// ------------------------------------------------------------------------
	// Creates a new Radio button and adds it to the specified sizer/parent combo, with optional tooltip.
	// Uses the default spacer setting for adding checkboxes, and the tooltip (if specified) is applied
	// to both the radio button and it's static subtext (if present).
	//
	// The first item in a group should pass True for the isFirst parameter.
	//
	wxRadioButton& AddRadioButtonTo( wxWindow* parent, wxSizer& sizer, const wxString& label, const wxString& subtext, const wxString& tooltip, int wrapLen, bool isFirst )
	{
		wxRadioButton* retval = new wxRadioButton( parent, wxID_ANY, label, wxDefaultPosition, wxDefaultSize, isFirst ? wxRB_GROUP : 0 );
		sizer.Add( retval, SizerFlags::Checkbox() );

		if( !tooltip.IsEmpty() )
			retval->SetToolTip( tooltip );

		_appendStaticSubtext( parent, sizer, subtext, tooltip, wrapLen );

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
	wxStaticText& AddStaticTextTo(wxWindow* parent, wxSizer& sizer, const wxString& label, int alignFlags, int size )
	{
		// No reason to ever have AutoResize enabled, quite frankly.  It just causes layout and centering problems.
		alignFlags |= wxST_NO_AUTORESIZE;
        wxStaticText& temp( *new wxStaticText(parent, wxID_ANY, label, wxDefaultPosition, wxDefaultSize, alignFlags ) );
        if( size > 0 ) temp.Wrap( size );

        sizer.Add( &temp, SizerFlags::StdSpace().Align( alignFlags & wxALIGN_MASK ) );
        return temp;
    }

	wxStaticText& InsertStaticTextAt(wxWindow* parent, wxSizer& sizer, int position, const wxString& label, int alignFlags, int size )
	{
		// No reason to ever have AutoResize enabled, quite frankly.  It just causes layout and centering problems.
		alignFlags |= wxST_NO_AUTORESIZE;
		wxStaticText& temp( *new wxStaticText(parent, wxID_ANY, label, wxDefaultPosition, wxDefaultSize, alignFlags ) );
		if( size > 0 ) temp.Wrap( size );

		sizer.Insert( position, &temp, SizerFlags::StdSpace().Align( alignFlags & wxALIGN_MASK ) );
		return temp;
	}

	// ------------------------------------------------------------------------
	// Launches the specified file according to its mime type
	//
	void Launch( const wxString& filename )
	{
		wxLaunchDefaultBrowser( filename );
	}

	void Launch(const char *filename)
	{
		Launch( wxString::FromAscii(filename) );
	}

	// ------------------------------------------------------------------------
	// Launches a file explorer window on the specified path.  If the given path is not
	// a qualified URI (with a prefix:// ), file:// is automatically prepended.  This
	// bypasses wxWidgets internal filename checking, which can end up launching things
	// through browser more often than desired.
	//
	void Explore( const wxString& path )
	{
		wxLaunchDefaultBrowser( !path.Contains( L"://") ? L"file://" + path : path );
	}

	void Explore(const char *path)
	{
		Explore( wxString::FromAscii(path) );
	}
}

// ----------------------------------------------------------------------------
void pxTextWrapperBase::Wrap( const wxWindow *win, const wxString& text, int widthMax )
{
    const wxChar *lastSpace = NULL;
    wxString line;
    line.Alloc( widthMax+12 );

    const wxChar *lineStart = text.c_str();
    for ( const wxChar *p = lineStart; ; p++ )
    {
        if ( IsStartOfNewLine() )
        {
            OnNewLine();

            lastSpace = NULL;
            line.clear();
            lineStart = p;
        }

        if ( *p == L'\n' || *p == L'\0' )
        {
            DoOutputLine(line);

            if ( *p == L'\0' )
                break;
        }
        else // not EOL
        {
            if ( *p == L' ' )
                lastSpace = p;

            line += *p;

            if ( widthMax >= 0 && lastSpace )
            {
                int width;
                win->GetTextExtent(line, &width, NULL);

                if ( width > widthMax )
                {
                    // remove the last word from this line
                    line.erase(lastSpace - lineStart, p + 1 - lineStart);
                    DoOutputLine(line);

                    // go back to the last word of this line which we didn't
                    // output yet
                    p = lastSpace;
                }
            }
            //else: no wrapping at all or impossible to wrap
        }
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

// ------------------------------------------------------------------------
// Creates a new checkbox and adds it to the specified sizer/parent combo, with optional tooltip.
// Uses the default spacer setting for adding checkboxes, and the tooltip (if specified) is applied
// to both the checkbox and it's static subtext (if present).
//
wxCheckBox& wxDialogWithHelpers::AddCheckBox( wxSizer& sizer, const wxString& label, const wxString& subtext, const wxString& tooltip )
{
	return wxHelpers::AddCheckBoxTo( this, sizer, label, subtext, tooltip);
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
wxPanelWithHelpers::wxPanelWithHelpers( wxWindow* parent, int idealWidth ) :
	wxPanel( parent, wxID_ANY )
,	m_idealWidth( idealWidth )
,	m_StartNewRadioGroup( true )
{
}

wxPanelWithHelpers::wxPanelWithHelpers( wxWindow* parent, const wxPoint& pos, const wxSize& size ) :
	wxPanel( parent, wxID_ANY, pos, size )
,	m_idealWidth( wxDefaultCoord )
,	m_StartNewRadioGroup( true )
{
}


// ------------------------------------------------------------------------
// Creates a new checkbox and adds it to the specified sizer, with optional tooltip.  Uses the default
// spacer setting for adding checkboxes, and the tooltip (if specified) is applied to both the checkbox
// and it's static subtext (if present).
//
// Static subtext, if specified, is displayed below the checkbox and is indented accordingly.
//
wxCheckBox& wxPanelWithHelpers::AddCheckBox( wxSizer& sizer, const wxString& label, const wxString& subtext, const wxString& tooltip )
{
	return wxHelpers::AddCheckBoxTo( this, sizer, label, subtext, tooltip, GetIdealWidth()-8 );
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
wxRadioButton& wxPanelWithHelpers::AddRadioButton( wxSizer& sizer, const wxString& label, const wxString& subtext, const wxString& tooltip )
{
	wxRadioButton& result = wxHelpers::AddRadioButtonTo( this, sizer, label, subtext, tooltip, GetIdealWidth()-8, m_StartNewRadioGroup );
	m_StartNewRadioGroup = false;
	return result;
}
