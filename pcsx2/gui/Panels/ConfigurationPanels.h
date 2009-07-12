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

// All of the options screens in PCSX2 are implemented as panels which can be bound to
// either their own dialog boxes, or made into children of a paged Properties box.  The
// paged Properties box is generally superior design, and there's a good chance we'll not
// want to deviate form that design anytime soon.  But there's no harm in keeping nice
// encapsulation of panels regardless, just in case we want to shuffle things around. :)

#pragma once

#include <wx/image.h>
#include <wx/statline.h>

#include "wxHelpers.h"
#include "Utilities/SafeArray.h"

namespace Panels
{
	//////////////////////////////////////////////////////////////////////////////////////////
	//
	class SpeedHacksPanel : public wxPanelWithHelpers
	{
	public:
		SpeedHacksPanel(wxWindow& parent, int id=wxID_ANY);

	protected:

	public:
		void IOPCycleDouble_Click(wxCommandEvent &event);
		void WaitCycleExt_Click(wxCommandEvent &event);
		void INTCSTATSlow_Click(wxCommandEvent &event);
		void IdleLoopFF_Click(wxCommandEvent &event);
	};

	//////////////////////////////////////////////////////////////////////////////////////////
	//
	class GameFixesPanel: public wxPanelWithHelpers
	{
	public:
		GameFixesPanel(wxWindow& parent, int id=wxID_ANY);

	protected:

	public:
		void FPUCompareHack_Click(wxCommandEvent &event);
		void FPUMultHack_Click(wxCommandEvent &event);
		void TriAce_Click(wxCommandEvent &event);
		void GodWar_Click(wxCommandEvent &event);
		void Ok_Click(wxCommandEvent &event);
		void Cancel_Click(wxCommandEvent &event);
	};

	//////////////////////////////////////////////////////////////////////////////////////////
	//
	class PathsPanel: public wxPanelWithHelpers
	{
	protected:
		class DirPickerPanel : public wxPanelWithHelpers
		{
		protected:
			wxDirName (*m_GetDefaultFunc)();
			wxDirPickerCtrl* m_pickerCtrl;
			wxCheckBox* m_checkCtrl;

		public:
			DirPickerPanel( wxWindow* parent, const wxDirName& initPath, wxDirName (*getDefault)(), const wxString& label, const wxString& dialogLabel );

		protected:
			void UseDefaultPath_Click(wxCommandEvent &event);
		};

		class MyBasePanel : public wxPanelWithHelpers
		{
		protected:
			wxBoxSizer& s_main;

		public:
			MyBasePanel(wxWindow& parent, int id=wxID_ANY);

		protected:
			void AddDirPicker( wxBoxSizer& sizer, const wxDirName& initPath, wxDirName (*getDefaultFunc)(),
				const wxString& label, const wxString& popupLabel, enum ExpandedMsgEnum tooltip );
		};

		class StandardPanel : public MyBasePanel
		{
		public:
			StandardPanel(wxWindow& parent, int id=wxID_ANY);

		protected:
			//DirPickerInfo m_BiosPicker;
			//DirPickerInfo m_SavestatesPicker;
			//DirPickerInfo m_SnapshotsPicker;
			//DirPickerInfo m_MemorycardsPicker;
			//DirPickerInfo m_LogsPicker;
		};

		class AdvancedPanel : public MyBasePanel
		{
		public:
			AdvancedPanel(wxWindow& parent, int id=wxID_ANY);
		};

	public:
		PathsPanel(wxWindow& parent, int id=wxID_ANY);
	};

	//////////////////////////////////////////////////////////////////////////////////////////
	//
	class PluginSelectorPanel: public wxPanelWithHelpers
	{
	public:
		PluginSelectorPanel(wxWindow& parent, int id=wxID_ANY);

	protected:

	public:
	};
}
