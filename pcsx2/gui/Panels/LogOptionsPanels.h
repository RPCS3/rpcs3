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


#include "BaseConfigPanel.h"
#include "CheckedStaticBox.h"

#include "Utilities/HashMap.h"

namespace Panels
{
	typedef HashTools::Dictionary< wxCheckBox* > CheckBoxDict;

	class LogOptionsPanel;

	class eeLogOptionsPanel : public CheckedStaticBox
	{
	public:
		eeLogOptionsPanel( LogOptionsPanel* parent );
		virtual ~eeLogOptionsPanel() throw() {}
	};


	class iopLogOptionsPanel : public CheckedStaticBox
	{
	public:
		iopLogOptionsPanel( LogOptionsPanel* parent );
		virtual ~iopLogOptionsPanel() throw() {}
	};

	class LogOptionsPanel : public BaseApplicableConfigPanel
	{
	public:
		CheckBoxDict		CheckBoxes;

	protected:
		eeLogOptionsPanel&	m_eeSection;
		iopLogOptionsPanel&	m_iopSection;
		bool				m_IsDirty;		// any settings modified since last apply will flag this "true"

	public:
		LogOptionsPanel( wxWindow* parent, int idealWidth );
		virtual ~LogOptionsPanel() throw() {}

		void OnCheckBoxClicked(wxCommandEvent &event);
		void Apply();
	};
}
