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
#include "App.h"

// This API is likely obsolete for the most part, so I've just included a few dummies
// to keep things compiling until I can get to the point of tying up loose ends.

namespace HostGui
{
	// Sets the status bar message without mirroring the output to the console.
	void SetStatusMsg( const wxString& text )
	{
		wxGetApp().GetMainWindow()->SetStatusText( text );
	}

	void Notice( const wxString& text )
	{
		// mirror output to the console!
		Console::Status( text.c_str() );
		SetStatusMsg( text );
	}

	void ResetMenuSlots()
	{
		// Probably obsolete if we do a savestate dialog.
	}
	
	void BeginExecution()
	{
	}
	
	void __fastcall KeyEvent( keyEvent* ev )
	{
	}
}