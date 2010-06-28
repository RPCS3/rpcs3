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

#include "GameDatabase.h"

// --------------------------------------------------------------------------------------
//  AppGameDatabase
// --------------------------------------------------------------------------------------
// This class extends BaseGameDatabase_Impl and provides interfaces for loading and saving
// the text-formatted game database.
//
// Example:
// ---------------------------------------------
// Serial = SLUS-20486
// Name   = Marvel vs. Capcom 2
// Region = NTSC-U
// ---------------------------------------------
//
// [-- separators are a standard part of the formatting]
//

// To Load this game data, use "Serial" as the initial Key
// then specify "SLUS-20486" as the value in the constructor.
// After the constructor loads the game data, you can use the
// GameDatabase class's methods to get the other key's values.
// Such as dbLoader.getString("Region") returns "NTSC-U"

class AppGameDatabase : public BaseGameDatabaseImpl
{
protected:
	wxString		header;			// Header of the database
	wxString		baseKey;		// Key to separate games by ("Serial")

public:
	AppGameDatabase() {}
	virtual ~AppGameDatabase() throw() {
		Console.WriteLn( "(GameDB) Unloading..." );
	}

	AppGameDatabase& LoadFromFile(const wxString& file = L"GameIndex.dbf", const wxString& key = L"Serial" );
	void SaveToFile(const wxString& file = L"GameIndex.dbf");
};

static wxString compatToStringWX(int compat) {
	switch (compat) {
		case 6:  return L"Perfect";
		case 5:  return L"Playable";
		case 4:  return L"In-Game";
		case 3:  return L"Menu";
		case 2:  return L"Intro";
		case 1:  return L"Nothing";
		default: return L"Unknown";
	}
}
