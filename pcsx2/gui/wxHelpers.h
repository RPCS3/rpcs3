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

// --------------------------------------------------------------------------------------
//  wxDialogWithHelpers
// --------------------------------------------------------------------------------------
class wxDialogWithHelpers : public wxDialog
{
protected:
	bool m_hasContextHelp;

public:
	wxDialogWithHelpers(wxWindow* parent, int id, const wxString& title, bool hasContextHelp, const wxPoint& pos=wxDefaultPosition, const wxSize& size=wxDefaultSize );
	virtual ~wxDialogWithHelpers() throw();

	wxCheckBox&		AddCheckBox( wxSizer& sizer, const wxString& label, const wxString& subtext=wxEmptyString, const wxString& tooltip=wxEmptyString );
	wxStaticText&	AddStaticText(wxSizer& sizer, const wxString& label, int alignFlags=wxALIGN_CENTRE, int size=wxDefaultCoord );
    void AddOkCancel( wxSizer& sizer, bool hasApply=false );

protected:
};


// --------------------------------------------------------------------------------------
//  wxPanelWithHelpers
// --------------------------------------------------------------------------------------
class wxPanelWithHelpers : public wxPanel
{
protected:
	int		m_idealWidth;
	bool	m_StartNewRadioGroup;

public:
	wxPanelWithHelpers( wxWindow* parent, int idealWidth=wxDefaultCoord );
	wxPanelWithHelpers( wxWindow* parent, const wxPoint& pos, const wxSize& size=wxDefaultSize );

	//wxRadioButton&	NewSpinCtrl( const wxString& label, const wxString& subtext=wxEmptyString, const wxString& tooltip=wxEmptyString );

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

// --------------------------------------------------------------------------------------
//  pxCheckBox
// --------------------------------------------------------------------------------------
class pxCheckBox : public wxPanel
{
protected:
	wxCheckBox*		m_checkbox;
	wxStaticText*	m_subtext;
	int				m_idealWidth;

public:
	pxCheckBox( wxPanelWithHelpers* parent, const wxString& label, const wxString& subtext=wxEmptyString );
	virtual ~pxCheckBox() throw() {}

	bool HasSubText() const { return m_subtext != NULL; }
	const wxStaticText* GetSubText() const { return m_subtext; }

	pxCheckBox& SetToolTip( const wxString& tip );

	operator wxCheckBox*() { return m_checkbox; }
	operator const wxCheckBox*() const { return m_checkbox; }

	operator wxCheckBox&() { pxAssert( m_checkbox != NULL ); return *m_checkbox; }
	operator const wxCheckBox&() const { pxAssert( m_checkbox != NULL ); return *m_checkbox; }
};


extern bool pxDialogExists( wxWindowID id );

