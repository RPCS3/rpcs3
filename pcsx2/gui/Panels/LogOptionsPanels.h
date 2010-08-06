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

#include "AppCommon.h"
#include "ApplyState.h"

namespace Panels
{
	class LogOptionsPanel;

	class BaseCpuLogOptionsPanel : public CheckedStaticBox
	{
	protected:
		wxStaticBoxSizer*	m_miscGroup;

	public:
		BaseCpuLogOptionsPanel( wxWindow* parent, const wxString& title, wxOrientation orient=wxVERTICAL )
			: CheckedStaticBox( parent, orient, title ) {}

		virtual wxStaticBoxSizer* GetMiscGroup() const { return m_miscGroup; }
		virtual CheckedStaticBox* GetStaticBox( const wxString& subgroup ) const=0;
	};

	class eeLogOptionsPanel : public BaseCpuLogOptionsPanel
	{
	protected:
		CheckedStaticBox*	m_disasmPanel;
		CheckedStaticBox*	m_hwPanel;
		CheckedStaticBox*	m_evtPanel;

	public:
		eeLogOptionsPanel( LogOptionsPanel* parent );
		virtual ~eeLogOptionsPanel() throw() {}

		CheckedStaticBox* GetStaticBox( const wxString& subgroup ) const;

		void OnSettingsChanged();
		void Apply();
	};


	class iopLogOptionsPanel : public BaseCpuLogOptionsPanel
	{
	protected:
		CheckedStaticBox*	m_disasmPanel;
		CheckedStaticBox*	m_hwPanel;
		CheckedStaticBox*	m_evtPanel;

	public:
		iopLogOptionsPanel( LogOptionsPanel* parent );
		virtual ~iopLogOptionsPanel() throw() {}

		CheckedStaticBox* GetStaticBox( const wxString& subgroup ) const;

		void OnSettingsChanged();
		void Apply();
	};

	class LogOptionsPanel : public BaseApplicableConfigPanel
	{
	protected:
		eeLogOptionsPanel*	m_eeSection;
		iopLogOptionsPanel*	m_iopSection;
		bool				m_IsDirty;		// any settings modified since last apply will flag this "true"

		pxCheckBox*			m_masterEnabler;

		ScopedArray<pxCheckBox*> m_checks;

	public:
		LogOptionsPanel( wxWindow* parent );
		virtual ~LogOptionsPanel() throw() {}

		void AppStatusEvent_OnSettingsApplied();
		void OnUpdateEnableAll();
		void OnCheckBoxClicked(wxCommandEvent &event);
		void Apply();
		
	protected:
		BaseCpuLogOptionsPanel* GetCpuPanel( const wxString& token ) const;
	};
}
