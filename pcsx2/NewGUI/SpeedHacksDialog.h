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


#ifndef SPEEDHACKSDIALOG_H_INCLUDED
#define SPEEDHACKSDIALOG_H_INCLUDED

#pragma once

#include <wx/wx.h>
#include <wx/image.h>

#include "wxHelpers.h"

namespace Dialogs
{
	class SpeedHacksDialog: public wxDialogWithHelpers
	{
	public:
		SpeedHacksDialog(wxWindow* parent, int id);

	protected:

	public:
		void IOPCycleDouble_Click(wxCommandEvent &event);
        void WaitCycleExt_Click(wxCommandEvent &event);
        void INTCSTATSlow_Click(wxCommandEvent &event);
        void IdleLoopFF_Click(wxCommandEvent &event);
	};
}

#endif // SPEEDHACKSDIALOG_H_INCLUDED
