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
#include "ConfigurationDialog.h"
#include "Panels/ConfigurationPanels.h"

#include <wx/image.h>
#include <wx/wizard.h>

static const wxWindowID pxID_CUSTOM = wxID_LOWEST - 1;

class FirstTimeWizard : public wxWizard
{
protected:
	class UsermodePage : public ApplicableWizardPage
	{
	protected:
		Panels::DirPickerPanel*			m_dirpick_settings;
		Panels::LanguageSelectionPanel*	m_panel_LangSel;
		Panels::UsermodeSelectionPanel*	m_panel_UserSel;

	public:
		UsermodePage( wxWizard* parent );

	protected:
		void OnUsermodeChanged( wxCommandEvent& evt );
		void OnCustomDirChanged( wxCommandEvent& evt );
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


namespace Dialogs
{
	class AboutBoxDialog: public wxDialogWithHelpers
	{
	protected:
		//wxStaticBitmap m_bitmap_logo;
		wxStaticBitmap m_bitmap_dualshock;

	public:
		AboutBoxDialog( wxWindow* parent=NULL );
		virtual ~AboutBoxDialog() throw() {}

		static const wxChar* GetNameStatic() { return L"Dialog:AboutBox"; }
	};

	
	class PickUserModeDialog : public BaseApplicableDialog
	{
	protected:
		Panels::UsermodeSelectionPanel* m_panel_usersel;
		Panels::LanguageSelectionPanel* m_panel_langsel;

	public:
		PickUserModeDialog( wxWindow* parent );
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

	class AssertionDialog : public wxDialogWithHelpers
	{
	public:
		AssertionDialog( const wxString& text, const wxString& stacktrace );
		virtual ~AssertionDialog() throw() {}
	};

	// There are two types of stuck threads:
	//  * Threads stuck on any action that is not a cancellation.
	//  * Threads stuck trying to cancel.
	//
	// The former means we can provide a "cancel" action for the user, which would itself
	// open a new dialog in the latter category.  The latter means that there's really nothing
	// we can do, since pthreads API provides no good way for killing threads.  The only
	// valid options for the user in that case is to either wait (boring!) or kill the 
	// process (awesome!).

	enum StuckThreadActionType
	{
		// Allows the user to attempt a cancellation of a stuck thread.  This should only be
		// used on threads which are not already stuck during a cancellation action (ie, suspension
		// or other job requests).  Also, if the running thread is known to not have any
		// cancellation points then this shouldn't be used either.
		StacT_TryCancel,

		// Allows the user to kill the entire process for a stuck thread.  Use this for any
		// thread which has failed to cancel in a reasonable timeframe, or for any stuck action
		// if the thread is known to have no cancellation points.
		StacT_KillProcess,
	};

	class StuckThreadDialog : public wxDialogWithHelpers,
		public EventListener_Thread
	{
	public:
		StuckThreadDialog( wxWindow* parent, StuckThreadActionType action, Threading::PersistentThread& stuck_thread );
		virtual ~StuckThreadDialog() throw() {}
		
	protected:
		void OnThreadCleanup();
	};
}

wxWindowID pxIssueConfirmation( wxDialogWithHelpers& confirmDlg, const MsgButtons& buttons );
wxWindowID pxIssueConfirmation( wxDialogWithHelpers& confirmDlg, const MsgButtons& buttons, const wxString& disablerKey );
