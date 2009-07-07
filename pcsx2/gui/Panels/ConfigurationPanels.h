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

#include "wxHelpers.h"

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
	public:
		PathsPanel(wxWindow& parent, int id=wxID_ANY);

	protected:

	public:
	};
}
