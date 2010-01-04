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

#include "AppCommon.h"
#include "ApplyState.h"

namespace Panels
{
	class LogOptionsPanel;

	class eeLogOptionsPanel : public CheckedStaticBox
	{
	public:
		CheckedStaticBox*	m_disasmPanel;
		CheckedStaticBox*	m_hwPanel;
		CheckedStaticBox*	m_evtPanel;
		
		pxCheckBox*			m_Memory;
		pxCheckBox*			m_Bios;
		pxCheckBox*			m_Cache;
		pxCheckBox*			m_SysCtrl;

		pxCheckBox*			m_R5900;
		pxCheckBox*			m_COP0;
		pxCheckBox*			m_COP1;
		pxCheckBox*			m_COP2;
		pxCheckBox*			m_VU0micro;
		pxCheckBox*			m_VU1micro;

		pxCheckBox*			m_KnownHw;
		pxCheckBox*			m_UnknownHw;
		pxCheckBox*			m_DMA;

		pxCheckBox*			m_Counters;
		pxCheckBox*			m_VIF;
		pxCheckBox*			m_GIF;
		pxCheckBox*			m_SPR;
		pxCheckBox*			m_IPU;

	public:
		eeLogOptionsPanel( LogOptionsPanel* parent );
		virtual ~eeLogOptionsPanel() throw() {}
		
		void OnSettingsChanged();
		void Apply();
	};


	class iopLogOptionsPanel : public CheckedStaticBox
	{
	public:
		CheckedStaticBox*	m_disasmPanel;
		CheckedStaticBox*	m_hwPanel;
		CheckedStaticBox*	m_evtPanel;

		pxCheckBox*			m_Bios;
		pxCheckBox*			m_Memory;
		pxCheckBox*			m_GPU;

		pxCheckBox*			m_R3000A;
		pxCheckBox*			m_COP2;

		pxCheckBox*			m_KnownHw;
		pxCheckBox*			m_UnknownHw;
		pxCheckBox*			m_DMA;

		pxCheckBox*			m_Counters;
		pxCheckBox*			m_Memcards;
		pxCheckBox*			m_PAD;
		pxCheckBox*			m_SPU2;
		pxCheckBox*			m_USB;
		pxCheckBox*			m_FW;
		pxCheckBox*			m_CDVD;

	public:
		iopLogOptionsPanel( LogOptionsPanel* parent );
		virtual ~iopLogOptionsPanel() throw() {}

		void OnSettingsChanged();
		void Apply();
	};

	class LogOptionsPanel : public BaseApplicableConfigPanel
	{
	protected:
		eeLogOptionsPanel&	m_eeSection;
		iopLogOptionsPanel&	m_iopSection;
		bool				m_IsDirty;		// any settings modified since last apply will flag this "true"

		pxCheckBox*			m_masterEnabler;
		pxCheckBox*			m_SIF;
		pxCheckBox*			m_VIFunpack;
		pxCheckBox*			m_GIFtag;

	public:
		LogOptionsPanel( wxWindow* parent );
		virtual ~LogOptionsPanel() throw() {}

		void OnSettingsChanged();
		void OnUpdateEnableAll();
		void OnCheckBoxClicked(wxCommandEvent &event);
		void Apply();
	};
}
