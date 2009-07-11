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

#include "PrecompiledHeader.h"

#include "ModalPopups.h"

using namespace wxHelpers;

Dialogs::PickUserModeDialog::PickUserModeDialog( wxWindow* parent, int id ) :
	wxDialogWithHelpers( parent, id, _("PCSX2 Select User Mode"), false )
{
	wxBoxSizer& s_main = *new wxBoxSizer( wxVERTICAL );

	AddStaticText( s_main, _("PCSX2 is starting from a new or unknown working directory and needs to be configured."), 400 );
	AddStaticText( s_main, _("Current Working Directory: ") + wxGetCwd(), 480 );

	AddOkCancel( s_main );

}

