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
#include "wxGuiTools.h"

// -------------------------------------------------------------------------------------
//  pxCheckBox
// --------------------------------------------------------------------------------------
// The checkbox panel created uses the default spacer setting for adding checkboxes (see
// SizerFlags).  The SetToolTip API provided by this function applies the tooltip to both
// both the checkbox and it's static subtext (if present), and performs word wrapping on
// platforms that need it (eg mswindows).
//
class pxCheckBox : public wxPanelWithHelpers
{
protected:
	wxCheckBox*		m_checkbox;
	pxStaticText*	m_subtext;

	// padding below the subtext (if there's subtext).  If there's no subtext, this value is unused.
	int				m_subPadding;

	wxSizerItem*	m_sizerItem_subtext;
public:
	pxCheckBox( wxWindow* parent, const wxString& label, const wxString& subtext=wxEmptyString, int flags = wxCHK_2STATE );
	pxCheckBox( wxWindow* parent, const wxString& label, int flags );
	virtual ~pxCheckBox() throw() {}

	bool HasSubText() const { return m_subtext != NULL; }
	const pxStaticText* GetSubText() const { return m_subtext; }

	pxCheckBox& SetSubPadding( int pad );
	pxCheckBox& SetToolTip( const wxString& tip );
	pxCheckBox& SetValue( bool val );
	pxCheckBox& SetIndeterminate();
	pxCheckBox& SetState( wxCheckBoxState state );

	wxCheckBoxState GetState() const	{ pxAssert( m_checkbox != NULL ); return m_checkbox->Get3StateValue(); }
	bool GetValue() const				{ pxAssert( m_checkbox != NULL ); return m_checkbox->GetValue(); }
	bool IsChecked() const				{ pxAssert( m_checkbox != NULL ); return m_checkbox->IsChecked(); }
	bool IsIndeterminate() const		{ pxAssert( m_checkbox != NULL ); return m_checkbox->Get3StateValue() == wxCHK_UNDETERMINED; }
	operator wxCheckBox&()				{ pxAssert( m_checkbox != NULL ); return *m_checkbox; }
	operator const wxCheckBox&() const	{ pxAssert( m_checkbox != NULL ); return *m_checkbox; }

	wxCheckBox* GetWxPtr() { return m_checkbox; }
	const wxCheckBox* GetWxPtr() const { return m_checkbox; }

	//wxWindowID GetId() const { pxAssert( m_checkbox != NULL ); return m_checkbox->GetId(); }

protected:
	void Init( const wxString& label, const wxString& subtext, int flags );
	void OnCheckpartCommand( wxCommandEvent& evt );
	void OnSubtextClicked( wxCommandEvent& evt );
};

extern void operator+=( wxSizer& target, pxCheckBox* src );
extern void operator+=( wxSizer& target, pxCheckBox& src );
extern void operator+=( wxSizer* target, pxCheckBox& src );

template<>
inline void operator+=( wxSizer& target, const pxWindowAndFlags<pxCheckBox>& src )
{
	target.Add( src.window, src.flags );
}

template<>
inline void operator+=( wxSizer* target, const pxWindowAndFlags<pxCheckBox>& src )
{
	target->Add( src.window, src.flags );
}
