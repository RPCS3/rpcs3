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

#include "PrecompiledHeader.h"
#include "GameDatabase.h"

// Sets the current game to the one matching the serial id given
// Returns true if game found, false if not found...
bool BaseGameDatabaseVector::setGame(const wxString& id) {
	GameDataArray::iterator it( gList.begin() );
	for ( ; it != gList.end(); ++it) {
		if (it[0].CompareId(id)) {
			curGame = &it[0];
			return true;
		}
	}
	curGame = NULL;
	return false;
}

Game_Data* BaseGameDatabaseVector::createNewGame(const wxString& id) {
	gList.push_back(Game_Data(id));
	curGame = &(gList.end()-1)[0];
	return curGame;
}

// Searches the current game's data to see if the given key exists
bool BaseGameDatabaseVector::keyExists(const wxChar* key) {
	if (curGame) {
		KeyPairArray::iterator it( curGame->kList.begin() );
		for ( ; it != curGame->kList.end(); ++it) {
			if (it[0].CompareKey(key)) {
				return true;
			}
		}
	}
	else Console.Error("(GameDB) Game not set!");
	return false;
}

// Totally Deletes the specified key/pair value from the current game's data
void BaseGameDatabaseVector::deleteKey(const wxChar* key) {
	if (curGame) {
		KeyPairArray::iterator it( curGame->kList.begin() );
		for ( ; it != curGame->kList.end(); ++it) {
			if (it[0].CompareKey(key)) {
				curGame->kList.erase(it);
				return;
			}
		}
	}
	else Console.Error("(GameDB) Game not set!");
}

// Gets a string representation of the 'value' for the given key
wxString BaseGameDatabaseVector::getString(const wxChar* key) {
	if (curGame) {
		KeyPairArray::iterator it( curGame->kList.begin() );
		for ( ; it != curGame->kList.end(); ++it) {
			if (it[0].CompareKey(key)) {
				return it[0].value;
			}
		}
	}
	else Console.Error("(GameDB) Game not set!");
	return wxString();
}

void BaseGameDatabaseVector::writeString(const wxString& key, const wxString& value) {
	if (key.CmpNoCase(m_baseKey) == 0)
		curGame = createNewGame(value);

	if (curGame) {
		KeyPairArray::iterator it( curGame->kList.begin() );
		for ( ; it != curGame->kList.end(); ++it) {
			if (it[0].CompareKey(key)) {
				if( value.IsEmpty() )
					curGame->kList.erase(it);
				else
					it[0].value = value;
				return;
			}
		}
		if( !value.IsEmpty() ) {
			key_pair tKey(key, value);
			curGame->kList.push_back(tKey);
		}
	}
	else Console.Error("(GameDB) Game not set!");
}

// Write a bool value to the specified key
void BaseGameDatabaseVector::writeBool(const wxString& key, bool value) {
	writeString(key, value ? L"1" : L"0");
}