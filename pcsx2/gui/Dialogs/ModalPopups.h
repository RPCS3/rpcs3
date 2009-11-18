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

#include "App.h"
#include "Panels/ConfigurationPanels.h"

#include <wx/image.h>
#include <wx/wizard.h>

static const wxWindowID pxID_CUSTOM = wxID_LOWEST - 1;

class FirstTimeWizard : public wxWizard
{
protected:
	class UsermodePage : public wxWizardPageSimple
	{
	protected:
		Panels::DirPickerPanel*			m_dirpick_settings;
		Panels::LanguageSelectionPanel*	m_panel_LangSel;
		Panels::UsermodeSelectionPanel*	m_panel_UserSel;

	public:
		UsermodePage( wxWizard* parent );

	protected:
		void OnUsermodeChanged( wxCommandEvent& evt );
	};
	
protected:
	UsermodePage&		m_page_usermode;
	wxWizardPageSimple& m_page_plugins;
	wxWizardPageSimple& m_page_bios;
	
	Panels::PluginSelectorPanel&	m_panel_PluginSel;
	Panels::BiosSelectorPanel&		m_panel_BiosSel;

public:
	FirstTimeWizard( wxWindow* parent );
	virtual ~FirstTimeWizard() throw();

	wxWizardPage *GetUsermodePage() const { return &m_page_usermode; }
	wxWizardPage *GetPostUsermodePage() const { return &m_page_plugins; }

	void ForceEnumPlugins()
	{
		m_panel_PluginSel.OnShown();
	}

protected:
	virtual void OnPageChanging( wxWizardEvent& evt );
	virtual void OnPageChanged( wxWizardEvent& evt );
	virtual void OnDoubleClicked( wxCommandEvent& evt );
};

class ConfButtons
{
protected:
	BITFIELD32()
		bool
			m_OK:1,
			m_Cancel:1,
			m_Yes:1,
			m_No:1,
			m_AllowToAll:1,
			m_Apply:1,
			m_Abort:1,
			m_Retry:1,
			m_Ignore:1,
			m_Reset:1;
	BITFIELD_END

	wxString m_CustomLabel;

public:
	ConfButtons() : bitset( 0 ) { }

	ConfButtons& OK()		{ m_OK			= true; return *this; }
	ConfButtons& Cancel()	{ m_Cancel		= true; return *this; }
	ConfButtons& Apply()	{ m_Apply		= true; return *this; }
	ConfButtons& Yes()		{ m_Yes			= true; return *this; }
	ConfButtons& No()		{ m_No			= true; return *this; }
	ConfButtons& ToAll()	{ m_AllowToAll	= true; return *this; }

	ConfButtons& Abort()	{ m_Abort		= true; return *this; }
	ConfButtons& Retry()	{ m_Retry		= true; return *this; }
	ConfButtons& Ignore()	{ m_Ignore		= true; return *this; }
	ConfButtons& Reset()	{ m_Reset		= true; return *this; }

	ConfButtons& Custom( const wxString& label)
	{
		m_CustomLabel = label;
		return *this;
	}

	ConfButtons& OKCancel()	{ m_OK = m_Cancel = true; return *this; }
	ConfButtons& YesNo()	{ m_Yes = m_No = true; return *this; }
		
	bool HasOK() const		{ return m_OK; }
	bool HasCancel() const	{ return m_Cancel; }
	bool HasApply() const	{ return m_Apply; }
	bool HasYes() const		{ return m_Yes; }
	bool HasNo() const		{ return m_No; }
	bool AllowsToAll() const{ return m_AllowToAll; }

	bool HasAbort() const	{ return m_Abort; }
	bool HasRetry() const	{ return m_Retry; }
	bool HasIgnore() const	{ return m_Ignore; }
	bool HasReset() const	{ return m_Reset; }
	
	bool HasCustom() const	{ return !m_CustomLabel.IsEmpty(); }
	const wxString& GetCustomLabel() const { return m_CustomLabel; }

	bool Allows( wxWindowID id ) const;

	bool operator ==( const ConfButtons& right ) const
	{
		return OpEqu( bitset );
	}

	bool operator !=( const ConfButtons& right ) const
	{
		return !OpEqu( bitset );
	}
};

namespace Dialogs
{
	class AboutBoxDialog: public wxDialogWithHelpers
	{
	public:
		AboutBoxDialog( wxWindow* parent=NULL, int id=DialogId_About );
		virtual ~AboutBoxDialog() throw() {}

	protected:
		//wxStaticBitmap m_bitmap_logo;
		wxStaticBitmap m_bitmap_dualshock;
	};

	
	class PickUserModeDialog : public wxDialogWithHelpers
	{
	protected:
		Panels::UsermodeSelectionPanel* m_panel_usersel;
		Panels::LanguageSelectionPanel* m_panel_langsel;

	public:
		PickUserModeDialog( wxWindow* parent, int id=wxID_ANY );
		virtual ~PickUserModeDialog() throw() {}

	protected:
		void OnOk_Click( wxCommandEvent& evt );
	};


	class ImportSettingsDialog : public wxDialogWithHelpers
	{
	public:
		ImportSettingsDialog( wxWindow* parent );
		virtual ~ImportSettingsDialog() throw() {}
		
	protected:
		void OnImport_Click( wxCommandEvent& evt );
		void OnOverwrite_Click( wxCommandEvent& evt );
	};


	// --------------------------------------------------------------------------------------
	//  ExtensibleConfirmation Dialog
	// --------------------------------------------------------------------------------------
	// Purpose: This dialog is a simple framework for providing common popups that have three
	// main sections of content:
	//
	//          [Upper Paragraph]
	//     [Optional Interior Content]
	//         [OK] [CANCEL] [ETC]
	//
	// The Upper Paragraph and buttons are specified in the constructor.  The Interior Content
	// can be added by fetching the ExtensibleSizer provided by th created dialog.  Add your
	// content, dance around a bit, call Fit(), and then ShowModal. :)

	class ExtensibleConfirmation : public wxDialogWithHelpers
	{
	protected:
		wxBoxSizer&		m_ExtensibleSizer;
		wxBoxSizer&		m_ButtonSizer;

	public:
		ExtensibleConfirmation( wxWindow* parent, const ConfButtons& type, const wxString& title, const wxString& msg );
		virtual ~ExtensibleConfirmation() throw() {}

		virtual wxBoxSizer& GetExtensibleSizer() const { return m_ExtensibleSizer; }

	protected:
		virtual void AddActionButton( wxWindowID id );
		virtual void AddCustomButton( wxWindowID id, const wxString& label );
		virtual void OnActionButtonClicked( wxCommandEvent& evt );
	};

	wxWindowID IssueConfirmation( wxWindow* parent, const wxString& disablerKey, const ConfButtons& type, const wxString& title, const wxString& msg );
}

