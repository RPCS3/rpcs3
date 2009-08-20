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

#pragma once

#include <wx/wx.h>
#include <wx/filepicker.h>

#include "Utilities/wxGuiTools.h"

namespace wxHelpers
{
	extern wxCheckBox&		AddCheckBoxTo( wxWindow* parent, wxSizer& sizer, const wxString& label, const wxString& subtext=wxEmptyString, const wxString& tooltip=wxEmptyString, int wrapLen=wxDefaultCoord );
	extern wxRadioButton&	AddRadioButtonTo( wxWindow* parent, wxSizer& sizer, const wxString& label, const wxString& subtext=wxEmptyString, const wxString& tooltip=wxEmptyString, int wrapLen=wxDefaultCoord, bool isFirst = false );
	extern wxStaticText&	AddStaticTextTo(wxWindow* parent, wxSizer& sizer, const wxString& label, int alignFlags=wxALIGN_CENTRE, int wrapLen=wxDefaultCoord );
	extern wxStaticText&	InsertStaticTextAt(wxWindow* parent, wxSizer& sizer, int position, const wxString& label, int alignFlags=wxALIGN_CENTRE, int wrapLen=wxDefaultCoord );

	extern void Explore( const wxString& path );
	extern void Explore( const char *path );

	extern void Launch( const wxString& path );
	extern void Launch( const char *path );


	namespace SizerFlags
	{
		extern wxSizerFlags StdSpace();
		extern wxSizerFlags StdCenter();
		extern wxSizerFlags StdExpand();
		extern wxSizerFlags TopLevelBox();
		extern wxSizerFlags SubGroup();
		extern wxSizerFlags StdButton();
		extern wxSizerFlags Checkbox();
	};
}

//////////////////////////////////////////////////////////////////////////////////////////
//
class wxDialogWithHelpers : public wxDialog
{
protected:
	bool m_hasContextHelp;

public:
	wxDialogWithHelpers(wxWindow* parent, int id, const wxString& title, bool hasContextHelp, const wxPoint& pos=wxDefaultPosition, const wxSize& size=wxDefaultSize );

	wxCheckBox&		AddCheckBox( wxSizer& sizer, const wxString& label, const wxString& subtext=wxEmptyString, const wxString& tooltip=wxEmptyString );
	wxStaticText&	AddStaticText(wxSizer& sizer, const wxString& label, int alignFlags=wxALIGN_CENTRE, int size=wxDefaultCoord );
    void AddOkCancel( wxSizer& sizer, bool hasApply=false );

protected:
};

//////////////////////////////////////////////////////////////////////////////////////////
//
class wxPanelWithHelpers : public wxPanel
{
protected:
	const int m_idealWidth;
	bool m_StartNewRadioGroup;

public:
	wxPanelWithHelpers( wxWindow* parent, int idealWidth=wxDefaultCoord );
	wxPanelWithHelpers( wxWindow* parent, const wxPoint& pos, const wxSize& size=wxDefaultSize );

	wxCheckBox&		AddCheckBox( wxSizer& sizer, const wxString& label, const wxString& subtext=wxEmptyString, const wxString& tooltip=wxEmptyString );
	wxRadioButton&	AddRadioButton( wxSizer& sizer, const wxString& label, const wxString& subtext=wxEmptyString, const wxString& tooltip=wxEmptyString );
	wxStaticText&	AddStaticText(wxSizer& sizer, const wxString& label, int alignFlags=wxALIGN_CENTRE, int size=wxDefaultCoord );

	int GetIdealWidth() const { return m_idealWidth; }
	bool HasIdealWidth() const { return m_idealWidth != wxDefaultCoord; }

protected:	
	void StartRadioGroup()
	{
		m_StartNewRadioGroup = true;
	}
};
