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
#include "Utilities/HashMap.h"

// --------------------------------------------------------------------------------------
//  KeyAcceleratorCode
//  A custom keyboard accelerator that I like better than wx's wxAcceleratorEntry.
// --------------------------------------------------------------------------------------
struct KeyAcceleratorCode
{
	union
	{
		struct
		{
			u16		keycode;
			u16		win:1,		// win32 only.
					cmd:1,		// ctrl in win32, Command in Mac
					alt:1,
					shift:1;
		};
		u32  val32;
	};

	KeyAcceleratorCode() : val32( 0 ) {}
	KeyAcceleratorCode( const wxKeyEvent& evt );
	
	//grab event attributes only
	KeyAcceleratorCode( const wxAcceleratorEntry& right)
	{
		val32 = 0;
		keycode = right.GetKeyCode();
		if( right.GetFlags() & wxACCEL_ALT )	Alt();
		if( right.GetFlags() & wxACCEL_CMD )	Cmd();
		if( right.GetFlags() & wxACCEL_SHIFT )	Shift();
	}

	KeyAcceleratorCode( wxKeyCode code )
	{
		val32 = 0;
		keycode = code;
	}

	KeyAcceleratorCode& Shift()
	{
		shift = true;
		return *this;
	}

	KeyAcceleratorCode& Alt()
	{
		alt = true;
		return *this;
	}

	KeyAcceleratorCode& Win()
	{
		win = true;
		return *this;
	}

	KeyAcceleratorCode& Cmd()
	{
		cmd = true;
		return *this;
	}

	wxString ToString() const;
};


// --------------------------------------------------------------------------------------
//  GlobalCommandDescriptor
// --------------------------------------------------------------------------------------
//  Describes a global command which can be invoked from the main GUI or GUI plugins.

struct GlobalCommandDescriptor
{
	const char*		Id;					// Identifier string
	void			(*Invoke)();		// Do it!!  Do it NOW!!!

	const wxChar*	Fullname;			// Name displayed in pulldown menus
	const wxChar*	Tooltip;			// text displayed in toolbar tooltips and menu status bars.

	bool			AlsoApplyToGui;		// Indicates that the GUI should be updated if possible.

	int				ToolbarIconId;		// not implemented yet, leave 0 for now.
};

// --------------------------------------------------------------------------------------
//  CommandDictionary
// --------------------------------------------------------------------------------------
class CommandDictionary : public HashTools::Dictionary<const GlobalCommandDescriptor*>
{
	typedef HashTools::Dictionary<const GlobalCommandDescriptor*> _parent;

protected:

public:
	using _parent::operator[];
	CommandDictionary();
	virtual ~CommandDictionary() throw();
};

// --------------------------------------------------------------------------------------
//  
// --------------------------------------------------------------------------------------
class AcceleratorDictionary : public HashTools::HashMap<int, const GlobalCommandDescriptor*>
{
	typedef HashTools::HashMap<int, const GlobalCommandDescriptor*> _parent;

protected:

public:
	using _parent::operator[];

	AcceleratorDictionary();
	virtual ~AcceleratorDictionary() throw();
	void Map( const KeyAcceleratorCode& acode, const char *searchfor );
};
