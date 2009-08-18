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

#include "Panels/ConfigurationPanels.h"

#include <wx/image.h>
#include <wx/wizard.h>

class FirstTimeWizard : public wxWizard
{
protected:
	wxWizardPageSimple& m_page_usermode;
	//wxWizardPageSimple& m_page_paths;
	wxWizardPageSimple& m_page_plugins;
	
	Panels::LanguageSelectionPanel&	m_panel_LangSel;
	Panels::UsermodeSelectionPanel&	m_panel_UsermodeSel;
	Panels::AdvancedPathsPanel&		m_panel_Paths;
	Panels::PluginSelectorPanel&	m_panel_PluginSel;

public:
	FirstTimeWizard( wxWindow* parent );
	virtual ~FirstTimeWizard();
	
	wxWizardPage *GetFirstPage() const { return &m_page_usermode; }

protected:
	virtual void OnPageChanging( wxWizardEvent& evt );
	virtual void OnPageChanged( wxWizardEvent& evt );
	virtual void OnUsermodeChanged( wxCommandEvent& evt );
};

namespace Dialogs
{
	class AboutBoxDialog: public wxDialog
	{
	public:
		AboutBoxDialog( wxWindow* parent, int id );

	protected:
		wxStaticBitmap m_bitmap_logo;
		wxStaticBitmap m_bitmap_ps2system;

	};
	
	class PickUserModeDialog : public wxDialogWithHelpers
	{
	protected:
		Panels::UsermodeSelectionPanel* m_panel_usersel;
		Panels::LanguageSelectionPanel* m_panel_langsel;

	public:
		PickUserModeDialog( wxWindow* parent, int id=wxID_ANY );

	protected:
		void OnOk_Click( wxCommandEvent& evt );
	};
}

